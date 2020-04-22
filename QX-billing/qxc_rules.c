/*
 * 2005 (c) -={ MikeP }=-
 */



/*
 * Here follow functions that handle rules applied by billing system.
 * 
 * Two type of rules are of interest:
 * 
 * - The rules that allow any client to reach specified host
 *   (or group of hosts) even though its NIC is not listed among
 *   allowed to communicate through billing.
 *   This is what TAX_EXEMPT table tells us to do.
 * 
 * - The rules that allow or deny any client communication through
 *   billing based on list supplied by logger process. If client
 *   matches IP+MAC pair on the list its packet is then allowed to
 *   pass through billing and information about the packet is logged.
 * 
 * Rule addition and deletion is also handled here.
 */

#include	"qxc_common.h"










/*
 * Variables, local to module, but global to all threads
 */
qxc_rule_exempt_t	*ex_rul_active	= NULL;
qxc_rule_exempt_t	*ex_rul_shadow	= NULL;
uint32_t			ex_count		= 0;
uint32_t			ex_active_count	= 0;
uint32_t			ex_limit		= 10;
pthread_rwlock_t	ex_rwlock		= PTHREAD_RWLOCK_INITIALIZER;

qxc_rule_nics_t		*nc_rul_active	= NULL;
qxc_rule_nics_t		*nc_rul_shadow	= NULL;
qxc_rule_nics_t		*nc_rul_list	= NULL;
uint32_t			nc_count		= 0;
uint32_t			nc_limit		= 10;
pthread_rwlock_t	nc_rwlock		= PTHREAD_RWLOCK_INITIALIZER;











/*
 * This functions returns:
 * if packet matched some rule on the list
 * of tax-exempted destinations, then:
 * 1 - traffic is allowed for active user
 * 2 - traffic is allowed for any user (either active or not)
 * ...
 * else it returns 0.
 */
int rule_match_exempt( qx_log_t *pkt ){
	
	int i = 0;
	int result = 0;
	
	pthread_rwlock_rdlock( &ex_rwlock );
	
	for( i=0 ; i<ex_active_count ; i++ ){
		
		/* check DLC protocol (can not be of type "ANY") - only strict
		 * match is allowed. This is done in case we want no taxes for
		 * ARP protocol or anything alike */
		if( ex_rul_active[i].dlc_proto != pkt->DLC_proto ) continue;
		
		/* Checking IP address and ports takes sence only for IPv4 protocol */
		if( pkt->DLC_proto == PROTO_DLC_IPv4 ){
			
			/* if server address does not match the rule - go on to next rule */
			if( (pkt->IP_host & ex_rul_active[i].ip_mask) != ex_rul_active[i].ip_addr ) continue;
			
			/* check IP family protocol */
			if( ex_rul_active[i].ip_proto != 0 ){
				/* this means IP family proto is required to match */
				if( pkt->IP_proto != ex_rul_active[i].ip_proto ) continue;
				
				/* So now we check ports. For now port checking is available for
				 * TCP and UDP protocols
				 */
				if( (pkt->IP_proto == PROTO_IPv4_TCP) || (pkt->IP_proto == PROTO_IPv4_UDP) ){
					/* Check that host port is within allowed range */
					if( (pkt->Port_host < ex_rul_active[i].ip_port_lo) || (pkt->Port_host > ex_rul_active[i].ip_port_hi) ) continue;
				}
			}
		}
		
		/* We are here because:
		 * - we matched DLC protocol
		 * - we matched host ip (or ANY host was allowed)
		 * - we hit the host port range for TCP/UDP
		 * so now we think we matched the rule - pass the packet
		 */
		result = ex_rul_active[i].active;
		break;
	}
	
	pthread_rwlock_unlock( &ex_rwlock );
	return result;
}










/*
 * This function searches the list of allowed MAC+IP pairs to see
 * if supplied one is allowed communication through billing system.
 */
