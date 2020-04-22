/*
 * -=Mikep=- (c) 2004
 */

/*
 * Spooler thread. It is responsible for delivery of messages
 * 
 * when message arrives by SMTP protocol, any thread that handles
 * connection on successful completition of DATA command
 * locks spooler mutex, adds the message to queue, signals spooler
 * to start delivery and releases spooler mutex.
 */

#include	"qmail_common.h"
#include	"qmail_smtp.h"
#include	"qmail_spooler.h"
#include	"qavird.h"
#include	<netdb.h>


pthread_cond_t		spooler_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t		spooler_mutex =  PTHREAD_MUTEX_INITIALIZER;


/* queue of messages pending for delivery */
smtpmsg_t *			spool_msgs = NULL;

/* SMTP Relay information */
int	UseSMTPRelay = 0;
int	AVEnable = 0;
struct sockaddr_in RelayAddress;


extern mailbox_t *	MBX;
extern char *		server_fqdn;
extern char *		server_dir;




/* need this for sending requests to DNS to resolve MX records */
typedef struct {
	uint16_t	ID;
	uint16_t	Flags;
	uint16_t	QUEScount;
	uint16_t	ANSWcount;
	uint16_t	AUTHcount;
	uint16_t	ARECcount;
} dnshead_t;

struct sockaddr_in dns_srv;


#define	SMTP_GR	0x00303232 /* 220 greeting    */
#define	SMTP_OK	0x00303532 /* 250 status code */
#define SMTP_DT	0x00343533 /* 354 status code */































