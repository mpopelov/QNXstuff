/*
 * A program to test the database
 */

#include	<stdio.h>
#include	"qbase_client.h"





int main()
{
	QB_connid_t			connection, connection1;
	QB_result_t			result;
	Qbase_table_desc_t*	TTable, * WTable;
	int					RowSize,i;
	char *				Row, *URule;
	uint32_t			*PK, *V;
	Qbase_SrvInfo_t * SrvInfo;
	
	
	
	
	/*
	 * Perform testing
	 */
	
	printf("Testing Qbase...\n");
	
	/* 
	 * ################################
	 * 1) open connection to the server
	 * ################################
	 */
	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &connection)) ){
		printf("%s", QBERROR[result]);
		return 0;
	}
	printf("Connection establishment... OK\n");
	result = Qbase_errno( connection );
	printf("Last operation resulted in: %s\n", QBERROR[result]);
	
	if( QBASE_E_OK != (result = Qbase_SrvInfo( connection, &SrvInfo )) ){
		printf("Unable to get server info: %s", QBERROR[result]);
		return 0;
	}
	/* print some server info */
	printf("----- Connected to server: -----\n");
	printf("Version:              %u\n", SrvInfo->Version);
	printf("Length of message:    %u\n", SrvInfo->MsgLen);
	printf("Length of table name: %u\n", SrvInfo->TblNameLen);
	printf("Server startup time:  %s\n", ctime(&SrvInfo->StartTime));
	printf("--------------------------------\n\n");
	
	getchar();
	
	/* 
	 * #########################
	 * 2) Test creating table 
	 * #########################
	 */
	TTable = Qbase_new_tbldesc( 5, 5, 3, "TestTable" );
	if( TTable == NULL ){
		printf("ERROR: Can't allocate new table descriptor.\n");
		return 0;
	}
	Qbase_set_field_metrics( TTable, 1, QBASE_TYPE_UINT, 0 );
	Qbase_set_field_metrics( TTable, 2, QBASE_TYPE_INT, 0 );
	Qbase_set_field_metrics( TTable, 3, QBASE_TYPE_DOUBLE, 0 );
	Qbase_set_field_metrics( TTable, 4, QBASE_TYPE_TIMESTAMP, 0 );
	Qbase_set_field_metrics( TTable, 5, QBASE_TYPE_BINARY, 64 );
	if( QBASE_E_OK != (result = Qbase_table_create(connection,TTable)) ){
		printf("%s", QBERROR[result]);
		if( result != QBASE_E_ALREADY ) return 0;
	}
	
	/* table was created - now test selecting table */

	/* 
	 * ########################
	 * 3) Test selecting table
	 * ########################
	 */
	result = Qbase_table_select( connection, "Some table Some table Some tble HA!" );
	printf("Selecting table with too long name: %s\n", QBERROR[result]);
	result = Qbase_table_select( connection, NULL );
	printf("Selecting table with empty name (NULL): %s\n", QBERROR[result]);
	result = Qbase_table_select( connection, "" );
	printf("Selecting table with empty name: %s\n", QBERROR[result]);
	
	/* this should result in not selecting - the table is already selected */
	result = Qbase_table_select( connection, "TestTable" );
	printf("Selecting table with correct name: %s\n", QBERROR[result]);
	
	/* close connection to flush table to disk */
	if( QBASE_E_OK != (result = Qbase_disconnect( connection )) ){
		printf("%s", QBERROR[result]);
		return 0;
	}
	
	/* now open 2 concurrent connections */
	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &connection)) ){
		printf("connection: %s", QBERROR[result]);
		return 0;
	}
	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &connection1)) ){
		printf("connection1: %s", QBERROR[result]);
		return 0;
	}
	
	/* select table for connection - should load it from disk */
	result = Qbase_table_select( connection, "TestTable" );
	printf("Selecting table for connection: %s\n", QBERROR[result]);
	
	/* select the same table for connection1 - should result in load from memory */
	result = Qbase_table_select( connection1, "TestTable" );
	printf("Selecting table for connection1: %s\n", QBERROR[result]);
	
	/* Close connections */
	if( QBASE_E_OK != (result = Qbase_disconnect( connection )) ){
		printf("%s", QBERROR[result]);
		return 0;
	}
	if( QBASE_E_OK != (result = Qbase_disconnect( connection1 )) ){
		printf("%s", QBERROR[result]);
		return 0;
	}
	printf("Connection close... OK\n");
	
	
	
	
	/*
	 * ######################
	 * 4) Test data insertion
	 * ######################
	 */
	printf("\n\nTest Data Insertion\n\n");
	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &connection)) ){
		printf("%s", QBERROR[result]);
		return 0;
	}
	result = Qbase_errno( connection );
	printf("Last operation resulted in: %s\n", QBERROR[result]);
	
	/* selecting test table */
	result = Qbase_table_select( connection, "TestTable" );
	printf("Selecting table for connection: %s\n", QBERROR[result]);
	
	/* getting its caps */
	result = Qbase_table_caps( connection, &WTable );
	printf("Retrieving table info for connection: %s\n", QBERROR[result]);
	
	/* print info on the table */
	printf("------------- TABLE INFO ------------\n");
	printf("Size of Table caption: %u\n", WTable->Size);
	printf("Table name: %s\n", WTable->Name);
	printf("Number of fields: %u\n", WTable->FieldNum);
	/* now print field info */
	for(i=0; i<WTable->FieldNum; i++)
		printf("Field %i: Type %u, Length %u, Offset %u\n", i, WTable->Fields[i].Type,WTable->Fields[i].Length,WTable->Fields[i].Offset);
		
	printf("-------------------------------------\n");
	
	
	/* allocating buffer for new row of data */
	Row = Qbase_row_newbuffer( WTable, &RowSize );
	if( Row == NULL ){
		printf("Was unable to allocate memory for a new row\n");
		return 0;
	}
	printf("Allocated new buffer for row: %i bytes\n", RowSize);

	/* get the pointers to Primary Key and Value in the row */
	PK = (uint32_t*)_QBASE_FIELD_OFFSET( 0, WTable, Row );
	V  = (uint32_t*)(Row + WTable->Fields[1].Offset);
	
	/* insert 250 rows of data */
	for(i = 0; i< 40; i++ ){
		/* renew field */
		/*memset( Row, 0, RowSize );*/
		/* insert data */
		result = Qbase_field_set(WTable,Row,1,&i,sizeof(int));
		if( result != QBASE_E_OK ){
			printf("Error while settng value in row of data: %s", QBERROR[result]);
			return 0;
		}
		result = Qbase_insert( connection, Row, RowSize );
		if( result != QBASE_E_OK ){
			printf("Error while inserting to database: %s", QBERROR[result]);
			return 0;
		}
	}
	
	
	
	
	
	/* define some filters */
	result = Qbase_filter_clear( connection );
	printf("Deleting filter rules resulted in: %s\n", QBERROR[result] );
	
	/* now say we want to fetch all rows, containing column #1 value
	 * 40 <= Value <= 50 */
	result = 14;
	result = Qbase_filter_push( connection, Q_CMP_L | Q_CMP_EQ , 1, sizeof(result), &result );
	printf("Pushing filter rule resulted in: %s\n", QBERROR[result] );
	result = 6;
	result = Qbase_filter_push( connection, Q_CMP_G | Q_CMP_EQ , 1, sizeof(result), &result );
	printf("Pushing filter rule resulted in: %s\n", QBERROR[result] );
	
	
	
	
	
	/* now read all the data we can from table */
	while( QBASE_E_OK == (result = Qbase_fetch(connection, Row, RowSize)) ){
		/* print some info on the record retrieved */
		printf("Got record with primary key: %u, value: %u\n", *PK, *V);
		getchar();
	}
	printf("Data fetching resulted in: %s", QBERROR[result]);
	
	
	/* now delete all the data we fetched recently */
	if( QBASE_E_OK != ( result=Qbase_delete(connection) ) ){
		printf("Deleting data resulted in: %s\n", QBERROR[result]);
		return 0;
	}
	printf("Deleted some rows of data!!!\n");
	
	
	/* now try to fetch the same data again - there should be no rows */
	while( QBASE_E_OK == (result = Qbase_fetch(connection, Row, RowSize)) ){
		printf("Got record with primary key: %u, value: %u\n", *PK, *V);
		getchar();
	}
	printf("Data fetching resulted in: %s", QBERROR[result]);
	
	
	
	/* now set database to filter all rows with column #1:
	 * 20 <= Value <= 3000 */
	Qbase_filter_clear( connection ); /* first - clear filters */
	result = 33;
	result = Qbase_filter_push( connection, Q_CMP_L | Q_CMP_EQ , 1, sizeof(result), &result );
	printf("Pushing filter rule resulted in: %s\n", QBERROR[result] );
	result = 12;
	result = Qbase_filter_push( connection, Q_CMP_G | Q_CMP_EQ , 1, sizeof(result), &result );
	printf("Pushing filter rule resulted in: %s\n", QBERROR[result] );
	
	
	/* update the data */
	result = 65535;
	Qbase_GetUpdateHeader( WTable, &URule );
	Qbase_SetUpdateField( WTable, 1, URule, &result, sizeof(result), QBASE_U_SET );

	if( QBASE_E_OK != ( result=Qbase_update(connection, URule) ) ){
		printf("Updating data resulted in: %s\n", QBERROR[result]);
		return 0;
	}
	printf("Updated some rows of data!!!\n");
	
	
	/* now try to fetch the same data again - there should be no rows */
	while( QBASE_E_OK == (result = Qbase_fetch(connection, Row, RowSize)) ){
		/* print some info on the record retrieved */
		printf("Got record with primary key: %u, value: %u\n", *PK, *V);
		getchar();
	}
	printf("Data fetching resulted in: %s", QBERROR[result]);
	
	/* now try to fetch all the data and see what we got */
	Qbase_filter_clear( connection );
	while( QBASE_E_OK == (result = Qbase_fetch(connection, Row, RowSize)) ){
		/* print some info on the record retrieved */
		printf("Got record with primary key: %u, value: %u\n", *PK, *V);
		getchar();
	}
	printf("Data fetching resulted in: %s", QBERROR[result]);
	
	
	/* flushing the table */	
	result = Qbase_flush( connection );
	printf("Flushing resulted in: %s\n", QBERROR[result]);
	
	getchar();
	
	/* see how Qbase_seek is working */
	Qbase_filter_clear( connection );
	Qbase_fetchrst(connection );
	result = Qbase_seek( connection, 18 );
	printf("Data seeking resulted in: %s", QBERROR[result]);
	while( QBASE_E_OK == (result = Qbase_fetch(connection, Row, RowSize)) ){
		/* print some info on the record retrieved */
		printf("Got record with primary key: %u, value: %u\n", *PK, *V);
		getchar();
	}
	printf("Data fetching resulted in: %s", QBERROR[result]);
	
	
	getchar();
			
	return 0;
}
