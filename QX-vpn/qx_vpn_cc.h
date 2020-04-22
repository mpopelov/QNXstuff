/*
 * -={ MikeP }=- (c) 2005
 */

/*
 * Definitions for PPTP Control Connection handlng.
 * structures and values defined in network byte order.
 */






#ifndef	_QX_VPN_CC_H_
#define	_QX_VPN_CC_H_



#define	QX_VPN_VENDOR_STRING	"PPTP server (c) -={ MikeP }=-"

/*
 * Control connection states
 */
#define	CC_STATE_IDLE			0
#define	CC_STATE_ESTABLISHED	1
#define	CC_STATE_WAIT			2


/*
 * Message-specific payload
 */

#define	PPTP_PROTO_VERSION	0x0100

/* Control connection message cookie */
/*#define	MAGIC_COOKIE	0x1A2B3C4D*/
#define	MAGIC_COOKIE	0x4D3C2B1A /* network byte order */


/* PPTP Control Connection message types */
#define PPTPMT_CM		0x0100	/* control message */
#define	PPTPMT_MM		0x0200	/* management message */


/* general errors (in Network byte order) */
#define	CC_GE_OK	0x0000	/* no error */
#define	CC_GE_NC	0x0100	/* no control connection yet */
#define	CC_GE_BF	0x0200	/* bad format: wrong length or magic cookie */
#define	CC_GE_BV	0x0300	/* bad value: some field out of range or rsvX != 0 */
#define	CC_GE_NR	0x0400	/* No resources to handle command now */
#define	CC_GE_BC	0x0500	/* Bad call ID - invalid in this context */
#define	CC_GE_PE	0x0600	/* vendor-specific error occured */




/*
 * Control message types
 */

/* Control connection management */
#define	CMT_STARTREQ	0x0100	/* Start control connection request */
#define	CMT_STARTREP	0x0200	/* Start control connection reply */
#define	CMT_STOPREQ		0x0300	/* Stop control connection request */
#define	CMT_STOPREP		0x0400	/* Stop control connection reply */
#define	CMT_ECHOREQ		0x0500	/* Echo request */
#define	CMT_ECHOREP		0x0600	/* Echo reply */

/* Call management */
#define	CMT_OCALLREQ	0x0700	/* Outgoing call request */
#define	CMT_OCALLREP	0x0800	/* Outgoing call reply */
#define	CMT_ICALLREQ	0x0900	/* Incoming call request */
#define	CMT_ICALLREP	0x0A00	/* Incoming call reply */
#define	CMT_ICALLCON	0x0B00	/* Incoming call connected */
#define	CMT_CCLEAREQ	0x0C00	/* Call clear request */
#define	CMT_CDISCNFY	0x0D00	/* Call disconnect notify */

/* Error reporting */
#define	CMT_WANERNFY	0x0E00	/* WAN error notify */

/* PPP Session Control */
#define	CMT_SLINKNFO	0x0F00	/* Set link info */




/*
 * Here comes description of standart control
 * messages payload.
 */

/* start-control-connection-request */
typedef struct{
	uint16_t pver;		/* protocol version = 0x0100 */
	uint16_t reserved;	/* reserved = 0 */
	uint32_t framing;	/* framing capabilities */
	uint32_t bearer;	/* bearer capabilities */
	uint16_t maxchan;	/* maximum channels - ignored? */
	uint16_t fwver;		/* firmware version */
	char     hname[64];	/* host name */
	char     vname[64];	/* vendor string */
} ccm_startreq_t;
/* length = 144, total length = 156 */




/* start-control-connection-reply */
typedef struct{
	uint16_t pver;		/* protocol version = 0x0100 */
	uint16_t reserc;	/* result code + error code  */
	uint32_t framing;	/* framing capabilities */
	uint32_t bearer;	/* bearer capabilities */
	uint16_t maxchan;	/* maximum channels - ignored? */
	uint16_t fwver;		/* firmware version */
	char     hname[64];	/* host name */
	char     vname[64];	/* vendor string */
} ccm_startrep_t;
/* length = 144, total length = 156 */

#define	STARTREP_RC_OK	0x0001	/* channel created */
#define	STARTREP_RC_GE	0x0002	/* general error */
#define	STARTREP_RC_CE	0x0003	/* channel already exists */
#define	STARTREP_RC_NA	0x0004	/* requester is not authorized */
#define	STARTREP_RC_PE	0x0005	/* requester proto version not supported */




/* stop-control-connection-request */
typedef struct{
	uint16_t reason;	/* reason for closing / result of closing */
	uint16_t reserved;	/* reserved = 0 */
} ccm_stopreq_t;
/* length = 4, total length = 16 */

#define	STOPREQ_RS_NO	0x0001	/* General request to clear connection*/
#define	STOPREQ_RS_SP	0x0002	/* Can not support peers version of protocol */
#define	STOPREQ_RS_LS	0x0003	/* logical shutdown */




/* stop-control-connection-reply */
typedef struct{
	uint16_t reserc;	/* result of closing */
	uint16_t reserved;	/* reserved = 0 */
} ccm_stoprep_t;
/* length = 4, total length = 16 */

#define	STOPREP_RC_OK	0x0001	/* channel destroyed */
#define	STOPREP_RC_GE	0x0002	/* general error */




