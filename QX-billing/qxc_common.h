/*
 * -={ MikeP }=- (c) 2005
 */
 
/*
 * This is the only header to include
 * 
 * This is the header, containing definitions of commonly used structures
 * and values
 */

#ifndef _QXC_COMMON_H_
#define _QXC_COMMON_H_

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
#include	"qx_rm.h"
#include	"qxc_rules.h"
#include	"qxc_logging.h"




/*
 * The following is a bit tricky.
 * We use the following structure to reflect interfaces
 * present in the system thus makinig it it possible to bypass
 * packet capturing on some cards:
 * I don't need to count traffic on my "external" interface en1
 * but it is obviously required on "internal" en0
 * 
 * Within io-net each "physical" interface is described
 * by three numbers: cell, endpoint, iface - to understand their meaning
 * one should be familiar with io-net's architecture.
 * See the "Network DDK" programmer's guide.
 */

typedef struct _qxc_if_info{
	/* statistics */
	if_counters_t	cntrs;
	/* related to io-net */
	uint16_t		cell;
	uint16_t		endpoint;
	uint16_t		iface;
	
} qxc_if_info_t;




#include	"qxc_rm.h"




#endif /* _QXC_COMMON_H_*/
