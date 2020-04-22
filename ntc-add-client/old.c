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



int main()
{
	int		ntc_fd, log_fd;
	int		size_read;
	int		npkts;
	pkt_info_t		packet[MAX_PKT_READ];





	int		isblocked = 0;	/* this will signal if we block something or not */

	/* this is MAC address of one of NIC's on my network
	 * I use it to test blocking.
	 * 
	 *  !!! REMEMBER !!!
	 * nfm-ntc.so is awaiting MAC in network byte order:
	 * higher bytes at lower addresses in memory.
	 * so you may see that real MAC of one of my NICs is
	 * 
	 *                 00-0a-5e-05-31-13
	 * 
	 * place in memory:	  0     1     2     3     4     5
	 * "weight" in MAC    5     4     3     2     1     0 */
	mac_addr_t mac = { 0x00, 0x0a, 0x5e, 0x05, 0x31, 0x13 };
	

	/* open logging device */
	if( !(ntc_fd = open("/dev/ntc", O_RDONLY)) ){
		printf("Can't open /dev/ntc for logging.\n");
		return (-1);
	}
	



	/* get info on interfaces */
	{
		if_info_t ifdesc[5];
		int num_ifs = 0;
		
		int result = devctl(ntc_fd, NTC_IFPRESENT, ifdesc, 5*sizeof(if_info_t), &num_ifs );
		if( EOK != result ){
			printf( "I was unable to read number of interfaces\n" );
			close(ntc_fd);
			return 0;
		}
		printf("there are %i interfaces present.\n", num_ifs);
		while(num_ifs >0){
			num_ifs--;
			printf("Interface: %s\n", ifdesc[num_ifs].ifname );
			printf("cell: %hu  endpoint: %hu  iface: %hu\n", ifdesc[num_ifs].cell, ifdesc[num_ifs].endpoint, ifdesc[num_ifs].iface );
			printf("Status: dead = %hhu, ofinterest = %hhu\n\n", ifdesc[num_ifs].dead, ifdesc[num_ifs].ofinterest );
		}
	}
	
	/* setting some interface to be of interest */
		/* get info on interfaces */
	{
		char ifname[IFNAMSIZ];
		int result;
		int rslt = 0;
		
		snprintf( ifname, IFNAMSIZ, "%s", "en0" );
		
		result = devctl(ntc_fd, NTC_ADDIF, ifname, IFNAMSIZ, &rslt );
		if( EOK != result ){
			printf( "I was unable to set the interface of interest\n" );
			close(ntc_fd);
			return 0;
		}
		if( rslt == 0 ){
			printf("No interface with such name...\n");
		}else{
			printf("Interface of interest set successfully...\n");
		}
	}
	
	/* do endless logging to screen */
	while(1){
		
		size_read = read(ntc_fd, packet, MAX_PKT_READ*sizeof(pkt_info_t));
		if(size_read == -1){
			printf("device lost\n");
			close(ntc_fd);
			return 0;
		}
		
		/* if we have read 0 bytes - the queue is empty
		 * and we need to wait a little
		 */
		if(size_read == 0){
			int somenumber;
			blkinfo_t blocked;
			int	result = 0;
			
			/* get statistics of working filter and fall into a sleep
			 * for some time
			 */
			result = devctl( ntc_fd, NTC_NUMMISSED, &somenumber, sizeof(int), NULL );
			if(EOK != result){
				printf("We've lost device!\n");
				close(ntc_fd);
				return 0;
			}else{
				printf("We missed %i packets during operation.\n", somenumber );
			}
			
			/* now block packets from test NIC */
			result = devctl( ntc_fd, NTC_ADDBLOCK, &mac, sizeof(mac_addr_t), NULL );
			if(EOK != result){
				printf("We've lost device!\n");
				close(ntc_fd);
				return 0;
			}
			printf("We are now blocking some network node. Press <Enter> to unblock & continue.\n");
			
			sleep(10); /* wait for user input to see result of blocking */
			
			result = devctl( ntc_fd, NTC_DELBLOCK, &mac, sizeof(mac_addr_t), NULL );
			if(EOK != result){
				printf("We've lost device!\n");
				close(ntc_fd);
				return 0;
			}
			printf("Now we remove blocking of some network node.\n");

			result = devctl( ntc_fd, NTC_NUMBLOCKED, &blocked, sizeof(blkinfo_t), NULL );
			if(EOK != result){
				printf("We've lost device!\n");
				close(ntc_fd);
				return 0;
			}else{
				printf("Total %i outgoing and %i incoming packets were blocked.\n", blocked.out_pkt, blocked.in_pkt );
			}
			
			sleep(20); /* we sleep so long so there are missing packets on the net */
		
		}else{
		
		int i = 0;
		npkts = size_read/sizeof(pkt_info_t);
		for(i=0; i<npkts; i++){
			/* show MAC and EtherType */
			{
				uint32_t m_l = ntohl( *((uint32_t *)packet[i].DLC.mac_src) );
				uint16_t m_s = ntohs( *((uint16_t *)&packet[i].DLC.mac_src[4]) );
				printf("from %08x%04hx ", m_l, m_s );
				m_l = ntohl( *((uint32_t *)packet[i].DLC.mac_dst) );
				m_s = ntohs( *((uint16_t *)&packet[i].DLC.mac_dst[4]) );
				printf("to %08x%04hx ", m_l, m_s );
			
				m_s = ntohs( packet[i].DLC.EtherType );
				
				/* show frame type */
				if(m_s == 0x800){
					printf("frame type IP\n");
					printf("%s -> ", inet_ntoa( packet[i].IP.ip_src ) );
					printf("%s\n", inet_ntoa( packet[i].IP.ip_dst ) );
					printf("total %i bytes\n",packet[i].len);
					
					/* show protocol type and info */
					if(packet[i].IP.proto == 1){
						printf("Protocol: ICMP\n");
					}else if(packet[i].IP.proto == 6){
						/* show TCP ports */
						printf("TCP ports: %hu -> %hu\n", ntohs(packet[i].app.TCP.src_port), ntohs(packet[i].app.TCP.dst_port));
					}else if(packet[i].IP.proto == 17){
						/* show UDP ports*/
						printf("UDP ports: %hu -> %hu\n", ntohs(packet[i].app.UDP.src_port), ntohs(packet[i].app.UDP.dst_port));
					}else{
						printf("Protocol: %hhu\n", packet[i].IP.proto);
					}
					
					}else{
					printf("frame type %04hx\n", m_s );
					}
			}
			
			/**/
		}
		}
	}
	
	close(ntc_fd);
	return 0;


}
