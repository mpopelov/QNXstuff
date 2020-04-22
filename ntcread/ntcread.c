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
#include	"qbase_client.h"
#include	<fcntl.h>

clnt_t * clients = NULL;






/* required to manage accounts */
account_t *			accounts = NULL;
QB_connid_t			acnt_con;
Qbase_table_desc_t*	AcntTable = NULL;
uint32_t			acntRowSize;
char *				acntRow;
uint32_t			nAccounts;

/* required to manage NICs */
nic_t *				nics = NULL;
QB_connid_t			nic_con;
Qbase_table_desc_t*	NicTable	= NULL;
uint32_t			nicRowSize;
char *				nicRow;
uint32_t			nNics;

/* required to log everything to database */
QB_connid_t			log_con;
Qbase_table_desc_t*	LogTable	= NULL;
uint32_t			logRowSize;
char *				logRow;
uint32_t			nNics;









int main()
{
	int			ntc_fd;
	int			size_read;
	int			npkts;
	pkt_info_t	packet[MAX_PKT_READ];
	QB_result_t	result;
	int			ItIsTimeToLog = 0;


	printf("Starting Up...\n");

	/* Open connections to database that we'll use for reading and updating */
	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &acnt_con)) ){
		printf("ACCOUNTS: %s\n", QBERROR[result]);
		exit(-1);
	}
	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &nic_con)) ){
		printf("NICS: %s\n", QBERROR[result]);
		exit(-1);
	}
	if( QBASE_E_OK != (result = Qbase_connect( QBASE_RM_PATH, &log_con)) ){
		printf("LOGGER: %s\n", QBERROR[result]);
		exit(-1);
	}
	
	printf("Connections to database (Qbase) opened successfully\n");	
	
	/* Select constantly-named databases to work with */
	if( QBASE_E_OK != (result = Qbase_table_select( acnt_con, "ACCOUNTS" ))){
		printf("ACCOUNTS table inaccessible: %s\n", QBERROR[result]);
		exit(-1);
	}
	if( QBASE_E_OK != (result = Qbase_table_select( nic_con, "NICS" ))){
		printf("NICSS table inaccessible: %s\n", QBERROR[result]);
		exit(-1);
	}
	
	printf("Tables selected successfully\n");
	
	/* Get table descriptors */
	if( QBASE_E_OK != (result = Qbase_table_caps( acnt_con, &AcntTable ))){
		printf("Table ACCOUNTS: %s\n", QBERROR[result]);
		exit(-1);
	}
	if( QBASE_E_OK != (result = Qbase_table_caps( nic_con, &NicTable ))){
		printf("Table NICS: %s\n", QBERROR[result]);
		exit(-1);
	}
	
	printf("Table descriptors retrieved successfully\n");
	
	
	
	/* read all ACCOUNTS into memory to update money nformation */
	acntRow = Qbase_row_newbuffer( AcntTable, &acntRowSize );
	if( acntRow == NULL ){
		printf("ACCOUNTS: Unable to allocate memory for a new row.\n");
		exit(-1);
	}
	nAccounts = 0;
	Qbase_fetchrst( acnt_con );
	while( QBASE_E_OK == (result = Qbase_fetch( acnt_con , acntRow, acntRowSize )) ){
		/* add new record to list of known accounts */
		account_t * CAccount;
		if(NULL == (CAccount = (account_t*)malloc(sizeof(account_t)))){
			printf("Memory allocation fault\n");
			exit(-1);
		}
		
		/* copy data we have read to newly allocated structure */
		memcpy( &CAccount->acnt, acntRow, acntRowSize );
		
		/* add new account record on top of account stack */
		CAccount->next = accounts;
		accounts = CAccount;
		
		printf("Account UIN: %u\n", CAccount->acnt.UIN);
		
		nAccounts++;
	}
	if( result != QBASE_E_LASTROW ){
		printf("Error retrieving acconts: %s\n", QBERROR[result]);
		exit(-1);
	}
	printf("Read %u actual accounts from database\n", nAccounts);
	
	
	
	/* read info about all registered NICs */
	nicRow = Qbase_row_newbuffer( NicTable, &nicRowSize );
	if( nicRow == NULL ){
		printf("NICS: Unable to allocate memory for a new row.\n");
		exit(-1);
	}
	nNics = 0;
	Qbase_fetchrst( nic_con );
	while( QBASE_E_OK == (result = Qbase_fetch( nic_con , nicRow, nicRowSize )) ){
		nic_t * CNic;
		account_t * CAccount;
		
		/* add new record to list of known accounts */
		if(NULL == (CNic = (nic_t*)malloc(sizeof(nic_t)))){
			printf("Memory allocation fault\n");
			exit(-1);
		}
		
		/* copy data we hav read to newly allocated structure */
		memcpy( &CNic->NCard, nicRow, nicRowSize );
		
		{
			int i;
			char * nbuff = (char*)(&(CNic->NCard.accnt));
			for(i=0; i<4; i++)
				printf("%hhx ", nbuff[i]);
			printf("\n\n");
			printf("the size in memory: %i \n",sizeof(nic_data_t));
		}
		
		printf("NIC UIN = %u\n", CNic->NCard.UIN);
		printf("NIC closed = %u\n", CNic->NCard.closed);
		printf("NIC account = %u\n",CNic->NCard.accnt);
		
		/* bind selected NIC to appropriate account */
		CAccount = accounts;
		while( CAccount != NULL ){
			if( CAccount->acnt.UIN == CNic->NCard.accnt ) break;
			CAccount = CAccount->next;
		}
		
		if( CAccount == NULL ){
			printf("Suspicious NIC with account %u\n", CNic->NCard.accnt);
			printf("Error - there is unbound NIC - exiting\n");
			exit(-1);
		}
		CNic->acnt_p = CAccount;
		CNic->LOG = NULL;
				
		/* add new account record on top of account stack */
		CNic->next = nics;
		nics = CNic;
		
		nNics++;
	}
	if( result != QBASE_E_LASTROW ){
		printf("Error retrieving data on NICs: %s\n", QBERROR[result]);
		exit(-1);
	}

	printf("Read %u actual NIC datasheets from database\n", nNics);
	
	
	
