/*
 * 2005 (c) -={ MikeP }=-
 */

/* $Date: Mon 13, Jun 2005 */

/*
 * Here comes implementation of functions dealing with cache
 */

#include	"qxdns_common.h"

pthread_rwlock_t cache_rwlock = PTHREAD_RWLOCK_INITIALIZER;

qxdns_cr_t ** CacheROOT;


int		CacheMAX = 100;
time_t	CacheTTL = 86400;
int		CacheCount = 0;


int qxdns_cache_init(int c_max, time_t c_ttl){
	
	CacheMAX = c_max;
	CacheTTL = c_ttl;
	
	/* create array of cache records */
	if( NULL == (CacheROOT = (qxdns_cr_t**) malloc(CacheMAX * sizeof(qxdns_cr_t*))) ) return 0;
	memset(CacheROOT, 0, CacheMAX*sizeof(qxdns_cr_t*));
	return 1;
}





/*
 * This routine adds records to cache and maintains cache cleanups
 * simultaneously so that expired records are removed from cache
 * as soon as possible.
 *
 * Do we need non-authoritative "don't know" records in cache?
 *
 * Returns 1 on success and 0 on failure.
 */
int qxdns_cache_add( dns_buffer_t * msg ){
	
	int				i;
	qxdns_cr_t *	rec		= NULL;
	qxdns_cr_t *	oldrec	= NULL;
	int				minhits	= INT_MAX; /* ? */
	int				index	= -1;
	time_t			exp		= msg->rcvtime + 172800; /* now + 2 days */
	dns_msg_header_t * dnsh	= (dns_msg_header_t*)msg->pkt;

	/* start scanning the cache to see if there is room for a new message */
	if( NULL == (rec = (qxdns_cr_t*)malloc(sizeof(qxdns_cr_t))) ) return 0;
	
    /* copy packet data to a fresh entry */
    memcpy( rec->pkt, msg->pkt, msg->datalen );
    rec->expires = msg->rcvtime + CacheTTL;
    rec->hits = 1;
    rec->datalen = msg->datalen;

    /* now it is time to really add fresh entry */
    pthread_rwlock_wrlock( &cache_rwlock );
    
    /* scan cache to find expired records */
    for(i=0;i<CacheCount;i++){
    	/* see if this record is expired on timeout */
    	if(CacheROOT[i]->expires < msg->rcvtime){
    		/* this record expired on timeout */
    		printf("CACHE: going to delete expired record\n");
    		index = i;
    		break;
    	}
        /* This is the case if record is not expired */
        if(CacheROOT[i]->hits <= minhits){
        	/* if the record is hit rarely than the noticed one
        	 * mark it as candidate for replacement.
             * If both records have hits count equal then notice the one that
             * would expire earlier */
        	if( (CacheROOT[i]->hits==minhits) && (CacheROOT[i]->expires>exp) ) continue;
        	minhits = CacheROOT[i]->hits;
        	exp = CacheROOT[i]->expires;
        	index = i;
        }
    }

    /* see if there were no expired records */
    /* if nothing expired and we have a room on Cache - add record to the
     * end of the cache */
    if((i==CacheCount) && (i<CacheMAX)){
    	/* we have a room at the end of cache */
    	printf("CACHE: adding one more record to cache\n");
    	CacheROOT[i] = rec;
    	CacheCount++;
    }else{
    	/* here we are because something expired or we should
    	 * replace some record in the cache */
    	printf("CACHE: replacing record in cache\n");
    	oldrec = CacheROOT[index];
    	CacheROOT[index] = rec;
    }

    pthread_rwlock_unlock( &cache_rwlock );

    /* if during scan we found a record that is to be deleted -
     * free it */
    if(oldrec != NULL) free(oldrec);
    
    /* !DEBUG! */
    printf("CACHE: ******  %i records in cache ******\n",CacheCount);
    
	return 1;
}






/*
 * This routine looks inside cache and if the record is found
 * it makes fixes in the supplied buffer, replacing original
 * query with the available answer.
 *
 * Returns 0 and msg untouched if nothing found
 * Returns 1 and valid answer in msg if all OK
 */
int qxdns_cache_lookup( dns_buffer_t * msg ){

	int i =0;
	dns_msg_header_t * dnsh = (dns_msg_header_t*)msg->pkt;

	pthread_rwlock_rdlock( &cache_rwlock );

	for(i=0;i<CacheCount;i++){
		if(CacheROOT[i]->expires < msg->rcvtime) continue; /* do not even try to look expired records */
		
		/* compare contents */
		if(!stricmp( &(msg->pkt[12]), &(CacheROOT[i]->pkt[12]) )){
			atomic_add( &(CacheROOT[i]->hits) , 1 );
			memcpy( &(msg->pkt[2]), &(CacheROOT[i]->pkt[2]), (CacheROOT[i]->datalen-2) );
			/* fix TTL ? */
			/*dnsh->TTL = ENDIAN_RET32(CacheROOT[i]->expires - msg->qinfo.rcvtime);*/
			msg->datalen = CacheROOT[i]->datalen;
			pthread_rwlock_unlock( &cache_rwlock );
			printf("CACHE: record found\n");
			return 1;
		}
	}

	pthread_rwlock_unlock( &cache_rwlock );
	printf("CACHE: Record not found\n");
	return 0;
}
