/*
 * -=MikeP=- (c) 2004
 */
 
/*
 * Here are definitions needed only by client
 */
#ifndef _QBASE_CLIENT_H_
#define _QBASE_CLIENT_H_

#include	<fcntl.h>
#include	<sys/iofunc.h>
#include	<devctl.h>
#include	<errno.h>
#include	<time.h>
#include	"qbase_common.h"
#include	"qbase_errors.h"
#include	"qbase_filterfunc.h"


/*
 * Here client functions are described
 */
extern char * const QBERROR[];

/* these are functions to manipulate the connection and state */
QB_result_t Qbase_connect( char * Server, QB_connid_t * Connection );
QB_result_t Qbase_disconnect( QB_connid_t Connection );
QB_result_t Qbase_errno( QB_connid_t Connection );
QB_result_t Qbase_SrvInfo( QB_connid_t Connection, Qbase_SrvInfo_t** Info );


/* these are functions to manipulate tables */
Qbase_table_desc_t * Qbase_new_tbldesc( int ColNum, int RowsPP, int LockPages, char * TName );
int Qbase_set_field_metrics( Qbase_table_desc_t * Tbl, int ColNum, uint32_t Type, uint32_t Length );
QB_result_t Qbase_table_create( QB_connid_t Connection, Qbase_table_desc_t * Table );
QB_result_t Qbase_table_select( QB_connid_t Connection, char * Tablename );
QB_result_t Qbase_table_caps( QB_connid_t Connection, Qbase_table_desc_t ** CTable );
QB_result_t Qbase_flush( QB_connid_t Connection );









/* these are functions to make data filtering */
QB_result_t	Qbase_filter_clear( QB_connid_t Connection );
QB_result_t	Qbase_filter_push( QB_connid_t Connection, uint32_t Flags, uint32_t FNum, uint32_t VSize, void * Value );
QB_result_t Qbase_filter_pop( QB_connid_t Connection );







/* these are functions to manipulat data */

char * Qbase_row_newbuffer( Qbase_table_desc_t * Table, int* Rowsize);
QB_result_t	Qbase_fetchrst( QB_connid_t Connection );
QB_result_t Qbase_seek( QB_connid_t Connection, uint32_t PKey );
QB_result_t	Qbase_field_set( Qbase_table_desc_t * WTable, char * Row, int FNum, void * Value, uint32_t Size );

QB_result_t Qbase_insert( QB_connid_t Connection, char* Row, uint32_t Size );
QB_result_t Qbase_fetch( QB_connid_t Connection, char* Row, uint32_t Size );

QB_result_t Qbase_GetUpdateHeader( Qbase_table_desc_t* UTable, char** URule );
QB_result_t Qbase_SetUpdateField( Qbase_table_desc_t* UTable, uint32_t FNum, char* URule, void* UValue, uint32_t UVSize, char Action );
QB_result_t Qbase_update( QB_connid_t Connection, char* URule );
QB_result_t Qbase_updcur( QB_connid_t Connection, char* URule );

QB_result_t Qbase_delete( QB_connid_t Connection );

/* this MACRO is used to get pointers to fields within the row */
#define _QBASE_FIELD_OFFSET(a,b,c)  ((b)->Fields[(a)].Offset + (c))

/* this MACRO is used to get data from tables */
#define QXB_FV_UINT(no,tc,row)		*((uint32_t*)( (row) + (tc)->Fields[(no)].Offset ))
#define QXB_FV_UINT16(no,tc,row)	*((uint16_t*)( (row) + (tc)->Fields[(no)].Offset ))
#define QXB_FV_INT(no,tc,row)		*((int*)( (row) + (tc)->Fields[(no)].Offset ))
#define QXB_FV_INT16(no,tc,row)		*((short int*)( (row) + (tc)->Fields[(no)].Offset ))
#define QXB_FV_DBL(no,tc,row)		*((double*)( (row) + (tc)->Fields[(no)].Offset ))
#define QXB_FV_STR(no,tc,row)		((char*)( (row) + (tc)->Fields[(no)].Offset ))
#define QXB_FV_BIN(no,tc,row)		((uint8_t*)( (row) + (tc)->Fields[(no)].Offset ))


#endif /* _QBASE_CLIENT_H_ */
