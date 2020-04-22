/*
 *  devn-ser.so by -=MikeP=-
 *  Copyright (c) 2004, 2005  Mike Anatoly Popelov ( AKA -=MikeP=- )
 *
 *  -=MikeP=- <mikep@qnx.org.ru>
 *            <mikep@kaluga.org>
 *
 *            WWW:	http://mikep.qnx.org.ru
 *            ICQ:	104287300
 *
 *  You are free to use this code in the way you like and for any purpose,
 *  not deprecated by your local law. I will have now claim for using this code
 *  in any project, provided that the reference to me as the author is given in
 *  in this file or any code derived from this one.
 *  I also DO NOT TAKE ANY RESPONSIBILITY if this code does not suite you in your
 *  particular case. Though this code was written to be stable and working,
 *  I will take NO RESPONSIBILITY if it does not work for you, on your particular
 *  equipment, in your particular configuration and environment, if it brings any
 *  kind of damage to your equipment.
 *
 */
 
/* 
 * Herein is the library interface, understood by io-net. Here we register
 * with io-net, say what kind of module we are and what information
 * we are anxcious about. Also we start a separate thread, responsible for
 * recieving packets from serial port.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<time.h>
#include	<atomic.h>
#include	<pthread.h>
#include	<malloc.h>
#include	<net/if.h>
#include	<net/if_arp.h>
#include	<net/if_types.h>
#include	<netinet/in.h>
#include	<stddef.h>
#include	<sys/iofunc.h>
#include	<sys/ioctl.h>
#include	<sys/dispatch.h>
#include	<devctl.h>
#include	<sys/io-net.h>
#include	<sys/nic.h>
#include	<sys/dcmd_io-net.h>



/* forward definitions */
static int devn_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options);
static int devn_destroy(void *dll_hdl);

static int devn_rx_down(npkt_t *npkt, void *rx_down_hdl);
static int devn_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl);
static int devn_shutdown1(int registrant_hdl, void *func_hdl);
static int devn_shutdown2(int registrant_hdl, void *func_hdl);
static int devn_advertise(int registrant_hdl, void *func_hdl);
static int devn_devctl(void * registrant_hdl, int dcmd, void *data, size_t size, union _io_net_dcmd_ret_cred *ret);

static void *devn_receive(void *arg);






/* this is description of entry into module - driver */
io_net_dll_entry_t io_net_dll_entry = {
	2,
	devn_init,
	devn_destroy
};


/* descriptor of module handler functions */
static io_net_registrant_funcs_t devn_funcs = {
	9,
	NULL,					/* we do not recieve upcoming packets */
	devn_rx_down,			/* send packets through serial line */
	devn_tx_done,			/* free the packets we issued */
	devn_shutdown1,
	devn_shutdown2,
	devn_advertise,
	devn_devctl,
	NULL,
	NULL
};



static io_net_registrant_t devn_reg = {
	_REG_PRODUCER_UP,
	"devn-ser.so",
	"en",
	NULL,
	NULL,
	&devn_funcs,
	0
};




void *			devn_dll_hdl;	/* module handler as of io-net */
dispatch_t *	devn_dpp;		/* dispatch handle */
FILE *			logfd = NULL;


#define				DEF_PKT_SIZE	1520	/* default packet size */
#define				MIN_PKTSIZE		200		/* minimum packet size */

/* in case somebody specifies different packet sizes for different
 * interfaces we will store here the largest of them */
int	max_pkt_size = DEF_PKT_SIZE;
uint32_t stop_receive = 0;

/* runtime global variable, that describes our interface */
typedef struct en_ser{
	int				reg_hdl;	/* registrant handler */
	uint16_t		cell;
	uint16_t		endpoint;
	uint16_t		iface;
	uint32_t		bytes_txed;
	uint32_t		bytes_rxed;
	int				FD;			/* this is descriptor to send data to/read from */
	int				pkt_size;	/* size of packet to send */
	char			mac[8];
	Nic_t			nic;
} en_ser_t;

io_net_self_t *	devn_ion = NULL;		/* io-net callback entries */
en_ser_t		serial;






/*
 * NOTE: I use the "full-featured" version of io-net interface instead of the
 * one, shown in the DDK to 6.2.1 - primary goal doing so is the illustrative
 * purpose of this code. So you can study how you can manage several cards
 * present in your system that are to be handled by your driver. 
 */



/* a simple routine to allocate a fresh npkt structure with a
 * buffer of a given size */
