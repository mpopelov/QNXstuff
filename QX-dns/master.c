/*
 * 2005 (c) -=MikeP=-
 */

/* $Date: Mon 13, Jun 2005 */


#include	"qxdns_common.h"


/*
 * Global variables
 */

/* this holds the first zone */
qxdns_mz_t	* root_zone = NULL;






/*
 * this function makes a coded string, representing the domain name
 * (length of label preceeds the actual text).
 * if the zone name to be appended - last character is 0xC0.
 */
void rcpy_string(const char * src, char * dst){
	int i = 0;
	int j = 1;
	
	/* copy the string */
	strcpy( &(dst[1]), src );
	/* replace '.' with label lengths */
	while(dst[j] != 0){
		if(dst[j] == '.'){
			dst[i] = (j-i-1);
			i=j;
		}
		j++;
	}
	/* we reached the end of the name.
	 * if we ended up with '.' - put 0xC0 there */
	if( 0 == (dst[i] = (j-i-1)) ){
		dst[i] = 0xC0;
	}
}





/*
 * Compare a host name in the query to one supplied
 * returns:
 * 1) matched
 * 0) not matched
 * zone_ptr - is where the name of the zone is found
 */
int rcmp_names(const uint8_t * qname, const uint8_t * name, const uint8_t * zname, uint16_t * zone_ptr){
	uint16_t qi = 0;
	uint16_t ni = 0;
	const uint8_t * cstr = name;
	
	*zone_ptr = 0;
	
	/* start comparing */
	while(1){
		if(toupper(qname[qi]) != toupper(cstr[ni])){
			/* this is the case we have to add zone name to CNAME */
			if(cstr[ni] == 0xC0){
				*zone_ptr = qi+12;		/* we have found a zone name here */
				ni = 0;
				cstr = zname;
				continue;
			}else{
				/* !debug! */
				/*printf("rcmp_: --- '%hhx' @qname[%hi] != '%hhx' @cstr[%hi]\n",qname[qi],qi,cstr[ni],ni);*/
				return 0;
			}
		}
		/* the two chars matched - check if they are 0 */
		if(qname[qi] == 0) return 1;	/* search is complete */
		/* now advance indices and continue */
		ni++;
		qi++;
	}
}




/*
 * Print the domai name into buffer, add the ptr at the end if needed
 * and return the offset of the first byte after the printed name
 * or NULL if truncation occured.
 */
uint8_t * rprt_name(uint8_t *buffer, const uint8_t *name, const uint8_t *zname, uint16_t *zone_ptr, int len){
	const uint8_t * cstr = name;
	int last = 0;
	int i = 0;
	
	while(last < len){
		/* print the name */
		buffer[last] = cstr[i];
		if(buffer[last] == 0) return (buffer + last + 1); /* finished */
		if(buffer[last] == 0xC0){
			/* switch to printing zone name */
			if(*zone_ptr != 0){
				*((uint16_t*)&(buffer[last])) = ENDIAN_RET16(0xC000 | (*zone_ptr));
				last += 2;
				return (buffer + last);
			}else{
				*zone_ptr = last;
				cstr = zname;
				i=0;
				continue;
			}
		}
		last++;
		i++;
	}
	return NULL; /* truncation occured */
}





/* 
 * Creates a new zone and adds it to the list of existing zones
 */
qxdns_mz_t * qxdns_new_mz(char * name){
	qxdns_mz_t *	new_zone = (qxdns_mz_t *)malloc(sizeof(qxdns_mz_t));
	
	if(new_zone == NULL) return NULL;
	
	/* now add name to the zone header */
	memset(new_zone,0,sizeof(qxdns_mz_t));
	rcpy_string(name,new_zone->zone_name);
	
	new_zone->next = root_zone;
	root_zone = new_zone;
	
	return new_zone;
}



/*
 * Creates, clears the new node record and adds its canonical name
 */
qxdns_mr_t * qxdns_new_mr(char * cname, uint32_t ttl){
	qxdns_mr_t *	new_node = (qxdns_mr_t*)malloc(sizeof(qxdns_mr_t));
	if(new_node == NULL) return NULL;
	memset(new_node,0,sizeof(qxdns_mr_t));
	rcpy_string(cname,new_node->cname);
	new_node->ttl = ENDIAN_RET32(ttl);
	return new_node;
}



/*
 * Adds one more alias to the host
 */
int qxdns_add_mr_alias(qxdns_mr_t * host, char * alias){
	mr_str_t * new_alias = (mr_str_t*)malloc(sizeof(mr_str_t));
	if(new_alias == NULL) return 0;
	memset(new_alias,0,sizeof(mr_str_t));
	rcpy_string(alias,new_alias->name);
	
	/* add alias to host */
	new_alias->next = host->aliases;
	host->aliases = new_alias;
	return 1;
}



/*
 * Adds one more address to the host
 */
