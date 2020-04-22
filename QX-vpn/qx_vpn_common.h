/*
 * 2005 (c) -={ MikeP }=-
 * Mike A Popelov
 * 
 * mikep@kaluga.org
 * mikep@qnx.org.ru
 * ICQ: 104287300
 */


#ifndef _VPN_COMMON_H_
#define _VPN_COMMON_H_

#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<pthread.h>
#include	<errno.h>
#include	<string.h>
#include	<fcntl.h>
#include	<time.h>

#include	<sys/socket.h>
#include	<sys/types.h>
#include	<sys/iofunc.h>
#include	<sys/dispatch.h>
#include	<sys/dir.h>

#include	<netinet/in.h>
#include	<netinet/tcp.h>

#include	<gulliver.h>






/* PPP over IP protocol number: IPPROTO_GRE */
#define		PPTP_PORT	1723


/* 
 * Logging functionality
 */

#define	QLL_SYSTEM	0
#define	QLL_ERROR	1
#define	QLL_WARNING	2
#define	QLL_INFO	3

extern int	LogLevel;
void qx_vpn_log(int log_level, const char * fmt, ...);

/******************************************************************************/






/*
 * Structure that describes specific client
 * and its state
 */
typedef struct client_connection_record client_t;
struct client_connection_record{
	/* management of record */
	pthread_mutex_t		mutex;
	
	/* PPtP control connection specific */
	int					cc_sock;		/* socket for control connection */
	struct sockaddr_in	cli_addr;		/* Client connected from that IP:port */
	socklen_t			cli_len;		/* latter length */
	
	uint32_t			cc_state;		/* actual state of control connection */
	uint32_t			cc_cookie;
	uint16_t			cc_call_id;
	uint16_t			cc_call_sn;
	uint16_t			call_id;
	uint32_t			saccm;
	uint32_t			raccm;
	
	/* specific to current PPP session */
	int					raw_sock;		/* socket for GRE packets */
	uint32_t			ppp_state;		/* state of ppp connection */
	
	uint32_t			snd_num;	/* number of next packet to be sent */
	uint32_t			snd_ack;	/* we are waiting ack for this or later packet */
	
	uint32_t			rcv_num;	/* number of packet we are waiting for */
	uint32_t			rcv_ack;	/* number of peer's packet we ack-ed last */
	
	uint32_t			ppp_data_in;
	uint32_t			ppp_data_out;
	
	/* specific to current user record in database */
	int					db_handle;	/* connection to db */
	int					c_n;		/* ID of user in database */
	
	/* communication */
	uint8_t				pkt_in[1500];
	uint8_t				pkt_out[1500];
};



extern dispatch_t		*dpp_tcp;
extern select_attr_t	attr_tcp;
int cc_action(select_context_t *ctp, int sock, unsigned flags, void * handle);





/* now we incluude all that we may need for
 * correct protocol handling */
#include	"qx_vpn_cc.h"
#include	"qx_vpn_ppp.h"





#endif /* _VPN_COMMON_H_ */
