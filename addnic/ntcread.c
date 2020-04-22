/*
 * NFM-NTC.SO reader by -=MikeP=-
 * Copyright (C) 2003, 2004  Mike Anatoly Popelov ( AKA -=MikeP=- )
 * 
 * -=MikeP=- <mikep@kaluga.org>
 * 
 * You are free to use this code in the way you like and for any purpose,
 * not deprecated by your local law. I will have now claim for using this code
 * in any project, provided that the reference to me as the author provided.
 * I also DO NOT TAKE ANY RESPONSIBILITY if this code does not suite you in your
 * particular case. Though this code was written to be stable and working,
 * I will take NO RESPONSIBILITY if it does not work for you, on your particular
 * equipment, in your particular configuration and environment, if it brings any
 * kind of damage to your equipment.
 */

/*
 * This program is part of nfm-ntc.so
 * Its purpose is to show the results of filter work
 * and demonstrate how one can use devctl()
 * to manipulate some aspects of filter job
 * and get its statistics
 */
 
#include	"ntc_common.h"
#include	"logging.h"
#include	"qbase_client.h"
#include	<fcntl.h>


char			MAC[6] = {0x00, 0x0a, 0x5e, 0x05, 0x31, 0x13};
uint32_t		account = 2;
uint32_t		closed = 0;




/* required to manage NICs */
QB_connid_t			nic_con;
Qbase_table_desc_t*	NicTable	= NULL;
uint32_t			nicRowSize;
char *				nicRow;



int main()
{
	int			ntc_fd;
	int			size_read;
	int			npkts;
	pkt_info_t	packet[MAX_PKT_READ];
	QB_result_t	result;
	int			ItIsTimeToLog = 0;


	printf("Starting Up...\n");

	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &nic_con)) ){
		printf("%s\n", QBERROR[result]);
		exit(-1);
	}
	
	printf("Connections to database (Qbase) opened successfully\n");	
	
	if( QBASE_E_OK != (result = Qbase_table_select( nic_con, "NICS" ))){
		printf("ACCOUNTS table inaccessible: %s\n", QBERROR[result]);
		exit(-1);
	}
	
	printf("Tables selected successfully\n");
	
	/* Get table descriptors */
	if( QBASE_E_OK != (result = Qbase_table_caps( nic_con, &NicTable ))){
		printf("Table NICS: %s\n", QBERROR[result]);
		exit(-1);
	}
	
	printf("Table descriptors retrieved successfully\n");
	
	
	/* add record to unblock corresponding NICs */
	nicRow = Qbase_row_newbuffer( NicTable, &nicRowSize );
	if( nicRow == NULL ){
		printf("Was unable to allocate memory for a new row\n");
		return 0;
	}
	printf("Allocated new buffer for row: %i bytes\n", nicRowSize);
	
	/* set MAC */
	result = Qbase_field_set(NicTable,nicRow,1,MAC,6);
	if( result != QBASE_E_OK ){
		printf("Error while settng MAC value in row of data: %s", QBERROR[result]);
		return 0;
	}
	/* set closed state */
	result = Qbase_field_set(NicTable,nicRow,2,&closed,sizeof(uint32_t));
	if( result != QBASE_E_OK ){
		printf("Error while settng CLOSED value in row of data: %s", QBERROR[result]);
		return 0;
	}
	/* set account number */
	result = Qbase_field_set(NicTable,nicRow,3,&account,sizeof(uint32_t));
	if( result != QBASE_E_OK ){
		printf("Error while settng ACCOUNT value in row of data: %s", QBERROR[result]);
		return 0;
	}
	
	result = Qbase_insert( nic_con, nicRow, nicRowSize );
	if( result != QBASE_E_OK ){
		printf("Error while inserting to database: %s", QBERROR[result]);
		return 0;
	}
	
	
	
	exit(0);
	
} /* int main() */


/* EOF */
