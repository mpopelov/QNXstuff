/*
 * Standart header of message - present in all types of messages
 * 
 * All fields are in network byte order.
 */

/* $Date: Mon 13, Jun 2005 */

#ifndef _QXDNS_FMT_H_
#define	_QXDNS_FMT_H_


typedef struct dns_msg_header{
	uint16_t	ID;
	uint16_t	OPT;
	uint16_t	QDCOUNT;
	uint16_t	ANCOUNT;
	uint16_t	NSCOUNT;
	uint16_t	ARCOUNT;
	char		data[4];	/* as pointer to the rest of the message */
} dns_msg_header_t;

/* the following options are defined in assumption
 * all data is in network byte order */

#define	DNS_OPT_QR			0x0080	/* if set = response, if cleared = request */

#define	DNS_OPT_OPCODE_MASK	0x0078	/* the mask to access opcode values */
#define	DNS_OPT_OPCODE_SQ	0x0000	/* standart query */
#define	DNS_OPT_OPCODE_IQ	0x0008	/* inverse query */
#define	DNS_OPT_OPCODE_SS	0x0010	/* server status request */

#define	DNS_OPT_AA			0x0004	/* authoritative answer */
#define	DNS_OPT_TC			0x0002	/* truncation of message happened */
#define	DNS_OPT_RD			0x0001	/* recursion desired */
#define	DNS_OPT_RA			0x8000	/* recursion available */
#define	DNS_OPT_ZERO_MASK	0x7000	/* masked bits should always be 0 */

#define	DNS_OPT_RCODE_MASK	0x0F00	/* the mask to access return codes */
#define	DNS_OPT_RCODE_OK	0x0000	/* no error condition */
#define	DNS_OPT_RCODE_FE	0x0100	/* format error */
#define	DNS_OPT_RCODE_SF	0x0200	/* server failure */
#define	DNS_OPT_RCODE_NE	0x0300	/* Name Error - nonexistant name */
#define	DNS_OPT_RCODE_NI	0x0400	/* ability not implemented */
#define	DNS_OPT_RCODE_RF	0x0500	/* refused */



/*
 * This structure describes question information section
 */
typedef struct dns_msg_qinfo{
	uint16_t	QTYPE;
	uint16_t	QCLASS;
} dns_msg_qinfo_t;



/*
 * This structure describes RR information section
 */
typedef struct dns_msg_rrinfo{
	uint16_t	ans;
	uint16_t	type;
	uint16_t	class;
	uint32_t	ttl;
	uint16_t	rdlen;
	char		rdata[4];
} __attribute__((__packed__)) dns_msg_rrinfo_t;



/* this defines TYPES of DNS records */
/* ALL IN NETWORK BYTE ORDER */
#define	DNS_T_A			0x0100	/* a host address */
#define	DNS_T_NS		0x0200	/* an authoritative name server */
#define	DNS_T_MD		0x0300	/* ---obsolete--- */
#define	DNS_T_MF		0x0400	/* ---obsolete--- */
#define	DNS_T_CNAME		0x0500	/* canonical name for alias */
#define	DNS_T_SOA		0x0600	/* start of authority */
#define	DNS_T_MB		0x0700	/* ---experimental--- */
#define	DNS_T_MG		0x0800	/* ---experimental--- */
#define	DNS_T_MR		0x0900	/* ---experimental--- */
#define	DNS_T_NULL		0x0A00	/* ---experimental--- */
#define	DNS_T_WKS		0x0B00	/* well known service descriptor */
#define	DNS_T_PTR		0x0C00	/* a domain name pointer */
#define	DNS_T_HINFO		0x0D00	/* host information */
#define	DNS_T_MINFO		0x0E00	/* mailbox or mail list info */
#define	DNS_T_MX		0x0F00	/* mail exchange */
#define	DNS_T_TXT		0x1000	/* text strings */

/* additional QTYPES */
#define	DNS_T_AXFR		0xFC00	/* request for entire zone transfer */
#define	DNS_T_MAILB		0xFD00	/* request for MB, MG, MR */
#define	DNS_T_MAILA		0xFE00	/* requesdt for mail agent RRs -> use MX */
#define	DNS_T_ALL		0xFF00	/* request for all records */


/* CLASSES of DNS records */
#define	DNS_C_IN		0x0100	/* the Internet DNS */
#define	DNS_C_CS		0x0200	/* CSNET ---obsolete--- */
#define	DNS_C_CH		0x0300	/* CHAOS */
#define	DNS_C_HS		0x0400	/* Hesiod [Dyer 87] */
#define	DNS_C_ANY		0xFF00	/* any class */



#define	DNS_DATA_BEGIN	12



#endif	/* _QXDNS_FMT_H_ */