/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Get interface to logging device (nfm-ntc),
 * add interface of interest, register allowed nicks,
 * start thread that will write to database
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	
	
	 
	if( -1 == (ntc_fd = open("/dev/ntc", O_RDONLY)) ){
		printf("Can't open /dev/ntc for logging.\n");
		exit(-1);
	}
	
	/* get info on interfaces */
	{
		if_info_t ifdesc[5];
		int num_ifs = 0;
		
		int result = devctl(ntc_fd, NTC_IFPRESENT, ifdesc, 5*sizeof(if_info_t), &num_ifs );
		if( EOK != result ){
			printf( "Unable to read number of interfaces\n" );
			exit(-1);
		}
		printf("there is(are) %i interface(s) known to nfm-ntc.\n\n", num_ifs);
		while(num_ifs >0){
			num_ifs--;
			printf("Interface: %s\n", ifdesc[num_ifs].ifname );
			printf("cell: %hu  endpoint: %hu  iface: %hu\n", ifdesc[num_ifs].cell, ifdesc[num_ifs].endpoint, ifdesc[num_ifs].iface );
			printf("Status: dead = %hhu, ofinterest = %hhu\n\n", ifdesc[num_ifs].dead, ifdesc[num_ifs].ofinterest );
		}
	}
	
	/* setting some interface to be of interest */
	{
		char ifname[IFNAMSIZ];
		int result;
		int rslt = 0;
		
		snprintf( ifname, IFNAMSIZ, "%s", "en0" ); /* here we decided we would be interested in en0 */
		
		result = devctl(ntc_fd, NTC_ADDIF, ifname, IFNAMSIZ, &rslt );
		if( EOK != result ){
			printf( "Unable to set the interface of interest.\n" );
			exit(-1);
		}
		if( rslt == 0 ){
			printf("No interface %s known to nfm-ntc.\n");
		}else{
			printf("Interface of interest set to %s successfully.\n\n", ifname);
		}
	}
	
	/* allow transport to the NICs as needed */
	{
		nic_t * CNic = nics;
		while( CNic != NULL ){
			if(CNic->NCard.closed == 0){
				if( EOK != devctl( ntc_fd, NTC_ADDBLOCK, &(CNic->NCard.MAC), sizeof(mac_addr_t), NULL ) ){
					printf("We've lost device!\n");
					exit(-1);
				}
				printf("---!!!--- Network client allowed to communicate\n");
			}
			CNic = CNic->next;
		}
	}
	



	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	 * 
	 * Here the endless logging loop starts
	 * 
	 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	
	while(1){
		
		/* step 1: read all the data available from filter */
		
		size_read = read(ntc_fd, packet, MAX_PKT_READ*sizeof(pkt_info_t));
		if(size_read == -1){
			printf("Error: logging device lost. Exiting.\n");
			exit(0);
		}
		
		/* step2: if there is data available - add everything to queue
		 *        and then to database; if not - sleep some seconds */
		
		if( size_read == 0 ){
			printf("No data currently available - yelding.\n");
			sleep(5);
			continue;
		
		}else{
			
			/* add logged data to database and along with it - calculate
			 * how much money we need to substract */
			nic_t * CNic;
			logdet_t * cur_sess;
			mac_cmp_t * cli_mac;
			int i;
			
			
			
			npkts = size_read/sizeof(pkt_info_t);
			
			for( i=0; i<npkts; i++ ){
				
				if( packet[i].dir == PKT_DIR_IN ){
					cli_mac = (mac_cmp_t *)&(packet[i].DLC.mac_src);
				}else{
					cli_mac = (mac_cmp_t *)&(packet[i].DLC.mac_dst);
				}
				
				/* find client, whose paket is this */
				CNic = nics;
				while( CNic != NULL ){
					mac_cmp_t * NMAC = (mac_cmp_t*)(&(CNic->NCard.MAC));
					if(cli_mac->NID == NMAC->NID){
						if(cli_mac->VID == NMAC->VID) break;
					}
					CNic = CNic->next;
				}
				
				/* if no corresponding Nic found - !!! ALERT - IT SHOULD NOT HAPPEN !!! */
				if(CNic == NULL){
					printf("!!!ALERT!!! frame from unknown MAC address passed !!!");
					exit(-1);
				}
				
				/* find appropriate session */
				cur_sess = CNic->LOG;
				while( cur_sess != NULL ){
					/* check the frame type */
					if( cur_sess->dtls.layer == packet[i].DLC.EtherType ){
						/* if this is not the IPv4 family traffic 
						 * we simply add byte count */
						if( cur_sess->dtls.layer != 0x0008 ){
							break;
						}else{
							/* compare IP protocol types */
							if( cur_sess->dtls.proto == packet[i].IP.proto ){
								/* compare IP addresses */
								struct in_addr clnIP, dstIP;
								if(packet[i].dir == PKT_DIR_IN){
									clnIP = packet[i].IP.ip_src;
									dstIP = packet[i].IP.ip_dst;
								}else{
									clnIP = packet[i].IP.ip_dst;
									dstIP = packet[i].IP.ip_src;
								}
								if( (cur_sess->dtls.IPcln.s_addr == clnIP.s_addr) && (cur_sess->dtls.IPdst.s_addr == dstIP.s_addr) ){
									uint16_t cli_port, rem_port;
									/* if this is not TCP or UDP - it is portless protocol
									 * we should check it and exit */
									if( (cur_sess->dtls.proto != 6) && (cur_sess->dtls.proto != 17) ) break;
									if(packet[i].dir == PKT_DIR_IN){
										cli_port = packet[i].app.TCP.src_port;
										rem_port = packet[i].app.TCP.dst_port;
									}else{
										cli_port = packet[i].app.TCP.dst_port;
										rem_port = packet[i].app.TCP.src_port;
									}
									/* finally compare ports */
									if( (cur_sess->dtls.PortCli == cli_port) && (cur_sess->dtls.PortDst == rem_port) ) break;
								}
							}
						}
					}
					cur_sess = cur_sess->next;
				}
				
				/* add appropriate values to the session or add a new session */
				
				if( cur_sess == NULL ){
					/* add a new session and fill it with initial values */
					cur_sess = (logdet_t*)malloc(sizeof(logdet_t));
					memset( cur_sess, 0, sizeof(logdet_t) );
					
					/* fill the initial values for session */
					/* cur_sess->dtls.UIN will be set automatically by database */
					cur_sess->dtls.NIC = CNic->NCard.UIN;
					
					cur_sess->dtls.layer = packet[i].DLC.EtherType;
					if( cur_sess->dtls.layer == 0x0008 ){
						
						/* this is IPv4 frame - set the IP addresses */
						if(packet[i].dir == PKT_DIR_IN){
							cur_sess->dtls.IPcln = packet[i].IP.ip_src;
							cur_sess->dtls.IPdst = packet[i].IP.ip_dst;
						}else{
							cur_sess->dtls.IPcln = packet[i].IP.ip_dst;
							cur_sess->dtls.IPdst = packet[i].IP.ip_src;
						}
						
						/* add protocol of IP family */
						cur_sess->dtls.proto = packet[i].IP.proto;
						
						/* add ports, if appliable */
						/*if((cur_sess->Proto == TCP) || (cur_sess->Proto == UDP))*/
						if((cur_sess->dtls.proto == 6) || (cur_sess->dtls.proto == 17)){
							if(packet[i].dir == PKT_DIR_IN){
								cur_sess->dtls.PortCli = packet[i].app.TCP.src_port;
								cur_sess->dtls.PortDst = packet[i].app.TCP.dst_port;
							}else{
								cur_sess->dtls.PortCli = packet[i].app.TCP.dst_port;
								cur_sess->dtls.PortDst = packet[i].app.TCP.src_port;
							}
						}
					}
					
					/* add session on top of session list for current MAC */
					cur_sess->next = CNic->LOG;
					CNic->LOG = cur_sess;
				}
				
				/* add ammount of bytes sent/recieved to current session */
				if(packet[i].dir == PKT_DIR_IN){
					cur_sess->dtls.BytesFromCli += packet[i].len;
				}else{
					cur_sess->dtls.BytesToCli += packet[i].len;
				}
				
			} /* end _for_ */
			
			
			
			printf("there is some data logged pending to be saved to database\n");
		
		} /* end _else_ */
		
		
		/* step 3: getting device statistiics */
		{
			int somenumber;
			blkinfo_t blocked;
			int	result = 0;
			
			/* get statistics of working filter */
			if( EOK != devctl( ntc_fd, NTC_NUMMISSED, &somenumber, sizeof(int), NULL )){
				printf("We've lost device!\n");
				exit(0);
			}
			if( EOK != devctl( ntc_fd, NTC_NUMBLOCKED, &blocked, sizeof(blkinfo_t), NULL )){
				printf("We've lost device!\n");
				exit(0);
			}
			
			printf("*** Packets: %i missed | %i blocked out | %i blocked in\n", somenumber, blocked.out_pkt, blocked.in_pkt );
		}
		
		
		
		/* step 4: log everything to database and refresh information about
		 * accounts and network interfaces */
		ItIsTimeToLog++;
		
		/* save log each 5*12 seconds to database */
		if( ItIsTimeToLog == 3 ){
			
			struct timespec	logtime;
			char			tblname[QBASE_TAB_NAMELEN];
			nic_t *			CNic;
			logdet_t * 		CLog;
			QB_result_t		result;


			/* get current time and select the table
			 * a new table is created every day */
			printf("LOG_>  opening table to log to\n");
			clock_gettime( CLOCK_REALTIME, &logtime );
			sprintf( tblname, "%u", logtime.tv_sec/86400 ); /* the table name is the day since 1970-01-01 */
			
			result = Qbase_table_select( log_con, tblname );
			if( result != QBASE_E_OK ){
				if( result != QBASE_E_NOTBLEXIST ){
					printf("LOG_> Error in connection to LogTable. Exiting\n");
					exit(-1);
				}
				
				printf("LOG_> time to create new table\n");
				
				LogTable = Qbase_new_tbldesc( 12, 100, 3, tblname );
				if( LogTable == NULL ){
					printf("LOG_> Can't allocate new table descriptor.\n");
					exit(-1);
				}
				Qbase_set_field_metrics( LogTable, 1, QBASE_TYPE_UINT, 0 );
				Qbase_set_field_metrics( LogTable, 2, QBASE_TYPE_UINT16, 0 );
				Qbase_set_field_metrics( LogTable, 3, QBASE_TYPE_UINT16, 0 );
				Qbase_set_field_metrics( LogTable, 4, QBASE_TYPE_UINT, 0 );
				Qbase_set_field_metrics( LogTable, 5, QBASE_TYPE_UINT, 0 );
				Qbase_set_field_metrics( LogTable, 6, QBASE_TYPE_UINT16, 0 );
				Qbase_set_field_metrics( LogTable, 7, QBASE_TYPE_UINT16, 0 );
				Qbase_set_field_metrics( LogTable, 8, QBASE_TYPE_UINT, 0 );
				Qbase_set_field_metrics( LogTable, 9, QBASE_TYPE_UINT, 0 );
				Qbase_set_field_metrics( LogTable,10, QBASE_TYPE_DOUBLE, 0 );
				Qbase_set_field_metrics( LogTable,11, QBASE_TYPE_DOUBLE, 0 );
				Qbase_set_field_metrics( LogTable,12, QBASE_TYPE_TIMESTAMP, 0 );
				
				printf("LOG_> table descriptor ok - creating\n");
								
				if( QBASE_E_OK != (result = Qbase_table_create(log_con,LogTable)) ){
					printf("LOG_> %s", QBERROR[result]);
					exit(-1);
				}
			}else{
				if( QBASE_E_OK != (result = Qbase_table_caps( log_con, &LogTable ))){
					printf("LOG_> Table %s: %s\n", tblname, QBERROR[result]);
					exit(-1);
				}
			}
			
			printf("LOG_> retrieving new buffer\n");
			logRow = Qbase_row_newbuffer( LogTable, &logRowSize );
			if( logRow == NULL ){
				printf("LOG_> Was unable to allocate memory for a new row\n");
				exit(-1);
			}
			
			/* set the time, at which logging occured */
			Qbase_field_set(LogTable,logRow,12,&logtime,sizeof(logtime));
			printf("Time set to: %u\n", logtime.tv_sec);
			
			/* reset sums of all accounts to 0 so we can update them with += */
			{
				account_t * CAcnt = accounts;
				while(CAcnt != NULL){
					CAcnt->acnt.sum_current = 0;
					CAcnt = CAcnt->next;
				}
			}
			
			
			/* 
			 * save log to database
			 */
			CNic = nics;
			/* for each MAC - surf all of its logged sessions */
			while( CNic != NULL ){
				
				CLog = CNic->LOG;
				while(CLog != NULL){
					/* 1) count money */
					CLog->dtls.SumToCli = (CLog->dtls.BytesToCli * CNic->acnt_p->acnt.pay_mb_in) / 1048576.0;
					CLog->dtls.SumFromCli = (CLog->dtls.BytesFromCli * CNic->acnt_p->acnt.pay_mb_out) / 1048576.0;
					CNic->acnt_p->acnt.sum_current += ( CLog->dtls.SumToCli + CLog->dtls.SumFromCli );

					/* 2) copy contents to buffer */
					memcpy(logRow, &CLog->dtls, sizeof(det_data_t) );
					
					/* 3) insert record into database */
					if( QBASE_E_OK != Qbase_insert( log_con, logRow, logRowSize ) ){
						printf("LOG_> Error inserting record to log database\n");
						exit(-1);
					}
					
					/* 4) free current statistics - to not count the same traffic many times */
					{
						logdet_t * TLog = CLog->next;
						free(CLog);
						CLog = TLog;
					}
				}
				CNic->LOG = CLog;
				CNic = CNic->next;
			}
			
			
			
			/* refresh information about accounts and nics -- block/deblock MAC addresses */
			
			
			/* release logging table */
			free( LogTable );
			LogTable = NULL;
			free( logRow );
			logRow = NULL;
			
			printf("Information saved to database\n");
			ItIsTimeToLog = 0;
		}
		
	}
	
	exit(0);
	
} /* int main() */


/* EOF */
