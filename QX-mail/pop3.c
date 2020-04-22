/*
 * -= MikeP =- (c) 2004
 */

/*
 * Routines to handle POP3 connection
 */

#include	"qmail_common.h"


extern mailbox_t *	MBX;
extern char *		server_fqdn;
extern int alc_buf_size;





/*
 * Builds a list of files available in mailbox
 * only files, but neither symlinks nor directories
 * are included in the session list of messages
 */
int POPBuildSession( QTS * ts )
{
	DIR *			dirp;
	struct dirent * tmp;
	struct stat		statp;
	struct dirent * direntp = (struct dirent *)malloc( sizeof(struct dirent) + NAME_MAX + 1 );
	
	if( (dirp = opendir( ts->conn->user )) == NULL ){
		return 0;
	}
	
	for(;;){
		if( EOK != readdir_r( dirp, direntp, &tmp ) ) break;
		if( tmp == NULL ) break;
		sprintf( ts->buffer, "%s/%s", ts->conn->user, direntp->d_name );
		lstat( ts->buffer, &statp );
		if( S_ISREG(statp.st_mode) ){
			msgent_t * msg = (msgent_t*)malloc(sizeof(msgent_t));
			msg->del = 0;
			msg->size = statp.st_size;
			msg->name = strdup(ts->buffer);
			msg->next = ts->conn->msgs;
			ts->conn->msgs = msg;
		}
	}
	
	free(direntp);
	closedir(dirp);
	
	return 1;
}







/*
 * Here we actually process the client's POP3 requests
 */

