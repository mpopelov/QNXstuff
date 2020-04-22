/*
 * -={ MikeP }=- (c) 2005
 * Mike A. Popelov
 */



#include	"qx_vpn_common.h"
#include	"GetConf.h"

#define		CONF_FILE	"/etc/qxvpn.cnf"





/* sockets, connecting us to outer world */
struct sockaddr_in	bind_addr;
int					sock_ctl	= -1;

/* dispatch handles to take care of tcp sockets */
dispatch_t			*dpp_tcp	= NULL;
select_attr_t		attr_tcp;

/* thread_pool */
thread_pool_attr_t	pool_attr;
thread_pool_t		*tpp;











/* forward declaration */
int cc_action(select_context_t *ctp, int sock, unsigned flags, void * handle);





/*
 * Here we destroy client record and all associated data
 */
void destroy_client(client_t *client){
	
	/* destroy mutex */
	pthread_mutex_destroy(&client->mutex);
	
	/* See if counters in client record are not flushed to database.
	 * If so - add them to database */
	qx_vpn_log(QLL_INFO,"[%s:%hu] bytes out: %u   bytes in: %u\n",inet_ntoa(client->cli_addr.sin_addr),client->cli_addr.sin_port,client->ppp_data_out,client->ppp_data_in);
	
	/* now clean up everything after control connection */
	if(client->cc_sock != -1){
		select_detach(dpp_tcp,client->cc_sock);
		close(client->cc_sock);
	}
	
	if(client->raw_sock != -1){
		select_detach(dpp_tcp,client->raw_sock);
		close(client->raw_sock);
	}
	
	free(client);
}







/*
 * Here we process listening socket.
 * 
 * Whenever a new connection arrives we do the following:
 * 
 * 1) ACCEPT connection and get a new socket.
 * 2) Check if control channel with correspondent already exists.
 * 3) Create structures to handle new control channel.
 * 4) add new socket to dispatch handle for reading.
 */
int lstn_action(select_context_t *ctp, int fd, unsigned flags, void *handle){
	struct sockaddr_in	new_addr;
	socklen_t			new_len;
	int					new_sock;
	client_t			*new_client;
	
	new_len = sizeof(new_addr);
	if( (new_sock=accept(fd,(struct sockaddr*)&new_addr,&new_len)) <0 ){
		qx_vpn_log(QLL_ERROR,"Could not accept new connection: %s\n",strerror(errno));
		return 0;
	}
	
	/* accept a new connection */
	qx_vpn_log(QLL_INFO,"New control connection: %s:%hu\n",inet_ntoa(new_addr.sin_addr),new_addr.sin_port);
	
	/* allocate memory for a new client */
	if( (new_client = (client_t*)malloc(sizeof(client_t))) == NULL ){
		qx_vpn_log(QLL_ERROR,"Unable to allocate new connection: %s\n",strerror(errno));
		close(new_sock);
		return 0;
	}
	
	/* set all fields of client to initial state */
	memset(new_client,0,sizeof(client_t));
	new_client->cli_addr	= new_addr;
	new_client->cli_len		= new_len;
	new_client->cc_sock		= new_sock;
	new_client->raw_sock	= -1;
	new_client->saccm		= 0xffffffff;
	new_client->raccm		= 0xffffffff;
	pthread_mutex_init(&new_client->mutex,NULL);
	
	/* attach cc_sock to a dispatch handle */
	if( select_attach(dpp_tcp,&attr_tcp,new_sock,SELECT_FLAG_READ|SELECT_FLAG_REARM,cc_action,new_client) <0 ){
		qx_vpn_log(QLL_ERROR,"Unable to attach socket to dispatch handle: %s\n",strerror(errno));
		destroy_client(new_client);
	}
	
	/* we are done - happily return */
	return 0;
}










/*
 * Here comes processing of accepted control connections and associated PPP
 * stream. We maintain states of connections and everything else
 * that concerns passed data counting, PPTP sessions, outgoing calls
 * and so on.
 * This function is attached to dispatch handle in the routine that handles
 * listening socket and recieves control connection descriptor
 * as a handler pointer.
 */
int cc_action(select_context_t *ctp, int sock, unsigned flags, void * handle){
	
	int				result = -1;
	client_t		*client = (client_t*)handle;
	
	/* lock the mutex */
	if( EOK != pthread_mutex_lock(&client->mutex) ){
		/* That means mutex was destroyed due to connection closure.
		 * That also means client may be already free()-ed */
		return 0;
	}
	
	/* Now that we have locked mutex - no thread can destroy client */
	
	if(sock == client->cc_sock){
		result = handle_ccm(client);
	}else if(sock == client->raw_sock){
		qx_vpn_log(QLL_INFO,"GRE packet!\n");
		result = handle_ppp(client);
		result = 0;
	}else{
		qx_vpn_log(QLL_ERROR,"Unknown file descriptor\n");
		result = 0;
	}
	
	if(result < 0){
		/* cleanup everything */
		qx_vpn_log(QLL_INFO,"Connection to %s:%hu closed\n",inet_ntoa(client->cli_addr.sin_addr),client->cli_addr.sin_port);
		destroy_client(client);
	}else{
		pthread_mutex_unlock(&client->mutex);
	}
	
	return 0;
}






















































































