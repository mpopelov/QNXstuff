/*
 * NFM-NTC.SO reader by -=MikeP=-
 * Copyright (C) 2003, 2004  Mike Anatoly Popelov ( AKA -=MikeP=- )
 * 
 * -=MikeP=- <mikep@kaluga.org>
 * 
 * You are free to use this code in the way you like and for any purpose,
 * not deprecated by your local law. I will have now claim for using this code
 * in any project, provided that the reference to me as the author provided.
 * I also DO NOT TAKE ANY RESPONSIBILITY if this code does not suite you in your
 * particular case. Though this code was written to be stable and working,
 * I will take NO RESPONSIBILITY if it does not work for you, on your particular
 * equipment, in your particular configuration and environment, if it brings any
 * kind of damage to your equipment.
 */

/*
 * This program is part of nfm-ntc.so
 * Its purpose is to show the results of filter work
 * and demonstrate how one can use devctl()
 * to manipulate some aspects of filter job
 * and get its statistics
 */
 
#include	"ntc_common.h"
#include	"logging.h"
#include	<fcntl.h>

pthread_mutex_t	log_mutex = PTHREAD_MUTEX_INITIALIZER;
clnt_t * clients = NULL;


/* this will be our logging thread */
static void *log_process(void *arg);






int main()
{
	int		ntc_fd;
	int		size_read;
	int		npkts;
	pkt_info_t		packet[MAX_PKT_READ];

	/* 
	 * open logging device
	 */
	if( -1 == (ntc_fd = open("/dev/ntc", O_RDONLY)) ){
		printf("Can't open /dev/ntc for logging.\n");
		return (-1);
	}
	
	/* 
	 * get info on interfaces 
	 */
	{
		if_info_t ifdesc[5];
		int num_ifs = 0;
		
		int result = devctl(ntc_fd, NTC_IFPRESENT, ifdesc, 5*sizeof(if_info_t), &num_ifs );
		if( EOK != result ){
			printf( "Unable to read number of interfaces\n" );
			close(ntc_fd);
			return 0;
		}
		printf("there is(are) %i interface(s) known to nfm-ntc.\n", num_ifs);
		while(num_ifs >0){
			num_ifs--;
			printf("Interface: %s\n", ifdesc[num_ifs].ifname );
			printf("cell: %hu  endpoint: %hu  iface: %hu\n", ifdesc[num_ifs].cell, ifdesc[num_ifs].endpoint, ifdesc[num_ifs].iface );
			printf("Status: dead = %hhu, ofinterest = %hhu\n\n", ifdesc[num_ifs].dead, ifdesc[num_ifs].ofinterest );
		}
	}
	

	/* 
	 * setting some interface to be of interest
	 */
	{
		char ifname[IFNAMSIZ];
		int result;
		int rslt = 0;
		
		snprintf( ifname, IFNAMSIZ, "%s", "en0" ); /* here we decided we would be interested in en0 */
		
		result = devctl(ntc_fd, NTC_ADDIF, ifname, IFNAMSIZ, &rslt );
		if( EOK != result ){
			printf( "Unable to set the interface of interest.\n" );
			close(ntc_fd);
			return 0;
		}
		if( rslt == 0 ){
			printf("No interface %s known to nfm-ntc.\n");
		}else{
			printf("Interface of interest set to %s successfully.\n", ifname);
		}
	}
	
	
	
	/* here we should create logging thread */
	if(EOK != pthread_create(NULL, NULL, log_process, NULL)){
		printf("Can't launch dumping thread. Exiting.\n");
		exit(0);
	}
	
	
	/*
	 * Now we do endless logging to file - we collect info and
	 * dump it to file each 5 minutes.
	 * meanwhile we do endless logging from nfm-ntc;
	 */
	while(1){
		
		
		size_read = read(ntc_fd, packet, MAX_PKT_READ*sizeof(pkt_info_t));
		if(size_read == -1){
			printf("device lost\n");
			exit(0);
		}
		
		/* if we have read 0 bytes - the queue is empty
		 * and we need to wait a little
		 */
		if(size_read == 0){
			int somenumber;
			blkinfo_t blocked;
			int	result = 0;
			
			/* get statistics of working filter */
			result = devctl( ntc_fd, NTC_NUMMISSED, &somenumber, sizeof(int), NULL );
			if(EOK != result){
				printf("We've lost device!\n");
				exit(0);
			}else{
				printf("We missed %i packets during operation.\n", somenumber );
			}
			result = devctl( ntc_fd, NTC_NUMBLOCKED, &blocked, sizeof(blkinfo_t), NULL );
			if(EOK != result){
				printf("We've lost device!\n");
				exit(0);
			}else{
				printf("Total %i outgoing and %i incoming packets were blocked.\n", blocked.out_pkt, blocked.in_pkt );
			}
			sleep(5); /* how long to sleep depends on network activity we expect: 5 sec to be OK in most cases */
		}else{
			
			/* we have read more than 0 bytees and our queue is filled
			 * with some useful data - add it to the logger */
			 clnt_t * cur_cln;
			 sess_t * cur_sess;
			 mac_cmp_t * cli_mac;
			
			/* how many descriptors are there */
			int i = 0;
			npkts = size_read/sizeof(pkt_info_t);
			
			/* now for each packet we find the appropriate session and add packet there */
			for(i=0; i<npkts; i++){
				/* tune in some useful variables */
				if( packet[i].dir == PKT_DIR_IN ){
					cli_mac = (mac_cmp_t *)&(packet[i].DLC.mac_src);
				}else{
					cli_mac = (mac_cmp_t *)&(packet[i].DLC.mac_dst);
				}
				
				/* lock the log so dumper thread could not modify it */
				pthread_mutex_lock( &log_mutex );
				
				/* find the appropriate client descriptor */
				cur_cln = clients;
				while( cur_cln != NULL ){
					if(cli_mac->NID == cur_cln->MAC.NID){
						if(cli_mac->VID == cur_cln->MAC.VID) break;
					}
					cur_cln = cur_cln->next;
				}
				
				/* if we haven't found one - we simply add him to the list */
				if( cur_cln == NULL ){
					cur_cln = (clnt_t*) malloc( sizeof(clnt_t) );
					memcpy( &(cur_cln->MAC), cli_mac, MAC_ADDR_LENGTH );
					cur_cln->sessions = NULL;
					cur_cln->next = clients;
					clients = cur_cln;
				}
				
				/* 
				 * find the appropriaate session descriptor 
				 */
				cur_sess = cur_cln->sessions;
				while( cur_sess != NULL ){
					
					/* first - check the frame type */
					if( (cur_sess->frame_type == packet[i].DLC.EtherType) ){
						
						/* if this is not the ip family frame - we simply add traffic */
						if( cur_sess->frame_type != 0x0008 ){
							break;
						}else{
							/* this is IP family  packet - find if this is the right session */
							
							/* compare IP protocol type first - it is easier than comparing 2 IPs */
							if( cur_sess->Proto == packet[i].IP.proto ){
								struct in_addr clnIP, dstIP;
								if(packet[i].dir == PKT_DIR_IN){
									clnIP = packet[i].IP.ip_src;
									dstIP = packet[i].IP.ip_dst;
								}else{
									clnIP = packet[i].IP.ip_dst;
									dstIP = packet[i].IP.ip_src;
								}
								
								/* compare IP addresses */
								if( (cur_sess->clnIP.s_addr == clnIP.s_addr) && (cur_sess->dstIP.s_addr == dstIP.s_addr) ){
									uint16_t cli_port, rem_port;
									if(packet[i].dir == PKT_DIR_IN){
										cli_port = packet[i].app.TCP.src_port;
										rem_port = packet[i].app.TCP.dst_port;
									}else{
										cli_port = packet[i].app.TCP.dst_port;
										rem_port = packet[i].app.TCP.src_port;
									}
									
									/* if this is not TCP or UDP - it is portless protocol
									 * we should check it and exit */
									if( (cur_sess->Proto != 6) && (cur_sess->Proto != 17) ) break;
									/* finally compare ports */
									if( (cur_sess->cli_port == cli_port) && (cur_sess->rem_port == rem_port) ) break;
								}
							}
						}
					}
					
					cur_sess = cur_sess->next;
				}
				
				/* 
				 * if there is no such session - we simply add new 
				 */
				if( cur_sess == NULL ){
					cur_sess = (sess_t*) malloc( sizeof(sess_t) );
					
					/* set frame type */
					cur_sess->frame_type = packet[i].DLC.EtherType;
					
					/* set bytes counters */
					if(packet[i].dir == PKT_DIR_IN){
						cur_sess->bytes_snd = packet[i].len;
						cur_sess->bytes_rcv = 0;
					}else{
						cur_sess->bytes_rcv = packet[i].len;
						cur_sess->bytes_snd = 0;
					}
					
					/* set protocols and ports */
					if(cur_sess->frame_type == 0x0008){
						/* this means the IP class frame - add addresses and so on */
						
						/* add addresses */
						if(packet[i].dir == PKT_DIR_IN){
							cur_sess->clnIP = packet[i].IP.ip_src;
							cur_sess->dstIP = packet[i].IP.ip_dst;
						}else{
							cur_sess->clnIP = packet[i].IP.ip_dst;
							cur_sess->dstIP = packet[i].IP.ip_src;
						}
						
						/* add protocol type */
						cur_sess->Proto = packet[i].IP.proto;
						
						/*if((cur_sess->Proto == TCP) || (cur_sess->Proto == UDP))*/
						if((cur_sess->Proto == 6) || (cur_sess->Proto == 17)){
							if(packet[i].dir == PKT_DIR_IN){
								cur_sess->cli_port = packet[i].app.TCP.src_port;
								cur_sess->rem_port = packet[i].app.TCP.dst_port;
							}else{
								cur_sess->cli_port = packet[i].app.TCP.dst_port;
								cur_sess->rem_port = packet[i].app.TCP.src_port;
							}
						}else{
							cur_sess->cli_port = 0;
							cur_sess->rem_port = 0;
						}
												
					}else{
						/* we can say nothing about ports and nothing about proto
						 * set everything to zero */
						 cur_sess->clnIP.s_addr = 0;
						 cur_sess->dstIP.s_addr = 0;
						 cur_sess->Proto = 0;
						 cur_sess->cli_port = 0;
						 cur_sess->rem_port = 0;
					}
					
					cur_sess->next = cur_cln->sessions;
					cur_cln->sessions = cur_sess;
				}else{
					if(packet[i].dir == PKT_DIR_IN){
						cur_sess->bytes_snd += packet[i].len;
					}else{
						cur_sess->bytes_rcv += packet[i].len;
					}
				}
				
				/* now we are free to unlock it before the next record so
				 * the dumping thread is not waiting too long
				 */
				pthread_mutex_unlock( &log_mutex );
			}
		}
	}




} /* int main() */











