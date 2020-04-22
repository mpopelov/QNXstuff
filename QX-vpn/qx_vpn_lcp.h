/*
 * -={ MikeP }=- (c) 2005
 */



#ifndef	_QX_VPN_LCP_H_
#define	_QX_VPN_LCP_H_



/* LCP message codes (1 byte) */
#define	LCP_CONF_VEN	0x00	/* vendor specific extentions to the PPP */
#define	LCP_CONF_REQ	0x01	/* request */
#define	LCP_CONF_ACK	0x02	/* confirm request */
#define	LCP_CONF_NAC	0x03	/* not acknowledged + hint */
#define	LCP_CONF_REJ	0x04	/* reject options */
#define	LCP_TERM_REQ	0x05
#define	LCP_TERM_ACK	0x06
#define	LCP_CODE_REJ	0x07
#define	LCP_PROT_REJ	0x08
#define	LCP_ECHO_REQ	0x09
#define	LCP_ECHO_REP	0x0A
#define	LCP_DISC_REQ	0x0B	/* discard request */
#define	LCP_IDNT_MSG	0x0C	/* identification message */
#define	LCP_TIME_REM	0x0D	/* time remaining */
#define	LCP_RSET_REQ	0x0E
#define	LCP_RSET_ACK	0x0F


/* LCP option codes (1 byte) */
#define	LCP_OPT_VNDR	0x00	/* vendor extentions to options */
#define	LCP_OPT_MRU		0x01	/* maximum receive unit */
#define	LCP_OPT_ACCM	0x02
#define	LCP_OPT_AP		0x03	/* authentication protocol */
#define	LCP_OPT_QP		0x04	/* quality protocol */
#define	LCP_OPT_MN		0x05	/* magic number */

/* the codes below are  not supported by this implementation as
 * we do not need it on PPTP connection */
/*
#define	LCP_OPT_RSV		0x06	// reserved
#define	LCP_OPT_PCF		0x07	// protocol field compression
#define	LCP_OPT_ACFC	0x08	// address & control field compression
#define	LCP_OPT_FCS		0x09
#define	LCP_OPT_SDP		0x0A
#define	LCP_OPT_NM		0x0B	// numbered mode
#define	LCP_OPT_MP		0x0C	// multi link PPP - do not support it
#define	LCP_OPT_CB		0x0D	// call-back
#define	LCP_OPT_CT		0x0E	// connect time - obsolete
#define	LCP_OPT_CF		0x0F	// compound frame - obsolete
#define	LCP_OPT_NDE		0x10	// obsolete
#define	LCP_OPT_		0x11
#define	LCP_OPT_		0x12
#define	LCP_OPT_		0x13
#define	LCP_OPT_		0x14
#define	LCP_OPT_		0x15
#define	LCP_OPT_		0x16
#define	LCP_OPT_		0x17
#define	LCP_OPT_		0x18
#define	LCP_OPT_		0x19
#define	LCP_OPT_		0x1A
#define	LCP_OPT_		0x1B
#define	LCP_OPT_		0x1C
#define	LCP_OPT_		0x1D
#define	LCP_OPT_		0x1E
*/





















#endif	/* _QX_VPN_LCP_H_ */