int qxdns_add_mr_addr(qxdns_mr_t * host, uint32_t addr){
	char	buffer[32];
	mr_inaddr_t * new_addr = (mr_inaddr_t*)malloc(sizeof(mr_inaddr_t));
	if(new_addr == NULL) return 0;
	memset(new_addr,0,sizeof(mr_inaddr_t));
	
	/* setup all fields */
	new_addr->ans.addr	= addr;
	new_addr->ans.len	= ENDIAN_RET16(4);
	new_addr->ans.ttl	= host->ttl;
	new_addr->ans.class	= DNS_C_IN;
	new_addr->ans.type	= DNS_T_A;
	new_addr->ans.n_pointer = 0x0CC0;	/* by default we point to the question */
	
	/* now tricks about in-addr.arpa names */
	{
		struct in_addr	tmp_in;
		tmp_in.s_addr = ENDIAN_RET32(addr);
		/*snprintf(buffer,32,"%s.IN-ADDR.ARPA",inet_ntoa( (struct in_addr)ENDIAN_RET32(addr) ) );*/
		snprintf(buffer,32,"%s.IN-ADDR.ARPA",inet_ntoa(tmp_in) );
		rcpy_string(buffer,new_addr->arpa);
	}
	
	/* add address to host */
	new_addr->next = host->inaddrs;
	host->inaddrs = new_addr;
	return 1;
}


/*
 * Adds a master database record to the zone
 */
int qxdns_add_mr(qxdns_mz_t * zone, qxdns_mr_t * host, int ns, uint16_t mx_pref){
	/* add record to the list of hosts on the zone */
	host->next = zone->hosts;
	zone->hosts = host;
	/* if a record corresponds to MX */
	if(mx_pref != 0){
		host->mx_pref = ENDIAN_RET16(mx_pref);
		host->mx_next = zone->zone_MX;
		zone->zone_MX = host;
	}
	/* if a record is a name server */
	if(ns != 0){
		host->ns_next = zone->zone_NS;
		zone->zone_NS = host;
	}
	return 1;
}






/*
 * Look zones for the record of specified type and class
 * and form the answer as appropriate
 */
