/*
 * main.c - contains the main() function
 */

/* $Date: Mon 13, Jun 2005 */




#include	"qxdns_common.h"





/*
 * * * * * * SUPER GLOBALS  * * * * * * * * * * * * * * * * * * * * * * * * * * 
 */

/* pool of threads */
static thread_pool_attr_t	qxdns_pool_attr;
void *						qxdns_thread_pool;

/* Connectors we use to communicate the outer world */
/* bind_connectors is a list of connectors starting
 * with listening sockets and concatenated with srvr_connectors
 * so we may use it in a loop after select() to check
 * sockets for new messages */
dns_connector_t *	bind_connectors = NULL;
dns_connector_t *	srvr_connectors = NULL;


/* here comes the list of ready buffers and the queue */
dns_buffer_t *		free_buf	= NULL;	/* list of free buffers */
pthread_mutex_t		free_mutex	= PTHREAD_MUTEX_INITIALIZER;

dns_buffer_t *		ready_buf	= NULL;	/* list of filled buffers */
pthread_mutex_t		ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		ready_cond	= PTHREAD_COND_INITIALIZER;

dns_buffer_t *		queue_head	= NULL;
dns_buffer_t *		queue_tail	= NULL;
pthread_mutex_t		queue_mutex	= PTHREAD_MUTEX_INITIALIZER;


uint16_t			IntReqNum = 1;




/*pthread_cond_t		query_cond	=	PTHREAD_COND_INITIALIZER;
pthread_mutex_t		query_mutex	=	PTHREAD_MUTEX_INITIALIZER;














































/*
 * functions of the pool
 */
THREAD_POOL_PARAM_T * thread_context_alloc( THREAD_POOL_HANDLE_T * handle )
{
    return (THREAD_POOL_PARAM_T*) 1;
}





void thread_context_free( THREAD_POOL_PARAM_T * param )
{
	return;
}





THREAD_POOL_PARAM_T * thread_block( THREAD_POOL_PARAM_T * msg )
{
	/*
	 * Here we block on awaiting for a new data from
	 * external servers or client.
	 */
	dns_buffer_t *		request = NULL;
	
	/* wait for new query to arrive */
	pthread_mutex_lock( &ready_mutex );
	while(ready_buf==NULL) pthread_cond_wait(&ready_cond,&ready_mutex);
	request = ready_buf;
	ready_buf = request->next;
	pthread_mutex_unlock( &ready_mutex );
	
	return (THREAD_POOL_PARAM_T*) request;
}





/*
 * Here we process incoming queries and decide what to do with them
 */