void* Spooler( void* arg )
{
	/* maybe some vars needed */
	smtpmsg_t *	msg;
	recipt_t *	rcpt;
	recipt_t *	crcpt;
	int			result;
	
	char *		destfn = (char*)malloc(NAME_MAX + 1);
	char *		netbuf = (char*)malloc(2048);
	
	int			mytid = pthread_self();
	
	int			AVfd = -1;
	
	if( AVEnable ){
		AVfd = open("/dev/Qav",O_RDWR);
		if( AVfd == -1 ){
			AVEnable = 0;
			qmail_log(QLL_ERROR,"SPOOLER(%i): no access to Qavird <%s>\n",mytid,qserr(errno));
		}
	}

	
	while(1){
		
		/* wait for a message to arrive for delivery */
		pthread_mutex_lock( &spooler_mutex );
		while(spool_msgs == NULL)
			pthread_cond_wait( &spooler_cond, &spooler_mutex );
			
		/* we aquired mutex and see that there are messages to deliver
		 * not to hold on SMTP server - we peek one message 
		 * and release mutex */
		
		msg = spool_msgs;
		spool_msgs = msg->next;
		pthread_mutex_unlock(&spooler_mutex);
		
		/* now we have a message and others may add new to spooler */
		
		/* Make checking for viruses via Qavird */
		if(AVEnable){
			int	retval;
			int ret;
			
			snprintf(netbuf,512,"%s/%s",server_dir,msg->spoolfn);
			ret = devctl( AVfd, QAVIRD_CMD_SCANFILE, netbuf, 512, &retval);
			if(ret != EOK){
				qmail_log(QLL_ERROR,"SPOOLER(%i): AV scan failed (%s) - mail passed\n",mytid,qserr(errno));
				AVEnable = 0;
			}else{
				/* parse the output of the Qavird */
				switch(retval){
					case QAV_CLEAN:
						/*qmail_log(QLL_INFO,"SPOOLER(%i): mail (%s) clean\n",mytid,netbuf);*/
						break;
					case QAV_INTERNAL:
						qmail_log(QLL_ERROR,"*! WARNING !* %s - mail passed\n",netbuf);
						break;
					case QAV_VIRUS:
						qmail_log(QLL_WARNING,"*! VIRUS !* <%s>\n",netbuf);
						/* move file to infected directory and block mail */
						snprintf(destfn,NAME_MAX,"infected/%i%i%i",mytid,time(NULL),rand());
						if(-1 == link(msg->spoolfn,destfn)){
							qmail_log(QLL_ERROR,"Infected file dropped: %s\n",qserr(errno));
						}else{
							qmail_log(QLL_INFO,"infected mail saved: %s\n",destfn);
						}
						/* release message */
						/* a) clear all the recipients */
						rcpt = msg->to;
						while(rcpt!=NULL){
							msg->to = rcpt->next;
							free(rcpt->to);
							free(rcpt);
							rcpt = msg->to;
						}
						/* b) free source */
						if(msg->from!=NULL) free(msg->from);
						/* close and delete spooler file */
						close(msg->spoolfd);
						if(msg->spoolfn!=NULL){
							remove(msg->spoolfn);
							free(msg->spoolfn);
						}
						free(msg);
						msg = NULL;
						break;
				} /* switch ... */
			} /* if ... else ... */
		}
		/* in case AV scanner blocked mail - we need to wait again */
		if(msg == NULL) continue;
		
		/* 
		 * Here comes message delivery.
		 * 
		 * The following terms apply to this process:
		 * 
		 * - local delivery takes precedence (mail is delivered to local mailboxes first)
		 * - we scan the list of recipients associated with the mail and group them by
		 *   mail domain, so we do not need to start data transfer for each target
		 *   in the same mail domain
		 * - references to recipients are deleted upon delivery.
		 * 
		 * 
		 * TO DO:
		 * - make server send back responces to the originator of mail,
		 *   explaining the reason of undelivery
		 * - make some antivirus checking (clamav)
		 */
		
		/* 1) LOCAL DELIVERY */
		rcpt = msg->to;
		crcpt = NULL;
		
		while(rcpt!=NULL){
			mailbox_t * mbxs = MBX;
			
			/* scan local mailboxes to see if can deliver locally */
			while(mbxs!=NULL){
				if(0==stricmp(rcpt->to,mbxs->user)) break;
				mbxs = mbxs->next;
			}
			
			if(mbxs != NULL){
				/* create the file name */
				sprintf(destfn,"%s/%i%i%i",mbxs->user,mytid,time(NULL),rand());
				
				/* lock the mailbox */
				pthread_mutex_lock(&mbxs->mtx);
				
				/* copy message to that mailbox */
				link(msg->spoolfn,destfn);
				
				/* unlock the mailbox */
				pthread_mutex_unlock(&mbxs->mtx);
				
				/* delete this recipient */
				msg->to = rcpt->next;
				free(rcpt->to);
				free(rcpt);
				rcpt = msg->to;
				
			}else{
				
				/* copy recipient to saparate list */
				msg->to = rcpt->next;
				rcpt->next = crcpt;
				crcpt = rcpt;
				rcpt = msg->to;
			}
		}
		
		
		
		
		
		
		
		
		
				
		/* 2) EXTERNAL DELIVERY */
		/* 
		 * crcpt  contains list of recipients.
		 * while it is not NULL we try to deliver mail
		 */
		while( crcpt != NULL){
			
			char *	rcpt_domain;			/* current recipient domain */
			int		sock;					/* this is the socket to be connected to relay */
			struct sockaddr_in direct;		/* address to connect to */
			struct sockaddr * Remote_Side;	/* useful pointer :) */
			recipt_t *	targets = NULL;		/* we'll store all MX-es here */
			
			/* debug */
			/*printf("SPOOLER(%i): External delivery\n",mytid);*/
			
			/* 1) step by step - collect recipients, who have the same domain.
			 *    In case we have the relay ON - we send mail to it. */
			 
			if( UseSMTPRelay==1 ){
				msg->to = crcpt;
				crcpt = NULL;
			}else{
				/* move the first recipient to the list on msg->to */
				msg->to = crcpt;
				crcpt = msg->to->next;
				msg->to->next = NULL;
				/* while there are still recipients on crcpt list
				 * scan them to see if there are ones with the same domain */
				rcpt_domain = strrchr(msg->to->to,'@');
				rcpt = NULL;
				while(crcpt!=NULL){
					recipt_t *	tmpr = crcpt;
					char *		cur_domain = strrchr(tmpr->to,'@');
					crcpt = tmpr->next;
					if(!stricmp(rcpt_domain,cur_domain)){
						/* we found one more target of the same domain */
						tmpr->next = msg->to;
						msg->to = tmpr;
					}else{
						/* this target has another domain */
						tmpr->next = rcpt;
						rcpt = tmpr;
					}
				}
				crcpt = rcpt;
			}
			
			
			/* 2) determine the host with SMTP relay for the mailbox:
			 * 		- connect to host directly 
			 *      - use DNS to resolve MX
			 *      - use specified relay
			 */
			if(UseSMTPRelay==0){
				/* We need to build the list of MXes */
				/* prepare DNS header */
				{
					dnshead_t * MXh = (dnshead_t*) netbuf;
					memset(MXh,0,sizeof(dnshead_t));
					MXh->ID = ENDIAN_RET16(2);
					MXh->Flags = 1;
					MXh->QUEScount = ENDIAN_RET16(1);
					
					result = sizeof(dnshead_t);
				}
				/* add MX question */
				memcpy(netbuf+result, rcpt_domain, strlen(rcpt_domain)+1);
				{
					int i = result + strlen(rcpt_domain)-1;
					unsigned char j = 0;
					result = i+2;
					while(netbuf[i] != '@'){
						if(netbuf[i] == '.'){
							netbuf[i] = j;
							j = 0;
						}else{
							j++;
						}
						i--;
					}
					netbuf[i] = j;
				}
				/* add type = 1 and class = 15 in !!! NETWORK BYTE ORDER !!! */
				*((uint32_t*)(netbuf+result)) = 0x01000f00;
				result += 4;
				
				/* ask DNS server if he knows the answer */
				sock=socket(AF_INET,SOCK_DGRAM,0);
				sendto(sock,netbuf,result,0,(struct sockaddr*)&dns_srv,sizeof(struct sockaddr));
				result = recv(sock,netbuf,512,0);
				if(result<0){
					/* there was no answer from DNS server */
					result = -1;
					goto bad_result;
				}
				close(sock); /* we do not need it for DNS */
				
				
				
				/* analize the answer */
				{
					uint16_t	ancount;
					dnshead_t *	MXh = (dnshead_t*) netbuf;
					
					ancount = ENDIAN_RET16(MXh->ANSWcount);
					if(ancount==0){
						/* in this case we try to connect to the host directly */
						recipt_t * newone = malloc(sizeof(recipt_t));
						newone->next = targets;
						newone->to = strdup(rcpt_domain+1);
						targets = newone;
						
					}else{
						char		rname[256];
						uint16_t	i;
						uint16_t	j;
						uint16_t	length;
						uint8_t		k;
						recipt_t *	newone;
						unsigned char * here = netbuf + sizeof(dnshead_t);
						
						/* skip the QUERY section: it can not be compressed - look for zero byte */
						while(*here) here++;
						here += 5; /* skip the zero byte and type+class */
												
						/* read all available answers */
						for(ancount;ancount>0;ancount--){
							
							/* skip the ZONE section in the answer */
							while(1){
								if((*here & 0xc0) == 0xc0){
									here += 2;
									break;
								}
								if(*here == 0x00){
									here++;
									break;
								}
								here++;
							}
							
							/* skip the start of the answer section */
							here += 8;
							length = ENDIAN_RET16(*(uint16_t*)here); here += 4;
							i = here - (unsigned char *)netbuf;
							here += (length-2);
							
							/* decode the answer */
							j = 0;
							do{
								if((netbuf[i] & 0xc0) == 0xc0){
									i = ( 0x3fff & ENDIAN_RET16(*(uint16_t*)(netbuf+i)) );
								}else{
									k = netbuf[i];
									if(k==0) break;
									i++;
									for(k;k>0;k--){
										rname[j] = netbuf[i];
										j++; i++;
									}
									rname[j] = '.'; j++;
								}
							}while(j<256);
							if(j) j--;
							rname[j] = 0;
							
							newone = malloc(sizeof(recipt_t));
							newone->next = targets;
							newone->to = strdup(rname);
							targets = newone;
							
							/* debug */
							/* printf("SPOOLER(%i): <%s> added to targets\n",mytid,targets->to); */
						}
					}
				}
			}
			
			
			
			while(1){
				
				/* use DNS to connect to SMTP for the domain */
				struct hostent rhost;
				struct hostent * rhost_p;
				
				if(UseSMTPRelay==0){
					
					/* if there are no more targets - give up */
					if(targets==NULL){
						qmail_log(QLL_INFO,"SPOOLER(%i): mail from <%s> not delivered\n",mytid,msg->from);
						result = -2;
						break;
					}
					/* resolve IP of the available MX */
					/* debug */
					/* printf("SPOOLER(%i): resolving IP for <%s>\n",mytid,targets->to); */
					
					rhost_p = gethostbyname_r(targets->to,&rhost,netbuf,2048,&result);
					if(rhost_p == NULL){
						recipt_t * oldone = targets;
						/* error resolving IP */
						qmail_log(QLL_WARNING,"SPOOLER(%i): Error resolving <%s>: %s\n",mytid,targets->to,qserr(result));
						targets = oldone->next;
						free(oldone->to);
						free(oldone);
						continue; /* go on to the next MX */
					}else{
						direct.sin_family = AF_INET;
						direct.sin_port = ENDIAN_RET16(25);
						direct.sin_addr = *((struct in_addr *)rhost.h_addr_list[0]);
						Remote_Side = (struct sockaddr*)&direct;
					}
					
				}else{
					Remote_Side = (struct sockaddr*)&RelayAddress;
				}
				
				
				/* now the IP seems to be available - create the socket */
				do{
					sock = socket(AF_INET, SOCK_STREAM, 0);
					if(sock == -1){
						qmail_log(QLL_ERROR,"socket: %s\n",qserr(errno));
						sleep(20);
					}
				}while(sock == -1);
				
				
				/* now try to connect to the peer */
				result = connect(sock,Remote_Side,sizeof(struct sockaddr_in));
				if(result == -1){
					/* error happened:
					 * - either we timedout or connection was refused
					 *   we go on to the next MX listed for the domain */
					if(UseSMTPRelay==0){
						if(errno==ECONNREFUSED){
							recipt_t * oldone = targets;
							targets = oldone->next;
							free(oldone->to);
							free(oldone);
						}else if((errno==ETIMEDOUT) && (targets->next!=NULL)){
							/* rotate targets if possible */
							recipt_t * tmpt = targets;
							
							/* debug */
							/* printf("SPOOLER(%i): rotating targets\n",mytid); */
							
							while(tmpt->next!=NULL) tmpt = tmpt->next;
							tmpt->next = targets;
							tmpt = tmpt->next;
							targets = tmpt->next;
							tmpt->next = NULL;
						}
					}
					qmail_log(QLL_ERROR,"connect: %s\n",qserr(errno));
					close(sock);
					if(UseSMTPRelay==1) sleep(20);
					continue;
				}else{
					/* handle socket options */
					struct timeval sto;
					int s = 1;
					sto.tv_sec = 20;
					sto.tv_usec = 0;
					setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &s, sizeof(int));
					setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &sto, sizeof(struct timeval) );
					break; /* go on with connected socket - send mail */
				}
			}
			
			
			/* 3) now we seem to be connected - try to send message */
			while(result >= 0){
				
				int num_valid_rcpt = 0;
				
				/* read greeting */
				/*result = recv(sock,netbuf,2047,0);*/
				result = ReadString(sock,netbuf,2047);
				if(result <= 0) continue; /* break the while loop */
				if( (*(uint32_t*)netbuf & 0xffffff) != SMTP_GR){
					qmail_log(QLL_WARNING,"SPOOLER(%i): Last action: READ GREETING\n",mytid);
					netbuf[result] = 0;
					result = -3;
					continue;
				}
				
				/* say EHLO to server */
				result = sprintf(netbuf,"EHLO %s\r\n",server_fqdn);
				send(sock,netbuf,result,0);
				/*result = recv(sock,netbuf,2047,0);*/
				result = ReadString(sock,netbuf,2047);
				if(result <= 0) continue; /* break the while loop */
				if( (*(uint32_t*)netbuf & 0xffffff) != SMTP_OK){
					/* some servers refuse EHLO mode. We are not proud - say helo */
					result = sprintf(netbuf,"HELO %s\r\n",server_fqdn);
					send(sock,netbuf,result,0);
					/*result = recv(sock,netbuf,2047,0);*/
					result = ReadString(sock,netbuf,2047);
					if(result <= 0) continue; /* break the while loop */
					if( (*(uint32_t*)netbuf & 0xffffff) != SMTP_OK){
						qmail_log(QLL_WARNING,"SPOOLER(%i): Last action: SENDING EHLO\n",mytid);
						netbuf[result] = 0;
						result = -3;
						continue;
					}
				}
								
				/* say MAIL FROM: */
				result = sprintf(netbuf,"MAIL FROM:<%s>\r\n",msg->from);
				send(sock,netbuf,result,0);
				/*result = recv(sock,netbuf,2047,0);*/
				result = ReadString(sock,netbuf,2047);
				if(result <= 0) continue; /* break the while loop */
				if( (*(uint32_t*)netbuf & 0xffffff) != SMTP_OK){
					qmail_log(QLL_WARNING,"SPOOLER(%i): Last action: SENDING MAIL FROM\n",mytid);
					netbuf[result] = 0;
					result = -3;
					continue;
				}
				
				/* send all RCPT TO: */
				rcpt = msg->to;
				while(rcpt!=NULL){
					/* send message to server */
					result = sprintf(netbuf,"RCPT TO:<%s>\r\n",rcpt->to);
					send(sock,netbuf,result,0);
					/* read answer */
					/*result = recv(sock,netbuf,2047,0);*/
					result = ReadString(sock,netbuf,2047);
					if(result <= 0) break;
					if( (*(uint32_t*)netbuf & 0xffffff) != SMTP_OK){
						/* not a critical thing - there may be other recipients 
						 * to get mail == just log the issue */
						netbuf[result] = 0;
						qmail_log(QLL_WARNING,"SPOOLER(%i): Recipient <%s> rejected: %s\n",mytid,rcpt->to,netbuf);
					}else{
						num_valid_rcpt++;
					}
					/* continue */
					rcpt = rcpt->next;
				}
				if(num_valid_rcpt == 0){
					qmail_log(QLL_ERROR,"SPOOLER(%i): Remote rejected all recipients\n",mytid);
					result = -2;
					continue;
				}
				
				/* send DATA */
				send(sock,"DATA\r\n",6,0);
				/*result = recv(sock,netbuf,2047,0);*/
				result = ReadString(sock,netbuf,2047);
				if(result <= 0) continue; /* break the while loop */
				if( (*(uint32_t*)netbuf & 0xffffff) != SMTP_DT){
					qmail_log(QLL_WARNING,"SPOOLER(%i): Last action: SENDING DATA\n",mytid);
					netbuf[result] = 0;
					result = -3;
					continue;
				}
				
				/* send body of the message */
				lseek(msg->spoolfd,0,SEEK_SET);
				while( 0 < (result = read(msg->spoolfd,netbuf,1400)) ){
					/* send a portion of file */
					result = send(sock,netbuf,result,0);
					if(result<0)break;
				}
				if(result<0) continue;
				/* send message terminator */
				send(sock,"\r\n.\r\n",5,0);
				/*result = recv(sock,netbuf,2047,0);*/
				result = ReadString(sock,netbuf,2047);
				if(result <= 0) continue; /* break the while loop */
				if( (*(uint32_t*)netbuf & 0xffffff) != SMTP_OK){
					qmail_log(QLL_WARNING,"SPOOLER(%i): Last action: SENDING body of message\n",mytid);
					netbuf[result] = 0;
					result = -3;
					continue;
				}
				
				result = 0; /* all OK */
				break;
			}
			
			/* if result < 0 - there was an error
			 * -1 and -2 errors are fatal => no firther data on the channel
			 * -3 and 0+ are cases when we should QUIT the server */
			bad_result:
			switch(result){
				case -1:
					qmail_log(QLL_ERROR,"Network: %s\n",qserr(errno));
					break;
				case -2:
					/* all errors logged - just skip this stage */
					break;
				case -3:
					qmail_log(QLL_WARNING,"SPOOLER(%i): Remote: %s\n",mytid,netbuf);
				default:
					/* say QUIT */
					send(sock,"QUIT\r\n",6,0);
			}
						
			/* 5) clear all the unneeded data */
			close(sock);
			/* debug */
			/*printf("SPOOLER(%d): socket closed\n",mytid);*/
			/* clear all the target MX peers */
			rcpt = targets;
			while(rcpt!=NULL){
				targets = rcpt->next;
				free(rcpt->to);
				free(rcpt);
				rcpt = targets;
			}
			/* clear all the recipients */
			rcpt = msg->to;
			while(rcpt!=NULL){
				msg->to = rcpt->next;
				free(rcpt->to);
				free(rcpt);
				rcpt = msg->to;
			}
			
			/* for now we processed mail. if there are still recipients on the
			 * crcpt list - we'll continue. otherwise the while() loop will break */
		}
		
		/* debug */
		/*printf("SPOOLER(%i): Mail processed\n",mytid); */
		
		/* release the message */
		if(msg->from!=NULL) free(msg->from);
				
		/* close and delete spooler file */
		close(msg->spoolfd);
		if(msg->spoolfn!=NULL){
			remove(msg->spoolfn);
			free(msg->spoolfn);
		}
		free(msg);
		
	} /* loop forever */
	
	/* normally we never get in here */
	return(0);

}
