/*
 * 2005 (c) -={ MikeP }=-
 */




/*
 * This file contains functions related to data logging within io-net module
 */


#include	"qxc_common.h"







typedef struct _IP_H{	/* IP header of the packet */
	uint8_t		h_ver;		/* 4bits = version + 4bits = length of header */
	uint8_t		TOS;		/* Type of service */
	uint16_t	len;		/* (IP header + data) length */
	uint16_t	ID;
	uint16_t	f_offset;	/* fragment flags and offset */
	uint8_t		TTL;		/* time to live hops/seconds */
	uint8_t		proto;		/* protocol type of IP family */
	uint16_t	CRC;		/* checksum */	
	uint32_t	ip_src;		/* source IP address */
	uint32_t	ip_dst;		/* destination IP address */
} __attribute__((__packed__)) IP_t;

typedef struct _ordinal_tcp_udp{
	uint16_t	src_port;
	uint16_t	dst_port;
} __attribute__((__packed__)) tcpudp_t;











/*
 * Variables - local to the module, but global for all threads
 */
int			a_log_fd;
int			s_log_fd;
int			log_fd_num = 0;

qx_log_t	*active_log;
qx_log_t	*shadow_log;

uint32_t	active_count;
uint32_t	shadow_count;
uint32_t	log_limit;

pthread_rwlock_t	log_rwlock = PTHREAD_RWLOCK_INITIALIZER;




















/*
 * This function parses raw data flowing the io-net pipe and
 * fills qx_log_t structure with appropriate data
 * 
 * returns:
 * 1 - parsed successfully
 * 0 - error parsing packet
 */

int log_parse_pkt( qx_log_t *pkt_parsed, uint8_t *pkt_raw, int pkt_raw_len, int pkt_dir ){
	
	/* at most we need 82 bytes:
	 * 
	 * DLC     | 14
	 * IP      | 20-60
	 * TCP/UDP | 4
	 * --------+---------        
	 * total   | 38-78
	 */
	
	int		ip_len;
	
	/* set everything to default */
	memset( pkt_parsed, 0, sizeof(qx_log_t) );
	
	/* get client MAC address and DLC protocol */
	if( pkt_raw_len < 14 ) return 0; /* wrong length */
	if( pkt_dir == QXC_PKT_TOCLI ){
		ip_len = 0;
	}else{
		ip_len = 6;
	}
	memcpy( pkt_parsed->mac, &pkt_raw[ip_len], 6 );
	pkt_parsed->DLC_proto = *((uint16_t*)&pkt_raw[12]);
	
	/* if DLC proto == IP - get IP addresses
	 * else happily return with default values */
	if( pkt_parsed->DLC_proto != PROTO_DLC_IPv4 ) return 1;
	
	/* protocol is IPv4 - get IP addresses */
	{
		IP_t *ip_hdr = (IP_t*)&pkt_raw[14];
		
		if( (ip_hdr->h_ver & 0xF0) != 0x40 ) return 0; /* not IPv4 */
		ip_len = (ip_hdr->h_ver & 0x0F) << 2;
		if( pkt_raw_len < (ip_len+14) ) return 0; /* wrong length */
		
		if(pkt_dir == QXC_PKT_TOCLI){
			pkt_parsed->IP_host = ip_hdr->ip_src;
			pkt_parsed->IP_clnt = ip_hdr->ip_dst;
		}else{
			pkt_parsed->IP_host = ip_hdr->ip_dst;
			pkt_parsed->IP_clnt = ip_hdr->ip_src;
		}
		pkt_parsed->IP_proto = ip_hdr->proto;
	}
	
	/* if IP family protocol is TCP or UDP - then parse ports */
	if( (pkt_parsed->IP_proto == PROTO_IPv4_TCP) ||
	    (pkt_parsed->IP_proto == PROTO_IPv4_UDP)    ){
	    	
	    	tcpudp_t *ports = (tcpudp_t*)&pkt_raw[ip_len+14];
	    	
	    	if( pkt_raw_len < (ip_len+18) ) return 0; /* wrong length */
	    	if(pkt_dir == QXC_PKT_TOCLI){
	    		pkt_parsed->Port_host = ENDIAN_BE16(ports->src_port);
	    		pkt_parsed->Port_clnt = ENDIAN_BE16(ports->dst_port);
			}else{
				pkt_parsed->Port_host = ENDIAN_BE16(ports->dst_port);
				pkt_parsed->Port_clnt = ENDIAN_BE16(ports->src_port);
			}
	}
	
	/* we parsed all we could successfully - say OK */
	return 1;
}








/*
 * This function inits what ever is needed for logging
 * It tries to allocate enough memory to store front- and shadow-
 * log queues. If it fails it says so.
 * 
 * log_destroy does opposite.
 */
int log_init( int num_slots ){
	
	int	size = num_slots*sizeof(qx_log_t);
	
	/* create 2 shared memory objects */
	a_log_fd = shm_open("/dev/qxc_log0", O_RDWR|O_CREAT, 0666);
	s_log_fd = shm_open("/dev/qxc_log1", O_RDWR|O_CREAT, 0666);
	if( (a_log_fd == -1) || (s_log_fd == -1) ) return 0;
	
	/* set sizes of shared memory objects */
	if( -1 == ftruncate(a_log_fd, size) ) return 0;
	if( -1 == ftruncate(s_log_fd, size) ) return 0;
	
	/* map shared memory regions into process' address space */
	active_log = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, a_log_fd, 0);
	shadow_log = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, s_log_fd, 0);
	
	if( active_log == MAP_FAILED ) return 0;
	if( shadow_log == MAP_FAILED ) return 0;
	
	active_count	= 0;
	shadow_count	= 0;
	log_limit		= num_slots;
	log_fd_num		= 0;
	
	return 1;
}

void log_destroy(){
	if( shadow_log != NULL ) free( shadow_log );
	if( active_log != NULL ) free( active_log );
}

/*
 * This function swaps shadow and active logs.
 */
void log_swap_log(int *rgn, int *cnt){
	
	qx_log_t	*tmp = active_log;
	
	pthread_rwlock_wrlock( &log_rwlock );
	shadow_count = active_count;
	active_count = 0;
	active_log = shadow_log;
	pthread_rwlock_unlock( &log_rwlock );
	
	shadow_log = tmp;
	*cnt = shadow_count;
	
	/* what shm_object is currently readable */
	/* we return #fd that was filled during last period and then swap fd's */
	*rgn = log_fd_num;
	log_fd_num = (log_fd_num==1 ? 0:1);
}

/*
 * This function does not nore than adding packet information to the log.
 * It returns 1 if packet info was successfully added to log, 0 otherwise.
 */
int log_store_pkt( qx_log_t *pkt ){
	
	uint32_t	slot;
	
	pthread_rwlock_rdlock( &log_rwlock );
	
	slot = atomic_add_value( &active_count, 1 );
	if( slot < log_limit ){
		memcpy( &active_log[slot], pkt, sizeof(qx_log_t) );
	}else{
		/* in case we increased counter and it happened to be more than
		 * allowed. Assigning a value to 32-bit integer should be atomic
		 * operation. At least on x86 architecture */
		active_count = log_limit;
	}
	
	pthread_rwlock_unlock( &log_rwlock );
	
	return (slot < log_limit );
}










/* End of file */

