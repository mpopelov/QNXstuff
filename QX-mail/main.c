/*
 * main.c - contains the main() function
 */

#include	"qmail_common.h"
#include	"GetConf.h"
#include	<netdb.h>


/*
 * * * * * * SUPER GLOBALS  * * * * * * * * * * * * * * * * * * * * * * * * * * 
 */

mailbox_t *	MBX			= NULL;	/* list of available mailboxes with all their settings */
int			sync_int	= 10*60;

static thread_pool_attr_t	the_pool_attr;
void *						the_pool;
listener_t *				listeners = NULL;
conn_info_t					*c_head, *c_tail;
pthread_cond_t				accept_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t				accept_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *						efd;
extern FILE *				qxm_stderr;



/* global variables */
char *			server_fqdn = "QX-mail";
char *			server_dir = "/tmp";
char *			server_log = "/tmp/QX-mail.log";
int				alc_buf_size = DEF_BUF_SIZE;
int				num_spoolers = 1;
struct timeval	sto;



extern int UseSMTPRelay;
extern int AVEnable;
extern int LogLevel;
extern struct sockaddr_in RelayAddress;
extern struct sockaddr_in dns_srv;















/*
 * * * * * * Cleaning everything on exit  * * * * * * * * * * * * * * * * * * *
 */

void cleanexit(int status)
{
	listener_t * lsnr = listeners;
	/* here we close all the listeners */
	while(lsnr != NULL){
		close(lsnr->sock);
		listeners = lsnr->next;
		free(lsnr);
		lsnr = listeners;
	}
    exit(status);
}


/*
 * * * * * * a pool of threads to handle connection * * * * * * * * * * * * * *
 */



/* Here we allocate connection buffer for each procesing thread */

THREAD_POOL_PARAM_T * pool_context_alloc( THREAD_POOL_HANDLE_T * handle )
{
	QTS * ts;
	ts = (QTS*) malloc( sizeof(QTS) );
	ts->buffer = (char*) malloc( alc_buf_size );
	ts->conn = NULL;
	/* we simply allocate connection descriptor */
    return (THREAD_POOL_PARAM_T*) ts;
}



/* Here we destroy connection descriptor */

void pool_context_free( THREAD_POOL_PARAM_T * param )
{
	/* here we destroy the connection descriptor */
	QTS * ts = (QTS*) param;
	free(ts->buffer);
	free(ts);
}



/* Here we block waiting for new connections */

THREAD_POOL_PARAM_T * pool_block( THREAD_POOL_PARAM_T * msg )
{
	
	QTS * ts = (QTS*) msg;
	
	/*
	 * Here we wait for connections to appear
	 */
	pthread_mutex_lock( &accept_mutex );
	while( c_head == NULL ){
		pthread_cond_wait( &accept_cond, &accept_mutex );
	}
	
	ts->conn = c_head;
	if(c_head == c_tail){
		c_head = NULL;
		c_tail = NULL;
	}else{
		c_head = c_head->next;
	}
	pthread_mutex_unlock( &accept_mutex );
	
	return (THREAD_POOL_PARAM_T*) ts;
}


/* Here we prepare to handle the connection.
 * these steps are taken:
 * 		- check if the client is allowed to access the server
 * 		  ( IP ban or something else )
 * 		- check the type of request we are about to serve and then
 * 		  make appropriate calls
 * 		- cleanup after connection
 * 		- maybe something else
 */

int pool_handler( THREAD_POOL_PARAM_T * prmt )
{
	QTS * ts = (QTS*) prmt;
	conn_info_t * conn = ts->conn;
	
	/* 1) make a call to appropriate function */
	if(conn->type == QMAIL_LISTEN_POP){
		thread_handler_pop( ts );
	}else{
		thread_handler_smtp( ts );
	}
	
	/* 2) cleanup everything */
	close( conn->tcp_sock );

	if(conn->user != NULL) free(conn->user);
	if(conn->greeting != NULL) free(conn->greeting);
	
	/* clean everything that is related to cc member of ts->conn */
	while(conn->msgs != NULL){
		msgent_t * msg = conn->msgs;
		conn->msgs = msg->next;
		free(msg->name);
		free(msg);
	}
	
	free(ts->conn);
	
	return 0;
}



















































/*
 * * * * * * Set up the daemon  * * * * * * * * * * * * * * * * * * * * * * * *
 */

