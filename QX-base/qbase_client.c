/*
 * -=MikeP=- (c)
 */

#include	"qbase_client.h"


/*
 * Here are client functions and macroses
 */


char * const QBERROR[] =
{
	"No error\n",
	"ERROR: Unable to connect or connection already closed\n",
	"ERROR: Client-side buffer has incufficient space\n",
	"ERROR: Database server was unable to allocate memory to fulfill request\n",
	"ERROR: The server was unable to understand the request\n",
	"ERROR: Current connection hasn't selected table to manipulate\n",
	"ERROR: No such table exist\n",
	"ERROR: Trying to create table that already exists\n",
	"ERROR: Trying to manage (flush) table while it is in use\n",
	"ERROR: Too few pages requested to lock in memory\n",
	"ERROR: Desired column number exceeds number of columns in current table\n",
	"ERROR: Bad record size to insert into selected table\n",
	"ERROR: The specified field number is out of range for current table\n",
	"INFO:  Last FETCH() was successful and found that the last row was reached\n",
	"ERROR: !!! CRITICAL !!! for some reason server was unable to place the datapage in memory\n",
	"ERROR: the specified Primary Key is out of range\n",
	NULL
};











void Qbase_fixtbl( Qbase_table_desc_t * Tbl )
{
	int i = 0;
	int	CurOffset = 0;
	
	for( i=0; i<Tbl->FieldNum; i++ ){
		Tbl->Fields[i].Offset = CurOffset;
		CurOffset += Tbl->Fields[i].Length;
	}
	
	Tbl->RowLen = CurOffset;
}























/*
 * 
 * General processing functions
 * 
 */

QB_result_t Qbase_connect( char * Server, QB_connid_t * Connection )
{
	*Connection = open( Server, O_RDWR );
	
	/* if erro occured - return error code */
	if( *Connection == -1 ) return QBASE_E_NOCONN;
	
	return QBASE_E_OK;
}

QB_result_t Qbase_disconnect( QB_connid_t Connection )
{
	close( Connection );
	return QBASE_E_OK;
}

QB_result_t Qbase_errno( QB_connid_t Connection ){
	int			result;
	QB_result_t Qbase_Code;
	
	result = devctl( Connection, QBASE_CMD_ERROR, NULL, 0, &Qbase_Code );
	
	if(result != EOK) return QBASE_E_NOCONN;
	return Qbase_Code;
}





/*
 * 
 * Functions, concerning tables
 * 
 */
Qbase_table_desc_t * Qbase_new_tbldesc( int ColNum, int RowsPP, int LockPages,  char * TName )
{
	Qbase_table_desc_t *	NewTable;
	int						Size;
	
	Size = sizeof(Qbase_table_desc_t) + ColNum*sizeof(Qbase_field_desc_t);
	
	NewTable = (Qbase_table_desc_t*)malloc( Size );
	if( NewTable == NULL ) return NULL;
	
	memset( NewTable, 0, Size );
	
	/* set table values to initial meaning */
	NewTable->Version = 1;
	NewTable->Size = Size;
	NewTable->PageRows = RowsPP;
	NewTable->LockPages = LockPages;
	memcpy( NewTable->Name, TName, min(QBASE_TAB_NAMELEN, strlen(TName)) );
	NewTable->FieldNum = ColNum+1;
	NewTable->Index = 0;
	
	/* set field lengths to default */
	for( Size=0; Size<=ColNum; Size++ ){
		NewTable->Fields[Size].Length = 4; /* defaults to QBASE_TYPE_UINT */
	}
	
	/* fix offsets and row length */
	Qbase_fixtbl( NewTable );
	
	return NewTable;
}

int Qbase_set_field_metrics( Qbase_table_desc_t * Tbl, int ColNum, uint32_t Type, uint32_t Length )
{
	uint32_t len = Length;
	
	if( ColNum >= Tbl->FieldNum ) return 0; /* we should not try to set "out of range" fields */
	
	Tbl->Fields[ColNum].Type = Type;
	
	if( Type < QBASE_TYPE_VARLEN )
		switch(Type){
			case QBASE_TYPE_INT:
				len = sizeof(int);
				break;
				
			case QBASE_TYPE_UINT:
				len = sizeof(uint32_t);
				break;
			
			case QBASE_TYPE_INT16:
				len = sizeof(int16_t);
				break;
				
			case QBASE_TYPE_UINT16:
				len = sizeof(uint16_t);
				break;
			
			case QBASE_TYPE_DOUBLE:
				len = sizeof(double);
				break;
				
			case QBASE_TYPE_TIMESTAMP:
				len = sizeof(struct timespec);
				break;
		}
		
	Tbl->Fields[ColNum].Length = len;
	
	Qbase_fixtbl( Tbl );
	return 1;
}


