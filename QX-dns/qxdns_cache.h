/*
 * 2005 (c) -={ MikeP }=-
 */

/* $Date: Mon 13, Jun 2005 */


#ifndef _QXDNS_CACHE_H_
#define	_QXDNS_CACHE_H_


typedef struct _cache_record{
	time_t		expires;	/* timeline when the record should expire */
	uint32_t	hits;		/* how often it is hit */
	uint32_t	flags;		/* flags of the original response message */
	uint32_t	datalen;	/* length of the response */
	uint8_t		pkt[512];	/* response itself */
} qxdns_cr_t;

/* Variables declared in cache.c
extern int	CacheMAX;
extern int	CacheTTL;
extern int	CacheCount;
*/


int qxdns_cache_init(int c_max, time_t c_ttl);
int qxdns_cache_add( dns_buffer_t * msg );
int qxdns_cache_lookup( dns_buffer_t * msg );




#endif	/* _QXDNS_CACHE_H_ */