int qxdns_master_lookup(dns_buffer_t * query){
	
	uint16_t *		q_ptr		= NULL;
	qxdns_mr_t *	tmp_host	= NULL;
	qxdns_mz_t *	tmp_zone	= root_zone;
	uint16_t		ans_ptr		= 0x0CC0;	/* the default is to point to query */
	uint16_t		zone_ptr	= 0;
	uint8_t *		qname		= NULL;
	int				add_cname	= 0;
	
	uint16_t		an_count	= 0;
	uint16_t		ns_count	= 0;
	uint16_t		ar_count	= 0;
	int				len			= 0;
	char *		an_ptr		= NULL;
	
	dns_msg_header_t *	dnsh	= (dns_msg_header_t*)query->pkt;
	
	
	/* determine what class of data they want and do appropriate */
	q_ptr = (uint16_t*)&(query->pkt[(query->datalen-2)]);
	if(*q_ptr != DNS_C_IN){
		/*printf("MASTER: capable only of DNS_C_IN queries\n");*/
		return 0;
	}
	qname = (uint8_t*)&(query->pkt[12]); /* this is the name in query */
	q_ptr--;
	/* determine what type of data they want */
	switch(*q_ptr){
		case DNS_T_A:
			{
				mr_inaddr_t * tmp_inaddr = NULL;
				
				/* let us see if we have the requested name in database and fill in the answer */
				
				while(tmp_zone != NULL){
					/* check all the nodes in selected zone */
					tmp_host = tmp_zone->hosts;
					while(tmp_host != NULL){
						mr_str_t * tmp_alias = tmp_host->aliases;
						
						/* check CNAME first */
						if(rcmp_names(qname,tmp_host->cname,tmp_zone->zone_name,&zone_ptr)) break;
						
						/* if CNAME didn't match - check all aliases */
						while(tmp_alias != NULL){
							if(rcmp_names(qname,tmp_alias->name,tmp_zone->zone_name,&zone_ptr)){
								add_cname = 1;
								break;
							}
							tmp_alias = tmp_alias->next;
						}
						if(tmp_alias != NULL) break;
						/* move to the next host */
						tmp_host = tmp_host->next;
					}
					if(tmp_host != NULL) break; /* we found something */
					tmp_zone = tmp_zone->next;
				}
				
				if(tmp_host == NULL){
					return 0; /* we have found nothing */
				}
				
				/* fill in the answer */
				printf("MASTER: answer found in database !!!\n");
				
				qname = &(query->pkt[query->datalen]);
				len = 512 - query->datalen;
				
				
				/* if adding canonical name is required - add it */
				if(add_cname == 1){
					/* add RR */
					dns_msg_rrinfo_t * rr = (dns_msg_rrinfo_t*)qname;
					
					rr->ans = 0x0CC0;
					rr->type = DNS_T_CNAME;
					rr->class = DNS_C_IN;
					rr->ttl = tmp_host->ttl;
					len = 502 - query->datalen;
					
					an_ptr = rprt_name(rr->rdata,tmp_host->cname,tmp_zone->zone_name,&zone_ptr,len);
					if(an_ptr == NULL){
						/* in case of truncation */
						dnsh->ANCOUNT = 0x0100;
						dnsh->OPT |= DNS_OPT_AA|DNS_OPT_TC|DNS_OPT_QR;
						return 1;
					}
					rr->rdlen = ENDIAN_RET16(an_ptr - rr->rdata);
					query->datalen = an_ptr - query->pkt;
					len = 512 - (an_ptr - query->pkt);
					qname = an_ptr;
					/* fix ans_ptr */
					ans_ptr = ENDIAN_RET16( 0xC000 | (rr->rdata - query->pkt) );
					an_count++;
				}
				
				
				/* now add all available IP addresses */
				tmp_inaddr = tmp_host->inaddrs;
				while(tmp_inaddr != NULL){
					if( len < sizeof(ans_a_t) ) break; /* no more room */
					an_count ++;
					len -= sizeof(ans_a_t);
					memcpy(qname,&tmp_inaddr->ans,sizeof(ans_a_t));
					*((uint16_t*)qname) = ans_ptr;
					qname += sizeof(ans_a_t);
					query->datalen += sizeof(ans_a_t);
					tmp_inaddr = tmp_inaddr->next;
				}
				
				/* add name servers ? */
				
				/* fix query header */
				dnsh->ANCOUNT = ENDIAN_RET16(an_count);
				dnsh->NSCOUNT = ENDIAN_RET16(ns_count);
				dnsh->ARCOUNT = ENDIAN_RET16(ar_count);
				dnsh->OPT |= DNS_OPT_QR;/*|DNS_OPT_AA;*/
				return 1;
			}
			
		case DNS_T_PTR:
			{
				/*printf("MASTER: they are looking for the CNAME by IP\n");*/
				
				/* look for the IP addresses */
				while(tmp_zone != NULL){
					/* check all the nodes in selected zone */
					tmp_host = tmp_zone->hosts;
					while(tmp_host != NULL){
						mr_inaddr_t * tmp_addr = tmp_host->inaddrs;
						
						/* check all addresses */
						while(tmp_addr != NULL){
							if(rcmp_names(qname,tmp_addr->arpa,tmp_zone->zone_name,&zone_ptr)){
								break;
							}
							tmp_addr = tmp_addr->next;
						}
						if(tmp_addr != NULL) break;
						/* move to the next host */
						tmp_host = tmp_host->next;
					}
					if(tmp_host != NULL) break; /* we found something */
					tmp_zone = tmp_zone->next;
				}
				
				if(tmp_host == NULL){
					return 0; /* we have found nothing */
				}
				
				/* !!! debug !!! */
				printf("MASTER: answer found in database\n");
				
				/* now add the name of the host to the answer */
				{
					dns_msg_rrinfo_t * rr = (dns_msg_rrinfo_t*)&(query->pkt[(query->datalen)]);
					
					rr->ans = 0x0CC0;
					rr->type = DNS_T_PTR;
					rr->class = DNS_C_IN;
					rr->ttl = tmp_host->ttl;
					len = 502 - query->datalen;
					
					an_ptr = rprt_name(rr->rdata,tmp_host->cname,tmp_zone->zone_name,&zone_ptr,len);
					if(an_ptr == NULL){
						/* in case of truncation */
						printf("MASTER: truncation happened\n");
						dnsh->ANCOUNT = 0x0100;
						dnsh->OPT |= DNS_OPT_AA|DNS_OPT_TC|DNS_OPT_QR;
						return 1;
					}
					rr->rdlen = ENDIAN_RET16(an_ptr - rr->rdata);
					query->datalen = an_ptr - query->pkt;
					/*printf("MASTER: RR length is %hi\n",ENDIAN_RET16(rr->rdlen));
					printf("MASTER: answer length is %i\n",query->datalen);*/
				}
				/* fix query header */
				dnsh->ANCOUNT = ENDIAN_RET16(1);
				dnsh->NSCOUNT = ENDIAN_RET16(0);
				dnsh->ARCOUNT = ENDIAN_RET16(0);
				dnsh->OPT |= DNS_OPT_QR | DNS_OPT_AA;
				return 1;
			}
			
		case DNS_T_NS:
			printf("MASTER: they are looking for the authoritative name server\n");
			return 0;
			
		case DNS_T_MX:
			printf("MASTER: they are looking for the domain MX record\n");
			return 0;
			
		default:
			printf("MASTER: some strange request\n");
			return 0;
	}
	
	/* nothing found */
	return 0;
}