void qmail_setup(void)
{
	char * token;
	
	/* default settings */
	c_head = NULL;
	c_tail = NULL;
	
    memset( &the_pool_attr, 0, sizeof(thread_pool_attr_t) );
    the_pool_attr.lo_water	= 2;
    the_pool_attr.hi_water	= 5;
    the_pool_attr.increment	= 2;
    the_pool_attr.maximum	= 9;
    the_pool_attr.block_func	= pool_block;
    the_pool_attr.context_alloc	= pool_context_alloc;
    the_pool_attr.handler_func	= pool_handler;
    the_pool_attr.context_free	= pool_context_free;
    
	sto.tv_sec  = DEF_RCVTIMEOUT; /* i think 5 minutes per letter is more then enough :) */
	sto.tv_usec = 0;

    
    /* here goes detailed configuration */
    if( GetConf_open( "/etc/qmail.cnf" ) == 0 ){
    	printf("Can't open config file\n");
    	exit(EXIT_FAILURE);
	}
    
    /* start parsing server config file */
    while( (token = GetConf_string()) != NULL ){
    	



    	/* parse realm splitters */
    	if( !stricmp("<server>",token) ){
    		
    		int pop_or_smtp = 0;
    		
    		/* here we configure the global server realm */
    		while( (token = GetConf_string()) != NULL ){
    			/* end of realm */
    			if( !stricmp("</server>",token) ){
    				break;
				}
    			/* directory, where all the mailboxes are stored */
    			if( !stricmp("MXROOT",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					server_dir = strdup(token);
					}
    				continue;
				}
				/* interval to sync mailboxes with external servers */
				if( !stricmp("SYNCINTERVAL",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					sync_int = atoi(token);
					}
    				continue;
				}
				if( !stricmp("SERVERFQDN",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					server_fqdn = strdup(token);
					}
    				continue;
				}
				if( !stricmp("LOGFILE",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					server_log = strdup(token);
					}
    				continue;
				}
				if( !stricmp("LOGLEVEL",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					LogLevel = atoi(token);
					}
    				continue;
				}
				if( !stricmp("CONNBUFSIZE",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					alc_buf_size = max(DEF_BUF_SIZE,atoi(token));
					}
    				continue;
				}
				if( !stricmp("POOLLOWATER",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					the_pool_attr.lo_water = atoi(token);
					}
    				continue;
				}
				if( !stricmp("POOLHIWATER",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					the_pool_attr.hi_water = atoi(token);
					}
    				continue;
				}
				if( !stricmp("POOLMAX",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					the_pool_attr.maximum = atoi(token);
					}
    				continue;
				}
				if( !stricmp("POOLINCREMENT",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					the_pool_attr.increment = atoi(token);
					}
    				continue;
				}
				if( !stricmp("NUMSPOOLERS",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					num_spoolers = max(num_spoolers,atoi(token));
					}
    				continue;
				}
				if( !stricmp("RCVTIMEOUT",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					sto.tv_sec = atoi(token);
					}
    				continue;
				}
				if( !stricmp("AVENABLE",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					AVEnable = atoi(token);
					}
    				continue;
				}
				if( !stricmp("SMTPRELAY",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					UseSMTPRelay = 1;
    					if(*token == '['){
    						/* if the name given is IP address */
    						char * delim = strchr(token,']');
    						if(delim==NULL){
    							printf("malformed relay IP\n");
    							UseSMTPRelay = 0;
							}else{
								*delim = 0;
								delim = strchr(token,':');
								if(delim!=NULL){
									*delim=0; delim++;
									RelayAddress.sin_port = ENDIAN_RET16(atoi(delim));
								}else{
									RelayAddress.sin_port = ENDIAN_RET16(25);
								}
								RelayAddress.sin_family = AF_INET;
								inet_aton( token+1, &(RelayAddress.sin_addr) );
							}
						}else{
							/* the name is FQDN */
							struct hostent * name = gethostbyname(token);
							if(name == NULL){
								printf("Can't resolve relay\n");
								UseSMTPRelay = 0;
							}else{
								/* select IP */
								RelayAddress.sin_family = AF_INET;
								RelayAddress.sin_port = ENDIAN_RET16(25);
								RelayAddress.sin_addr = *((struct in_addr *)name->h_addr_list[0]);
							}
						}
					}else{
						UseSMTPRelay = 0;
					}
    				continue;
				}
				/* we differ POP3 and SMTP listeners
				 * sintax is:
				 * Pop3Listen xxx.yyy.zzz.aaa:nnn
				 * SmtpListen xxx.yyy.xxx.bbb:nnn
				 * 
				 * No checking for duplicate listen directives yet.
				 */
				if( !stricmp("POP3LISTEN",token) ){
					pop_or_smtp = 1;
				}
				if( (!stricmp("SMTPLISTEN",token))||(pop_or_smtp==1) ){
    				if( NULL != (token = GetConf_token()) ){
    					char * portnum;
    					
    					/* allocate a new listener */
						listener_t * new_l = (listener_t*) malloc(sizeof(listener_t));
    					
    					/* configure */
    					if(pop_or_smtp==1){
    						new_l->type = QMAIL_LISTEN_POP;
						}else{
							new_l->type = QMAIL_LISTEN_SMTP;
						}
    					new_l->bind_addr.sin_family = AF_INET;
    					portnum = strchr(token,':');
    					if(portnum != NULL){
    						*portnum = '\0';
    						portnum++;
    						new_l->bind_addr.sin_port = htons(atoi(portnum));
						}else{
							new_l->bind_addr.sin_port = htons((pop_or_smtp?POP3_PORT:SMTP_PORT));
						}
    					inet_aton( token, &(new_l->bind_addr.sin_addr) );
    					
    					/* add configured listener */
    					new_l->next = listeners;
    					listeners = new_l;
					}
					pop_or_smtp = 0;
    				continue;
				}
				if( !stricmp("USEDNS",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					inet_aton(token,&dns_srv.sin_addr);
    					dns_srv.sin_family = AF_INET;
    					dns_srv.sin_port = ENDIAN_RET16(53);
					}
    				continue;
				}
				printf("Bad directive > %s < in <server> realm\n", token);
				exit(EXIT_FAILURE);
			}
			if(token == NULL){
				printf("Error in <server> realm\n");
				exit(EXIT_FAILURE);
			}else{
				continue;
			}
		}
		



		if( !stricmp("<mailbox>",token) ){
    		/* here we configure realms of mailboxes */
    		/* we met a new mailbox - create one more and add it on top of list */
    		mailbox_t * mbox	=	(mailbox_t*)malloc(sizeof(mailbox_t));
    		memset(mbox,0,sizeof(mailbox_t));
    		
    		while( (token = GetConf_string()) != NULL ){
    			/* end of realm */
    			if( !stricmp("</mailbox>",token) ){
    				if( (mbox->user==NULL) || (mbox->password==NULL) ){
    					printf("no username or password for mailbox\n");
    					exit(EXIT_FAILURE);
					}
					pthread_mutex_init( &mbox->mtx, NULL );
					mbox->next = MBX;
					MBX = mbox;
    				break;
				}
				if( !stricmp("USER",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					mbox->user = strdup(token);
					}
    				continue;
				}
				if( !stricmp("PASSWORD",token) ){
    				if( NULL != (token = GetConf_token()) ){
    					mbox->password = strdup(token);
					}
    				continue;
				}
				printf("Bad directive > %s < in <mailbox> realm\n", token);
				exit(EXIT_FAILURE);
			}
			if(token == NULL){
				printf("Error in <mailbox> realm\n");
				exit(EXIT_FAILURE);
			}else{
				continue;
			}
		}
		
		
		/* we got unknown command in config file ! */
		printf("Unknown realm\n");
		exit(EXIT_FAILURE);
	}

    GetConf_close();
}












