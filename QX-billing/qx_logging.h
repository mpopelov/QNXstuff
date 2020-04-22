/*
 * -={ MikeP }=- (c) 2005
 */


/*
 * This file contains data definition that is shared by io-net
 * module and its logger client
 * 
 * - Log structure description
 * - Constants for handling logging queue
 * - etc
 */


#ifndef	_QX_LOGGING_H_
#define	_QX_LOGGING_H_


/* This many packets our queue can store.
 * Client software should be able to get all the queue within
 * one devctl(). If there is not enough place for the queue to be stored
 * in client-supplied buffer - as many full packets as possible
 * are stored in buffer and the rest are dismissed, allowing buffer for reuse
 */

#define		QX_DEF_LOG_LIMIT	1000

/*
 * This structure contains data we capture from the packet.
 * We are supposed to support only IP family of protocols
 */
typedef struct _qx_logging_info{
	uint8_t		mac[6];			/* MAC of client */
	uint16_t	DLC_proto;		/* DLC protocol used in packet */
	
	uint32_t	IP_clnt;		/* client IP - if possible */
	uint32_t	IP_host;		/* host IP - if possible */
	uint16_t	IP_proto;		/* IP family protocol - if possible */
	uint16_t	Port_clnt;		/* client port - if possible */
	uint16_t	Port_host;		/* host port - if possible */
	
	uint32_t	bytes_to_cli;	/* bytes sent to client by host */
	uint32_t	bytes_to_host;	/* bytes sent to host by client */
} qx_log_t;

/* size of one log record */
#define		QX_LOG_MSG_SIZE		sizeof(qx_log_t)

/* default log buffer size (message granularity) */
#define		QX_LOG_BUFFER_SIZE	QX_DEF_LOG_LIMIT*QX_LOG_BUFFER_SIZE


/*
 * Here follow structures and definitions we use to parse packet data
 */
#define	QXC_PKT_TOCLI	1
#define	QXC_PKT_FROMCLI	0

#define	PKT_LOG_LEN		80

#define	PROTO_DLC_ARP	0x0608
#define	PROTO_DLC_IPv4	0x0008

#define	PROTO_IPv4_ICMP	1	/* ICMP = 1 */
#define	PROTO_IPv4_TCP	6	/* TCP = 6 */
#define	PROTO_IPv4_UDP	17	/* UDP = 17 */



#endif	/* _QX_LOGGING_H_ */
