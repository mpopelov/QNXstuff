/*
 * -=MikeP=- (c) 2004
 */
 
/*
 * Here are definitions needed only by server
 */
#ifndef _QBASE_SERVER_H_
#define _QBASE_SERVER_H_

#include	<stdio.h>
#include	<stdlib.h>
#include	<stddef.h>
#include	<unistd.h>

struct	Qbase_ocb;
#define	IOFUNC_OCB_T struct Qbase_ocb

#include	<sys/iofunc.h>
#include	<sys/dispatch.h>
#include	<devctl.h>
#include	<errno.h>
#include	<pthread.h>
#include	<fcntl.h>
#include	<atomic.h>
#include	<time.h>
#include	"qbase_common.h"
#include	"qbase_errors.h"
#include	"qbase_filterfunc.h"


/*
 * descriptor of page, containing table data
 */
typedef struct Qbase_page{
	char *				Data;		/* pointer to data itself */
	uint32_t			Size;		/* size of the page in bytes */
	uint32_t			Index;		/* index of the page */
	uint32_t			LastRow;	/* index of the last row on the page */
	uint32_t			NeedSync;	/* set to nonzero if page requires sync to disk */
	pthread_rwlock_t	rwlock;
	struct Qbase_page *	Prev;
	struct Qbase_page * Next;
} Qbase_page_t;



/*
 * This is the structure, containing info about table at runtime
 */
typedef struct Qbase_table{
	Qbase_table_desc_t * Table;		/* reference to described table */
	Qbase_page_t *		Pages;		/* reference to table data pages */
	Qbase_page_t *		LastPage;
	struct Qbase_table * Next;
	struct Qbase_table * Prev;
	pthread_rwlock_t	rwlock;
	uint32_t			locks;
	uint32_t			PLocks;
	int					HFile;		/* header file */
	int					DFile;		/* data file */
} Qbase_table_t;


/*
 * Description of our own OCB
 */
struct Qbase_ocb{
	iofunc_ocb_t	hdr;	/* standart ocb - goes here because libc requires it to be this way */
	QB_result_t		Qerrno;	/* this member stores the return code of the last operation performed by client */
	Qbase_table_t *	Table;	/* table currently selected for the connection */
	uint32_t		Record;	/* index of last selected record */
	Qbase_filter_t * Filters;	/* these are filters to apply */
};



#endif /* _QBASE_SERVER_H_ */