npkt_t *devn_alloc_npkt(size_t size){
	npkt_t		*npkt;
	net_buf_t	*nb;
	net_iov_t	*iov;
	char		*ptr;
	
	if((npkt = devn_ion->alloc_up_npkt(sizeof(net_buf_t)+sizeof(net_iov_t),(void **) &nb)) == NULL){
		return( NULL );
	}
	
	if((ptr = devn_ion->alloc(size, 0)) == NULL){
		devn_ion->free(npkt);
		return(NULL);
	}
	
	TAILQ_INSERT_HEAD(&npkt->buffers, nb, ptrs);
	
	iov = (net_iov_t *)(nb + 1);
	
	nb->niov = 1;
	nb->net_iov = iov;
	
	iov->iov_base = (void *) ptr;
	iov->iov_len = size;
	iov->iov_phys = (paddr_t)(devn_ion->mphys(iov->iov_base));
	
	npkt->org_data = ptr;
	npkt->next = NULL;
	npkt->tot_iov = 1;
	npkt->framelen = size;
	return (npkt);
}














char * tokens[] = {
	"fd",
	NULL
};

char * device_name = "/dev/ser1";








/*
 * Here we start module:
 *	- register with io-net
 *	- ask for +data we want
 *	- start resource manager thread
 */
static int devn_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *ion, char *options)
{
	
	/* for options parsing */
	char * valuep;
	
	/* save needed arguments for future use */
	devn_dll_hdl = dll_hdl;
	devn_ion = ion;
	devn_dpp = dpp;
	
	/* open file for logging */
	logfd = fopen("/devnser.log","a");
	setvbuf(logfd,NULL,_IOLBF,0);
	fprintf(logfd,"Starting the driver\n");

	/* parse options and open either dev/ser1 or the file specified */
	if(options != NULL )
	if( 0 == getsubopt(&options,tokens,&valuep) ){
		/* we got a name of file to open */
		if((valuep == NULL) || (*valuep=='\0')){
			fprintf(logfd,"option 'fd' requires a name of file as value\n");
			return (-1);
		}
		device_name = strdup(valuep);
	}
	
	memset(&serial,0,sizeof(en_ser_t));
	serial.pkt_size = DEF_PKT_SIZE;
	if( (serial.FD = open(device_name,O_RDWR)) == -1 ){
		/* we are unable to open the requested descriptor :( */
		fprintf(logfd,"Unable to open requested descriptor (%s)\n",device_name);
		return (-1);
	}
	
	fprintf(logfd,"(init) Starting operation on %s\n",device_name);
	
	/* describe our devices */
	devn_reg.func_hdl = &serial;
	
	/* register our device with io-net */
	if(devn_ion->reg(devn_dll_hdl, &devn_reg, &serial.reg_hdl, &serial.cell, &serial.endpoint) < 0){
		fprintf(logfd,"Unable to register with io-net\n");
		close(serial.FD);
		return (-1);
	}
	
	fprintf(logfd,"(init) registered device\n");
	
	/* set MAC address and fill in the Nic_t structure */
	*(uint16_t*)(serial.mac) = 0x0100;
	*(int*)&(serial.mac[2]) = rand()*getpid()*pthread_self();
	
	serial.nic.flags = NIC_FLAG_AUI;
	serial.nic.astate = 0;
	serial.nic.media = NIC_MEDIA_802_3;
	serial.nic.media_rate = 115; /* like we use 115 Kbits/second */
	serial.nic.mtu = serial.pkt_size-4;
	serial.nic.node = serial.endpoint;
	serial.nic.phy = PHY_INTEL_82503;
	serial.nic.mac_length = 6;
	/* now fill in mac addresses */
	memcpy(&(serial.nic.permanent_address),serial.mac,6);
	memcpy(&(serial.nic.current_address),serial.mac,6);
	/* fill in some cfg info */
	strcpy(serial.nic.cfg.Description,"Serial pseudo device");
	
	/* advertise this device */
	devn_advertise(serial.reg_hdl,&serial);
	fprintf(logfd,"(init) advertised\n");
	
	/* now start the thread that will recieve packets */
	if(EOK != pthread_create(NULL, NULL, devn_receive, &serial)){
		close(serial.FD);
		devn_ion->dereg(serial.reg_hdl);
		return (-1);
	}
	
	/* By this time we are done :) yupeee! */
	return EOK;
}










/*
 * Here we destroy all the data previously allocated.
 * We should be especially careful about this so
 * we don't through entire io-net
 */
static int devn_destroy(void *dll_hdl)
{
	fprintf(logfd,"(destroy) driver is exitingn\n");
	fclose(logfd);
	return EOK;
}










/*
 * We are called this function when io-net wants to unmount
 * our filter. We are still connected into io-net's pipe
 * and may prevent shutdown of filter.
 * But here we need to close all file handlers.
 */
