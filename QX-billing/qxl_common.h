/*
 * 2005 (c) -={ MikeP }=-
 */



#ifndef _QXL_COMMON_H_
#define	_QXL_COMMON_H_



#include	<stdio.h>
#include	<stdlib.h>
#include	<stddef.h>

#include	<unistd.h>
#include	<errno.h>
#include	<time.h>

#include	<atomic.h>
#include	<pthread.h>

#include	<malloc.h>
#include	<alloca.h>

#include	<net/if.h>
#include	<net/if_arp.h>
#include	<net/if_types.h>
#include	<netinet/in.h>

#include	<sys/iofunc.h>
#include	<sys/ioctl.h>
#include	<sys/dispatch.h>
#include	<devctl.h>
#include	<sys/io-net.h>
#include	<sys/mman.h>
#include	<fcntl.h>

#include	<string.h>

#include	<gulliver.h>

#include	"qx_common.h"
#include	"qx_logging.h"
#include	"qx_rules.h"
#include	"qx_rm.h"

#include	"qxl_db.h"




/* this structure data that describes how much costs
 * incoming and outgoing traffic during this hour of
 * day.
 */
typedef struct _tax_times{
	uint32_t	UID;
	double		cur_tax_to_cli;
	double		cur_tax_from_cli;
} qxl_tax_time_t;



/* Details of active taxes are stored in a list */
typedef struct _tax_details qxl_tax_det_t;
struct _tax_details{
	uint32_t		ip_mask;
	uint32_t		ip_addr;
	uint16_t		ip_proto;
	uint16_t		ip_port;
	double			tax_to_cli;
	double			tax_from_cli;
	qxl_tax_det_t	*next;
};

/* Placeholder for active taxes */
typedef struct _active_tax{
	uint32_t		UID;
	double			def_tax_to_cli;
	double			def_tax_from_cli;
	qxl_tax_det_t	*td;
} qxl_tax_t;

/* Placeholder for active accounts */
typedef struct _active_account{
	uint32_t	UID;
	qxl_tax_t	*tax;
} qxl_accnt_t;

/* Placeholder for active NICs */
typedef struct _active_NIC{
	uint32_t		UID;
	qxl_accnt_t		*account;
	qx_rule_nics_t	rule;
} qxl_NIC_t;
























#endif	/* _QXL_COMMON_H_ */

