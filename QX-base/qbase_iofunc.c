/*
 * -=MikeP=-
 */


/*
 * This file contains I/O functions for the resource manager
 */

#include	"qbase_server.h"
#include	"qbase_filefunc.h"



/* globals, used to store database related data */
Qbase_table_t * Tables = NULL;		/* this is the pointer to records about tables and their data */
pthread_mutex_t tables_mutex = PTHREAD_MUTEX_INITIALIZER;

#define QBASE_TBL_H ".qdbh"
#define QBASE_TBL_D ".qdbd"


Qbase_SrvInfo_t INFO = { 1, QBASE_MSG_LENGTH, QBASE_TAB_NAMELEN, 0 };












/*
 * as we need to store information about each connection
 * to database - the best place is to store it in
 * Open Control Block - ocb
 * That's why we need to redefine these two functions
 */
IOFUNC_OCB_T * Qbase_OCB_calloc( resmgr_context_t * ctp, IOFUNC_ATTR_T *attr){
	struct Qbase_ocb * ocb;
	
	if( !(ocb = calloc(1, sizeof(struct Qbase_ocb)))){
		return 0;
	}
	
	/* tune all of OUR OWN members */
	ocb->Qerrno = QBASE_E_OK;
	ocb->Table = NULL;
	ocb->Filters = NULL;
	ocb->Record = 0;
	
	return (ocb);
}

void Qbase_OCB_free( IOFUNC_OCB_T * ocb ){
	
	/* clean up all the resources - close database if needed */
	if( ocb == NULL ) return;
	if( ocb->Table != NULL ){
		pthread_mutex_lock( &tables_mutex );
		atomic_sub(&ocb->Table->locks,1);
		if( ocb->Table->locks == 0 ) Qbase_TableRelease(ocb->Table);
		pthread_mutex_unlock( &tables_mutex );
	}
	
	/* clear all filters */
	{
		Qbase_filter_t * FFilter = ocb->Filters;
		while( FFilter != NULL ){
			/* free current filter and move to next filter */
			ocb->Filters = FFilter->Next;
			free( FFilter );
			FFilter = ocb->Filters;
		}
	}
	
	/* release memory */
	free( ocb );
}








int Qbase_data_filter(char * Row, RESMGR_OCB_T *ocb )
{
	Qbase_filter_t * CFilter;
	
	/* 1) check if the record is valid */
	if( *((uint32_t*)Row) == 0 ){
		return 0;
	}
		
	/* Apply data filtering:
	 * The results of applying each filter are merged using "AND" rule.
	 * That is if one of conditions is not met - we return with error;
	 * See help for more information.
	 */
	CFilter = ocb->Filters;
	while( CFilter != NULL ){
		/* apply filter rule */
		if( 1 != CFilter->cmp_func(CFilter,Row) ){
			if( CFilter->Flags & Q_CMP_STRICT ) return (-1); /* the strict rule is not met */
			return 0;
		}
		CFilter = CFilter->Next;
	}
	
	/* we are here because all the criterias are met - say: "That's It!" */
	return 1;
}






/*
 * This function looks for page with a given number
 * among pages of the given table. It does its best to lock
 * the page in memory.
 * Precaution: table sync MUST be locked before calling this function.
 * Assumption is made this function is provided with full
 * write table caption access
 */
Qbase_page_t* LockTablePage( Qbase_table_t *CurTbl, int nPageID, int* error)
{
	Qbase_page_t	*RPage;
	
	if( CurTbl == NULL ) return NULL;
	
	/* the table should be locked - we may safely search for required  page */
	RPage = CurTbl->Pages;
	while( RPage != NULL ){
		
		if( RPage->Index == nPageID ) break; /* we have found the page we need */
		
		if( RPage->Index > nPageID ){
			/* we found the page that follows the one we are looking for.
			 * Try to load requested page from disk */
			
			Qbase_page_t * TmpPage;
			
			/* if we can - we allocate a new page - else replace existing */
			if( CurTbl->PLocks >= CurTbl->Table->LockPages ){
				/* we have to replace the page */
				if( RPage == CurTbl->LastPage ){
					/* if this is not the last page - we will simply replace it, but we are here
					 * because we found the last page - so let us try to reuse the first page */
					TmpPage = CurTbl->Pages;
					
					pthread_rwlock_wrlock( &TmpPage->rwlock );
					
					TmpPage->Next->Prev = NULL;
					CurTbl->Pages = TmpPage->Next;
					TmpPage->Next = RPage;
					TmpPage->Prev = RPage->Prev;
					RPage->Prev = TmpPage;
					if( TmpPage->Prev != NULL ) TmpPage->Prev->Next = TmpPage;
					RPage = TmpPage;
					
					/* 2006-02-10; Sync page to disk as it may contain raw data */
					Qbase_PageSync( RPage, CurTbl->DFile );
					
					pthread_rwlock_unlock( &TmpPage->rwlock );
				}
			}else{
				/* allocate new one */
				
				/* allocate page descriptor */
				if( (TmpPage = (Qbase_page_t*)malloc(sizeof(Qbase_page_t))) == NULL ){
						*error = QBASE_E_PAGING;
						return NULL;
				}
				TmpPage->Size = CurTbl->Table->RowLen * CurTbl->Table->PageRows;
				if( (TmpPage->Data = (char*)malloc(TmpPage->Size)) == NULL ){
					free(TmpPage);
					*error = QBASE_E_PAGING;
					return NULL;
				}
				pthread_rwlock_init( &TmpPage->rwlock, NULL );
				CurTbl->PLocks++; /* do not forget to notice that we allocated one more page */
				/* add the page to the pool of pages */
				TmpPage->Next = RPage;
				TmpPage->Prev = RPage->Prev;
				RPage->Prev = TmpPage;
				if( TmpPage->Prev != NULL ){
					TmpPage->Prev->Next = TmpPage;
				}else{
					/* we are here because we add the first page */
					CurTbl->Pages = TmpPage;
				}
				RPage = TmpPage;
			}
				
			/* everything is ok - we may read the page into memory */
			RPage->Index = nPageID;
			RPage->NeedSync = 0; /* we read full page into memory */
			RPage->LastRow = CurTbl->Table->PageRows;
			lseek(CurTbl->DFile, (nPageID*RPage->Size), SEEK_SET);
			pthread_rwlock_wrlock( &RPage->rwlock );
			read(CurTbl->DFile, RPage->Data, RPage->Size);
			pthread_rwlock_unlock( &RPage->rwlock );
			break;
		}
			
		RPage = RPage->Next;
	}
	
	/* return what we have found */
	return RPage;
}






























