/*
 * 2006 (c) -={ MikeP }=-
 * 
 * Consystency checking of the billing databases
 * 
 */



#include	"qxl_common.h"
#include	"qbase_client.h"







char	log_tbl_name[QBASE_TAB_NAMELEN+1];


/*
 * Data definition
 */
typedef struct _NIC_item_{
	uint32_t	NID;	/* NIC ID - primary key */
	uint32_t	AID;	/* Associated account */
	uint32_t	ip;		/* Associated ip address */
	uint8_t		mac[6];	/* Associated MAC address */
}nic_item_t;



nic_item_t		*nics = NULL;
int				num_nics;











/*
 * The following are the tables we need for checking consistency.
 */

/* log table for reading */
QB_connid_t				LT_read;
Qbase_table_desc_t *	LT_read_desc = NULL;
char*					LT_read_buf = NULL;
int						LT_read_size;

/* log table for update */
QB_connid_t				LT_upd;
Qbase_table_desc_t *	LT_upd_desc = NULL;
char*					LT_upd_buf = NULL;
int						LT_upd_size;

/* NICs table for reading */
QB_connid_t				NT_read;
Qbase_table_desc_t *	NT_read_desc = NULL;
char*					NT_read_buf = NULL;
int						NT_read_size;

/* Accounts table for updates */
QB_connid_t				AT_upd;
Qbase_table_desc_t *	AT_upd_desc = NULL;
char*					AT_upd_buf = NULL;
int						AT_upd_size;





/* 
 * pointers to fields for faster access
 */
/* into read log buffer */
uint32_t		*log_N;
uint32_t		*log_NID;
uint32_t		*log_AID;
uint32_t		*log_IP;
uint8_t			*log_MAC;
double			*log_STOCLI;
double			*log_STOHST;
/* into update log buffer */
uint32_t		*ulog_NID;
uint32_t		*ulog_AID;
/* into update accounts buffer */
double			*sum_pay;










/* stats */
int		num_total_recs = 0;
int		num_fixed_recs = 0;
int		num_skipped_recs = 0;












