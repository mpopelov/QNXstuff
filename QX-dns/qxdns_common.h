/*
 * -=MikeP=- (c) 2005
 * 
 * A simple DNS server with proxy capabilities and caching
 * of queries
 * 
 */

/* $Date: Mon 13, Jun 2005 */

#ifndef _QXDNS_COMMON_H_
#define _QXDNS_COMMON_H_



#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<time.h>
#include	<pthread.h>
#include	<errno.h>
#include	<netinet/in.h>
#include	<sys/dispatch.h>
#include	<sys/socket.h>
#include	<sys/select.h>
#include	<gulliver.h>

#include	"qxdns_general.h"
#include	"qxdns_fmt.h"
#include	"GetConf.h"



#define		DNS_DEF_PORT	53
#define		DNS_DEF_BUFFERS	200
#define		DNS_DEF_TTL		30


typedef	struct _dns_buffer		dns_buffer_t;
typedef	struct _dns_connector	dns_connector_t;




struct _dns_buffer{
	/* this stuff deals with connection info */
	struct sockaddr_in	from;		/* address of client we got query from */
	int					fromlen;
	dns_connector_t *	qclient;	/* address of client we got query from */
	dns_connector_t *	qserver;	/* this server was queried */
	
	/* query related info */
	uint16_t			ref_ID;		/* ID of query when sent to server */
	uint16_t			src_ID;		/* ID of query when received from client */
	time_t				rcvtime;	/* when the query was received */
	
	/* linking stuff */
	dns_buffer_t *		next;
	dns_buffer_t *		prev;
	
	/* the packet itself */
	int					datalen;	/* length of data in the packet */
	char				pkt[512];	/* data from client */
};




/* this structure describes each of sockets, created for communication */

struct _dns_connector{
	int					sock;	/* socket to the server */
	int					misses;	/* how many times there were no answer from server */
	struct sockaddr_in	addr;	/* address of the server */
	dns_connector_t *	next;	/* reference to the next connector */
};




/* include cache header here so that dns_buffer_t is declared beforehand */
#include	"qxdns_cache.h"
#include	"qxdns_master.h"

#endif /* _QDNS_COMMON_H_ */