/* 
 * handle _IO_READ message 
 * This means client wants some data back
 */
int Qbase_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
	int				nparts;
	uint32_t		nbytes;
	char *			where;
	uint32_t		PIndex, RIndex;
	Qbase_page_t * 	RPage;

	/* we support no special types! */
	if (msg->i.xtype & _IO_XTYPE_MASK != _IO_XTYPE_NONE) return (ENOSYS);
	
	if( ocb->Table == NULL ){
		ocb->Qerrno = QBASE_E_NOTBLSEL;
		return _RESMGR_NPARTS(0);
	}

	/* check if client has enough space in buffer to store one row of data */
	nbytes = ocb->Table->Table->RowLen;
	if( msg->i.nbytes < nbytes ){
		ocb->Qerrno = QBASE_E_MEM;
		return _RESMGR_NPARTS(0);
	}
	
	/* 1) Find values of the page and row where we are going to start reading */
	PIndex = (ocb->Record)/(ocb->Table->Table->PageRows); /* the number of page to start with */
	RIndex = (ocb->Record)%(ocb->Table->Table->PageRows); /* the number of record to start with */
	
	
	pthread_rwlock_wrlock( &ocb->Table->rwlock );
	
	/* scan until we find what we want or reach the end of table */
	while( ocb->Record < ocb->Table->Table->Index ){
		
		/* the table is locked - we may safely search for required  page */
		RPage = LockTablePage(ocb->Table, PIndex, &ocb->Qerrno );
		if( RPage == NULL ){
			/* something bad happened */
			pthread_rwlock_unlock( &ocb->Table->rwlock );
			return _RESMGR_NPARTS(0);
		}
		
		/* required page is found and locked - scan it through. */
		/* if there is what we look for - give it to client and release page 
		 * else move to the next page */
		pthread_rwlock_rdlock( &RPage->rwlock );
		pthread_rwlock_unlock( &ocb->Table->rwlock );
		
		/* scan the page */
		where = RPage->Data + RIndex*nbytes;
		while( RIndex < RPage->LastRow ){
			
			int CResult = Qbase_data_filter(where,ocb); /* result of filter checking */
			
			/* if filter says OK - we return the record */
			if( 1 == CResult ){
				/* return the row happily */
				{
					SETIOV(ctp->iov, where, nbytes);
					_IO_SET_READ_NBYTES(ctp, nbytes);
					nparts = 1;
				}
				ocb->Record++;
				pthread_rwlock_unlock( &RPage->rwlock );
				ocb->Qerrno = QBASE_E_OK;
				if(msg->i.nbytes >0 ) ocb->hdr.attr->flags |= IOFUNC_ATTR_ATIME;
				return _RESMGR_NPARTS(nparts);
			}
			
			/* if filter says "THE STRICT RULE IS BROKEN" - return QBASE_E_LASTROW */
			if( (-1) == CResult ){
				pthread_rwlock_unlock( &RPage->rwlock );
				ocb->Qerrno = QBASE_E_LASTROW;
				return _RESMGR_NPARTS(0);
			}
			
			/* filter said NO - we continue to search */
			RIndex++;
			ocb->Record++;
			where += nbytes;
		}
		
		/* we have not found what we wanted - release page and move to the next one */
		pthread_rwlock_unlock( &RPage->rwlock);
		PIndex++;
		RIndex=0;
		pthread_rwlock_wrlock( &ocb->Table->rwlock );
	}
	
	/* we reached the end of the table by current moment - so we say QBASE_E_LASTROW */
	pthread_rwlock_unlock( &ocb->Table->rwlock );
	ocb->Qerrno = QBASE_E_LASTROW;
	return _RESMGR_NPARTS(0);
}




