static int devn_shutdown1(int registrant_hdl, void *func_hdl)
{
	/* there still may be packets in progress it is safe only
	 * to close file descriptors */
	atomic_add(&stop_receive,1);
	close(serial.FD);
	return EOK;
}










/*
 * We are called this function when io-net detached us
 * from its pipe and wants us to shutdown.
 * We need to stop all of our threads and release all memory.
 * 
 * NOTE: as we do only driver stuff in this library - we will close
 * everything in our "master shutdown function".
 */
static int devn_shutdown2(int registrant_hdl, void *func_hdl)
{
	/* now noone can disturb us and send data througt this card
	 * so we can freely deregister it with io-net */
	devn_ion->dereg(serial.reg_hdl);
	return 0;
}










/*
 * Here we process messages from every protocol stack above us,
 * going in downward direction through io-net's stack
 * As we seat just above Ethernet Driver - we got every byte
 * trying to go out of the host.
 */
static int devn_rx_down(npkt_t *npkt, void *rx_down_hdl)
{
	char * txbuf;
	iov_t txbuf_iov;
		
	fprintf(logfd,"(rx_down) received packet: %d bytes\n",npkt->framelen);
	
	/* switch the type of packet data */
	if( npkt->flags & _NPKT_MSG ){
		/* internal messages always come as standalone data packets.
		 * we have to call tx_don as we are the final point and
		 * supposed to consume the data data */
		fprintf(logfd,"(rx_down) packet contains message\n");
		devn_ion->tx_done(serial.reg_hdl,npkt);
	}else{
		/* real data loaded packets may come as a linked list of packets */
		npkt_t * tmp_pkt = npkt;
		
		/* allocate space once for sending chain of packets */
		if(NULL == (txbuf = devn_ion->alloc(serial.pkt_size,0)))
				return TX_DOWN_FAILED;
		
		while( tmp_pkt != NULL ){
			/* try to send data */
			*(uint32_t*)txbuf = tmp_pkt->framelen;
			SETIOV(&txbuf_iov,(txbuf+4),tmp_pkt->framelen);
			if(tmp_pkt->framelen != devn_ion->memcpy_from_npkt(&txbuf_iov,1,0,tmp_pkt,0,tmp_pkt->framelen)){
				devn_ion->free(txbuf);
				return TX_DOWN_FAILED;
			}
			
			/* now we see data as a single continous buffer - send it actually */
			if(serial.pkt_size != write(serial.FD,txbuf,serial.pkt_size)){
				devn_ion->free(txbuf);
				return TX_DOWN_FAILED;
			}
			
			/* we happily transmitted the packet */
			atomic_add(&serial.bytes_txed,tmp_pkt->framelen);
			
			/* move to the next available packet */
			devn_ion->tx_done(serial.reg_hdl,tmp_pkt);
			tmp_pkt = tmp_pkt->next;
		}
		
		devn_ion->free(txbuf);
	}
	
	return TX_DOWN_OK;
}










/* Here we release the packet allocated for upper layers */
static int devn_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl)
{
	fprintf(logfd,"(tx_done) destroing packet\n");
	devn_ion->free(npkt->org_data);
	devn_ion->free(npkt);
	return 0;
}










/*
 * Here we block on receiving packets from serial port
 */
static void *devn_receive(void *arg)
{
	int			res = 0;
	int			read_bytes = 0;
	uint32_t	pkt_len;
	char		* recv_buf;
	npkt_t * npkt = NULL;
	
	if( (recv_buf = devn_ion->alloc(max_pkt_size,0)) == NULL ) return NULL;
	
	fprintf(logfd,"(receive) thread started\n");
	
	while( stop_receive == 0 ){
		
		/* try our best to read something from descriptor */
		read_bytes = 0;
		while( read_bytes != serial.pkt_size ){
			res = read( serial.FD, (recv_buf+read_bytes) , (serial.pkt_size-read_bytes) );
			if(res == -1){
				/* in case we closed our serial device we have this error - no need to continue */
				if(errno==EBADF) break;
			}else{
				read_bytes += res;
			}
		}
		if(res == -1) continue;
		
		/* increase received bytes count */
		pkt_len = *(uint32_t*)(recv_buf); /* the first 4 octets represent
		                                   * actual length of data */
		atomic_add(&serial.bytes_rxed,pkt_len);
		
		fprintf(logfd,"(receive) got %d bytes on interface:\n",pkt_len);
		
		/* try to allocate the packet */
		if(!(npkt=devn_alloc_npkt(pkt_len))){
			/* we have nothing to do here - we are unable to allocate a packet
			 * go to next ready interface and hope we'll be able to allocate packet
			 * for that interface
			 */
			continue;
		}
		
		/* copy data to fresh packet and send the packet up */
		npkt->cell = serial.cell;
		npkt->endpoint = serial.endpoint;
		npkt->iface = 0;
		memcpy(npkt->org_data,recv_buf+4,pkt_len);
		
		/* now packet contains data - send it up */
		if(devn_ion->reg_tx_done(serial.reg_hdl,npkt,NULL) == -1){
			devn_ion->free(npkt->org_data);
			devn_ion->free(npkt);
			continue;
		}
		fprintf(logfd,"(recv td) tx_done registered\n");
		
		res = devn_ion->tx_up(serial.reg_hdl,npkt,0,0,serial.cell,serial.endpoint, 0);
		if( res == 0 ){
			devn_ion->tx_done(serial.reg_hdl, npkt);
		}else{
			fprintf(logfd,"(recv td) packet sent UPWARD - processed by %d modules above us\n",res);
		}
		/* now the packet is sent !!!! yupeeee !!!! */
	}
	
	/* we were asked to exit */
	devn_ion->free( recv_buf );
	pthread_exit(NULL);
}










