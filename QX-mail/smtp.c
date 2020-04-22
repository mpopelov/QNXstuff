/*
 * -= MikeP =- (c) 2004
 */

/*
 * Routines to handle SMTP connection
 */

#include	"qmail_common.h"

extern mailbox_t *	MBX;
extern char *		server_fqdn;
extern int alc_buf_size;
extern pthread_cond_t	spooler_cond;
extern pthread_mutex_t	spooler_mutex;
extern smtpmsg_t *		spool_msgs;






/* free all data associated with current session */
void	FreeSMTPSession( conn_info_t * conn )
{
	smtpmsg_t * msg = conn->msg;
	
	/* free allocated memory */
	
	/* --!!-- this prevents from sending several messages
	 * so it is commented out */
	/*conn->sstate = SS_NONE;
	conn->auth = 0;*/

	if(conn->user!=NULL){
		free(conn->user);
		conn->user = NULL;
	}
	if(conn->greeting!=NULL){
		free(conn->greeting);
		conn->greeting = NULL;
	}
	
	/* free all messages and delete temporary files */
	while( msg != NULL ){
		
		recipt_t * rcpt = msg->to;
		
		if(msg->from!=NULL) free(msg->from);
		if(msg->spoolfn!=NULL){
			unlink(msg->spoolfn);
			free(msg->spoolfn);
		}
		if(msg->spoolfd!=0) close(msg->spoolfd);
		
		/* free all recipients */
		while(rcpt!=NULL){
			
			if(rcpt->to!=NULL) free(rcpt->to);
			
			msg->to = rcpt->next;
			free(rcpt);
			rcpt = msg->to;
		}
		
		conn->msg = msg->next;
		free(msg);
		msg = conn->msg;
	}
}





/*
 * Here we actually process the client's SMTP requests
 */