/*
 * handle _IO_WRITE message
 * This means client wants us to consume some data
 */
int Qbase_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb)
{
	Qbase_page_t * WPage; /* the page where we'll write info */
	char * where = NULL;
	int	nbytes = 0;
	uint32_t PrKey = 0;
	
	/* we support no special types! */
    if (msg->i.xtype & _IO_XTYPE_MASK != _IO_XTYPE_NONE) return (ENOSYS);
    
    /* 1) check if there is table selected to paste records to */
    if( ocb->Table == NULL ){
    	ocb->Qerrno = QBASE_E_NOTBLSEL;
    	return _RESMGR_NPARTS(0);
	}
    
    /* 2) check if client supplied enough bytes for one row of data */
    if( msg->i.nbytes < ocb->Table->Table->RowLen ){
    	ocb->Qerrno = QBASE_E_BUF;
    	return _RESMGR_NPARTS(0);
	}

    /* 3) get the last page in the table - cause we'll add data there  */
    pthread_rwlock_wrlock( &ocb->Table->rwlock );
    
    WPage = ocb->Table->LastPage;	/* we do it this way to avoid curruption of pointer to the last page by another thread */
    
    /* 3.1 - see if there is place for the record on last page */
    if( WPage->LastRow >= ocb->Table->Table->PageRows ){
    	/* 3.1.1 - sync data to prevent its loss */
    	pthread_rwlock_wrlock( &WPage->rwlock );
    	Qbase_PageSync( WPage, ocb->Table->DFile );
    	Qbase_HeadSync( ocb->Table );
    	
    	/* We DO NOT allocate memory for a new page - we reuse last page
    	 * to prevent exessive memory usage */
		
		/* comment out - the page should be tuned already */
		/*WPage->Size = ocb->Table->Table->RowLen * ocb->Table->Table->PageRows;
		WPage->Next = NULL;*/
		
		WPage->Index++;
		WPage->LastRow = 0;
		pthread_rwlock_unlock( &WPage->rwlock );
	}
	
	/* 3.2 - either we have place on last page or we added new one
	 * in any case - we are ready to add new record - tune all values */
	ocb->Table->Table->Index++;
	PrKey = ocb->Table->Table->Index;
	
	nbytes = ocb->Table->Table->RowLen;
	pthread_rwlock_wrlock( &WPage->rwlock ); /* lock the page for writing and release table */
    pthread_rwlock_unlock( &ocb->Table->rwlock );
    /* end of 3) - page locked - table unlocked */
    
	/* 4) now really add record to table */
	where = WPage->Data + (WPage->LastRow * nbytes);
	if( nbytes != resmgr_msgread(ctp, where, nbytes, sizeof(msg->i))){
		pthread_rwlock_unlock( &WPage->rwlock );
		ocb->Qerrno = QBASE_E_INS;
		return _RESMGR_NPARTS(0);
	}

	/* set up the correct primary key */
    *((uint32_t*)where) = PrKey;
    
    /* If client reads just after the write operation and there are no filters
     * it should get the record that was just posted */
    ocb->Record = (PrKey-1);
    
    /* fix number of records on the page */
    WPage->LastRow++;
    WPage->NeedSync = 1;
    
	/* set up the number of bytes (returned by client's write()) */
    _IO_SET_WRITE_NBYTES (ctp, nbytes);
    
    /* release the page rwlock */
    pthread_rwlock_unlock( &WPage->rwlock );

    /* set up time of modification of our resource */
    ocb->hdr.attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
    return _RESMGR_NPARTS (0);
}





























































/* handle _IO_DEVCTL message
 * This means some kind of interaction with client
 */