/*
 * configuring server
 */
void qx_vpn_configure(char * cfn){
	char * token = NULL;
	
	/* default settings */
	LogLevel = QLL_INFO;
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(PPTP_PORT);
	bind_addr.sin_addr.s_addr = INADDR_ANY;
	
	/* open and read configuration from file */
	if( GetConf_open(cfn) == 0 ){
		qx_vpn_log(QLL_ERROR,"Can't open config file\n");
		exit(EXIT_FAILURE);
	}
	
	while( (token = GetConf_string()) != NULL ){
		
		
		if( !stricmp("<server>",token) ){
			while( (token = GetConf_string()) != NULL ){
				/* end of <server> realm */
				if( !stricmp("</server>",token) ){
					break;
				}
				if( !stricmp("Bind",token) ){
					if( NULL != (token = GetConf_token()) ){
						qx_vpn_log(QLL_INFO,"Binding server to %s\n",token);
						inet_aton( token, &bind_addr.sin_addr);
					}
					continue;
				}
				qx_vpn_log(QLL_ERROR,"Bad directive [%s,%i : %s]\n",CONF_FILE,GetConf_ln,token);
				exit(EXIT_FAILURE);
			}
			
			/* ERROR */
			if( token == NULL ){
				qx_vpn_log(QLL_ERROR,"Error in <server> realm [%s]\n",CONF_FILE);
				exit(EXIT_FAILURE);
			}else{
				continue;
			}
		}
		
		
		
		
		
		if( !stricmp("<client>",token) ){
			while( (token = GetConf_string()) != NULL ){
				/* end of <server> realm */
				if( !stricmp("</client>",token) ){
					/* add sanity checks */
					break;
				}
				qx_vpn_log(QLL_ERROR,"Bad directive [%s,%i : %s]\n",CONF_FILE,GetConf_ln,token);
				exit(EXIT_FAILURE);
			}
			
			/* ERROR */
			if( token == NULL ){
				qx_vpn_log(QLL_ERROR,"Error in <client> realm [%s]\n",CONF_FILE);
				exit(EXIT_FAILURE);
			}else{
				continue;
			}
		}
		
		
		
		
		
		qx_vpn_log(QLL_ERROR,"Unknown realm [%s,%i : %s]\n",CONF_FILE,GetConf_ln,token);
		exit(EXIT_FAILURE);
	}
	
	GetConf_close();
}




























/* implementing main program processing loop */
int main(){
	
	int					opt			= 1;
	dispatch_context_t	*new_dctp	= NULL;
	
	/* configure any required data */
	qx_vpn_configure(CONF_FILE);
	
	
	
	
	/* create socket to accept PPtP control connections */
	if( (sock_ctl = socket(AF_INET,SOCK_STREAM,0)) <0 ){
		qx_vpn_log(QLL_ERROR,"CTL socket: %s\n",strerror(errno));
		return EXIT_FAILURE;
	}
	setsockopt(sock_ctl, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(sock_ctl, (struct sockaddr *)&(bind_addr), sizeof(struct sockaddr_in)) < 0){
		qx_vpn_log(QLL_ERROR,"CTL bind: %s\n",strerror(errno));
		return EXIT_FAILURE;
	}
	if(listen(sock_ctl, 5) != 0){
		qx_vpn_log(QLL_ERROR,"CTL listen: %s\n",strerror(errno));
		return EXIT_FAILURE;
	}
	
	/*
	 * Here we prepare a dispatch interface for handling
	 * incoming TCP connections
	 */
	if( (dpp_tcp = dispatch_create()) == NULL ){
		qx_vpn_log(QLL_ERROR,"Unable to allocate dispatch handle: %s\n",strerror(errno));
		return EXIT_FAILURE;
	}
	if( select_attach(dpp_tcp,&attr_tcp,sock_ctl,SELECT_FLAG_READ|SELECT_FLAG_REARM,lstn_action,NULL) <0 ){
		qx_vpn_log(QLL_ERROR,"Unable to attach socket to dispatch handle: %s\n",strerror(errno));
		return EXIT_FAILURE;
	}
	
	/* prepare a pool of threads */
	memset(&pool_attr,0,sizeof(thread_pool_attr_t));
	pool_attr.handle = dpp_tcp;
	pool_attr.context_alloc = dispatch_context_alloc;
	pool_attr.block_func = dispatch_block;
	pool_attr.unblock_func = dispatch_unblock;
	pool_attr.handler_func = dispatch_handler;
	pool_attr.context_free = dispatch_context_free;
	pool_attr.lo_water = 2;
	pool_attr.hi_water = 4;
	pool_attr.increment = 1;
	pool_attr.maximum = 50;
	
	if( (tpp = thread_pool_create(&pool_attr,POOL_FLAG_USE_SELF)) == NULL ){
		qx_vpn_log(QLL_ERROR,"Can't create thread pool\n");
		return EXIT_FAILURE;
	}
	
	thread_pool_start(tpp);
	
	/* probably we do not get in here */
	return EOK;
}