int thread_handler_smtp( QTS * ts )
{
	int				result;
	struct timeval	sto;
	smtpmsg_t *		message = NULL;
	conn_info_t *	conn = ts->conn;
	
	/* explicitly set connection state */
	conn->sstate = SS_NONE;
	conn->auth = 0;

	/* 1) send greeting to client */
	result = sprintf(ts->buffer,"220 %s\r\n",server_fqdn);
	result = send( conn->tcp_sock, ts->buffer, result, 0 );
	if( result == -1 ) return 0;
	
	
    /* 2) now wait for commands from client and process them */
    while(1){
    	
    	/* 2.1) read a command from client */
    	result = ReadString(conn->tcp_sock, ts->buffer, alc_buf_size-1);
    	if( result == -1 ){
    		/* client failed to command either on timeout or on connection failure
    		 * if there were any changes - rollback them */
    		FreeSMTPSession(conn);
    		return 0;
		}
		
		/* 2.2) convert 4 first characters of command to uppercase */
		{
			int ix;
			for( ix=0; ix<4; ix++ ) ts->buffer[ix] = toupper( ts->buffer[ix] );
		}
		
		
		/* 2.3) switch command types */
		switch( *(uint32_t*)(ts->buffer) ){
			
			case SMTP_HELO: /*(-)*/
				{
					if(conn->sstate!=SS_NONE) FreeSMTPSession(conn);
					/* save the fqdn, given by client */
					if(result<7){
						send(conn->tcp_sock,"501\r\n",5,0);
						break;
					}
					/* to prevent looping: if HELO FQDN == our FQDN - drop connection */
					if(!strcmp(ts->buffer+5,server_fqdn)){
						/* kick loop */
						send(conn->tcp_sock,"550 LOOP\r\n",10,0);
						qmail_log(QLL_WARNING,"Message loop detected\n");
						FreeSMTPSession(conn);
						return 0;
					}
					conn->user = strdup(&(ts->buffer[5]));
					conn->sstate = SS_HELO;
					conn->auth = 0;
					/* !TODO! check black list for this hostname */
					result = sprintf(ts->buffer,"250 %s\r\n",server_fqdn);
					send(conn->tcp_sock,ts->buffer,result,0);
					break;
				}
				
			case SMTP_EHLO: /*(-)*/
				{
					/* if connection in any other state than SS_NONE
					 * HELO and EHLO commands should act as RSET */
					if(conn->sstate!=SS_NONE) FreeSMTPSession(conn);
					/* save the fqdn, given by client */
					if(result<7){
						send(conn->tcp_sock,"501\r\n",5,0);
						break;
					}
					/* to prevent looping: if HELO FQDN == our FQDN - drop connection */
					if(!strcmp(ts->buffer+5,server_fqdn)){
						/* kick loop */
						send(conn->tcp_sock,"550 LOOP\r\n",10,0);
						qmail_log(QLL_WARNING,"Message loop detected\n");
						FreeSMTPSession(conn);
						return 0;
					}
										
					conn->user = strdup(&(ts->buffer[5]));
					conn->sstate = SS_HELO;
					conn->auth = 0;
					/* !TODO! check black list for this hostname */
					/* !TODO! check IP of the host in black list */
					result = sprintf(ts->buffer,"250-%s\r\n250-8BITMIME\r\n250 AUTH LOGIN CRAM-MD5\r\n",server_fqdn);
					send(conn->tcp_sock,ts->buffer,result,0);
					break;
				}
				
			case SMTP_MAIL: /*(-)*/
				{
					char * s_beg, * s_end;
					smtpmsg_t * msg;
					
					/* mail command allowed only in SS_HELO state */
					if(conn->sstate!=SS_HELO){
						/* bad sequence of commands */
						send(conn->tcp_sock,"503\r\n",5,0);
						break;
					}
					
					/* parse the MAIL FROM: and return-path */
					s_beg = strchr(ts->buffer,'<');
					s_end = strchr(ts->buffer,'>');
					
					s_beg++;
					if((s_beg==NULL)||(s_end==NULL)){
						/* parameter error */
						send(conn->tcp_sock,"501\r\n",5,0);
						break;
					}
					
					*s_end = '\0';
					
					/* validate sender: maybe on spam list, relay disabled by host name or IP */
					
					/* we accept mail from this user */
					msg = (smtpmsg_t*)malloc(sizeof(smtpmsg_t));
					memset(msg,0,sizeof(smtpmsg_t));
					if(*s_beg == '\0'){
						msg->from = strdup("Bounced");
					}else{
						msg->from = strdup(s_beg);
					}
					msg->next = conn->msg;
					conn->msg = msg;
					
					conn->sstate = SS_RCPT;
					
					/* say OK */
					send(conn->tcp_sock,"250\r\n",5,0);
					break;
				}
			
			case SMTP_RCPT: /*(-)*/
				{
					char * s_beg, * s_end;
					mailbox_t * mbxs = MBX;
					
					if(conn->sstate!=SS_RCPT){
						/* bad sequence of commands */
						send(conn->tcp_sock,"503\r\n",5,0);
						break;
					}
					
					/* parse the MAIL FROM: and return-path */
					s_beg = strchr(ts->buffer,'<');
					s_end = strchr(ts->buffer,'>');
					
					s_beg++;
					if((s_beg==NULL)||(s_end==NULL)||(s_beg==s_end)){
						/* parameter error */
						send(conn->tcp_sock,"501\r\n",5,0);
						break;
					}
					
					*s_end = '\0';
					
					/* validate recipient:
					 * if local - everyone permitted to mail him/her
					 * non local - relay enabled only for authenticated users */
					while(mbxs!=NULL){
						if(0 == stricmp(s_beg,mbxs->user)) break;
						mbxs = mbxs->next;
					}
					
					if((mbxs==NULL)&&(conn->auth==0)){
						send(conn->tcp_sock,"553\r\n",5,0);
						break;
					}
					
					/* recipient ok - add its address to the list of recips */
					{
						recipt_t * rcpt = (recipt_t*)malloc(sizeof(recipt_t));
						rcpt->to = strdup(s_beg);
						rcpt->next = conn->msg->to;
						conn->msg->to = rcpt;
					}
					
					/* we do not change state - as there may be more recipts
					 * send ok and continue */
					send(conn->tcp_sock,"250\r\n",5,0);
					break;
				}
				
			case SMTP_DATA: /*(-)*/
				{
					int f_eom = 0;
					/* command available only in state RCPT */
					if(conn->sstate!=SS_RCPT){
						send(conn->tcp_sock,"503\r\n",5,0);
						break;
					}
					
					/* prepare spooler file and write
					 * Received: from ... by... */
					
					conn->msg->spoolfn = strdup(SPOOLER_TMPF);
					conn->msg->spoolfd = mkstemp(conn->msg->spoolfn);
					
					/* write a line to file which says where the mail came from */
					result = sprintf(ts->buffer,"Received: from %s(",conn->user);
					{
						struct sockaddr_in	peer;
						int					peer_len = sizeof(struct sockaddr_in);
						getpeername(conn->tcp_sock,(struct sockaddr*)&peer,&peer_len);
						inet_ntoa_r(peer.sin_addr,ts->buffer+result,20);
						while(ts->buffer[result]) result++;
					}
					result += sprintf(ts->buffer+result,") by %s\r\n",server_fqdn);
					
					/*result = sprintf(ts->buffer,"Received: from %s (???.???.???.???) by %s ; date\r\n",conn->user,server_fqdn);*/
					write(conn->msg->spoolfd,ts->buffer,result);
					
					/* say client: 354 Go on */
					send(conn->tcp_sock,"354\r\n",5,0);
					
					/* read data from client until <CRLF>.<CRLF> */
					{
						int		start = 0;
						
						while(f_eom==0){
							int	rpos;
							
							result = recv(conn->tcp_sock, &(ts->buffer[start]), (alc_buf_size-start),0);
							/* if( result == -1 ){ */
							if( result <= 0 ){
								/* client failed to command either on timeout or on connection failure
								 * or simply colsed its side of TCP connection pipe.
								 * if there were any changes - rollback them */
								FreeSMTPSession(conn);
								return 0;
							}
							result += start;
							/* find the last ocurance of '\r' character in received data */
							for(rpos=(result-1);rpos>=0;rpos--) if(ts->buffer[rpos]=='\r') break;
							
							if((rpos<3)||(rpos==(result-1))){
								start = result;
								continue;
							}else{
								if(ts->buffer[rpos-1]=='.')
									if(ts->buffer[rpos-2]=='\n')
										if(ts->buffer[rpos-3]=='\r'){
											rpos -= 3;
											f_eom = 1;
										}
							}
							
							write(conn->msg->spoolfd,ts->buffer,rpos);
														
							start = result - rpos;
							memcpy(ts->buffer,&(ts->buffer[rpos]),start);
						}
					}
					
					/* do not close spooler file descriptor so spooler
					 * thread could have it handy. besides keeping file open
					 * makes use of buffering. */
					
					/*close(conn->msg->spoolfd);*/
					
					pthread_mutex_lock(&spooler_mutex);
					conn->msg->next = spool_msgs;
					spool_msgs = conn->msg;
					conn->msg = NULL;
					pthread_cond_signal(&spooler_cond);
					pthread_mutex_unlock(&spooler_mutex);
					
					conn->sstate = SS_HELO;
					
					send(conn->tcp_sock,"250\r\n",5,0);
					break;
				}
				
			case SMTP_RSET: /*(+)*/
				{
					/* preserve current user */
					char * olduser = conn->user;
					conn->user = NULL;
					FreeSMTPSession(conn);
					conn->user = olduser;
					conn->sstate = SS_HELO;
					/* conn->auth is preserved */
					send(conn->tcp_sock,"250\r\n",5,0);
					break;
				}
				
			case SMTP_NOOP: /*(+)*/
				{
					send(conn->tcp_sock,"250\r\n",5,0);
					break;
				}
				
			case SMTP_QUIT: /*(+)*/
				{
					/* 1) close connection */
					result = sprintf(ts->buffer,"221 %s Bye\r\n", server_fqdn);
					send(conn->tcp_sock,ts->buffer,result,0);
					/* 2) clear everything */
					FreeSMTPSession(conn);
					return 0;
				}
			
			/* all other unsupported commands */
			case SMTP_AUTH: /*(-)*/
				{
					/* AUTH command allowed only in SS_HELO state */
					if(conn->sstate!=SS_HELO){
						/* bad sequence of commands */
						send(conn->tcp_sock,"503\r\n",5,0);
						break;
					}
					
					/* see if the kind of AUTH is LOGIN */
					if(!stricmp(ts->buffer+5,"LOGIN")){
						/* handle LOGIN type of connection */
						char	*	Username;
						int			Ulen = 0;
						int			Plen = 0;
						uint32_t	Enc64Len = 0;
						mailbox_t *	mbox = NULL;
						
						/* ask for username */
						send(conn->tcp_sock,"334 VXNlcm5hbWU6\r\n",18,0);
						if(-1 == (Ulen=ReadString(conn->tcp_sock, ts->buffer, 256)) ){
							FreeSMTPSession(conn);
							return 0;
						}
						
						/* check if the peer can be authenticated */
						if(B64_OK != decode64(ts->buffer,Ulen,ts->buffer,&Enc64Len)){
							/* repor "command not allowed" */
							send(conn->tcp_sock,"551\r\n",5,0);
							break;
						}
						Username = strdup(ts->buffer);
						
						/* ask for password */
						send(conn->tcp_sock,"334 UGFzc3dvcmQ6\r\n",18,0);
						if(-1 == (Plen=ReadString(conn->tcp_sock, ts->buffer, 256)) ){
							free(Username);
							FreeSMTPSession(conn);
							return 0;
						}
						
						/* user name is decoded - try to find appropriate mailbox */
						mbox = MBX;
						conn->auth = 0;
						while( mbox!=NULL ){
							if(!stricmp(mbox->user,Username)){
								/* compare passwords */
								if(B64_OK != decode64(ts->buffer,Plen,ts->buffer,&Enc64Len)){
									/* report bad arguments */
									send(conn->tcp_sock,"551\r\n",5,0);
									break;
								}
								
								if(!strcmp(mbox->password,ts->buffer)){
									conn->auth = 1;
									if(conn->user!=NULL) free(conn->user);
									conn->user = Username;
									send(conn->tcp_sock,"235\r\n",5,0);
								}
								break;
							}
							mbox = mbox->next;
						}
						if( conn->auth == 0 ){
							/* we close connection to not make issues with AUTH */
							free(Username);
							FreeSMTPSession(conn);
							send( conn->tcp_sock, "535\r\n", 5, 0 );
							return 0;
						}
						
						/* skip other auth */
						break;
					}
					
					/* see if the kind of auth is CRAM-MD5: */
					if(0 != stricmp(ts->buffer+5,"CRAM-MD5")){
						/* repor "command not allowed" */
						send(conn->tcp_sock,"550\r\n",5,0);
						break;
					}
					/* now it is time to send client a Base64 encoded salt-string */
					{
						char *		hash;
						char *		myhash;
						mailbox_t *	mbox = NULL;
						uint32_t	Enc64Len = 0;
						char		Challenge[512] = "";
						uint32_t	rawlen = snprintf(Challenge,511,"<%u@%s>",time(NULL),server_fqdn);
						
						conn->greeting = strdup(Challenge);
						
						/* encode and send greeting */
						*(uint32_t*)(ts->buffer) = 0x20343333;
						encode64(Challenge,rawlen,&(ts->buffer[4]),alc_buf_size,&Enc64Len);
						ts->buffer[Enc64Len+4]='\r';
						ts->buffer[Enc64Len+5]='\n';
						send(conn->tcp_sock,ts->buffer,Enc64Len+6,0);
						
						/* get back response and try to interprete it */
						result = ReadString(conn->tcp_sock, Challenge, 511);
						if( result == -1 ){
							/* client failed to command either on timeout or on connection failure
							 * if there were any changes - rollback them */
							FreeSMTPSession(conn);
							return 0;
						}
						
						/* decode client data and try to auth user */
						if(B64_OK != decode64(Challenge,result,ts->buffer,&Enc64Len)){
							/* repor "command not allowed" */
							send(conn->tcp_sock,"551\r\n",5,0);
							break;
						}
						
						/* apply CRAM-MD5 */
						hash = strchr(ts->buffer,' ');
						if(hash == NULL){
							send( conn->tcp_sock,"501\r\n", 5, 0 );
							break;
						}
						*hash = '\0'; hash++;
						if(conn->user!=NULL) free(conn->user);
						conn->user = strdup(ts->buffer);
						hash = strdup(hash);
						
						mbox = MBX;
						while( mbox!=NULL ){
							if(!stricmp(mbox->user,conn->user)){
								/* build CRAM-MD5 hash */
								int i;
								int efflen = strlen(conn->greeting);
								
								/* inner */
								memset(ts->buffer,0,64);
								memcpy(ts->buffer,mbox->password,strlen(mbox->password));
								for(i=0;i<64;i++) ts->buffer[i] ^= 0x36;
								memcpy(&(ts->buffer[64]),conn->greeting,efflen);
								myhash = MD5bin(ts->buffer,efflen+64);
								
								/* outer */
								memset(ts->buffer,0,64);
								memcpy(ts->buffer,mbox->password,strlen(mbox->password));
								for(i=0;i<64;i++) ts->buffer[i] ^= 0x5c;
								memcpy(&(ts->buffer[64]),myhash,16);
								free(myhash);
								myhash = GetMD5(ts->buffer,80);
								
								/* compare hash to one supplied */
								if(!memicmp(hash,myhash,32)){
									conn->auth = 1;
									send(conn->tcp_sock,"235\r\n",5,0);
								}
								free(myhash);
								break;
							}
							mbox = mbox->next;
						}
						free(hash);
						if( conn->auth == 0 ){
							/* we close connection to not make issues with AUTH */
							FreeSMTPSession(conn);
							send( conn->tcp_sock, "535\r\n", 5, 0 );
							return 0;
						}
						break;
					}
				}
				
			default:
				{
					/* report "command not allowed" */
					send(conn->tcp_sock,"550\r\n",5,0);
					break;
				}
		} /* {{{ end of switch(...) }}} */
		
		/* we processed command in some way and may continue waiting for further commands */
    }    

    /* we never get in here */
}






/* EOF */
