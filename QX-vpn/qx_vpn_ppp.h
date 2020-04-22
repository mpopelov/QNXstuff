/*
 * -={ MikeP }=- (c) 2005
 */



/*
 * Here comes description of all that we have to know
 * about PPTP GRE session and PPP packets
 */


#ifndef _QX_VPN_PPP_H_
#define _QX_VPN_PPP_H_





typedef struct{
	uint16_t	flags;
	uint16_t	protocol;
	uint16_t	key_hw;	/* payload length */
	uint16_t	key_lw;	/* call ID */
	uint32_t	dat1;	/* sequence number - OPTIONAL */
	uint32_t	dat2;	/* acknowledgment number - OPTIONAL */
	uint32_t	dat3;	/* used to manage pointers */
} gre_packet_t;


#define	GRE_PROTOCOL	0x0B88	/* Ethertype for PPP (in network byte order) */

#define	GRE_FL_S	0x0010	/* ==1 if payload present / ==0 if only ACC is present */
#define	GRE_FL_A	0x8000	/* ==1 if ACC present / ==0 if no ACC */

/*                              AffffvvvCRKSsrrr   */
/*#define	GRE_SANITY_MASK		0111111111101111*/
#define	GRE_SANITY_MASK		0x7fef
/*#define	GRE_SANITY_VALUE	0000000100100000*/
#define	GRE_SANITY_VALUE	0x0120
/*#define	GRE_RESPONSE_FLAGS	1000000100100000 */
#define	GRE_RESPONSE_FLAGS	0x8120




/*
 * PPP protocol typres we understand
 */
#define	PPPPROTO_IP		0x2100
#define	PPPPROTO_LCP	0x21C0
#define	PPPPROTO_IPCP	0x2180
#define	PPPPROTO_ECP	0x5380
#define	PPPPROTO_CCP	0xFD80
#define	PPPPROTO_PAP	0x23C0
#define	PPPPROTO_CHAP	0x24C2







int handle_ppp(client_t *client);

#endif /* _QX_VPN_PPP_H_ */