/*
 * * * * * * main init & run  * * * * * * * * * * * * * * * * * * * * * * * * *
 */

int main(void)
{
    listener_t *	listn;
    
    fd_set			rfd;
    fd_set			rfd_mask;
    
    struct linger	li;
    int				max_fd = 1;
    struct timeval	tout;
    
	qmail_setup();
	/* now set signal handlers */
	signal(SIGQUIT, SIG_IGN);
    
    /* now redirect stderr to log file */
    qxm_stderr = fopen(server_log,"a");
    if(qxm_stderr == NULL){
    	printf("Log file: %s",strerror(errno));
    	exit(-1);
	}
	setvbuf(qxm_stderr,NULL,_IOLBF,0);
	
    if(listeners == NULL){
    	qmail_log(QLL_ERROR,"Bind server to some IP\n");
    	exit(-1);
	}


	/* turn our listeners into listening state :) */
	listn = listeners;
	while(listn != NULL){
		int opt = 1;
		
		if((listn->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			qmail_log(QLL_ERROR,"socket: %s\n",qserr(errno));
			cleanexit(-1);
		}
		setsockopt(listn->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if(bind(listn->sock, (struct sockaddr *)&(listn->bind_addr), sizeof(struct sockaddr_in)) < 0){
			qmail_log(QLL_ERROR,"bind: %s\n",qserr(errno));
			cleanexit(-1);
		}
		if (listen(listn->sock, 5) != 0){
			qmail_log(QLL_ERROR,"listen: %s\n",qserr(errno));
			cleanexit(-1);
		}
		max_fd = max(listn->sock, max_fd);
		listn = listn->next;
	}
	
	/* fill lingering options */
	li.l_onoff = 1;
	li.l_linger = 5; /* max seconds to linger */

    /* Change our current working directory to server_dir.
     * Since we open socket for each lookup we don't
     * know answer for - we can not change root - we'll
     * be unable to create socket.
     */
    if (chdir(server_dir)) {
    	qmail_log(QLL_ERROR,"Can't chdir to %s: %s\n",server_dir,qserr(errno));
    	cleanexit(-1);
    }
    
/*
 * Now we become daemon :)
 */

#ifndef QMAIL_DEBUG_ON
    
    {
    	pid_t pid = fork();
    	if (pid < 0) {
    		perror("Can't fork");
    		cleanexit(-1);
		}
		if (pid != 0) cleanexit(0);
	}
/*    setsid();
    umask(077);*/
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
#endif

	/* start spooler threads */
	for(num_spoolers;num_spoolers>0;num_spoolers--)
		if(EOK != pthread_create(NULL, NULL, &Spooler, NULL )){
			qmail_log(QLL_ERROR,"spooler: %s\n",qserr(errno));
			cleanexit(-1);
		}
	
    /* start thread pool */
    the_pool = thread_pool_create( &the_pool_attr, 0 ); /* use 0 as flags to continue */
    if( the_pool == NULL ){
    	qmail_log(QLL_ERROR,"pool create: %s\n",qserr(errno));
    	cleanexit(-1);
    }
    if( -1 == thread_pool_start( the_pool ) ){
    	qmail_log(QLL_ERROR,"pool start: %s\n",qserr(errno));
    	cleanexit(-1);
	}
	
	/* for analysis reason */
	{
		char sttimes[26];
		time_t sttime;
		time(&sttime);
		ctime_r(&sttime,sttimes);
		sttimes[24] = '\0';
		qmail_log(QLL_SYSTEM,"*** Q-MAIL started on %s\n",sttimes);
		qmail_log(QLL_SYSTEM,"*** LogLevel set to %d\n",LogLevel);
	}
	
	/* prepare rfd_mask */
	FD_ZERO( &rfd_mask );
	listn = listeners;
	while(listn != NULL){
		FD_SET(listn->sock, &rfd_mask);
		listn = listn->next;
	}
	
	/* now enter endless loop: select ready listeners and handle connections */
	while(1){
		int s;
		
		/* reset listeners */
		rfd = rfd_mask;
		
		/* reset timeout */
		tout.tv_sec = 2;
		tout.tv_usec = 0;
		
		/* wait for connection to arrive */
		s = select(max_fd+1, &rfd, NULL, NULL, &tout);
		if( s == -1 ) cleanexit(-1);
		if( s == 0 ){
			/* timeout happened so do nothing but continue the loop */
			continue;
		}
		
		/* surf the listeners and watch the sockets */
		listn = listeners;
		while( listn!=NULL ){
			if(FD_ISSET(listn->sock,&rfd)){
				/* make connection */
				conn_info_t * n_c = (conn_info_t*) malloc(sizeof(conn_info_t));
				memset(n_c,0,sizeof(conn_info_t));
				n_c->fromlen = sizeof(struct sockaddr);
				n_c->tcp_sock = accept(listn->sock,(struct sockaddr*)&n_c->from,&n_c->fromlen);
				if(n_c->tcp_sock == -1){
					qmail_log(QLL_ERROR,"accept: %s\n",qserr(errno));
					free(n_c);
					listn = listn->next;
					continue;
				}
				n_c->type = listn->type;

				/* --!!-- enable lingering closure of socket: failing is not fatal */
				setsockopt(n_c->tcp_sock, SOL_SOCKET, SO_LINGER, &li, sizeof(struct linger));
				
				/* --!!-- disable Nagle algorithm: failing to do so isn't fatal */
				s = 1;
				setsockopt(n_c->tcp_sock, IPPROTO_TCP, TCP_NODELAY, &s, sizeof(int));
				
				/* --!!-- set socket timeout to prevent hanging connections */
				setsockopt( n_c->tcp_sock, SOL_SOCKET, SO_RCVTIMEO, &sto, sizeof(struct timeval) );

				
				/* add connection at the end of the list */
				pthread_mutex_lock( &accept_mutex );
				if(c_head == c_tail){
					c_head = n_c;
					c_tail = n_c;
				}else{
					c_tail->next = n_c;
					c_tail = n_c;
				}
				pthread_cond_signal( &accept_cond );
				pthread_mutex_unlock( &accept_mutex );
			}
			listn = listn->next;
		}
	}
	
    cleanexit(0); /* get out of here - normally we do not :) */
}


/* EOF */