QB_result_t Qbase_table_select( QB_connid_t Connection, char * Tablename )
{
	int			result;
	int			i;
	QB_result_t	Qbase_Code;
	
	if( Tablename == NULL ) return QBASE_E_BUF;
	
	/* limit length of name to minimum of strlen() or QBASE_TAB_NAMELEN */
	for( i=0; (Tablename[i] != 0); i++);
	if( (i >= QBASE_TAB_NAMELEN) || (i == 0) ) return QBASE_E_BUF;
	
	result = devctl( Connection, QBASE_CMD_TBLSELECT, Tablename, (i+1) , &Qbase_Code );

	if(result != EOK) return QBASE_E_NOCONN;
	return Qbase_Code;
}


QB_result_t Qbase_table_create( QB_connid_t Connection, Qbase_table_desc_t * Table )
{
	int			result;
	int			i;
	QB_result_t Qbase_Code;
	
	if( Table == NULL ) return QBASE_E_BUF;
	
	result = devctl( Connection, QBASE_CMD_TBLCREATE, Table, Table->Size , &Qbase_Code );
	if(result != EOK) return QBASE_E_NOCONN;
	return Qbase_Code;
}


/*
 * Retrieve caption of currently selected table
 */
QB_result_t Qbase_table_caps( QB_connid_t Connection, Qbase_table_desc_t ** CTable )
{
	uint32_t	HSize;
	int			result;
	QB_result_t	Qbase_Code;
	
	/* 1) get number of bytes header contains */
	result = devctl( Connection, QBASE_CMD_TBLCAPSLEN, &HSize, sizeof(uint32_t), &Qbase_Code );
	if( result != EOK ) return QBASE_E_NOCONN;
	if( Qbase_Code != QBASE_E_OK ) return Qbase_Code;
		
	/* 2) allocate memory for table descriptor */
	*CTable = (Qbase_table_desc_t*)malloc(HSize);
	if( *CTable == NULL ) return QBASE_E_MEM;
	
	/* 3) read table  descriptor from table */
	result = devctl( Connection, QBASE_CMD_TBLCAPS, *CTable, HSize, &Qbase_Code );
	if( result != EOK ){
		free(*CTable);
		*CTable = NULL;
		return QBASE_E_NOCONN;
	}
	if( Qbase_Code != QBASE_E_OK ){
		free(*CTable);
		*CTable = NULL;
		return Qbase_Code;
	}
	
	/* 4) happily return */
	return QBASE_E_OK;
}









/*
 * Functions dealing with data manipulation
 */

QB_result_t	Qbase_fetchrst( QB_connid_t Connection )
{
	int			result;
	QB_result_t Qbase_Code;

	result = devctl( Connection, QBASE_CMD_FETCHRST, NULL, 0, &Qbase_Code );
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}

/* set the position in database to the specified primary key */
QB_result_t Qbase_seek( QB_connid_t Connection, uint32_t PKey )
{
	int			result;
	QB_result_t	Qbase_Code;
	uint32_t	p_key = PKey;
	
	result = devctl( Connection, QBASE_CMD_SEEK, &p_key, sizeof(uint32_t), &Qbase_Code );
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}


char * Qbase_row_newbuffer( Qbase_table_desc_t * Table, int* Rowsize)
{
	if( Table == NULL ) return NULL;
	*Rowsize = Table->RowLen;
	return ((char*)malloc(*Rowsize));
}


/* set the value of the field in current table */
QB_result_t	Qbase_field_set( Qbase_table_desc_t * WTable, char * Row, int FNum, void * Value, uint32_t Size )
{
	int nbytes;
	if( (WTable == NULL) || (Row==NULL) || (Value==NULL) ) return QBASE_E_BUF;
	if( FNum >= WTable->FieldNum ) return QBASE_E_NOFLD;
	
	nbytes = min(WTable->Fields[FNum].Length,Size);
	memcpy( (Row + WTable->Fields[FNum].Offset), Value, nbytes );
	
	return QBASE_E_OK;
}


/* Write the row to database */
QB_result_t Qbase_insert( QB_connid_t Connection, char* Row, uint32_t Size )
{
	int	nbytes;
	
	if( (Row==NULL) || (Size==0) ) return QBASE_E_BUF;
	nbytes = write( Connection, Row, Size );
	if( nbytes != Size ){
		return Qbase_errno( Connection ); /* if there is an error - find what is the error */
	}
	
	return QBASE_E_OK;
}


/* Read one row of info from database */
QB_result_t Qbase_fetch( QB_connid_t Connection, char* Row, uint32_t Size )
{
	int			result;
	
	if( (Row == NULL) || (Size == 0) ) return QBASE_E_BUF;
	
	result = read(Connection, Row, Size);
	if( result != Size ){
		return Qbase_errno( Connection );
	}
	
	return QBASE_E_OK;
}


