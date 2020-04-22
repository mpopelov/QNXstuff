#ifndef _QBASE_FILEFUNC_H_
#define _QBASE_FILEFUNC_H_

/*
 * These functions are needed to sync data to the disk
 */
int Qbase_TableRelease( Qbase_table_t * R_Table );
int Qbase_PageSync( Qbase_page_t * S_Page, int S_File );
int Qbase_HeadSync( Qbase_table_t * S_Table );
int Qbase_TableSync( Qbase_table_t * S_Table );

#endif /* _QBASE_FILEFUNC_H_ */