int thread_handler_pop( QTS * ts )
{
	int				result;
	struct timeval	sto;
	mailbox_t *		mbox = NULL;
	time_t			curtime;
	
	conn_info_t * conn = ts->conn;
	
	/* explicitly set connection state */
	conn->pstate = MBX_NONE;
	
	/* form the greeting part to allow APOP */
	curtime = time(NULL);
	sprintf(ts->buffer,"<%u@%s>",curtime,server_fqdn);
	conn->greeting = strdup(ts->buffer);
	
	/* 1) send greeting to client */
	result = sprintf(ts->buffer,"+OK %s\r\n",conn->greeting);
	result = send( conn->tcp_sock, ts->buffer, result, 0 );
	if( result == -1 ) return 0;
	
	
    /* 2) now wait for commands from client and process them */
    while(1){
    	
    	/* 2.1) read a command from client */
    	result = ReadString(conn->tcp_sock, ts->buffer, alc_buf_size-1);
    	if( result == -1 ){
    		/* client failed to command either on timeout or on connection failure
    		 * if there were any changes - rollback them */
    		/* cleanup all locks and allocated data */
    		/* here we should commit changes if any */
			if(mbox != NULL) pthread_mutex_unlock( &mbox->mtx );
    		return 0;
		}
		
		/* 2.2) convert 4 first characters of command to uppercase */
		{
			int ix;
			for( ix=0; ix<4; ix++ ) ts->buffer[ix] = toupper( ts->buffer[ix] );
		}
		
		
		/* 2.3) switch command types */
		switch( *(uint32_t*)(ts->buffer) ){
			
			/* AUTHORISATION stage commands */
			case POP3_APOP: /*(+)*/
				{
					char * myhash;
					char * hash;
					
					/* check there is valid request */
					if( (conn->pstate!=MBX_NONE) || (result<40) ){
						send( conn->tcp_sock, POP3_NEG, 6, 0 );
						break;
					}
					/* split the username and MD5 hash */
					hash = strchr(&(ts->buffer[5]),' ');
					if(hash == NULL){
						send( conn->tcp_sock, POP3_NEG, 6, 0 );
						break;
					}
					*hash = '\0'; hash++;
					conn->user = strdup(&(ts->buffer[5]));
					hash = strdup(hash);
					
					mbox = MBX;
					while( mbox!=NULL ){
						if(!stricmp(mbox->user,conn->user)){
							/* build MD5 hash */
							result = sprintf(ts->buffer,"%s%s",conn->greeting,mbox->password);
							myhash = GetMD5(ts->buffer,result);
							/* compare hash to one supplied */
							if(!memicmp(hash,myhash,32)){
								if( EOK == pthread_mutex_trylock(&mbox->mtx) ){
									conn->pstate = MBX_TRAN;
									POPBuildSession( ts ); /* builds list of messages */
									send( conn->tcp_sock, POP3_POS, 5, 0);
								}
							}
							free(myhash);
							break;
						}
						mbox = mbox->next;
					}
					free(hash);
					if( conn->pstate != MBX_TRAN ){
						if(conn->user != NULL) free(conn->user);
						conn->user = NULL;
						conn->pstate = MBX_NONE;
						send( conn->tcp_sock, POP3_NEG, 6, 0 );
					}
					break;
				}
				
			case POP3_USER: /*(+)*/
				{
					/* We check if the username == mailbox is supplied.
					 * if no username given - we say -ERR.
					 * if there is any username - we simply remember it and set
					 * connection state to AUTH. We'll check if the username is
					 * valid when user sends password */
					if( (conn->pstate!=MBX_NONE) || (result<7) ){
						send( conn->tcp_sock, POP3_NEG, 6, 0 );
					}else{
						conn->user = strdup( &(ts->buffer[5]) );
						conn->pstate = MBX_AUTH;
						send( conn->tcp_sock, POP3_POS, 5, 0 );
					}
					break;
				}
				
			case POP3_PASS: /*(+)*/
				{
					/* - check we are in AUTH state
					 * - check the password lenght is at least == 1
					 * - check there is mailbox with the username given
					 * - check the password is correct for that mailbox
					 * - try to lock it
					 */
					if( (conn->pstate!=MBX_AUTH) || (result<7) ){
						send( conn->tcp_sock, POP3_NEG, 6, 0);
						break;
					}
					
					mbox = MBX;
					while( mbox!=NULL ){
						if(!stricmp(mbox->user,conn->user)){
							/* check password */
							if(!stricmp(mbox->password,&ts->buffer[5])){
								if( EOK == pthread_mutex_trylock(&mbox->mtx) ){
									conn->pstate = MBX_TRAN;
									POPBuildSession( ts ); /* builds list of messages */
									send( conn->tcp_sock, POP3_POS, 5, 0);
								}
							}
							break;
						}
						mbox = mbox->next;
					}
					if( conn->pstate != MBX_TRAN ){
						if(conn->user != NULL) free(conn->user);
						conn->user = NULL;
						conn->pstate = MBX_NONE;
						send( conn->tcp_sock, POP3_NEG, 6, 0 );
					}
					break;
				}
			
			/* TRANSACTION state commands */
			case POP3_STAT: /*(+)*/
				{
					int			send_len;
					int			total_msgs = 0;
					int			total_size = 0;
					msgent_t *	msgs = conn->msgs;
					
					/* send client statistics on mailbox */
					if( conn->pstate != MBX_TRAN ){
						send( conn->tcp_sock, POP3_NEG, 6, 0);
						break;
					}
					
					while( msgs != NULL ){
						if(msgs->del == 0){
							total_msgs++;
							total_size += msgs->size;
						}
						msgs = msgs->next;
					}
					send_len = sprintf(ts->buffer,"+OK %i %i\r\n",total_msgs,total_size);
					send( conn->tcp_sock, ts->buffer, send_len, 0 );
					break;
				}
				
			case POP3_LIST: /*(+)*/
				{
					int			send_len;
					int			msgnum = 0;
					msgent_t *	msgs = conn->msgs;
					
					/* list all or required message to client */
					if( conn->pstate!=MBX_TRAN ){
						/* allowed only in TRANSACTION state - send error */
						send( conn->tcp_sock,POP3_NEG,6,0);
						break;
					}
					
					/* see what client wants from us */
					if( ts->buffer[4]==0 ){
						int i = 1;
						/* we are asked to LIST all messages */
						send_len = sprintf(ts->buffer,POP3_POS);
						while(msgs!=NULL){
							/* add info on the message */
							if(msgs->del==0){
								/* add info on the message */
								send_len += sprintf(&(ts->buffer[send_len]),"%i %i\r\n",i,msgs->size);
							}
							i++;
							msgs = msgs->next;
						}
						/* finally add termination octets */
						/*send_len += sprintf(&(ts->buffer[send_len]),".\r\n");*/
						*(uint32_t*)(ts->buffer+send_len) = 0x000a0d2e;
						send_len += 3;
					}else{
						/* check the number is supplied */
						if( (ts->buffer[4]==' ') && (ts->buffer[5]!=0) && ((msgnum=atoi(&(ts->buffer[5])))!=0) ){
							/* print info on that message */
							int i;
							for(i=1;((i<msgnum)&&(msgs!=NULL));i++) msgs = msgs->next;
							if((msgs==NULL)||(msgs->del!=0)){
								send(conn->tcp_sock,POP3_NEG,6,0);
								break;
							}else{
								send_len = sprintf(ts->buffer,"+OK %i %i\r\n",msgnum,msgs->size);
							}
						}else{
							/* there was an error determining the number of message */
							send(conn->tcp_sock,POP3_NEG,6,0);
							break;
						}
					}
					
					send( conn->tcp_sock,ts->buffer, send_len, 0 );
					break;
				}
				
			case POP3_RETR: /*(+)*/
				{
					int			msgnum = 0;
					msgent_t *	msgs = conn->msgs;
					
					/* say OK and send required message */
					if(conn->pstate!=MBX_TRAN){
						send(conn->tcp_sock,POP3_NEG,6,0);
						break;
					}
					
					/* check the number is supplied */
					if( (ts->buffer[4]==' ') && (ts->buffer[5]!=0) && ((msgnum=atoi(&(ts->buffer[5])))!=0) ){
						/* send that message */
						int i;
						for(i=1;((i<msgnum)&&(msgs!=NULL));i++) msgs = msgs->next;
						if((msgs==NULL)||(msgs->del!=0)){
							send(conn->tcp_sock,POP3_NEG,6,0);
							break;
						}else{
							/* send body of the massage */
							i = open(msgs->name,O_RDONLY);
							if(i==(-1)){
								send(conn->tcp_sock,POP3_NEG,6,0);
								break;
							}
							/* send positive response */
							send(conn->tcp_sock,POP3_POS,5,0);
							while(0!=(msgnum = read(i,ts->buffer,alc_buf_size))){
								/* ??? fix up the <CRLF>.<CRLF> sequence at end of file
								 * if we use SMTP to inject mail into server, then we may
								 * be sure there is <CRLF> at the end of file.
								 * Now send the block */
								send(conn->tcp_sock,ts->buffer,msgnum,0);
							}
							close(i);
							/* send the terminating line */
							send(conn->tcp_sock,POP3_EOM,5,0);
							break;
						}
					}else{
						/* there was an error determining the number of message */
						send(conn->tcp_sock,POP3_NEG,6,0);
						break;
					}
				}
				
			case POP3_DELE: /*(+)*/
				{
					int			msgnum = 0;
					msgent_t *	msgs = conn->msgs;
					
					/* delete required message and say OK */
					if(conn->pstate!=MBX_TRAN){
						send(conn->tcp_sock,POP3_NEG,6,0);
						break;
					}
					
					/* check the number is supplied */
					if( (ts->buffer[4]==' ') && (ts->buffer[5]!=0) && ((msgnum=atoi(&(ts->buffer[5])))!=0) ){
						/* delete that message */
						int i;
						for(i=1;((i<msgnum)&&(msgs!=NULL));i++) msgs = msgs->next;
						if((msgs==NULL)||(msgs->del!=0)){
							send(conn->tcp_sock,POP3_NEG,6,0);
							break;
						}else{
							msgs->del = 1;
							send(conn->tcp_sock,POP3_POS,5,0);
							break;
						}
					}else{
						/* there was an error determining the number of message */
						send(conn->tcp_sock,POP3_NEG,6,0);
						break;
					}
				}
				
			case POP3_RSET: /*(+)*/
				{
					msgent_t *	msgs = conn->msgs;
					/* mark all deleted messages as VALID and say OK */
					if(conn->pstate!=MBX_TRAN){
						send(conn->tcp_sock,POP3_NEG,6,0);
						break;
					}
					while( msgs!=NULL ){
						msgs->del = 0;
						msgs = msgs->next;
					}
					send(conn->tcp_sock,POP3_POS,5,0);
					break;
				}
				
			case POP3_NOOP: /*(+)*/
				{
					/* send client reply - we are still alive */
					if( conn->pstate != MBX_TRAN ){
						send(conn->tcp_sock,POP3_NEG,6,0);
						return 0;
					}else{
						send(conn->tcp_sock,POP3_POS,5,0);
					}
					break;
				}
				
			case POP3_QUIT: /*(+)*/
				{
					/* send client OK and unblock him */
					send( conn->tcp_sock, POP3_POS, 5, 0 );
					
					/* here we should commit changes if any */
					while(conn->msgs != NULL){
						msgent_t * msg = conn->msgs;
						conn->msgs = msg->next;
						if(msg->del) remove(msg->name);
						free(msg->name);
						free(msg);
					}
					conn->msgs = NULL;
					
					/* free all unneeded data and close connection */
					if(mbox != NULL){
						pthread_mutex_unlock(&mbox->mtx);
					}
					return 0; /* AMBA - finish processing */
				}
			
			/* yet UNSUPPORTED commands */
			case POP3_UIDL: /*(-)*/
			case POP3_TOP: /*(-)*/
			default:
				{
					/* we do not support these commands - send negative reply and continue */
					send( conn->tcp_sock, POP3_NEG, 6, 0 );
					break;
				}
		} /* {{{ end of switch(...) }}} */
		
		/* we processed command in some way and may continue waiting for further commands */
    }    

    /* we never get in here */
}






/* EOF */