/* Push filter rule to the stack of rules */
QB_result_t	Qbase_filter_push( QB_connid_t Connection, uint32_t Flags, uint32_t FNum, uint32_t VSize, void * Value )
{
	int			result;
	uint32_t	Qbase_Code;
	Qbase_filter_t * NFilter;
	
	/* 1) allocate memory to pass filter rule */
	if( (NFilter = (Qbase_filter_t*)malloc(sizeof(Qbase_filter_t)+VSize)) == NULL ){
		return QBASE_E_MEM;
	}
	
	/* 2) Fine tune filter rule */
	memcpy( NFilter->Value, Value, VSize );
	NFilter->FNum = FNum;
	NFilter->Flags = Flags;
	NFilter->VSize = VSize;
	
	/* 3) pass filter to database server */
	result = devctl( Connection, QBASE_CMD_FLTRDEFINE, NFilter, (sizeof(Qbase_filter_t)+VSize), &Qbase_Code );
	free( NFilter );
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}


/* Clear the current ruleset */
QB_result_t	Qbase_filter_clear( QB_connid_t Connection )
{
	int			result;
	uint32_t	Qbase_Code;
	
	result = devctl( Connection, QBASE_CMD_FLTRDELETE, NULL, 0, &Qbase_Code );
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}

/* pop the top rule from stack */
QB_result_t Qbase_filter_pop( QB_connid_t Connection )
{
	int			result;
	uint32_t	Qbase_Code;
	
	result = devctl( Connection, QBASE_CMD_FLTRPOP, NULL, 0, &Qbase_Code);
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}


/* delete rows from table, based on current ruleset */
QB_result_t Qbase_delete( QB_connid_t Connection )
{
	int			result;
	uint32_t	Qbase_Code;

	result = devctl( Connection, QBASE_CMD_DELETE, NULL, 0, &Qbase_Code );
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}









/* create header, descrybing update fields */
QB_result_t Qbase_GetUpdateHeader( Qbase_table_desc_t* UTable, char** URule )
{
	uint32_t	HSize;
	char *		Rule;
	
	if( UTable == NULL ) return QBASE_E_BUF;
	
	HSize = sizeof(uint32_t) + UTable->FieldNum + UTable->RowLen;
	
	if( NULL == (Rule = (char*)malloc(HSize)) )	return QBASE_E_MEM;
	
	memset( Rule, 0, HSize );
	*((uint32_t*)Rule) = HSize;
	*URule = Rule;
	
	return QBASE_E_OK;
}


QB_result_t Qbase_SetUpdateField( Qbase_table_desc_t* UTable, uint32_t FNum, char* URule, void* UValue, uint32_t UVSize, char Action )
{
	/* refuse any NULL pointers */
	if( (UTable == NULL) || (URule == NULL) || (UValue == NULL) || (UVSize == 0) ) return QBASE_E_BUF;
	
	/* refuse any attempt to set outbound fields fields */
	if( FNum >= UTable->FieldNum ) return QBASE_E_NOFLD;
	
	/* refuse any data, exceeding the size of the field to update */
	if( UTable->Fields[FNum].Length < UVSize ) return QBASE_E_BUF;
	
	/* set up the values to update */
	memcpy( (URule + sizeof(uint32_t) + UTable->FieldNum + UTable->Fields[FNum].Offset), UValue, UVSize );
	((char*)(URule + sizeof(uint32_t)))[FNum] = Action;
	
	return QBASE_E_OK;
}


/* update rows in current table using current ruleset */

QB_result_t Qbase_update( QB_connid_t Connection, char* URule )
{
	int			result;
	uint32_t	Qbase_Code;
	
	/* refuse NULL pointers */
	if( URule == NULL ) return QBASE_E_BUF;

	result = devctl( Connection, QBASE_CMD_UPDATE, URule, *((uint32_t*)URule), &Qbase_Code );
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}

QB_result_t Qbase_updcur( QB_connid_t Connection, char* URule )
{
	int			result;
	uint32_t	Qbase_Code;
	
	/* refuse NULL pointers */
	if( URule == NULL ) return QBASE_E_BUF;

	result = devctl( Connection, QBASE_CMD_UPDCUR, URule, *((uint32_t*)URule), &Qbase_Code );
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}

QB_result_t Qbase_flush( QB_connid_t Connection )
{
	int			result;
	uint32_t	Qbase_Code;

	result = devctl( Connection, QBASE_CMD_TBLFLUSH, NULL, 0, &Qbase_Code );
	if( result != EOK ) return QBASE_E_NOCONN;
	return Qbase_Code;
}


QB_result_t Qbase_SrvInfo( QB_connid_t Connection, Qbase_SrvInfo_t ** Info )
{
	int			result;
	QB_result_t	Qbase_Code;
	
	/* allocate memory for SrvInfo structure */
	*Info = (Qbase_SrvInfo_t*)malloc(sizeof(Qbase_SrvInfo_t));
	if( *Info == NULL ) return QBASE_E_MEM;
	
	/* read info about server */
	result = devctl( Connection, QBASE_CMD_INFO, *Info, sizeof(Qbase_SrvInfo_t), &Qbase_Code );
	if( result != EOK ){
		free(*Info);
		*Info = NULL;
		return QBASE_E_NOCONN;
	}
	if( Qbase_Code != QBASE_E_OK ){
		free(*Info);
		*Info = NULL;
		return Qbase_Code;
	}
	
	/* happily return */
	return QBASE_E_OK;
}
/* EOF */
