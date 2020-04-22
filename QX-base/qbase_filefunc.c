/* here are functions used to manipulate the header and the data files of tables */

#include	"qbase_server.h"

extern Qbase_table_t * Tables;


/*
 * Writes page data to the disk
 */
int Qbase_PageSync( Qbase_page_t * S_Page, int S_File )
{
	/* in case page does not require sync */
	if( S_Page->NeedSync != 0 ){
		
		int P_Offset = S_Page->Size * S_Page->Index;
		
		lseek( S_File, P_Offset, SEEK_SET );
		write( S_File, S_Page->Data, S_Page->Size );
		
		/* page does not contain raw data any more */
		S_Page->NeedSync = 0;
	}
	return 1;
}

int Qbase_HeadSync( Qbase_table_t * S_Table ){
	lseek( S_Table->HFile, 0, SEEK_SET );
	write( S_Table->HFile, S_Table->Table, S_Table->Table->Size );
	return 1;
}




/*
 * Writes table header and data to disk
 */
int Qbase_TableSync( Qbase_table_t * S_Table )
{
	Qbase_page_t * S_Page;
	
	if( S_Table == NULL ) return 0;
	if( S_Table->Table == NULL ) return 0;
	
	/* sync header */
	if( S_Table->HFile != 0 ){
		Qbase_HeadSync( S_Table );
	}
		
	/* scan through allocated pages and sync them to the disk */
	if( S_Table->DFile != 0 ){
		S_Page = S_Table->Pages;
		while( S_Page != NULL ){
			pthread_rwlock_wrlock( &S_Page->rwlock );
			Qbase_PageSync( S_Page, S_Table->DFile );
			pthread_rwlock_unlock( &S_Page->rwlock );
			S_Page = S_Page->Next;
		}
	}
	
	return 1;
}





/*
 * Returns all memory and resources allocated for given table to system
 */
int Qbase_TableRelease( Qbase_table_t * R_Table )
{
	Qbase_page_t * R_Page, * P_Page;
	
	if( R_Table == NULL ) return 0;
	
	/* 1) to close safely - we sync all data */
	pthread_rwlock_wrlock( &R_Table->rwlock );
	Qbase_TableSync( R_Table );
	pthread_rwlock_unlock( &R_Table->rwlock );
	
	/* 2) free table descriptor */
	if( R_Table->Table != NULL ) free( R_Table->Table );
	
	/* 3) free all data pages */
	R_Page = R_Table->Pages;
	while( R_Page != NULL ){
		if( R_Page->Data != NULL ) free( R_Page->Data );
		pthread_rwlock_destroy( &R_Page->rwlock );
		P_Page = R_Page;
		R_Page = R_Page->Next;
		free( P_Page );
	}
	
	/* 4) destroy table rwlock */
	pthread_rwlock_destroy( &R_Table->rwlock );
	
	/* 5) close all table files */
	if( R_Table->HFile != 0 ) close( R_Table->HFile );
	if( R_Table->DFile != 0 ) close( R_Table->DFile );
	
	/* 6) fix global tables list */
	if( R_Table->Prev != NULL ){
		R_Table->Prev->Next = R_Table->Next;
	}else{
		Tables = R_Table->Next;
	}
	if( R_Table->Next != NULL ){
		R_Table->Next->Prev = R_Table->Prev;
	}
	
	/* 7) release table itself */
	free( R_Table );
	
	return 1;
}
