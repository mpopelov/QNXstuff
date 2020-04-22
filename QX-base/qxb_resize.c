/*
 * 2006 (c) -={ MikeP }=-
 * 
 * This tool is intended to resize tables:
 * - add columns
 * - remove columns
 * 
 * To use this tool one should use script file as
 * the firs parameter to the program.
 * 
 * Script file consists of the following commands:
 * 
 * QXBASE_ROOT	-	root directory of the QX-base server.
 * QXBASE_DEV	-	device to open.
 * TABLE		-	a table to change.
 * RECREATEONLY	-	Create a new table based on rules but do not copy data.
 * 
 * The following directive defines the new column:
 * 
 * 	COLUMN	<TYPE>	<SIZE>
 * 
 * or
 * 
 * 	COLUMN	OLD		<number>
 * 
 * Each COLUMN statement should be placed on a separate line.
 * The new table will contain as many columns as there are
 * COLUMN statements. This way you can not only change the
 * number of columns, but also change their order.
 */


#include	<stdio.h>
#include	<stdlib.h>
#include	<stddef.h>
#include	<string.h>
#include	<errno.h>
#include	<devctl.h>
#include	"qbase_client.h"
#include	"GetConf.h"



/* Linked list of new columns */
typedef struct _ch_col{
	uint32_t	col_type;		/* type of data */
	uint32_t	col_length;		/* the length of data */
	uint32_t	col_old_index;	/* index of the field in old column */
	uint32_t	col_old_offset;
	uint32_t	col_new_offset;
	struct _ch_col *next;		/* the next column in the table */
} ch_col_t;




/* config-time variables */
char		*token;

/* Global variables */
char		*qb_root = NULL;
char		*qb_device = NULL;
char		qb_table[QBASE_TAB_NAMELEN];

int			recreateonly = 0;
int			old_tbl_exists = 0;

ch_col_t	*new_cols	=	NULL;
ch_col_t	*last_col	=	NULL;

int			col_number	=	0;
int			rows_pp		=	0;
int			lock_pages	=	0;

char		*tmp_tab_name	=	"tmp_tbl";
char		szTemp[255];
char		szTemp1[255];




int			fd_tmp = 0;
int			result = 0;



/* Tables */
QB_connid_t		new_tbl;
QB_connid_t		old_tbl;
Qbase_table_desc_t *	new_tbl_desc = NULL;
Qbase_table_desc_t *	old_tbl_desc = NULL;
char			*new_buf = NULL;
char			*old_buf = NULL;
int				new_buf_size = 0;
int				old_buf_size = 0;