int rule_match_client( qx_log_t *pkt ){
	
	int result = 0;
	qxc_rule_nics_t *list;
	
	pthread_rwlock_rdlock( &nc_rwlock );
	
	list = nc_rul_list;
	
	while( list != NULL ){
		
		result = memcmp( list->rule.mac, pkt->mac, 6 );
		
		/* if both mac and ip matched we return 1 ... */
		if( (result == 0) && (list->rule.ip_addr == pkt->IP_clnt) ){
			pthread_rwlock_unlock( &nc_rwlock );
			return 1;
		}
		
		/* ... otherwise we determine where to go further */
		if( result <0 ){
			list = list->left;
		}else{
			list = list->right;
		}
	}
	
	pthread_rwlock_unlock( &nc_rwlock );
	return 0;
}





/*
 * This manipulates rules
 */
int rule_init_lists( int ex_cnt, int nc_cnt ){
	
	ex_rul_active = (qxc_rule_exempt_t*)malloc(ex_cnt*sizeof(qxc_rule_exempt_t));
	if( ex_rul_active == NULL ) return 0;
	
	ex_rul_shadow = (qxc_rule_exempt_t*)malloc(ex_cnt*sizeof(qxc_rule_exempt_t));
	if( ex_rul_shadow == NULL ) return 0;
	
	nc_rul_active = (qxc_rule_nics_t*)malloc(nc_cnt*sizeof(qxc_rule_nics_t));
	if( nc_rul_active == NULL ) return 0;
	
	nc_rul_shadow = (qxc_rule_nics_t*)malloc(nc_cnt*sizeof(qxc_rule_nics_t));
	if( nc_rul_shadow == NULL ) return 0;
	
	ex_limit = ex_cnt;
	nc_limit = nc_cnt;
	
	return 1;
}





int rule_add_exempt(qx_rule_exempt_t *rule){
	
	if( ex_count == ex_limit ) return 0;
	
	memcpy( &ex_rul_shadow[ex_count], rule, sizeof(qx_rule_exempt_t) );
	ex_count++;
	
	return 1;
}





int rule_add_nic(qx_rule_nics_t *rule){
	
	uint8_t	*mac;
	int		result;
	qxc_rule_nics_t *nic = nc_rul_shadow;
	
	if( nc_count == nc_limit ) return 0;
	
	/* copy rule to the shadow buffer */
	memcpy( &(nc_rul_shadow[nc_count].rule), rule, sizeof(qx_rule_nics_t) );
	nc_rul_shadow[nc_count].left = NULL;
	nc_rul_shadow[nc_count].right = NULL;
	mac = nc_rul_shadow[nc_count].rule.mac;
	
	if( nc_count == 0 ) return (++nc_count); /* this was the first rule we added */
	
	/* correctly link in the rule */
	while(1){
		result = memcmp( nic->rule.mac, mac, 6 );
		
		if( result <0 ){
			if( nic->left == NULL ){
				nic->left = &nc_rul_shadow[nc_count];
				break;
			}
			nic = nic->left;
		}else{
			if( nic->right == NULL ){
				nic->right = &nc_rul_shadow[nc_count];
				break;
			}
			nic = nic->right;
		}
	}
	
	nc_count++;
	return 1;
}





void rule_swap_lists(){
	
	qxc_rule_exempt_t	*tmp_ex;
	qxc_rule_nics_t		*tmp_nc;
	
	/* swap shadow and active lists of exempts */
	tmp_ex = ex_rul_active;
	pthread_rwlock_wrlock( &ex_rwlock );
	ex_rul_active = ex_rul_shadow;
	ex_active_count = ex_count;
	pthread_rwlock_unlock( &ex_rwlock );
	ex_rul_shadow = tmp_ex;
	ex_count = 0;
	
	/* swap shadow and active lists of nics */
	pthread_rwlock_wrlock( &nc_rwlock );
	nc_rul_list = ( nc_count == 0 ? NULL : nc_rul_shadow );
	pthread_rwlock_unlock( &nc_rwlock );
	nc_count = 0;
	tmp_nc = nc_rul_active;
	nc_rul_active = nc_rul_shadow;
	nc_rul_shadow = tmp_nc;
	
	return;
}


