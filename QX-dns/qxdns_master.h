/*
 * 2005 (c) -={ MikeP }=-
 */

/* $Date: Mon 13, Jun 2005 */


#ifndef _QXDNS_MASTER_H_
#define	_QXDNS_MASTER_H_

#define	MASTER_EXPIRE 33600


typedef struct _inaddr_r		mr_inaddr_t;
typedef struct _master_record	qxdns_mr_t;
typedef struct _master_zone		qxdns_mz_t;




/* records that describe different types of answers */

typedef struct _mr_str{
	char				name[252];
	struct _mr_str *	next;
} mr_str_t;




typedef struct _ans_a{
	uint16_t	n_pointer;	/*	pointer to the FQDN of the following IP */
	uint16_t	type;		/* type of the record = DNS_T_A */
	uint16_t	class;		/* class of data = DNS_C_IN */
	uint32_t	ttl;		/* time of record to live */
	uint16_t	len;		/* length of data = 4 */
	uint32_t	addr;		/* IP address of the node */
} __attribute__((__packed__)) ans_a_t;




struct _inaddr_r{
	mr_inaddr_t *	next;
	ans_a_t			ans;			/* answer to the request */
	char			arpa[32];		/* the inaddr.arpa name (e.g. 3.1.0.10.inaddr.arpa) */
};




struct _master_record{
	qxdns_mr_t *		next;
	qxdns_mr_t *		ns_next;
	qxdns_mr_t *		mx_next;
	uint16_t			mx_pref;	/* if the host is to be MX - it is the preference of the host */
	uint32_t			ttl;
	mr_str_t *			aliases;	/* alias names of the host */
	mr_inaddr_t *		inaddrs;	/* IP addresses of the host */
	char				cname[256];	/* CNAME of the host */
};




struct _master_zone{
	qxdns_mz_t * 	next;
	qxdns_mr_t *	zone_NS;		/* name servers for the zone */
	qxdns_mr_t *	zone_MX;		/* mail servers for the zone */
	qxdns_mr_t *	hosts;
	char			zone_name[256];	/* name of the zone we support */
};






qxdns_mz_t *	qxdns_new_mz(char * name);
qxdns_mr_t *	qxdns_new_mr(char * cname, uint32_t ttl);
int				qxdns_add_mr_alias(qxdns_mr_t * host, char * alias);
int				qxdns_add_mr_addr(qxdns_mr_t * host, uint32_t addr);
int				qxdns_add_mr(qxdns_mz_t * zone, qxdns_mr_t * host, int ns, uint16_t mx_pref);
int				qxdns_master_lookup(dns_buffer_t * query);








#endif	/* _QXDNS_MASTER_H_ */
