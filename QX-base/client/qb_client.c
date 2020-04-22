/*
 * A program to test the database
 */

#include	<stdio.h>
#include	"qbase_client.h"





int main()
{
	QB_connid_t			connection;
	QB_result_t			result;
	int					RowSize,i;
	char				buffer[255];
	char				command;
	
	Qbase_table_desc_t*	WTable	= NULL;
	Qbase_SrvInfo_t *	SrvInfo	= NULL;
	char *				Row		= NULL;
	char *				Rule	= NULL;
	
	
	
	printf("Qbase Client. (c) -=MikeP=- 2004\n\n");
	
	
	/*############################
	 * Get connected to the server
	 *############################*/
	
	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &connection)) ){
		printf("%s", QBERROR[result]);
		return 0;
	}
	
	/*############################
	 * Get & show server info
	 *############################*/
	 
	if( QBASE_E_OK != (result = Qbase_SrvInfo( connection, &SrvInfo )) ){
		printf("Unable to get server info: %s", QBERROR[result]);
		return 0;
	}
	printf("----- Connected to server: -----\n");
	printf("Version:              %u\n", SrvInfo->Version);
	printf("Length of message:    %u\n", SrvInfo->MsgLen);
	printf("Length of table name: %u\n", SrvInfo->TblNameLen);
	printf("Server startup time:  %s\n", ctime(&SrvInfo->StartTime));
	printf("--------------------------------\n\n");
	

	while(1){
		
		/* Show menu */
		printf("\n--= Menu =------------------\n");
		printf("----------------------------\n");
		printf("1. Select Table\n");
		printf("2. Create Table\n");
		printf("3. Insert record(s)\n");
		printf("4. Delete record(s)\n");
		printf("5. Update record(s)\n");
		printf("6. Read from table\n");
		printf("7. Define filter\n");
		printf("8. Clear filter\n");
		printf("9. Reset reading position\n");
		printf("0. Flush table\n");
		printf("q. Quit\n");
		printf("\n>> ");
		
		/* read command */
		command = getchar();
		getchar(); /* to eliminate newline character */
		
		switch( command ){
			case '1':
			{
				/* Select table for operation */
				printf("Name of table to select:\n");
				scanf("%s",buffer);
				if( QBASE_E_OK != (result = Qbase_table_select( connection, buffer ))){
					printf("Select Table resulted in: %s\n", QBERROR[result]);
					getchar();
					break;
				}
				
				if( WTable != NULL ) free( WTable );
				
				/* Get table info */
				if( QBASE_E_OK != (result = Qbase_table_caps( connection, &WTable ))){
					printf("Retrieving table info for connection: %s\n", QBERROR[result]);
					getchar();
					break;
				}
				
				/* Show table info */
				printf("------------- TABLE INFO ------------\n");
				printf("Size of table caption:     %u\n", WTable->Size);
				printf("Table name:                %s\n", WTable->Name);
				printf("Length of one row of data: %u (bytes)\n", WTable->RowLen);
				printf("Concurrent connections:    %u\n", WTable->RefCounter);
				printf("Current row number:        %u\n", WTable->Index);
				printf("Number of rows per page:   %u\n", WTable->PageRows);
				printf("Number of fields:          %u\n", WTable->FieldNum);
				/* now print field info */
				for(i=0; i<WTable->FieldNum; i++)
					printf("Field %i: Type %u, Length %u, Offset %u\n", i, WTable->Fields[i].Type,WTable->Fields[i].Length,WTable->Fields[i].Offset);
				printf("-------------------------------------\n");
				printf("This is now your current working table. Press any key to continue\n");
				
				/* clean anything left from previous table */
				if( Row != NULL ) free( Row );
				if( Rule != NULL ) free( Rule );
				RowSize = 0;
				
				/* get new buffers */
				Row = Qbase_row_newbuffer( WTable, &RowSize );
				if( Row == NULL ){
					printf("Unable to allocate memory for a new row. Exiting.\n");
					return 0;
				}
				
				getchar();
				break;
			}
			case '2':
			{
				/* Create a new table */
				Qbase_table_desc_t*	CTable;
				uint32_t			NumFields;
				uint32_t			RowsPP;
				uint32_t			FieldType, FieldLen;
				uint32_t			LockPages;
				
				printf("Creating table\n");
				
				/* allocate new descriptor */
				printf("New table name:\n");
				scanf("%s",buffer);
				printf("Number of fields in new table:\n");
				scanf("%u", &NumFields);
				printf("Number of rows per page:\n");
				scanf("%u", &RowsPP);
				printf("Number of pages to lock at one time:\n");
				scanf("%u", &LockPages);
				CTable = Qbase_new_tbldesc( NumFields, RowsPP, LockPages, buffer );
				if( CTable == NULL ){
					printf("ERROR: Can't allocate new table descriptor.\n");
					getchar();
					break;
				}
				
				/* fill in the types of table fields */
				for( i=1; i<=NumFields; i++ ){
					/* print available field types */
					printf("QBASE_TYPE_UINT: %u\n", (uint32_t)QBASE_TYPE_UINT );
					printf("QBASE_TYPE_INT: %u\n", (uint32_t)QBASE_TYPE_INT );
					printf("QBASE_TYPE_UINT16: %u\n", (uint32_t)QBASE_TYPE_UINT16 );
					printf("QBASE_TYPE_INT16: %u\n", (uint32_t)QBASE_TYPE_INT16 );
					printf("QBASE_TYPE_DOUBLE: %u\n", (uint32_t)QBASE_TYPE_DOUBLE );
					printf("QBASE_TYPE_TIMESTAMP: %u\n", (uint32_t)QBASE_TYPE_TIMESTAMP );
					printf("QBASE_TYPE_BINARY: %u\n", (uint32_t)QBASE_TYPE_BINARY );
					printf("QBASE_TYPE_STRING: %u\n", (uint32_t)QBASE_TYPE_STRING );
					
					/* scan the field type and length */
					printf("Type of field:\n");
					scanf("%u", &FieldType);
					printf("Length of field in bytes:\n");
					scanf("%u", &FieldLen);
					
					/* Set corresponding field metrics */
					Qbase_set_field_metrics( CTable, i, FieldType, FieldLen );
				}
				
				/* Try to create defined table */
				if( QBASE_E_OK != (result = Qbase_table_create(connection,CTable)) ){
					printf("%s", QBERROR[result]);
					free( CTable );
					getchar();
					break;
				}
				
				printf("Table created successfully. Now it is your working table.\n");
				
				getchar();
				break;
			}
			
			case '3':
			{
				/* INSERT */
				int FieldNum = 0;
				char * Row = NULL;
				uint32_t RowSize = 0;
				
				if( WTable == NULL ){
					printf("You should create or select a table to operate\n");
					break;
				}
				printf("Inserting record to table %s\n", WTable->Name);
				
				/* allocate memory for a new row of data */
				Row = Qbase_row_newbuffer( WTable, &RowSize );
				if( Row == NULL ){
					printf("Was unable to allocate memory for a new row\n");
					break;
				}
				
				/* scan values to insert into the table */
				for( FieldNum=1; FieldNum<WTable->FieldNum; FieldNum++){
					char * FPointer = (Row + WTable->Fields[FieldNum].Offset);
					
					/* scan the value */
					switch(WTable->Fields[FieldNum].Type){
						case QBASE_TYPE_UINT:
							{
								printf("Input unsigned integer:\n");
								scanf("%u", FPointer);
								getchar();
								break;
							}
						case QBASE_TYPE_INT:
							{
								printf("input signed integer:\n");
								scanf("%i", FPointer);
								getchar();
								break;
							}
						case QBASE_TYPE_UINT16:
							{
								printf("input unsigned short integer:\n");
								scanf("%hu", FPointer);
								getchar();
								break;
							}
						case QBASE_TYPE_INT16:
							{
								printf("input signed short integer:\n");
								scanf("%hi", FPointer);
								getchar();
								break;
							}
						case QBASE_TYPE_DOUBLE:
							{
								printf("Input double:\n");
								scanf("%lf", FPointer);
								getchar();
								break;
							}
						case QBASE_TYPE_TIMESTAMP:
							{
								printf("No input for timestamp from console - skipped\n");
								break;
							}
						case QBASE_TYPE_BINARY:
							{
								printf("No input for binary data from console - skipped\n");
								break;
							}
						case QBASE_TYPE_STRING:
							{
								printf("input a string of text (not more than %u characters):\n", WTable->Fields[FieldNum].Length);
								gets(FPointer);
								break;
							}
					}
				}
				
				/* insert into the table */
				result = Qbase_insert( connection, Row, RowSize );
				if( result != QBASE_E_OK ){
					printf("Error while inserting to database: %s", QBERROR[result]);
				}
				
				free(Row);
				break;
			}
			
			case '4':
			{
				printf("Delete record\n");
				break;
			}
			case '5':
			{
				printf("Update record\n");
				break;
			}
			case '6':
			{
				/* retrieving data */
				Qbase_fetchrst( connection );
				while( 1 ){
					if( QBASE_E_OK != (result = Qbase_fetch(connection, Row, RowSize)) ){
						printf("Fetching row resulted in: %s\n", QBERROR[result]);
						break;
					}
					
					/* fetching OK - print row of data */
					for( i=1; i< WTable->FieldNum; i++ ){
						printf("Field[%i]:\n\t\t", i);
						switch( WTable->Fields[i].Type ){
							case QBASE_TYPE_UINT:
							{
								printf("%u\n", *((uint32_t*)(WTable->Fields[i].Offset + Row)) );
								break;
							}
							case QBASE_TYPE_INT:
							{
								printf("%i\n", *((int*)(WTable->Fields[i].Offset + Row)) );
								break;
							}
							case QBASE_TYPE_UINT16:
							{
								printf("%hu\n", *((int*)(WTable->Fields[i].Offset + Row)) );
								break;
							}
							case QBASE_TYPE_INT16:
							{
								printf("%hi\n", *((int*)(WTable->Fields[i].Offset + Row)) );
								break;
							}
							case QBASE_TYPE_DOUBLE:
							{
								printf("%f\n", *((double*)(WTable->Fields[i].Offset + Row)) );
								break;
							}
							case QBASE_TYPE_TIMESTAMP:
							{
								printf("%s\n", ctime((uint32_t*)(WTable->Fields[i].Offset + Row)) );
								break;
							}
							case QBASE_TYPE_BINARY:
							{
								uint32_t j = 0;
								while( j<WTable->Fields[i].Length ){
									printf("%02hhx ", *(char*)(WTable->Fields[i].Offset + Row + j));
									j++;
									if( (j%16)==0 ) printf("\n\t\t");
								}
								printf("\n");
								break;
							}
							case QBASE_TYPE_STRING:
							{
								printf("%s\n",(char*)(WTable->Fields[i].Offset + Row));
								break;
							}
						}
					}
					
					/* ask if client wants more data */
					printf("\nFetch next row (y/n) ? :\n");
					command = getchar();
					getchar();
					if( command != 'y' ) break;
				}
				break;
			}
			case '7':
			{
				printf("Define filter\n");
				break;
			}
			case '8':
			{
				result = Qbase_filter_clear( connection );
				printf("Deleting filtering rules: %s\n", QBERROR[result] );
				break;
			}
			case '9':
			{
				printf("Reset reading position\n");
				break;
			}
			case '0':
			{
				printf("Flush table to disk\n");
				break;
			}
			case 'q':
			{
				printf("\nBye!\n");
				Qbase_disconnect( connection );
				return 0;
			}
			default:
			{
				printf("You should use one of the commands in Menu\n If you want to quit - use 'q' or CTRL+C\n");
				break;
			}
		}
		
		/* now show the menu again */
	}
			
	return 0;
}
