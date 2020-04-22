/*
 * 2005 (c) -={ MikeP }=-
 */

/*
 * Data definition for rule parsing goes here.
 * Only data types common for logger client and io-net module come here.
 */


#ifndef _QX_RULES_H_
#define _QX_RULES_H_


/*
 * This info is for granting communication to
 * resources tax free
 */
typedef struct _qx_rule_exempt{
	uint32_t	active;	/* ==1 means active for listed users, ==2 means active for everyone */
	uint32_t	ip_mask;
	uint32_t	ip_addr;
	uint16_t	ip_proto;
	uint16_t	ip_port_lo;
	uint16_t	ip_port_hi;
	uint16_t	dlc_proto;
} qx_rule_exempt_t;


/*
 * This infor is for granting access to clients
 * explicitly listed on the rule tree.
 */
typedef struct _qx_rule_nics{
	uint32_t	UID;	/* user IDs to link them to allowed services */
	uint32_t	ip_addr;
	uint8_t		mac[6];
} qx_rule_nics_t;




#endif /* _QX_RULES_H_ */