int Qbase_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
	int ret;
	int retval = QBASE_E_OK;
	int nbytes = 0;
	char * data = _DEVCTL_DATA(msg->i);
	char FName[QBASE_TAB_NAMELEN+8];
	Qbase_table_t * CurTbl;
	
	ret = EOK;
	
	switch(msg->i.dcmd){
		
		/*#################### SELECT TABLE FOR OPERATION #####################*/		
		case QBASE_CMD_TBLSELECT:
		{
			/* we are asked to select the table to operate on */
			int				TabFile;
			
			/* 1) check the size of data supplied in case of error - return error */
			if( msg->i.nbytes > QBASE_TAB_NAMELEN ){
				retval = QBASE_E_BUF;
				nbytes = 0;
				break;
			}
			
			
			/* 2) scan through the tables in memory */
			pthread_mutex_lock( &tables_mutex );
			
			CurTbl = Tables;
			while( CurTbl != NULL ){
				if( 0 == strcmp(CurTbl->Table->Name, data) ) break;
				CurTbl = CurTbl->Next;
			}
			
			if( CurTbl != NULL ){
				
				/* check if we are changing from another table */
				if(ocb->Table == CurTbl){
					pthread_mutex_unlock( &tables_mutex );
					retval = QBASE_E_OK;
					nbytes = 0;
					break;
				}
				
				if( ocb->Table != NULL ){
					atomic_sub(&ocb->Table->locks,1);
					if( ocb->Table->locks == 0 ) Qbase_TableRelease(ocb->Table);
					
					/* as we are changing from another table - free all the filters,
					 * defined for that table */
					{
						Qbase_filter_t * FFilter = ocb->Filters;
						while( FFilter != NULL ){
							/* free current filter and move to next filter */
							ocb->Filters = FFilter->Next;
							free( FFilter );
							FFilter = ocb->Filters;
						}
					}
				}
				
				ocb->Table = CurTbl;
				atomic_add( &(CurTbl->locks), 1);
				pthread_mutex_unlock( &tables_mutex );
				retval = QBASE_E_OK;
				nbytes = 0;
				break;
			}
			
			/* 3) Try loading table from file */
			memcpy( FName, data, msg->i.nbytes );
			strcat( FName, QBASE_TBL_H );
			
			TabFile = open( FName, O_RDWR|O_SYNC );
			if( TabFile == -1 ){
				pthread_mutex_unlock( &tables_mutex );
				retval = QBASE_E_NOTBLEXIST;
				nbytes = 0;
				break;
			}
			
			/* 4) header file found - load it */
			if( NULL == (CurTbl = (Qbase_table_t*)malloc(sizeof(Qbase_table_t)))){
				pthread_mutex_unlock( &tables_mutex );
				retval = QBASE_E_MEM;
				close(TabFile);
				nbytes = 0;
				break;
			}
			pthread_rwlock_init( &CurTbl->rwlock, NULL );
			CurTbl->locks = 1;
			CurTbl->Next = NULL;
			CurTbl->Prev = NULL;
			CurTbl->HFile = TabFile;
			
			/* allocate table descriptor and read info from file */
			retval = lseek( CurTbl->HFile, 0, SEEK_END );
			if( retval <= 0 ){
				/* header file is empty */
				Qbase_TableRelease(CurTbl);
				pthread_mutex_unlock( &tables_mutex );
				retval = QBASE_E_MEM;
				nbytes = 0;
				break;
			}
			lseek( CurTbl->HFile, 0, SEEK_SET );
			
			/* allocate memory for table descriptor given filesize */
			if((CurTbl->Table = malloc(retval)) == NULL){
				pthread_mutex_unlock( &tables_mutex );
				retval = QBASE_E_MEM;
				Qbase_TableRelease( CurTbl );
				nbytes = 0;
				break;
			}
			read( CurTbl->HFile, CurTbl->Table, retval );
			
			/*
			 * The things below are a bit tricky:
			 * we need to load last page to add info there.
			 * later we may need the first page to read data from table.
			 * 
			 * For now we will only load last page.
			 * Loading first page is the problem of read() handler.
			 */
			
			/* open the body of the table */
			memcpy( FName, CurTbl->Table->Name, QBASE_TAB_NAMELEN+1 );
			strcat( FName, QBASE_TBL_D );
			CurTbl->DFile = open( FName, O_SYNC|O_RDWR );
			
			/* allocate table datapage entry */
			if( (CurTbl->Pages = (Qbase_page_t*)malloc(sizeof(Qbase_page_t))) == NULL ){
				pthread_mutex_unlock( &tables_mutex );
				Qbase_TableRelease( CurTbl );
				retval = QBASE_E_MEM;
				nbytes = 0;
				break;
			}
			CurTbl->LastPage = CurTbl->Pages;
			CurTbl->Pages->Next = NULL;
			CurTbl->Pages->Prev = NULL;
			CurTbl->Pages->Size = CurTbl->Table->RowLen * CurTbl->Table->PageRows;
			pthread_rwlock_init( &CurTbl->Pages->rwlock, NULL );
			CurTbl->Pages->NeedSync = 0; /* a fresh table from disk - do not need to sync() */
			
			/* allocate place for data storage */
			if( (CurTbl->Pages->Data = (char*)malloc(CurTbl->Pages->Size)) == NULL ){
				pthread_mutex_unlock( &tables_mutex );
				Qbase_TableRelease( CurTbl );
				retval = QBASE_E_MEM;
				nbytes = 0;
				break;
			}
			
			/* find the index of last page and move filepointer to read it */
			CurTbl->Pages->Index = (CurTbl->Table->Index)/(CurTbl->Table->PageRows);
			/* set number of rows on current page */
			CurTbl->Pages->LastRow = (CurTbl->Table->Index)%(CurTbl->Table->PageRows);
			lseek(CurTbl->DFile, (CurTbl->Pages->Index*CurTbl->Pages->Size), SEEK_SET);
			
			/* read data of the last page into memory */
			read( CurTbl->DFile, CurTbl->Pages->Data, CurTbl->Pages->Size );
			CurTbl->PLocks = 1;
			
			/* add table to loaded tables list and to current ocb */
			CurTbl->Next = Tables;
			if(Tables != NULL) Tables->Prev = CurTbl;
			Tables = CurTbl;
			

			/* check if we are changing from another table */
			if( ocb->Table != NULL ){
				atomic_sub(&ocb->Table->locks,1);
				if( ocb->Table->locks == 0 ) Qbase_TableRelease(ocb->Table);
			}
			
			ocb->Table = CurTbl;
			ocb->Record = 0;
			ocb->Filters = NULL;
			
			pthread_mutex_unlock( &tables_mutex );
			retval = QBASE_E_OK;
			nbytes = 0;
			break;
		}
		

		/*############### CREATE A NEW TABLE ##################################*/
		case QBASE_CMD_TBLCREATE:
		{
			int	TabFile;
			Qbase_table_desc_t * NewTbl = (Qbase_table_desc_t *)data;
			
			/* 1) check if the table descriptor is at least minimum size */
			if( msg->i.nbytes < sizeof(Qbase_table_desc_t) ){
				retval = QBASE_E_BUF;
				nbytes = 0;
				break;
			}
			
			/* 2) check if the table already exist */
			pthread_mutex_lock( &tables_mutex );
			
			memcpy( FName, NewTbl->Name, QBASE_TAB_NAMELEN+1 );
			strcat( FName, QBASE_TBL_H );
			
			TabFile = open( FName, O_RDWR );
			if( TabFile != -1 ){
				pthread_mutex_unlock( &tables_mutex );
				retval = QBASE_E_ALREADY;
				nbytes = 0;
				break;
			}
			
			/* 4) Finally - this is a new table */
			/* alloocate table entry */
			if( (CurTbl = (Qbase_table_t*)malloc(sizeof(Qbase_table_t))) == NULL ){
				pthread_mutex_unlock( &tables_mutex );
				retval = QBASE_E_MEM;
				nbytes = 0;
				break;
			}
			/* set any data for current table entry and write everything to disk */
			pthread_rwlock_init( &CurTbl->rwlock, NULL );
			CurTbl->locks = 1;
			CurTbl->PLocks = 1; /* we always start table with the last page locked in memory */
			CurTbl->Next = NULL;
			CurTbl->Prev = NULL;
			CurTbl->HFile = 0;
			CurTbl->DFile = 0;
			
			/* allocate table descriptor */
			if( (CurTbl->Table = (Qbase_table_desc_t*)malloc(NewTbl->Size)) == NULL ){
				pthread_mutex_unlock( &tables_mutex );
				Qbase_TableRelease( CurTbl );
				retval = QBASE_E_MEM;
				nbytes = 0;
				break;
			}
			/* copy table data */
			memcpy( CurTbl->Table, NewTbl, NewTbl->Size );

			/* check if user wanted enough pages to be locked in memory
			 * we require at least 3 pages in memory:
			 * one for writing and two for manipulating.
			 * The more pages in memory - the better!
			 */
			if( CurTbl->Table->LockPages < 3 ){
				pthread_mutex_unlock( &tables_mutex );
				Qbase_TableRelease( CurTbl );
				retval = QBASE_E_FEWPLOCKS;
				nbytes = 0;
				break;
			}
			
			/* allocate table datapage entry */
			if( (CurTbl->Pages = (Qbase_page_t*)malloc(sizeof(Qbase_page_t))) == NULL ){
				pthread_mutex_unlock( &tables_mutex );
				Qbase_TableRelease( CurTbl );
				retval = QBASE_E_MEM;
				nbytes = 0;
				break;
			}
			memset( CurTbl->Pages, 0, sizeof(Qbase_page_t) );
			CurTbl->LastPage = CurTbl->Pages;
			CurTbl->Pages->Next = NULL;
			CurTbl->Pages->Prev = NULL;
			CurTbl->Pages->Size = CurTbl->Table->RowLen * CurTbl->Table->PageRows;
			pthread_rwlock_init( &CurTbl->Pages->rwlock, NULL );
			CurTbl->Pages->NeedSync = 0;
			
			
			/* allocate place for data storage */
			if( (CurTbl->Pages->Data = (char*)malloc(CurTbl->Pages->Size)) == NULL ){
				pthread_mutex_unlock( &tables_mutex );
				Qbase_TableRelease( CurTbl );
				retval = QBASE_E_MEM;
				nbytes = 0;
				break;
			}
			
			/* create file for tables and write header and pages there */
			CurTbl->HFile = open( FName, O_SYNC|O_CREAT|O_RDWR|O_TRUNC, 0666 );
			write( CurTbl->HFile, CurTbl->Table, CurTbl->Table->Size );
			
			memcpy( FName, CurTbl->Table->Name, QBASE_TAB_NAMELEN+1 );
			strcat( FName, QBASE_TBL_D );
			CurTbl->DFile = open( FName, O_SYNC|O_CREAT|O_RDWR|O_TRUNC, 0666 );
			write( CurTbl->DFile, CurTbl->Pages->Data, CurTbl->Pages->Size );
			
			/* as we create table - there is only one page to store records.
			 * so "Pages" and "LastPage" point to the same page in memoory.
			 * we do not need to differ last page from others.
			 * The only page has index "0"
			 */
			
			/* check if we are changing from another table */
			if( ocb->Table != NULL ){
				atomic_sub(&ocb->Table->locks,1);
				if( ocb->Table->locks == 0 ) Qbase_TableRelease(ocb->Table);
				
				/* as we are changing from another table - free all the filters,
				 * defined for that table */
				{
					Qbase_filter_t * FFilter = ocb->Filters;
					while( FFilter != NULL ){
						/* free current filter and move to next filter */
						ocb->Filters = FFilter->Next;
						free( FFilter );
						FFilter = ocb->Filters;
					}
				}
			}
			
			/* add table to tables list */
			CurTbl->Next = Tables;
			if(Tables != NULL) Tables->Prev = CurTbl;
			Tables = CurTbl;
			
			ocb->Table = CurTbl;
			ocb->Record = 0;
			ocb->Filters = NULL;
			
			pthread_mutex_unlock( &tables_mutex );
			
			retval = QBASE_E_OK;
			break;
		}
		

		/*############# GET RESULT OF LAST OPERATION ##########################*/
		case QBASE_CMD_ERROR:
		{
			/* we are asked to say the result of the last operation */
			retval = ocb->Qerrno;
			break;
		}
		
		/*############# FLUSH CURRENTLY SELECTED TABLE TO DISK ################*/
		case QBASE_CMD_TBLFLUSH:
		{
			if( ocb->Table == NULL ){
				retval = QBASE_E_NOTBLSEL;
				break;
			}
			pthread_rwlock_wrlock( &ocb->Table->rwlock );
			Qbase_TableSync( ocb->Table );
			pthread_rwlock_unlock( &ocb->Table->rwlock );
			retval = QBASE_E_OK;
			break;
		}
		
		/*#################### GET INFO ABOUT THE SERVER ######################*/
		case QBASE_CMD_INFO:
		{
			if( msg->o.nbytes < sizeof(Qbase_SrvInfo_t) ){
				retval = QBASE_E_BUF;
				break;
			}
			memcpy(data,&INFO,sizeof(Qbase_SrvInfo_t));
			nbytes = sizeof(Qbase_SrvInfo_t);
			retval = QBASE_E_OK;
			break;
		}
		
		/*##### RESET CURRENT ROW INDEX TO POINT TO THE VERY FIRST RECORD #####*/
		case QBASE_CMD_FETCHRST:
		{
			if( ocb->Table != NULL ){
				ocb->Record = 0;
				retval = QBASE_E_OK;
			}else{
				retval = QBASE_E_NOTBLSEL;
			}
			break;
		}
		
		/*################ GET LENGTH OF TABLE HEADER IN BYTES ################*/
		case QBASE_CMD_TBLCAPSLEN:
		{
			if( ocb->Table == NULL ){
				retval = QBASE_E_NOTBLSEL;
				break;
			}
			
			/* yet no check of size of buffer */
			
			*((uint32_t*)data) = ocb->Table->Table->Size;
			nbytes = sizeof( uint32_t );
			retval = QBASE_E_OK;
			break;
		}
		
		/*################ GET CAPTION OF CURRENTLY SELECTED TABLE ############*/
		case QBASE_CMD_TBLCAPS:
		{
			if( ocb->Table == NULL ){
				retval = QBASE_E_NOTBLSEL;
				break;
			}
			nbytes = ocb->Table->Table->Size;
			memcpy( data, ocb->Table->Table, nbytes );
			retval = QBASE_E_OK;
			break;
		}
		
		/*################# DELETE ALL CURRENTLY DEFINED FILTERS ##############*/
		case QBASE_CMD_FLTRDELETE:
		{
			Qbase_filter_t * FFilter = ocb->Filters;
			
			while( FFilter != NULL ){
				/* free current filter and move to next filter */
				ocb->Filters = FFilter->Next;
				free( FFilter );
				FFilter = ocb->Filters;
			}
			
			ocb->Record = 0;
			retval = QBASE_E_OK;
			break;
		}
		
		/*################## DELETE THE RULE THAT WAS ADDED LAST ##############*/
		case QBASE_CMD_FLTRPOP:
		{
			Qbase_filter_t * FFilter = ocb->Filters;
			
			if( ocb->Filters != NULL ){
				ocb->Filters = FFilter->Next;
				free(FFilter);
			}
			
			retval = QBASE_E_OK;
			break;
		}
		
		/*############# ADD FILTER RULE TO CURRENTLY DEFINED FILTERS ##########*/
		case QBASE_CMD_FLTRDEFINE:
		{
			Qbase_filter_t * NFilter;
			
			/* no table - no filters :0) */
			if( ocb->Table == NULL ){
				retval = QBASE_E_NOTBLSEL;
				break;
			}
			
			/* check that filter has at least sizeof(Qbase_filter_t) bytes */
			if( msg->i.nbytes < sizeof(Qbase_filter_t) ){
				retval = QBASE_E_BUF;
				break;
			}
			
			/* allocate memory for filter definition and copy it */
			if( (NFilter = (Qbase_filter_t*)malloc(msg->i.nbytes)) == NULL ){
				retval = QBASE_E_MEM;
				break;
			}
			memcpy( NFilter, data, msg->i.nbytes );
			
			/* fine tune the record and add it to the list */
			if( NFilter->FNum >= ocb->Table->Table->FieldNum ){
				free( NFilter );
				retval = QBASE_E_FLTR;
				break;
			}
			
			NFilter->Field = ocb->Table->Table->Fields[NFilter->FNum].Offset;
			NFilter->FSize = ocb->Table->Table->Fields[NFilter->FNum].Length;
			NFilter->cmp_func = FFUNCS[ocb->Table->Table->Fields[NFilter->FNum].Type];
			
			/* finally adding !!! */
			NFilter->Next = ocb->Filters;
			ocb->Filters = NFilter;
			ocb->Record = 0; /* adding new filter resets current position to zero */
			
			retval = QBASE_E_OK;
			break;
		}
		
		/*########## UPDATE RECORDS IN TABLE BASED ON CURRENT RULEST ##########*/
		case QBASE_CMD_UPDATE:
		case QBASE_CMD_UPDCUR:
		
		/*####### DELETE RECORDS FROM TABLE BASED UPON CURRENT RULESET ########*/
		case QBASE_CMD_DELETE:
		{
			Qbase_page_t *	RPage;
			uint32_t		rowlen;
			char *			where;
			uint32_t		PIndex = 0;	/* index of current page */
			uint32_t		RIndex = 0;	/* index of current record */
			uint32_t		TIndex = 0;	/* index of last record on the moment of command issue */
			
			/* is there any table selected? */
			if( ocb->Table == NULL ){
				retval = QBASE_E_NOTBLSEL;
				break;
			}
			
			rowlen = ocb->Table->Table->RowLen;
			/* if we are asked to update the table - we have to check incoming data also */
			if((msg->i.dcmd == QBASE_CMD_UPDATE) || (msg->i.dcmd == QBASE_CMD_UPDCUR)){
				uint32_t USize;
				/* check that the size of update header is correct */
				USize = sizeof(uint32_t) + ocb->Table->Table->FieldNum + rowlen;
				if(msg->i.nbytes < USize){
					retval = QBASE_E_BUF;
					break;
				}
			}
			
			/* if we are asked to update current record only */
			if(msg->i.dcmd == QBASE_CMD_UPDCUR){
				/* get the record and page */
				PIndex = (ocb->Record)/(ocb->Table->Table->PageRows); /* the number of page to start with */
				RIndex = (ocb->Record)%(ocb->Table->Table->PageRows); /* the number of record to start with */
			}
			
			
			/* 1) Get access to table and find appropriate field */
			pthread_rwlock_wrlock( &ocb->Table->rwlock );
			
			TIndex = ocb->Table->Table->Index;
			
			/* now we are scanning through the whole dataset :0) - yeh - too long */
			retval = QBASE_E_OK;
			
			while( TIndex > 0 ){
				
				/* the table is locked - we may safely search for required  page */
				RPage = LockTablePage(ocb->Table, PIndex, &ocb->Qerrno );
				if( RPage == NULL ){
					/* something bad happened */
					retval = ocb->Qerrno;
					pthread_rwlock_unlock( &ocb->Table->rwlock );
					break;
				}
				
				/* if there was an erro allocating memory for a page - we break the while loop */
				if( retval != QBASE_E_OK ) break;
				
				/* required page is found and locked - scan it through. */
				/* if there is what we look for - update/delete it, release page, 
				 * then move to the next page */
				pthread_rwlock_wrlock( &RPage->rwlock );
				pthread_rwlock_unlock( &ocb->Table->rwlock );
				
				/* scan the page */
				where = RPage->Data + RIndex*rowlen;
				while( (RIndex<RPage->LastRow) && (TIndex>0) ){
					
					int CResult = Qbase_data_filter(where,ocb);
					
					/* if filter says OK - we return the record */
					if( 1 == CResult ){
						
						/* WE APPLY UPDATE/DELETE HERE */
						if(msg->i.dcmd == QBASE_CMD_DELETE){
				 			*((uint32_t*)where) = 0;
				 		}else{
							/* we are about to update the field */
							int j = 0;
							uint32_t Fnum = ocb->Table->Table->FieldNum;
							char * URule = data + sizeof(uint32_t);
							for( j = 0; j<Fnum; j++ )
								if( URule[j] != QBASE_U_NONE ){
									/* get the type of the field and take appropriate action */
									uint32_t	UFType = ocb->Table->Table->Fields[j].Type;
									switch(UFType){
										case QBASE_TYPE_UINT:
										{
											uint32_t * UOffset = (uint32_t*)(where + ocb->Table->Table->Fields[j].Offset);
											uint32_t * VOffset = (uint32_t*)(URule+Fnum+ocb->Table->Table->Fields[j].Offset);
											if(URule[j] == QBASE_U_SET){
												*UOffset = *VOffset;
												break;
											}
											if(URule[j] == QBASE_U_ADD){
												*UOffset = (*UOffset) + (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_SUB){
												*UOffset = (*UOffset) - (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_DIV){
												*UOffset = (*UOffset) / (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_MUL){
												*UOffset = (*UOffset) * (*VOffset);
												break;
											}
											break;
										}
										case QBASE_TYPE_INT:
										{
											int * UOffset = (int*)(where + ocb->Table->Table->Fields[j].Offset);
											int * VOffset = (int*)(URule+Fnum+ocb->Table->Table->Fields[j].Offset);
											if(URule[j] == QBASE_U_SET){
												*UOffset = *VOffset;
												break;
											}
											if(URule[j] == QBASE_U_ADD){
												*UOffset = (*UOffset) + (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_SUB){
												*UOffset = (*UOffset) - (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_DIV){
												*UOffset = (*UOffset) / (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_MUL){
												*UOffset = (*UOffset) * (*VOffset);
												break;
											}
											break;
										}
										case QBASE_TYPE_UINT16:
										{
											uint16_t * UOffset = (uint16_t*)(where + ocb->Table->Table->Fields[j].Offset);
											uint16_t * VOffset = (uint16_t*)(URule+Fnum+ocb->Table->Table->Fields[j].Offset);
											if(URule[j] == QBASE_U_SET){
												*UOffset = *VOffset;
												break;
											}
											if(URule[j] == QBASE_U_ADD){
												*UOffset = (*UOffset) + (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_SUB){
												*UOffset = (*UOffset) - (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_DIV){
												*UOffset = (*UOffset) / (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_MUL){
												*UOffset = (*UOffset) * (*VOffset);
												break;
											}
											break;
										}
										case QBASE_TYPE_INT16:
										{
											int16_t * UOffset = (int16_t*)(where + ocb->Table->Table->Fields[j].Offset);
											int16_t * VOffset = (int16_t*)(URule+Fnum+ocb->Table->Table->Fields[j].Offset);
											if(URule[j] == QBASE_U_SET){
												*UOffset = *VOffset;
												break;
											}
											if(URule[j] == QBASE_U_ADD){
												*UOffset = (*UOffset) + (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_SUB){
												*UOffset = (*UOffset) - (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_DIV){
												*UOffset = (*UOffset) / (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_MUL){
												*UOffset = (*UOffset) * (*VOffset);
												break;
											}
											break;
										}
										case QBASE_TYPE_DOUBLE:
										{
											double * UOffset = (double*)(where + ocb->Table->Table->Fields[j].Offset);
											double * VOffset = (double*)(URule+Fnum+ocb->Table->Table->Fields[j].Offset);
											if(URule[j] == QBASE_U_SET){
												*UOffset = *VOffset;
												break;
											}
											if(URule[j] == QBASE_U_ADD){
												*UOffset = (*UOffset) + (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_SUB){
												*UOffset = (*UOffset) - (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_DIV){
												*UOffset = (*UOffset) / (*VOffset);
												break;
											}
											if(URule[j] == QBASE_U_MUL){
												*UOffset = (*UOffset) * (*VOffset);
												break;
											}
											break;
										}
										default:
										{
											if(URule[j] == QBASE_U_SET){
												uint32_t UOffset = ocb->Table->Table->Fields[j].Offset;	
												uint32_t ULength = ocb->Table->Table->Fields[j].Length;									
												memcpy( where+UOffset, URule+Fnum+UOffset, ULength );
											}
											break;
										}
									}
								}
						}
						
						/* filter said - go on and we made changes into page data
						 * so do not forget to mark page as dirty
						 */
						RPage->NeedSync = 1;
						
					}/*** END APPLYING UPDATE RULES ***/
					
					/* if filter said "STRICT RULE IS BROKEN" - we stop updating/deleting records */
					if( CResult == (-1) ){
						TIndex = 0;
						break;
					}
					
					/* if only one record to be updated - stop updating */
					if(msg->i.dcmd == QBASE_CMD_UPDCUR){
						/*printf("One record updated\n");*/
						TIndex = 0;
						break;
					}
					
					/* filter said NO - we continue to search */
					RIndex++;
					TIndex--;
					where += rowlen;
				}
				
				/*
				 * 2006-02-10: Code below commented out. Now server maintains NeedSync field for
				 * each page. The page goes to be sync()-ed next time it is going to be
				 * replaced by new one or explicitly flushed by user.
				 */
				
				/* sync the page to disk so our modifications take place */
				/*Qbase_PageSync( RPage, ocb->Table->DFile );*/
				
				/* we did what we wanted - release page and move to the next one */
				pthread_rwlock_unlock( &RPage->rwlock);
				PIndex++;
				RIndex=0;
				pthread_rwlock_wrlock( &ocb->Table->rwlock );
			
			}/*** END while( TIndex > 0 ) ***/
			
			pthread_rwlock_unlock( &ocb->Table->rwlock );
			break;
		}
		
		case QBASE_CMD_SEEK:
		{
			/* we are asked to set the position in database by primary key */
			pthread_rwlock_rdlock( &ocb->Table->rwlock );
			if( ocb->Table != NULL ){
				if(msg->i.nbytes == 4){
					ocb->Record = *(uint32_t*)data;
					ocb->Record--;
					if(ocb->Record > ocb->Table->Table->Index){
						ocb->Record = 0;
						retval = QBASE_E_PKEY;
					}else{
						retval = QBASE_E_OK;
					}
				}else{
					retval = QBASE_E_BUF;
				}
			}else{
				retval = QBASE_E_NOTBLSEL;
			}
			pthread_rwlock_unlock( &ocb->Table->rwlock );
			break;
		}
				
		default:
			/* in normal case should never get in here */
			ocb->Qerrno = QBASE_E_NOTSUPP;
			return ENOSYS;
	}
	
	memset(&msg->o, 0, sizeof(msg->o) );
	ocb->Qerrno = retval;
	msg->o.ret_val = retval;
	msg->o.nbytes = nbytes;
	return(_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o)+nbytes ));
}
