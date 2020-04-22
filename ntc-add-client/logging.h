#ifndef _LOGGING_H_
#define _LOGGING_H_

#include "ntc_common.h"




/* type used to perform detailed logging */

typedef struct sess{
	uint16_t		frame_type; /* frame type */
	struct in_addr	clnIP;			/* if frame is IP -> address of remote side */
	struct in_addr	dstIP;
	uint8_t			Proto;		/* if protocol of IP family */
	uint16_t		cli_port;	/* port from side of client - if appliable */
	uint16_t		rem_port;	/* port on the remote side - if appliable */
	uint32_t		bytes_snd;	/* bytes, sent by client */
	uint32_t		bytes_rcv;	/* bytes recieved by client */
	struct sess * next;
} sess_t;
	






/* type used to describe clients */

typedef struct clnt{
	mac_cmp_t	MAC;
	sess_t *	sessions;
	struct clnt * next;
} clnt_t;



#endif /* _LOGGING_H_ */
