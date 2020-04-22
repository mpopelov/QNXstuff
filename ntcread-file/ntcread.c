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


char 	dlbuf[2048];
int		dlbuf_siz;

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
	 * Now dump the _io_net_msg_dl_advert_t
	 */
	{
		int result = devctl(ntc_fd, NTC_DUMP_ADVERT, dlbuf, 2048, &dlbuf_siz );
		if( EOK != result ){
			printf("Unable to read dl_advert\n");
			close(ntc_fd);
			return 0;
		}
		printf("\n***\nThere are %d bytes sized dlbuf\n",dlbuf_siz);
		{
			int flags, retval;
			int	lfd = open("/advert.dbg",O_WRONLY|O_CREAT|O_TRUNC);
			perror("open");
			flags = fcntl( lfd, F_GETFL );
			perror("fcntl");
			flags |= O_DSYNC;
			retval = fcntl( lfd, F_SETFL, flags );
			perror("fcntl");
			retval = write(lfd,dlbuf,dlbuf_siz);
			perror("write");
			close(lfd);
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