/* Here we advertize capabilities of our drivers for each serial port we find
 * or are asked to bind to */
int devn_advertise(int registrant_hdl, void *func_hdl)
{
	npkt_t                  *npkt;
	net_buf_t               *nb;
	net_iov_t               *iov;
	io_net_msg_dl_advert_t  *ap;
	int						ret;
	
	
	fprintf(logfd,"(advertise) advertising\n");
	
	/* Allocate a packet; */
	if((npkt = devn_alloc_npkt(sizeof(*ap)))==NULL){
		return (0);
	}
	
	ap = (io_net_msg_dl_advert_t*)(npkt->org_data);
	
	memset(ap, 0x00, sizeof (*ap));
	ap->type = _IO_NET_MSG_DL_ADVERT;
	ap->iflags = (IFF_SIMPLEX | IFF_BROADCAST | IFF_RUNNING | IFF_UP );
	ap->mtu_min = 0;
	ap->mtu_max = (serial.pkt_size-4);
	ap->mtu_preferred = (serial.pkt_size-4);
	
	/*ap->capabilities_rx = 7;
	ap->capabilities_tx = 7;*/
	
	sprintf(ap->up_type,"en%d",serial.endpoint);
	sprintf(ap->dl.sdl_data,"en%d",serial.endpoint);
	
	ap->dl.sdl_len = sizeof(struct sockaddr_dl);
	ap->dl.sdl_family = AF_LINK;
	ap->dl.sdl_index  = serial.endpoint;
	ap->dl.sdl_type = IFT_ETHER;
	
	/* !!! Not terminated strings */
	ap->dl.sdl_nlen = strlen(ap->dl.sdl_data);
	ap -> dl.sdl_alen = 6;
	memcpy(ap->dl.sdl_data + ap->dl.sdl_nlen,serial.mac, 6);
	
	npkt->flags |= _NPKT_MSG;
	npkt->iface = 0;
	npkt->framelen = sizeof(*ap);
	
	/* a way to send... */
	if(devn_ion->reg_tx_done( registrant_hdl, npkt, NULL ) == -1) {
		devn_ion->free(npkt->org_data);
		devn_ion->free(npkt);
		return(0);
	}
	fprintf(logfd,"(advertise) tx_done registered\n");
	
	ret = devn_ion->tx_up(registrant_hdl, npkt, 0, 0, serial.cell, serial.endpoint, 0);
	if(ret == 0)
	{
		devn_ion->tx_done(registrant_hdl, npkt);
	}
	
	fprintf(logfd,"(advertise) there are %d upper layers informed of us\n",ret);
	
	return (0);
}










/* here we handle devctl() messages */
static int devn_devctl(void * registrant_hdl, int dcmd, void *data, size_t size, union _io_net_dcmd_ret_cred *ret)
{
	NicTxRxCount_t	txrxcount;
	int				status = EOK;
	
	switch(dcmd){
		case DCMD_IO_NET_NICINFO:
			memcpy( data, &(serial.nic), min(sizeof(Nic_t),size) );
			break;
		case DCMD_IO_NET_TXRX_COUNT:
			txrxcount.tx_bytes = serial.bytes_txed;
			txrxcount.rx_bytes = serial.bytes_rxed;
			memcpy( data, &txrxcount, min(sizeof(NicTxRxCount_t),size) );
			break;
		default:
			status = ENOTSUP;
			break;
	}
	return status;
}