int main(int argc, char** argv){
	
	QB_result_t		QXB_rslt;
	uint32_t		ret_val;
	int				res;
	
	
	if( argc == 1 ){
		printf("You should tell what table to check:\n");
		printf("QX-consistency <table_name>\n");
		return 0;
	}else{
		snprintf(log_tbl_name,QBASE_TAB_NAMELEN,"%s",argv[1]);
	}
	
	printf("Checking consystency of %s table...\n", log_tbl_name);
	
	/* Mount all tables */
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &LT_read)) ){
		printf("Log table: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &LT_upd)) ){
		printf("Log table: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &NT_read)) ){
		printf("NICs table: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &AT_upd)) ){
		printf("Accounts table: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	
	/* 
	 * get all needed buffers
	 */
	/* logging table */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( LT_read, log_tbl_name ))){
		printf("log read: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( LT_read, &LT_read_desc ))){
		printf("log read: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	LT_read_buf = Qbase_row_newbuffer( LT_read_desc, &LT_read_size );
	log_N		= (uint32_t*)(LT_read_buf + LT_read_desc->Fields[QXL_LOG_N].Offset);
	log_NID		= (uint32_t*)(LT_read_buf + LT_read_desc->Fields[QXL_LOG_NIC].Offset);
	log_AID		= (uint32_t*)(LT_read_buf + LT_read_desc->Fields[QXL_LOG_ACCPAY].Offset);
	log_IP		= (uint32_t*)(LT_read_buf + LT_read_desc->Fields[QXL_LOG_IPCLNT].Offset);
	log_MAC		= (uint8_t*)(LT_read_buf + LT_read_desc->Fields[QXL_LOG_MAC].Offset);
	log_STOCLI	= (double*)(LT_read_buf + LT_read_desc->Fields[QXL_LOG_STOCLI].Offset);
	log_STOHST	= (double*)(LT_read_buf + LT_read_desc->Fields[QXL_LOG_STOHST].Offset);
	
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( LT_upd, log_tbl_name ))){
		printf("log update: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( LT_upd, &LT_upd_desc ))){
		printf("log update: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_GetUpdateHeader( LT_upd_desc, &LT_upd_buf ))){
		printf("log update: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	ulog_NID = (uint32_t*)(LT_upd_buf + LT_upd_desc->Fields[QXL_LOG_NIC].Offset + LT_upd_desc->FieldNum + sizeof(uint32_t));
	ulog_AID = (uint32_t*)(LT_upd_buf + LT_upd_desc->Fields[QXL_LOG_ACCPAY].Offset + LT_upd_desc->FieldNum + sizeof(uint32_t));
	((uint8_t*)(LT_upd_buf + sizeof(uint32_t)))[QXL_LOG_NIC] = QBASE_U_SET;
	((uint8_t*)(LT_upd_buf + sizeof(uint32_t)))[QXL_LOG_ACCPAY] = QBASE_U_SET;
	
	
	/* NICs table */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( NT_read, "NICs" ))){
		printf("NICs read: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( NT_read, &NT_read_desc ))){
		printf("NICs read: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	NT_read_buf = Qbase_row_newbuffer( NT_read_desc, &NT_read_size );
	
	
	/* accounts table */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( AT_upd, "accounts" ))){
		printf("accounts update: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( AT_upd, &AT_upd_desc ))){
		printf("accounts update: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_GetUpdateHeader( AT_upd_desc, &AT_upd_buf ))){
		printf("accounts update: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	sum_pay = (double*)(AT_upd_buf + AT_upd_desc->Fields[QXL_ACCOUNTS_SUM].Offset + AT_upd_desc->FieldNum + sizeof(uint32_t));
	
	
	printf("Tables mounted...\n");
	
	
	
	/* read all NICs */
	nics = (nic_item_t*)malloc(200*sizeof(nic_item_t));
	num_nics = 0;
	Qbase_fetchrst( NT_read );
	while( QBASE_E_OK == (QXB_rslt = Qbase_fetch(NT_read,NT_read_buf,NT_read_size)) ){
		/* read nic info */
		nics[num_nics].NID	= QXB_FV_UINT( QXL_NICS_N, NT_read_desc, NT_read_buf);
		nics[num_nics].AID	= QXB_FV_UINT( QXL_NICS_ACCOUNT, NT_read_desc, NT_read_buf);
		nics[num_nics].ip	= QXB_FV_UINT( QXL_NICS_IP, NT_read_desc, NT_read_buf);
		memcpy( &nics[num_nics].mac, QXB_FV_BIN(QXL_NICS_MAC,NT_read_desc, NT_read_buf), 6 );
		
		num_nics++;
	}
	if( QXB_rslt != QBASE_E_LASTROW ){
		printf("Error reading NICs; %s\n", QBERROR[QXB_rslt]);
		return 0;
	}else{
		printf("Loaded info about %i NICs\n\n\n", num_nics);
	}
	
	
	
	/* 
	 * scan through logging table and check for differences
	 */
	Qbase_fetchrst( LT_read );
	num_total_recs = 0;
	num_fixed_recs = 0;
	num_skipped_recs = 0;
	while( QBASE_E_OK == (QXB_rslt = Qbase_fetch(LT_read,LT_read_buf,LT_read_size)) ){
		
		int nic_i = 0;
		int	nic_new = 0;
		
		/* find the NIC that was assigned by filter */
		num_total_recs++;
		printf("\b\rChecking record [%10u] ...\n", *log_N);
		
		
		for( nic_i=0; nic_i<num_nics; nic_i++ ){
			if( nics[nic_i].NID == *log_NID ) break;
		}
		if( nic_i == num_nics ){
			printf("CRITICAL: can't find NIC for record %u currently pointing to NIC %u\n",*log_N,*log_NID);
			return 0;
		}
		
		/* Check if found NIC matches logged data */
		if( nics[nic_i].ip == *log_IP ){
			/* ip matched. check for mac */
			if(0 == memcmp(log_MAC,nics[nic_i].mac,6) ){
				/* MAC also matched - we have nothing to do here
				 * just tune stats and go to the next record */
				continue;
			}
		}
		
		/* the NIC in log does not match log contents */
		printf("\b\r! Fixing record [%10u] ...\n",*log_N);
		for( nic_new=0; nic_new<num_nics; nic_new++ ){
			if(0 == memcmp(log_MAC,nics[nic_new].mac,6) ){
				if( nics[nic_new].ip == *log_IP ){
					break;
				}
			}
		}
		if( nic_new == num_nics ){
			printf("CRITICAL: log record %u mismatches NIC %u\n",*log_N,*log_NID);
			printf("CRITICAL: but no matching NIC found!!!!\n");
			printf("Skipping...\n\n");
			num_skipped_recs++;
			continue;
		}
		
		/* we have found appropriate nic - fix accounts and log record */
		{
			/* 1) get correct sum and update accounts */
			*sum_pay = (*log_STOCLI) + (*log_STOHST);
			/* substract from new account */
			((uint8_t*)(AT_upd_buf + sizeof(uint32_t)))[QXL_ACCOUNTS_SUM] = QBASE_U_SUB;
			if( QBASE_E_OK != (QXB_rslt = Qbase_seek(AT_upd,nics[nic_new].AID)) ){
				printf("ERROR account(%u): %s\n", nics[nic_new].AID, QBERROR[QXB_rslt]);
				return 0;
			}
			if( QBASE_E_OK != (QXB_rslt = Qbase_updcur(AT_upd,AT_upd_buf)) ){
				printf("ERROR updating account(%u): %s\n", nics[nic_new].AID, QBERROR[QXB_rslt]);
				return 0;
			}
			
			
			/* add to mismatched account */
			((uint8_t*)(AT_upd_buf + sizeof(uint32_t)))[QXL_ACCOUNTS_SUM] = QBASE_U_ADD;
			if( QBASE_E_OK != (QXB_rslt = Qbase_seek(AT_upd,nics[nic_i].AID)) ){
				printf("ERROR account(%u): %s\n", nics[nic_i].AID, QBERROR[QXB_rslt]);
				return 0;
			}
			if( QBASE_E_OK != (QXB_rslt = Qbase_updcur(AT_upd,AT_upd_buf)) ){
				printf("ERROR updating account(%u): %s\n", nics[nic_i].AID, QBERROR[QXB_rslt]);
				return 0;
			}
			
			/* 2) update logging table */
			*ulog_AID = nics[nic_new].AID;
			*ulog_NID = nics[nic_new].NID;
			if( QBASE_E_OK != (QXB_rslt = Qbase_seek(LT_upd,*log_N)) ){
				printf("ERROR log(%u): %s\n", *log_N, QBERROR[QXB_rslt]);
				return 0;
			}
			if( QBASE_E_OK != (QXB_rslt = Qbase_updcur(LT_upd,LT_upd_buf)) ){
				printf("ERROR updating log(%u): %s\n", *log_N, QBERROR[QXB_rslt]);
				return 0;
			}
			
		}
		
		/* move on to the next record in log */
		num_fixed_recs++;
	}
	
	/* report statistics */
	printf("\n\n\n---RESULTS---\n\n");
	printf("Total records in checked table: %15i\n", num_total_recs);
	printf("Total records fixed in table:   %15i\n", num_fixed_recs);
	printf("Total records skipped:          %15i\n", num_skipped_recs);
	printf("-------------\n");
	
	return 1;
}

/* EOF */