static void *log_process(void *arg){
	
	FILE *	log_fd;
	clnt_t * cli_list;
	clnt_t * del_cli;
	
	sess_t * cli_sess;
	sess_t * del_sess;
	

	/* 
	 * open text file to write log to
	 */
	if( NULL == (log_fd = fopen("session.log", "w")) ){
		printf("Can't open file to write log to.\n");
		exit(0);
	}
	
	while(1){
		sleep( 1*60 ); /* sleep 5 minutes and then write something to log */
		
		/* just for debug */
		printf("Writing to log...\n");
		
		/* get the list and say real logger there is no one and don't stop the work of main thread */
		pthread_mutex_lock( &log_mutex );
		cli_list = clients;
		clients = NULL;
		pthread_mutex_unlock( &log_mutex );
		
		/* add time at which dumping occured */
		{
			char	buf[26];
			struct timespec timestamp;
			clock_gettime(CLOCK_REALTIME ,&timestamp);
			ctime_r( &timestamp.tv_sec, buf );
			fprintf(log_fd,"***\nLogging on: %s***\n\n",buf);
		}
		
		/*now we should one by one log all clients and all their sessions to file */
		while(cli_list != NULL){
			/* print the MAC of the client */
			fprintf(log_fd, "Client MAC: %08x%04hx\n", ntohl(cli_list->MAC.NID), ntohs(cli_list->MAC.VID) );
			
			
			/* 
			 * here we process all the sessions during dumping period 
			 */
			cli_sess = cli_list->sessions;
			while( cli_sess != NULL ){
				char * ipproto;
				if( cli_sess->Proto == 1 ) ipproto = "ICMP";
				if( cli_sess->Proto == 6 ) ipproto = "TCP";
				if( cli_sess->Proto == 17 ) ipproto = "UDP";
				if( cli_sess->frame_type != 0x008 ) ipproto = "Unknown";
				fprintf(log_fd,"Frame type 0x%04hx IPproto: %s session: %s:%hu <-- %u -/- %u --> ", ntohs(cli_sess->frame_type), ipproto, inet_ntoa(cli_sess->clnIP), ntohs(cli_sess->cli_port), cli_sess->bytes_rcv, cli_sess->bytes_snd);
				fprintf(log_fd,"%s:%hu\n", inet_ntoa(cli_sess->dstIP), ntohs(cli_sess->rem_port));
				
				/* here we release session info and move to the next session */
				del_sess = cli_sess;
				cli_sess = cli_sess->next;
				free(del_sess);
			}
			
			/* here we release the client info and move on */
			fprintf(log_fd,"\n");
			del_cli = cli_list;
			cli_list = del_cli->next;
			free(del_cli);
		}
		
		fflush( log_fd );
	}
	
	return NULL;
}















/* EOF */
