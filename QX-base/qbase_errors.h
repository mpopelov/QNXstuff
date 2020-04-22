/*
 * -=MikeP=- (c) 2004
 */



#ifndef _QBASE_ERRORS_H_
#define _QBASE_ERRORS_H_

#include	"qbase_common.h"


/* errors that may arise during operation */

/* different common errors */
#define QBASE_E_OK			0x00000000	/* OK */
#define QBASE_E_NOCONN		0x00000001	/* wrong connection handler or unable to connect */
#define QBASE_E_BUF			0x00000002	/* mismatch in client-side buffer size */
#define QBASE_E_MEM			0x00000003	/* server was unable to allocate appropriate memory */
#define QBASE_E_NOTSUPP		0x00000004

/* different table errors */
#define QBASE_E_NOTBLSEL	0x00000005	/* trying to manipulate data while no target table defined */
#define QBASE_E_NOTBLEXIST	0x00000006	/* trying to select table that does not exist */
#define QBASE_E_ALREADY		0x00000007	/* trying to create table that already exists */
#define QBASE_E_TBLLOCK		0x00000008	/* trying to manage (flush) table while it is in use */
#define	QBASE_E_FEWPLOCKS	0x00000009

/* different errors during data manipulation */
#define QBASE_E_FLTR		0x0000000A	/* types mismatch in filter definition */
#define QBASE_E_INS			0x0000000B	/* Was unable to read record from client */
#define	QBASE_E_NOFLD		0x0000000C	/* the specified field number is out of range for current table */
#define QBASE_E_LASTROW		0x0000000D	/* the last row was reached on the moment of request, but still later there can be more rows available */
#define	QBASE_E_PAGING		0x0000000E	/* CRITICAL! - we can not allocate page, we can not replace page */
#define	QBASE_E_PKEY		0x0000000F	/* the specified Primary key is out of range */


#endif /* _QBAASE_ERRORS_H_ */
