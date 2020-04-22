#ifndef _LOGGING_H_
#define _LOGGING_H_

#include "ntc_common.h"


/* type used to perform detailed logging */

typedef struct sess{
	uint16_t		frame_type; /* frame type */
	struct in_addr	clnIP;		/* if frame is IP -> address of remote side */
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



















/*
 * Structures needed by logger to keep tracking of the databases
 */

/* account information
 * we use __attribute__((__packed__)) modifier
 * to disable memory alignment of the structure
 * as it'll be used as row identifier
 */
typedef struct acnt_data{
	uint32_t	UIN;
	uint32_t	client;
	double		sum_current;
	char		account_number[20];
	double		pay_mb_out;
	double		pay_mb_in;
	uint32_t	closed;
} __attribute__((__packed__)) acnt_data_t;

typedef struct account{
	acnt_data_t		acnt;
	struct account * next;
} account_t;



/* details log to write to */
/* this structure does not contain information on timestamp */
typedef struct det_data{
	uint32_t	UIN;
	uint32_t	NIC;
	uint16_t	layer;
	uint16_t	proto;
	struct in_addr	IPcln;
	struct in_addr	IPdst;
	uint16_t	PortCli;
	uint16_t	PortDst;
	uint32_t	BytesToCli;
	uint32_t	BytesFromCli;
	double		SumToCli;
	double		SumFromCli;
}  __attribute__((__packed__)) det_data_t;

typedef struct logdet{
	det_data_t	dtls;
	struct logdet * next;
} logdet_t;



/* NIC descriptor to keep track of */
typedef struct nic_data{
	uint32_t	UIN;
	mac_addr_t	MAC;
	uint32_t	closed;
	uint32_t	accnt;
} __attribute__((__packed__)) nic_data_t;

typedef struct nic{
	nic_data_t	NCard;
	account_t *	acnt_p;
	logdet_t *	LOG;
	struct nic * next;
} nic_t;


#endif /* _LOGGING_H_ */