int main(int argc, char **argv){
	
	printf("QXB-resize utility v.0.1 2006 (c) -={ MikeP }=-\n");
	
	/* Read configuration file and setup all of the options and columns */
	if(argc == 1){
		printf("\n Usage: QXB-resize script_file\n");
		return 0;
	}
	
	/* Now try to open config file */
	if( GetConf_open( argv[1] ) == 0 ){
		printf("Error reading script from %s\n", argv[1] );
		return 0;
	}
	
	while( (token = GetConf_string()) != NULL ){
		if( 0 == stricmp("QXBASE_ROOT",token)){
			if( NULL != (token = GetConf_token()) ){
				qb_root = strdup(token);
			}
			continue;
		}
		
		if( 0 == stricmp("QXBASE_DEV",token)){
			if( NULL != (token = GetConf_token()) ){
				qb_device = strdup(token);
			}
			continue;
		}
		
		if( 0 == stricmp("RECREATEONLY",token)){
			recreateonly = 1;
			continue;
		}
		
		if( 0 == stricmp("TABLE",token)){
			if( NULL != (token = GetConf_token()) ){
				strncpy(qb_table,token,QBASE_TAB_NAMELEN);
				qb_table[QBASE_TAB_NAMELEN] = 0;
			}
			continue;
		}
		
		if( 0 == stricmp("ROWS_PP",token)){
			if( NULL != (token = GetConf_token()) ){
				rows_pp = atoi(token);
			}
			continue;
		}
		
		if( 0 == stricmp("LOCK_PAGES",token)){
			if( NULL != (token = GetConf_token()) ){
				lock_pages = atoi(token);
			}
			continue;
		}
		
		/* read column data */
		if( 0 == stricmp("COLUMN",token)){
			if( NULL != (token = GetConf_token()) ){
				ch_col_t *n_c = (ch_col_t*)malloc(sizeof(ch_col_t));
				
				if(n_c == NULL){
					printf("Can not allocate memory\n");
					return 0;
				}
				memset(n_c,0,sizeof(ch_col_t));
				
				/* What kind of column is it? */
				if( 0 == stricmp("OLD",token)){
					if( NULL != (token = GetConf_token()) ){
						n_c->col_old_index = atoi(token);
						n_c->col_type = QBASE_TYPE_LAST_EFFECTIVE+1;
					}else{
						printf("Error on line [%d]:\n", GetConf_ln);
						printf("Synatx: COLUMN OLD <number_of_column_in_old_table>\n");
						return 0;
					}
					
				}else if( 0 == stricmp("UINT",token)){
					/* Add column of type uint */
					n_c->col_type = QBASE_TYPE_UINT;
				}else if( 0 == stricmp("INT",token)){
					/* Add column of type int */
					n_c->col_type = QBASE_TYPE_INT;
				}else if( 0 == stricmp("UINT16",token)){
					/* Add column of type uint16 */
					n_c->col_type = QBASE_TYPE_UINT16;
				}else if( 0 == stricmp("INT16",token)){
					/* Add column of type int16 */
					n_c->col_type = QBASE_TYPE_INT16;
				}else if( 0 == stricmp("DOUBLE",token)){
					/* Add column of type double */
					n_c->col_type = QBASE_TYPE_DOUBLE;
				}else if( 0 == stricmp("TIMESTAMP",token)){
					/* Add column of type timestamp */
					n_c->col_type = QBASE_TYPE_TIMESTAMP;
				}else if( 0 == stricmp("BINARY",token)){
					/* Add column of type binary */
					n_c->col_type = QBASE_TYPE_BINARY;
					if( NULL != (token = GetConf_token()) ){
						n_c->col_length = atoi(token);
					}else{
						printf("Error on line [%d]:\n", GetConf_ln);
						printf("You should specify a length for this type\n");
						return 0;
					}
				}else if( 0 == stricmp("STRING",token)){
					/* Add column of type string */
					n_c->col_type = QBASE_TYPE_STRING;
					if( NULL != (token = GetConf_token()) ){
						n_c->col_length = atoi(token);
					}else{
						printf("Error on line [%d]:\n", GetConf_ln);
						printf("You should specify a length for this type\n");
						return 0;
					}
				}else{
					printf("Error on line [%d]:\n", GetConf_ln);
					printf("COLUMN may be of some <type> or like OLD #\n");
					return 0;
				}
				
				/* we have filled the column descriptor
				 * now add it to the list of columns */
				if( last_col == NULL ){
					new_cols = n_c;
				}else{
					last_col->next = n_c;
				}
				last_col = n_c;
				col_number++;
				
			}else{
				printf("Error on line [%d]:\n", GetConf_ln);
				printf("Second argument should be <type> or OLD\n");
				return 0;
			}
			
			continue;
		}
		
		printf("Unknown directive on line [%d]\n", GetConf_ln);
		return 0;
	}
	GetConf_close();
	
	
	/* Check default values */
	if( (qb_root == NULL) |
		(qb_device == NULL) |
		(qb_table[0] == 0) |
		(new_cols == NULL) ){
			printf("Some required options are not set or nothing to do.\n");
			return 0;
	}
	
	
	/* Now try to swap tables. If the old one xists - change its filename
	 * and create a new one */
	if( -1 == chdir(qb_root) ){
		printf("Error changigng directory to %s:\n%s\n",qb_root,strerror(errno));
		return 0;
	}
	
	/* if table exists - rename it to temporary */
	snprintf(szTemp,255,"%s.%s",qb_table,"qdbd");
	fd_tmp = open(szTemp,O_RDONLY);
	if(fd_tmp != -1){
		printf("Moving old table to temporary file\n");
		old_tbl_exists = 1;
		close(fd_tmp);
		
		snprintf(szTemp1,255,"%s.%s",tmp_tab_name,"qdbd");
		link(szTemp,szTemp1);
		unlink(szTemp);
		
		snprintf(szTemp,255,"%s.%s",qb_table,"qdbh");
		snprintf(szTemp1,255,"%s.%s",tmp_tab_name,"qdbh");
		link(szTemp,szTemp1);
		unlink(szTemp);
	}
	
	if( (old_tbl = open( qb_device , O_RDWR )) == -1 ){
		printf("Can't open QX-base\n");
		return 0;
	}
	if( (new_tbl = open( qb_device , O_RDWR )) == -1 ){
		printf("Can't open QX-base\n");
		return 0;
	}
	
	/* Open temporary table ... */
	if( old_tbl_exists == 1 ){
		ch_col_t * n_c = new_cols;
		
		if( QBASE_E_OK != (result = Qbase_table_select( old_tbl, tmp_tab_name )) ){
			printf("Can't open renamed table: %s\n",QBERROR[result]);
			return 0;
		}
		/* ... and get its descriptor */
		if( QBASE_E_OK != (result = Qbase_table_caps(old_tbl, &old_tbl_desc)) ){
			printf("Old table caption unavailable: %s\n",QBERROR[result]);
			return 0;
		}
		
		/* now scan new columns to see if there is something to get from
		 * old table */
		while( n_c != NULL ){
			if( (n_c->col_type == QBASE_TYPE_LAST_EFFECTIVE+1) &&
				(n_c->col_old_index != 0)){
				if(n_c->col_old_index > old_tbl_desc->FieldNum){
					printf("Filed index exeeds number of fields in old table\n");
					return 0;
				}
				/* get values from previous table */
				n_c->col_type = old_tbl_desc->Fields[n_c->col_old_index].Type;
				n_c->col_length = old_tbl_desc->Fields[n_c->col_old_index].Length;
				n_c->col_old_offset = old_tbl_desc->Fields[n_c->col_old_index].Offset;
			}
			n_c = n_c->next;
		}
		
	}
	
	
	/* Create a new table descriptor */
	new_tbl_desc = Qbase_new_tbldesc( col_number, rows_pp, lock_pages, qb_table );
	if( new_tbl_desc == NULL ){
		printf("Error creating table descriptor\n");
		return 0;
	}
	
	/* Set field types of new table */
	{
		ch_col_t *n_c = new_cols;
		int		ColNum = 1;
		
		while( n_c != NULL ){
			if(1 != Qbase_set_field_metrics(new_tbl_desc,ColNum,n_c->col_type,n_c->col_length)){
				printf("Error setting column %d metrics\n",ColNum);
				return 0;
			}
			n_c = n_c->next;
			ColNum++;
		}
		
		/* get the lengths of the fields */
		n_c = new_cols;
		ColNum = 1;
		while( n_c != NULL ){
			n_c->col_length = new_tbl_desc->Fields[ColNum].Length;
			n_c = n_c->next;
			ColNum++;
		}
	}
	
	/* Create table */
	close( old_tbl );
	/* As the server opens body of the table based on the name, found in 
	 * the caption - we should rewrite caption */
	snprintf(old_tbl_desc->Name,QBASE_TAB_NAMELEN,"%s",tmp_tab_name);
	snprintf(szTemp1,255,"%s.%s",tmp_tab_name,"qdbh");
	fd_tmp = open( szTemp1, O_RDWR );
	write(fd_tmp,old_tbl_desc,old_tbl_desc->Size);
	close(fd_tmp);
	
	if( QBASE_E_OK != (result = Qbase_table_create( new_tbl, new_tbl_desc )) ){
		printf("Can't create table: %s\n",QBERROR[result]);
		return 0;
	}
	if( (old_tbl = open( qb_device , O_RDWR )) == -1 ){
		printf("Can't REopen QX-base\n");
		return 0;
	}
	if( QBASE_E_OK != (result = Qbase_table_select( old_tbl, tmp_tab_name )) ){
		printf("Can't REopen old table: %s\n",QBERROR[result]);
		return 0;
	}
	
	
	
	if( recreateonly == 0 ){
		
		printf("Moving table data..........\n");
		
		/* prepare buffers */
		old_buf = Qbase_row_newbuffer( old_tbl_desc, &old_buf_size);
		new_buf = Qbase_row_newbuffer( new_tbl_desc, &new_buf_size);
		
		/* copy old data to new table checking order of columns */
		while(1){
			result = Qbase_fetch(old_tbl,old_buf,old_buf_size);
			if( result != QBASE_E_OK ) break;
			printf("one more record...\n");
			memset(new_buf,0,new_buf_size);
			/* Copy values of requested columns to new buffer */
			{
				ch_col_t *n_c = new_cols;
				int		ColNum = 1;
				
				while( n_c != NULL ){
					if( n_c->col_old_index != 0){
						char * Value;
						if(n_c->col_old_index > old_tbl_desc->FieldNum){
							printf("Filed index exeeds number of fields in old table\n");
							return 0;
						}
						/*printf("Moving field %d...\n",ColNum);*/
						Value = (old_buf + n_c->col_old_offset);
						result = Qbase_field_set( new_tbl_desc, new_buf, ColNum, Value, n_c->col_length );
						if( result != QBASE_E_OK ){
							printf("Error setting value of field %d\n",ColNum);
							return 0;
						}
					}
					n_c = n_c->next;
					ColNum++;
				}
				
				/* insert row into table */
				if(QBASE_E_OK != (result=Qbase_insert(new_tbl,new_buf,new_buf_size))){
					printf("Error inserting record: %s\n",QBERROR[result]);
					printf("\n!!!   New table may be CORRUPTED   !!!\n\n");
					return 0;
				}
			}
		}/* end while */
		if(result != QBASE_E_LASTROW){
			printf("Error occured reading old table: %s\n",QBERROR[result]);
			return 0;
		}
		
	}
	
	
	
	printf("Done.\n");
	
	return 1;
}














/* EOF */