/* echo-request */
typedef struct{
	uint32_t id;		/* identifier */
} ccm_echoreq_t;
/* length = 4, total length = 16 */




/* echo-reply */
typedef struct{
	uint32_t id;		/* identifier */
	uint16_t reserc;	/* result code + error code */
	uint16_t reserved;	/* reserved */
} ccm_echorep_t;
/* length = 8, total length = 20 */

#define	ECHOREP_RC_OK	0x0001	/* no error */
#define	ECHOREP_RC_GE	0x0002	/* general error */




/* Outgoing-call-request */
typedef struct{
	uint16_t call_id;
	uint16_t call_sn;
	uint32_t min_bps;
	uint32_t max_bps;
	uint32_t bearer;
	uint32_t framing;
	uint16_t pkt_window_size;
	uint16_t pkt_delay;
	uint16_t phone_len;
	uint16_t reserved;
	char     phone[64];
	char     subaddr[64];
} ccm_ocallreq_t;
/* length = 156, total length = 168 */




/* Outgoing-call-reply */
typedef struct{
	uint16_t call_id;
	uint16_t call_id_peer;
	uint16_t reserc;
	uint16_t cause;
	uint32_t speed;
	uint16_t pkt_window_size;
	uint16_t pkt_delay;
	uint32_t pchan_id;
} ccm_ocallrep_t;
/* length = 20, total length = 32 */

#define	OCALLREP_RC_OK	0x0001	/* call established */
#define	OCALLREP_RC_GE	0x0002	/* general error */
#define	OCALLREP_RC_NC	0x0003	/* no carrier */
#define	OCALLREP_RC_BS	0x0004	/* line busy */
#define	OCALLREP_RC_NT	0x0005	/* no dialtone */
#define	OCALLREP_RC_TO	0x0006	/* call timed out */
#define	OCALLREP_RC_NA	0x0007	/* administratively prohibited */




/* Incoming-call-rerequest */
typedef struct{
	uint16_t call_id;
	uint16_t call_sn;
	uint32_t bearer;
	uint32_t pchan_id;
	uint16_t dialed_l;
	uint16_t dialing_l;
	char     dialedpn[64];
	char     dialingn[64];
	char     subaddr[64];
} ccm_icallreq_t;
/* length = 208, total length = 220 */




/* Incoming-call-reply */
typedef struct{
	uint16_t call_id;
	uint16_t call_id_peer;
	uint16_t reserc;
	uint16_t pkt_window_size;
	uint16_t pkt_delay;
	uint16_t reserved;
} ccm_icallrep_t;
/* length = 12, total length = 24 */

#define	ICALLREP_RC_OK	0x0001	/* PAC should answer the call */
#define	ICALLREP_RC_GE	0x0002	/* general error */
#define	ICALLREP_RC_NO	0x0003	/* PAC should not accept the call */




/* Incoming-call-connected */
typedef struct{
	uint16_t call_id_peer;
	uint16_t reserved;
	uint32_t speed;
	uint16_t pkt_window_size;
	uint16_t pkt_delay;
	uint32_t framing;
} ccm_icallcon_t;
/* length = 16, total length = 28 */




/* Call-clear-request */
typedef struct{
	uint16_t call_id;
	uint16_t reserved;
} ccm_ccleareq_t;
/* length = 4, total length = 16 */




/* Call-disconnect-notify */
typedef struct{
	uint16_t call_id;
	uint16_t reserc;
	uint16_t cause;
	uint16_t reserved;
	char     stats[128];
} ccm_cdiscnfy_t;
/* length = 136, total length = 148 */

#define	CDISCNFY_RC_LC	0x0001	/* disconnected due to loss of carrier */
#define	CDISCNFY_RC_GE	0x0002	/* general error */
#define	CDISCNFY_RC_AM	0x0003	/* Administrative reasons for shutdown */
#define	CDISCNFY_RC_RQ	0x0004	/* call cleared by peer's request */




/* wan-error-notify */
typedef struct{
	uint16_t call_id;
	uint16_t reserved;
	uint32_t crc_errs;
	uint32_t frm_errs;
	uint32_t hw_overruns;
	uint32_t buf_overruns;
	uint32_t tout_errs;
	uint32_t align_errs;
} ccm_wanernfy_t;
/* length = 28, total length = 40 */




/* set-link-info */
typedef struct{
	uint16_t call_id;
	uint16_t reserved;
	uint32_t send_accm;
	uint32_t recv_accm;
} ccm_slinknfo_t;
/* length = 12, total length = 24 */




/* this is a general structure that accepts message from client.
 * On excepting message "data" member is the casted to whatever
 * type is appropriate */
typedef struct{
	uint16_t msglen;
	uint16_t pptpm_type;
	uint32_t cookie;
	uint16_t ccm_type;
	uint16_t reserved;	/* should always be 0 */
	char     data[208];
	/* for str* functions do not crash us */
	uint32_t fin;		/* should always be 0 */
} cc_message_t;
/* length = 12 + size of the largest payload (208) + padding to 4 byte boundary */

#define MAX_CCM_LEN 220






int handle_ccm(client_t *client);




#endif	/* _QX_VPN_CC_H_ */