int thread_handler( THREAD_POOL_PARAM_T * msg )
{
	int					save_rq	= 0;
	int					c_sock	= -1;
	struct sockaddr *	c_addr	= NULL;
	dns_buffer_t *		tmp_q	= NULL;
	dns_buffer_t *		request = (dns_buffer_t*)msg;
	dns_msg_header_t *	dnsh	= (dns_msg_header_t*)request->pkt;
	
	
	/* determine the type of the record and see what to do with it */
	if( dnsh->OPT & DNS_OPT_QR ){
		
		/* 
		 * Here we handle a response from one of the external servers
		 */
		
		/* 1) find initial query */
		pthread_mutex_lock(&queue_mutex);
    	tmp_q = queue_head;
    	while(tmp_q != NULL){
    		if(tmp_q->ref_ID == dnsh->ID ){
    			if(tmp_q->prev != NULL){
    				tmp_q->prev->next = tmp_q->next;
				}else{
					queue_head = tmp_q->next;
				}
				
				if(tmp_q->next != NULL){
					tmp_q->next->prev = tmp_q->prev;
				}else{
					queue_tail = tmp_q->prev;
				}
				
				break;			
			}
			tmp_q = tmp_q->next;
		}
    	pthread_mutex_unlock(&queue_mutex);
    	
    	/* 2) if there is no source query - log warning and do nothing */
    	if(tmp_q == NULL){
    		qxdns_log(QLL_WARNING,QXDNSERR[QXDNS_E_LOSTQUERY]);
		}else{
			/* prepare flags for testing */
			uint16_t	opts	=	(dnsh->OPT & DNS_OPT_RCODE_MASK);
			
			
			if( ((opts==DNS_OPT_RCODE_SF) || (opts>DNS_OPT_RCODE_NE))
				&& (tmp_q->qserver->next != NULL) ){
				
				/* We get here in the following cases:
				 * - respond with "server failure"
				 * - respond with "not implemented"
				 * - respond with "refused"
				 */
					
				/* if response is not valid - try to send to another server */
				dns_buffer_t * tmp = request;
				request = tmp_q;
				tmp_q = tmp;
				request->qserver = request->qserver->next;
				save_rq = 1;
				c_sock = request->qserver->sock;
				c_addr = (struct sockaddr*)&(request->qserver->addr);
				
			}else{
				
				/* we consider response to be either valid or telling us about
				 * some error in query or server - we send it back to client
				 * - respond with "OK"
				 * - respond with "format error"
				 * - respond with "non existant domain"
				 */
				
				/* add record to cache */
				if((opts == DNS_OPT_RCODE_OK) || (opts == DNS_OPT_RCODE_NE)){
					qxdns_cache_add(request);
				}
				/* answer client */
				dnsh->ID = tmp_q->src_ID;
				c_sock = tmp_q->qclient->sock;
				c_addr = (struct sockaddr*)&(tmp_q->from);
			}
		}
		
		
		
	}else{
		
		
		/* 
		 * Here we handle a request from client.
		 * We check if the query contains only one question, the class
		 * of the query is INET. If query does not match our criteria we
		 * answer that the capability is not implemented.
		 */
		
		
		int	ret = 0;
		
		/* check if there is exactly one question */
		if(dnsh->QDCOUNT != 0x0100){
			
			qxdns_log(QLL_WARNING,"Too many questions: %hx\n",dnsh->QDCOUNT);
			dnsh->OPT |= DNS_OPT_RCODE_FE;	/* say that there is format error */
			c_sock = request->qclient->sock;
			c_addr = (struct sockaddr*)&(request->from);
			
		}else{
			
			/* look into master database */
			ret = qxdns_master_lookup(request);
			if(0 == ret) ret = qxdns_cache_lookup(request);
			
			if((0 == ret) && (srvr_connectors != NULL)){
				/* we didn't find answer neither in master dtabase nor in cache
				 * send request to the first known external server and add query
				 * to the tail of the queue */
				printf("sending request to external server\n");
				request->src_ID = dnsh->ID;
				request->qserver = srvr_connectors;
				request->next = NULL;
				
				pthread_mutex_lock( &queue_mutex);
				request->ref_ID = IntReqNum++;
				pthread_mutex_unlock( &queue_mutex );
				
				dnsh->ID = request->ref_ID;
				save_rq = 1;
				c_sock = srvr_connectors->sock;
				c_addr = (struct sockaddr*)&(srvr_connectors->addr);
				
			}else{
				if(0 == ret) dnsh->OPT |= DNS_OPT_RCODE_NE; /* this is done in case there are no external servers */
				c_sock = request->qclient->sock;
				c_addr = (struct sockaddr*)&(request->from);
			}
		}
	}
	
	
	
	
	/* last) send whatever we have to send to address pointed. We always send
	 * the data pointed to by request variable. */
	if(c_sock != -1){
		/* for now we processed query somehow - we may send something somewhere */
		printf("sending message to: %s\n",inet_ntoa(((struct sockaddr_in*)c_addr)->sin_addr));
		sendto(c_sock,request->pkt,request->datalen,0,c_addr,sizeof(struct sockaddr_in));
	}
	
	/* if appropriate - return buffers back to free list */
	pthread_mutex_lock( &free_mutex );
	if(tmp_q != NULL){
		tmp_q->next = free_buf;
		free_buf = tmp_q;
	}
	if(save_rq == 0){
		request->next = free_buf;
		free_buf = request;
	}
	pthread_mutex_unlock( &free_mutex );
	
	/* if we need to add request to queue - do it */
	if(save_rq == 1){
		pthread_mutex_lock( &queue_mutex);
		request->prev = queue_tail;
		if(queue_tail != NULL){
			queue_tail->next = request;
		}else{
			queue_head = request;
		}
		queue_tail = request;
		pthread_mutex_unlock( &queue_mutex );
	}
	
    return 0;
}








































































/*
 * * * * * * Set up the daemon  * * * * * * * * * * * * * * * * * * * * * * * *
 */

void qxdns_setup(void)
{
	char *	token;
	int		c_max = 100;
	time_t	c_ttl = 86400;
	int		num_bufs = DNS_DEF_BUFFERS;
	
    /* prepare thread pools */
    memset( &qxdns_pool_attr, 0, sizeof(thread_pool_attr_t) );

    qxdns_pool_attr.lo_water		= 2;
    qxdns_pool_attr.hi_water		= 5;
    qxdns_pool_attr.increment		= 2;
    qxdns_pool_attr.maximum			= 9;
    qxdns_pool_attr.block_func		= thread_block;
    qxdns_pool_attr.handler_func	= thread_handler;
    qxdns_pool_attr.context_alloc	= thread_context_alloc;
    qxdns_pool_attr.context_free	= thread_context_free;
    
	
	/* open conf file for reading */
	if( GetConf_open( "/etc/QX-dns.cnf" ) == 0 ){
		perror("config file");
		exit(EXIT_FAILURE);
	}
	
	while( ( token = GetConf_string()) != NULL ){
		
		/* 
		 * 
		 * parse server realm
		 * 
		 */
		if( !stricmp("<server>",token) ){
			
			/* here we parse server realm */
			while( (token = GetConf_string()) != NULL ){
				/* end of realm */
				if( !stricmp("</server>",token) ){
					break;
				}
				
				/* pool configuration */
				if( !stricmp("POOLLOWATER",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					qxdns_pool_attr.lo_water = atoi(token);
					}
    				continue;
				}
				if( !stricmp("POOLHIWATER",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					qxdns_pool_attr.hi_water = atoi(token);
					}
    				continue;
				}
				if( !stricmp("POOLMAX",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					qxdns_pool_attr.maximum = atoi(token);
					}
    				continue;
				}
				if( !stricmp("POOLINCREMENT",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					qxdns_pool_attr.increment = atoi(token);
					}
    				continue;
				}
				
				/* cache configuration */
				if( !stricmp("CACHEMAX",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					c_max = atoi(token);
					}
    				continue;
				}
				if( !stricmp("CACHETTL",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					c_ttl = atoi(token);
					}
    				continue;
				}
				
				/* query queue configuration */
				if( !stricmp("QUEUELEN",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					num_bufs = atoi(token);
					}
    				continue;
				}
				
				/* binding server to some interface */
				if( !stricmp("BIND",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					dns_connector_t * new_conn = (dns_connector_t*)malloc(sizeof(dns_connector_t));
						memset(new_conn,0,sizeof(dns_connector_t));
						new_conn->addr.sin_family = AF_INET;
						new_conn->addr.sin_port = htons(DNS_DEF_PORT);
						inet_aton(token,&(new_conn->addr.sin_addr));
						if((new_conn->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
							perror("bind_connectors");
							exit(EXIT_FAILURE);
						}
						new_conn->next = bind_connectors;
						bind_connectors = new_conn;
					}
    				continue;
				}
				
				/* add one more external DNS server to list of servers */
				if( !stricmp("EXTERNALDNS",token) ){
					/* add one more external server */
					if( NULL != (token = GetConf_token()) ){
						dns_connector_t * new_conn = (dns_connector_t*)malloc(sizeof(dns_connector_t));
						memset(new_conn,0,sizeof(dns_connector_t));
						new_conn->addr.sin_family = AF_INET;
						new_conn->addr.sin_port = htons(DNS_DEF_PORT);
						inet_aton(token,&(new_conn->addr.sin_addr));
						if((new_conn->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
							perror("ExtDNS");
							exit(EXIT_FAILURE);
						}
						new_conn->next = srvr_connectors;
						srvr_connectors = new_conn;
					}
    				continue;
				}
				
				/* unknown directive */
				printf("Unknown directive %s\n",token);
				exit(EXIT_FAILURE);
			}
			
			if(token==NULL){
				printf("Error in <server> realm\n");
				exit(EXIT_FAILURE);
			}else{
				continue;
			}
		}
		
		
		
		
		
		/* 
		 * 
		 * parse zone realm
		 * 
		 */
		if( !stricmp("<zone>",token) ){
			/* create a new record zone */
			qxdns_mz_t * tmp_zone = NULL;
			if(NULL == (token = GetConf_token())){
				printf("no zone name\n");
				exit(EXIT_FAILURE);
			}
			tmp_zone = qxdns_new_mz(token);
			
			/* here we parse host records realm */
			while( (token = GetConf_string()) != NULL ){
				/* end of realm */
				if( !stricmp("</zone>",token) ){
					break;
				}
				
				/* host configuration */
				if( !stricmp("<host>",token) ){
					qxdns_mr_t *	tmp_host = NULL;
					int				ns = 0;
					uint16_t		mx_pref = 0;
					uint32_t		ttl = DNS_DEF_TTL;
					
					/* get the name and ttl of the host */
					if(NULL == (token = GetConf_token())){
						printf("no host ttl");
						exit(EXIT_FAILURE);
					}
					ttl = atoi(token);
					if(NULL == (token = GetConf_token())){
						printf("no host cname");
						exit(EXIT_FAILURE);
					}
					tmp_host = qxdns_new_mr(token, ttl);
					
					/* read all host parameters */
					while( (token = GetConf_string()) != NULL ){
						
						/* cleanup */
						if(!stricmp("</host>",token)){
							qxdns_add_mr(tmp_zone,tmp_host,ns,mx_pref);
							break;
						}
						
						/* add aliases */
						if(!stricmp("alias",token)){
							while(NULL != (token = GetConf_token())){
								qxdns_add_mr_alias(tmp_host,token);
							}
							continue;
						}
						
						/* add IP addresses */
						if(!stricmp("address",token)){
							while(NULL != (token = GetConf_token())){
								struct in_addr tmp_addr;
								if( 1 == inet_aton(token,&tmp_addr) ){
									qxdns_add_mr_addr(tmp_host,tmp_addr.s_addr);
								}
							}
							continue;
						}
						
						/* set host to be NS for the zone */
						if(!stricmp("ns",token)){
							if(NULL != (token = GetConf_token())){
								ns = atoi(token);
							}
							continue;
						}
						
						/* set host to be MX for the zone */
						if(!stricmp("mx",token)){
							if(NULL != (token = GetConf_token())){
								mx_pref = atoi(token);
							}
							continue;
						}
						
						printf("Unknown directive %s\n",token);
						exit(EXIT_FAILURE);
					}
					
					if(token==NULL){
						printf("Error in <host> realm\n");
						exit(EXIT_FAILURE);
					}else{
						continue;
					}
				}
				
				/* unknown directive */
				printf("Unknown directive %s\n",token);
				exit(EXIT_FAILURE);
			}
			
			if(token==NULL){
				printf("Error in <zone> realm\n");
				exit(EXIT_FAILURE);
			}else{
				continue;
			}
		}
		
		
		
		
		
		/* something wrong in config file */
		printf("Unknown realm\n");
		exit(EXIT_FAILURE);
		
	} /* end parsing realms */
	
	GetConf_close();
	
	/* if there are no binding connectors - make one default */
	if(bind_connectors == NULL){
		bind_connectors = (dns_connector_t*)malloc(sizeof(dns_connector_t));
		memset(bind_connectors,0,sizeof(dns_connector_t));
		bind_connectors->addr.sin_family = AF_INET;
		bind_connectors->addr.sin_port = htons(DNS_DEF_PORT);
		bind_connectors->addr.sin_addr.s_addr = INADDR_ANY;
		if((bind_connectors->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
			perror("bind_connectors");
			exit(EXIT_FAILURE);
		}
	}
	
	/*init the cache */
	if(!qxdns_cache_init(c_max,c_ttl)) exit(EXIT_FAILURE);
	
	/* prepare buffers for query processing */
	for(num_bufs; num_bufs>0; num_bufs--){
		dns_buffer_t * tmp_buf = (dns_buffer_t*) malloc( sizeof(dns_buffer_t) );
		if(tmp_buf == NULL){
			perror("Queue:");
			exit(EXIT_FAILURE);
		}
		tmp_buf->next = free_buf;
		free_buf = tmp_buf;
	}
}



























































/*
 * * * * * * main init & run  * * * * * * * * * * * * * * * * * * * * * * * * *
 */

int main(void)
{
	fd_set				fdmask;
	fd_set				fds;
	int					maxfd;
	struct timeval		tout;
	int					ret;
	time_t				cur_time;
	dns_connector_t *	tmp_conn;
	dns_buffer_t *		tmp_q;
	
    /* Parse options from /etc/QX-dns.cnf */

    qxdns_setup();
    
    /* revers the list of connectors to external servers */
    tmp_conn = NULL;
    while(srvr_connectors != NULL){
    	dns_connector_t * tmp = srvr_connectors->next;
    	srvr_connectors->next = tmp_conn;
    	tmp_conn = srvr_connectors;
    	srvr_connectors = tmp;
	}
	srvr_connectors = tmp_conn;
    
    /* Setup our udp DNS query reception sockets. */
    tmp_conn = bind_connectors;
    while(tmp_conn != NULL){
    	/* bind listening sockets to local address */
		if (bind(tmp_conn->sock, (struct sockaddr *)&(tmp_conn->addr), sizeof(struct sockaddr_in)) < 0){
			qxdns_log(QLL_ERROR,"bind_connectors: %s\n",strerror(errno));
			exit(EXIT_FAILURE);
		}
    	/* move to the next connector or link server connectors to the tail and break */
    	if(tmp_conn->next != NULL){
    		tmp_conn = tmp_conn->next;
		}else{
			tmp_conn->next = srvr_connectors;
			break;
		}
	}
	
	/* make our initial fd_set to mask created sockets */
	FD_ZERO(&fdmask);
	maxfd = 0;
	tmp_conn = bind_connectors;
	while(tmp_conn != NULL){
		FD_SET(tmp_conn->sock, &fdmask);
		maxfd = ( maxfd > tmp_conn->sock ? maxfd : tmp_conn->sock );
		tmp_conn = tmp_conn->next;
	}
	maxfd++;
	
	
    /* start thread pool */
    qxdns_thread_pool = thread_pool_create( &qxdns_pool_attr, 0 );
    if( qxdns_thread_pool == NULL )
    {
    	qxdns_log(QLL_ERROR,"pool: %s\n",strerror(errno));
    	exit(EXIT_FAILURE);
    }
	if(0 > thread_pool_start( qxdns_thread_pool )){
		qxdns_log(QLL_ERROR,"pool: %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
    
    
    /* here comes the main loop where we recieve messages */
    while(1){
    	
    	/* each 2 seconds we look for queries that may need to go to external server.
    	 * If there are no more servers available - we remove query. */
    	tout.tv_sec		= 2;
		tout.tv_usec	= 0;
    	fds = fdmask;
    	
    	/* 1) get new queries */
    	ret = select(maxfd, &fds, NULL, NULL, &tout);
    	
    	/* check return code of select:
    	 *		0: timeout expired
    	 *	   <0: error occured or signal was delivered
    	 */
    	if( ret > 0 ){
    		cur_time = time(NULL);
    		tmp_conn = bind_connectors;
    		while(tmp_conn != NULL){
    			if(FD_ISSET(tmp_conn->sock,&fds)){
    				/* get the first available free buffer or wait until one is available */
    				pthread_mutex_lock( &free_mutex );
    				tmp_q = free_buf;
    				if(tmp_q != NULL) free_buf = tmp_q->next;
    				pthread_mutex_unlock( &free_mutex );
    				
    				/* if there were no free buffers - allocate new one */
    				if(tmp_q == NULL) tmp_q = (dns_buffer_t*)malloc(sizeof(dns_buffer_t));
    				memset(tmp_q,0,sizeof(dns_buffer_t));
    				
    				/* recv() the new message */
    				tmp_q->fromlen = sizeof(struct sockaddr_in);
    				tmp_q->qclient = tmp_conn;
    				tmp_q->datalen = recvfrom(tmp_conn->sock,tmp_q->pkt,512,0,(struct sockaddr*)&tmp_q->from,&tmp_q->fromlen);
    				
    				if(tmp_q->datalen<17){
    					if(tmp_q->datalen<0){
    						qxdns_log(QLL_ERROR,"recv: %s\n",strerror(errno));
						}else{
							qxdns_log(QLL_ERROR,"recv: too short query\n");
						}
    					/* return free buffer back */
    					pthread_mutex_lock( &free_mutex );
    					tmp_q->next = free_buf;
    					free_buf = tmp_q;
    					pthread_mutex_unlock( &free_mutex );
    					continue;
					}
					tmp_q->rcvtime = cur_time;
					
					/* signal threads we have a new query */
					pthread_mutex_lock( &ready_mutex );
					tmp_q->next = ready_buf;
					ready_buf = tmp_q;
					pthread_cond_signal( &ready_cond );
					pthread_mutex_unlock( &ready_mutex );
				}
				tmp_conn = tmp_conn->next;
			}
		}
		
		
		/* 2) remove expired queries or resend them to other servers */
    	cur_time -= 2; /* we will reexamine queries each 2 seconds */
    	
    	pthread_mutex_lock(&queue_mutex);
    	
    	tmp_q = queue_head;
    	while(tmp_q != NULL){
    		/* check if this record has expired: if yes - delete it */
    		if(tmp_q->rcvtime < cur_time ){
    			
    			/* this is the case when we may need to resend query
    			 * to external server or delete it from list */
    			
    			dns_buffer_t * tq = tmp_q->next;
    			qxdns_log(QLL_INFO,"Resending query to another server\n");
    			
    			if(tmp_q->qserver->next != NULL){
    				/* send query to the next server */
    				tmp_q->qserver = tmp_q->qserver->next;
    				printf("Resending message to: %s\n",inet_ntoa(tmp_q->qserver->addr.sin_addr));
    				sendto(tmp_q->qserver->sock,tmp_q->pkt,tmp_q->datalen,0,(struct sockaddr*)&tmp_q->qserver->addr,sizeof(struct sockaddr_in));
				}else{
					/* no more servers to query - return buffer back */
					qxdns_log(QLL_INFO,"Removing expired query\n");
					
					if(tmp_q->prev != NULL){
						tmp_q->prev->next = tmp_q->next;
					}else{
						queue_head = tmp_q->next;
					}
					
					if(tmp_q->next != NULL){
						tmp_q->next->prev = tmp_q->prev;
					}else{
						queue_tail = tmp_q->prev;
					}
					
					pthread_mutex_lock( &free_mutex );
					tmp_q->next = free_buf;
					free_buf = tmp_q;
					pthread_mutex_unlock( &free_mutex );
					tmp_q = tq;
				}
			
			}else{
				/* this query is still valid - move ahead */
				tmp_q = tmp_q->next;
			}
		}
		
    	pthread_mutex_unlock(&queue_mutex);
	}
	
    exit(0); /* in ideal case we should never get out of endless loop ;) */
}
