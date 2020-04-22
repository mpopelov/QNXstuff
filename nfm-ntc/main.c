/*
 * NFM-NTC.SO by -=MikeP=-
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
 * This is the main portion of code of the project named
 * network traffic counter (ntc)
 * It represents the io-net module for QNX NTO OS
 * in terms used in this operating system the output produced is called
 * nfm-ntc.so ( Network Filter Module NTC )
 * 
 * Herein is the library interface, understood by io-net. Here we register
 * with io-net, say what kind of module we are and what information
 * we are anxcious about. Also we start a separate thread, responsible for
 * maintaining our Resource Manager - the only interface to control our filter
 * at runtime.
 */

#include	"ntc_common.h"

#define		ARP_FRAME	0x0806


/* forward definitions */
static int ntc_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options);
static int ntc_destroy(void *dll_hdl);
static int ntc_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface);
static int ntc_rx_down(npkt_t *npkt, void *rx_down_hdl);
static int ntc_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl);
static int ntc_shutdown1(int registrant_hdl, void *func_hdl);
static int ntc_shutdown2(int registrant_hdl, void *func_hdl);
static void *ntc_pkt_process(void *arg);



/* this is description of entry into module */
io_net_dll_entry_t io_net_dll_entry = {
	2,
	ntc_init,
	ntc_destroy
};

/* descriptor of module handler functions */
static io_net_registrant_funcs_t ntc_funcs = {
	5,
	ntc_rx_up,
	ntc_rx_down,
	ntc_tx_done,
	ntc_shutdown1,
	ntc_shutdown2
};

/* description of module capabilities */
static io_net_registrant_t ntc_reg = {
	_REG_FILTER_ABOVE | _REG_NO_REMOUNT | _REG_DEREG_ALL,
	"nfm-ntc.so",
	"en", 
	"en", 
	NULL,	/* do we need to supply our handlers with extra info ? */
	&ntc_funcs,
	0
};

















/* runtime global variables */
void		* ntc_dll_hdl;	/* module handler as of io-net */
dispatch_t	* ntc_dpp;		/* dispatch handle */
io_net_self_t	* ntc_ion;	/* io-net callback entries */
int		ntc_reg_hdl;	/* registrant handler */


pthread_mutex_t	pkt_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t pkt_rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t if_rwlock = PTHREAD_RWLOCK_INITIALIZER;


/* Control structures */
static resmgr_connect_funcs_t    ntc_connect_funcs;
static resmgr_io_funcs_t         ntc_io_funcs;
static iofunc_attr_t             ntc_attr;



/* the descriptor of our packet logging queue */
int		num_packets = 0;		/* number of packets in the queue */
int		num_pkt_missed = 0;		/* number of missed packets       */
blkinfo_t		num_pkt_blocked = {0,0};	/* number of packets blocked      */
pkt_info_t * pktq;				/* the queue itself               */
block_list_t * blocklist;

/* description of all interfaces in our system */
if_info_t * ntc_ifs = NULL;

int	pkt_log_limit = MAX_PACKETS_LOGGED; /* how many packets to log */



























/*
 * Here we start module:
 *	- register with io-net
 *	- ask for +data we want
 *	- start resource manager thread
 */
static int ntc_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *ion, char *options)
{
	/* save needed arguments for future use */
	ntc_dll_hdl = dll_hdl;
	ntc_ion = ion;
	ntc_dpp = dpp;
	
	/* set our blocklist to NULL */
	blocklist = NULL;
	
	/* register our module with io-net */
	if(ntc_ion->reg(ntc_dll_hdl, &ntc_reg, &ntc_reg_hdl, NULL, NULL) == -1){
		return (-1);
	}
	
	/* say io-net we want all the data travelling across network */
	if(ntc_ion->reg_byte_pat(ntc_reg_hdl, 0, 0, NULL, _BYTE_PAT_ALL) == -1){
		ntc_ion->dereg(ntc_reg_hdl);
		return (-1);
	}
	
	/* now allocate space for logging queue */
	pktq = ntc_ion->alloc( pkt_log_limit*sizeof(pkt_info_t) , 0);
	
	/* do not need to advertise our capabilities - we are filter.
	 * Now start the resource manager thread with default parameters
	 * and no input to the thread
	 */
	if(EOK != pthread_create(NULL, NULL, ntc_pkt_process, NULL)){
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
static int ntc_destroy(void *dll_hdl)
{
	/* by this time we have nothing to destroy */
	return EOK;
}





/*
 * Here we process messages from the NIC or any other driver below us,
 *  going in upward direction through io-net's stack
 */
static int ntc_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface)
{
	uint16_t	* special;
	
	if( (npkt->flags & _NPKT_MSG) == 0 ){
		/*
		 * here we process the packet coming into our host
		 */
		
		/*	1) check if this is the interface of interest */
		{
			if_info_t * ifs = ntc_ifs;
			
			/* first of all - we should see if the source or destination NIC
			 * is of interest and UP - or we'll simply pass the packet without logging
			 */
			pthread_rwlock_rdlock( &if_rwlock );
			while( ifs != NULL ){
				if( (ifs->cell == cell) && (ifs->endpoint == endpoint) && (ifs->iface == iface) ){
					if( ifs->ofinterest == 0 ){
						pthread_rwlock_unlock( &if_rwlock );
						goto done;
					}else{
						break; /* if we found the interface to be of interest we need to block/log packet */
					}
				}
				ifs = ifs->next;
			}
			pthread_rwlock_unlock( &if_rwlock );
		}
		
		/*	2) we found that the interface is of interest to us and need some idea
		 *     wether to block it or not */
		{
			block_list_t *	blist;
			mac_cmp_t *		frame_mac;
			
			/* see if this is an ARP packet - if yes - allow it always but never log */
			{
				uint16_t * TOP = (uint16_t *) ( (char*)(npkt->buffers.tqh_first->net_iov->iov_base) + 2*MAC_ADDR_LENGTH );
				if( *TOP == ARP_FRAME){
					if( ntc_ion->tx_up(ntc_reg_hdl, npkt, off, len_sub, cell, endpoint, iface) == 0 )
						ntc_ion->tx_done(ntc_reg_hdl, npkt);
					return 0;
				}
			}
			
			/* tune in the MAC address of the destination host */
			frame_mac = (mac_cmp_t*) ( (char*)(npkt->buffers.tqh_first->net_iov->iov_base) + MAC_ADDR_LENGTH );
			
			/* read-lock the rwlock to ensure that RM is not fixing it */
			pthread_rwlock_rdlock( &pkt_rwlock );
			blist = blocklist;
			
			/* scan through the rules list and try to determine if we should block the packet */
			while( blist != NULL ){
				if( frame_mac->NID == blist->block_addr.NID ){
					if( frame_mac->VID == blist->block_addr.VID ){
						/* we found the record allowing us to pass the packet */
						break;
					}
				}
				
				/* if this rule didn't match we go forth */
				blist = blist->next;
			}
			pthread_rwlock_unlock(&pkt_rwlock);
			
			/* if HAVE NOT found the record in blist - this means we need to block it */
			if( blist == NULL ){
				/* YES - we should block!!! there is no record allowing us to pass it !!! */
				/* increase blocked inbound packets counter */
				atomic_add(&num_pkt_blocked.in_pkt, 1);
				/* issue tx_done so that this packet may be freed and not sent to modules above us */
				ntc_ion->tx_done(ntc_reg_hdl, npkt);
				return 0; /* return */
			}
		}
		
		
		/*	3) finally, we are here because we decided this packet may be really
		 *     transmitted over the network. LOG it */
		{
			iov_t	iov;
			int		bufsize;
			
			/* lock the queue to copy data */
			pthread_mutex_lock(&pkt_mutex);
			
			/* tune indices into packet queue :
			 * if there are too many packets - we simply show we missed one :)
			 * indeed we have no difference which one to miss - the one
			 * stored before or the current one.
			 */
			if((num_packets+1) > pkt_log_limit){
				num_pkt_missed++;
				pthread_mutex_unlock(&pkt_mutex);
				goto done;
			}
			
			/* find out how many bytes to copy from frame */
			bufsize = min(npkt->framelen, PKT_LOG_SIZE );
			
			/* set the pointers to where copy the information */
			SETIOV(&iov, &(pktq[num_packets].DLC), PKT_LOG_SIZE);
			
			/* copy contents of the frame to logging queue */
			ntc_ion->memcpy_from_npkt(&iov,1,0,npkt,0,bufsize);
			
			/* log total frame length and direction */
			pktq[num_packets].len = npkt->framelen;
			pktq[num_packets].dir = PKT_DIR_IN;
			
			/* now release the queue */
			num_packets++;
			pthread_mutex_unlock(&pkt_mutex);
		}
		
		/* we have logged packet and now we may send it up the io-net's flow
		 * with full respect */
		goto done;
	}
	
	
	/* We are sitting on top of every adapter so when the adapter goes down
	 * i.e. sends _NPKT_MSG_DYING we need to set the "ofinterest"=0 flag
	 * so we are not processing messages from dead adapter
	 */
	if( (npkt->flags & _NPKT_MSG_DYING) != 0 ){
		
		if_info_t * ifs = ntc_ifs;
		
		/* here we try to find the inteface that is dying in our own
		 * interface list. if it is found - we set up ofinterest field
		 * to zero - otherwize we simply pass the message to upper levbels 
		 */
		pthread_rwlock_wrlock( &if_rwlock );
		while(ifs != NULL){
			if( (ifs->cell == cell) && (ifs->endpoint == endpoint) && (ifs->iface == iface) ){
				ifs->dead = 1; /* say this interface is down */
				break;
			}
			ifs = ifs->next;
		}
		pthread_rwlock_unlock( &if_rwlock );
		goto done;
	}
	
	/* When we are just mounted - every adapter on the io-net's pipe sends
	 * us advertizing information - capabilities, state, type and so on.
	 * we build the list of adapters on the system so that later we may
	 * filter packets only on the adapters of interest.
	 * 
	 * The things below are a bit tricky - so you have to surf
	 * the code for some drivers of NIC's supplied with the QNX
	 * to see how it happened that there is info about adapter.
	 * 
	 * It may be useful to see <net/if.h>, <net/if_types.h>
	 */
	special = (uint16_t *)(npkt->buffers.tqh_first->net_iov->iov_base);
	if(*special == _IO_NET_MSG_DL_ADVERT){
		
		/* every driver that sends advertising info to upper levels
		 * fills out the following structure and sends it as packet data
		 */
		io_net_msg_dl_advert_t *ad = (io_net_msg_dl_advert_t *)special;
		
		/* we need to find if the advertized interface is on our list
		 * so that we need not to add one more record.
		 */
		if_info_t * ifs = ntc_ifs;
		pthread_rwlock_rdlock( &if_rwlock );
		while( ifs != NULL ){
			if( stricmp(ifs->ifname, ad->dl.sdl_data) == 0 ){
				ifs->cell = cell;
				ifs->endpoint = endpoint;
				ifs->iface = iface;
				ifs->dead = 0;	/* the interface is up, interest is defined explicitly */
				break;	/* for sure - there may be only one interface with the same name */
			}
			ifs = ifs->next;
		}
		pthread_rwlock_unlock( &if_rwlock );
		
		/* if we have not found the corresponding name:
		 * either we have empty list or reached the end of list.
		 * In all cases we'll have ifs == NULL
		 * We are about to add this interface to the list
		 */
		if( ifs == NULL ){
			ifs = (if_info_t *) malloc( sizeof(if_info_t) );
			
			ifs->cell = cell;
			ifs->endpoint = endpoint;
			ifs->iface = iface;
			ifs->dead = 0;
			ifs->ofinterest = 0; /* all new interfaces are not of interest */
			
			snprintf(ifs->ifname, IFNAMSIZ, "%s", ad->dl.sdl_data);
			
			pthread_rwlock_wrlock( &if_rwlock );
			ifs->next = ntc_ifs;
			ntc_ifs = ifs;
			pthread_rwlock_unlock( &if_rwlock );
		}
		
	}
	
	/* you see, using goto is a bad practice, but having tha same code many times
	 * in different places is not a good idea for memory consumption. one little
	 * jmp ptr32 is much better and makes program a bit more clear */
done:
	/* go forth in UP direction over io-net's pipe */
	if( ntc_ion->tx_up(ntc_reg_hdl, npkt, off, len_sub, cell, endpoint, iface) == 0 )
		ntc_ion->tx_done(ntc_reg_hdl, npkt);
	
	return 0;
}
























































/*
 * Here we process messages from every protocol stack above us,
 * going in downward direction through io-net's stack
 * As we seat just above Ethernet Driver - we got every byte
 * trying to go out of the host.
 */
static int ntc_rx_down(npkt_t *npkt, void *rx_down_hdl)
{
	/* 
	 * If the packet is a message we simply transfer it down.
	 * 
	 * By now the only only message type that is to be passed down
	 * is that, setting multicast options to network adapters
	 */
	if( (npkt->flags & _NPKT_MSG) != 0 ) return ntc_ion->tx_down(ntc_reg_hdl, npkt);
	
	
	/* 
	 * here we process the packet going out of our host 
	 */
	
	
	/*	1) check if this is the interface of interest */
	{
		if_info_t * ifs = ntc_ifs;
		
		/* first of all - we should see if the source or destination NIC
		 * is of interest and UP - or we'll simply pass the packet without logging
		 */
		pthread_rwlock_rdlock( &if_rwlock );
		while( ifs != NULL ){
			if( (ifs->cell == npkt->cell) && (ifs->endpoint == npkt->endpoint) && (ifs->iface == npkt->iface) ){
				if( ifs->ofinterest == 0 ){
					pthread_rwlock_unlock( &if_rwlock );
					return ntc_ion->tx_down(ntc_reg_hdl, npkt);
				}else{
					break; /* if we found the interface to be of interest we need to block/log packet */
				}
			}
			ifs = ifs->next;
		}
		pthread_rwlock_unlock( &if_rwlock );
	}
	
	
	/*	2) we found that the interface is of interest to us and need some idea
	 *     wether to block it or not */
	{
		block_list_t *	blist;
		mac_cmp_t *		frame_mac;
		
		
		/* see if this is an ARP packet - if yes - allow it always but never log */
		{
			uint16_t * TOP = (uint16_t *) ( (char*)(npkt->buffers.tqh_first->net_iov->iov_base) + 2*MAC_ADDR_LENGTH );
			if( *TOP == ARP_FRAME) return ntc_ion->tx_down(ntc_reg_hdl, npkt);
		}
		
		/* tune in the MAC address of the destination host */
		frame_mac = (mac_cmp_t*) (npkt->buffers.tqh_first->net_iov->iov_base);
		
		/* read-lock the rwlock to ensure that RM is not fixing it */
		pthread_rwlock_rdlock( &pkt_rwlock );
		blist = blocklist;
		
		/* scan through the rules list and try to determine if we should block the packet */
		while( blist != NULL ){
			if( frame_mac->NID == blist->block_addr.NID ){
				if( frame_mac->VID == blist->block_addr.VID ){
					/* We found the appropriate record and we pass the packet and log it */
					break;
				}
			}
			
			/* if this rule didn't match we go forth */
			blist = blist->next;
		}
		
		pthread_rwlock_unlock(&pkt_rwlock);
		
		/* if HAVE NOT found the record in blist - this means we need to block it */
		if( blist == NULL ){
			/* YES - we should block!!! there is no record allowing us to pass it !!! */
			/* increase blocked outbound packets counter */
			atomic_add(&num_pkt_blocked.out_pkt, 1);
			/* issue tx_done so that this packet may be freed and not sent to modules below us */
			ntc_ion->tx_done(ntc_reg_hdl, npkt);
			return TX_DOWN_OK; /* return with TX_DOWN_OK so that TCP don't bother us with frame retransmissions */
		}
	}
	
	
	/*	3) finally, we are here because we decided this packet may be really
	 *     transmitted over the network. LOG it */
	{
		iov_t	iov;
		int		bufsize;
		
		/* lock the queue to copy data */
		pthread_mutex_lock(&pkt_mutex);
		
		/* tune indices into packet queue :
		 * if there are too many packets - we simply show we missed one :)
		 * indeed we have no difference which one to miss - the one
		 * stored before or the current one.
		 */
		if((num_packets+1) > pkt_log_limit){
			num_pkt_missed++;
			pthread_mutex_unlock(&pkt_mutex);
			return ntc_ion->tx_down(ntc_reg_hdl, npkt);
		}
		
		/* find out how many bytes to copy from frame */
		bufsize = min(npkt->framelen, PKT_LOG_SIZE );
		
		/* set the pointers to where copy the information */
		SETIOV(&iov, &(pktq[num_packets].DLC), PKT_LOG_SIZE);
		
		/* copy contents of the frame to logging queue */
		ntc_ion->memcpy_from_npkt(&iov,1,0,npkt,0,bufsize);
		
		/* log total frame length and direction */
		pktq[num_packets].len = npkt->framelen;
		pktq[num_packets].dir = PKT_DIR_OUT;
		
		/* now release the queue */
		num_packets++;
		pthread_mutex_unlock(&pkt_mutex);
	}
	
	/* we have logged packet and now we may send it down the io-net's flow
	 * with full respect */
	return ntc_ion->tx_down(ntc_reg_hdl, npkt);
}
















































/*
 * This function is called when we are about to be done
 * with packet processing - i.e. changing it.
 * We have nothing to add to or delete from a packet
 * so we simply return 0 to indicate success
 * 
 * We need to register this function: as we want to seat
 * just above the NIC driver - we are always asked by io-net's
 * pipe to confirm changes.
 */
static int ntc_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl)
{
	return 0;
}




/*
 * We are called this function when io-net wants to unmount
 * our filter. We are still connected into io-net's pipe
 * and may prevent shutdown of filter.
 * But here we need to close all file handlers and release any
 * memory we allocated for work.
 */
static int ntc_shutdown1(int registrant_hdl, void *func_hdl)
{
	/* free all data, where the packet info was stored,
	 * free log queue. though it is not critical for now
	 * to clen everything because I don't unmount the
	 * filter (I slay the whole io-net) ->
	 * 
	 * !!! THIS STILL NEEDS TO BE DONE !!!
	 * 
	 */
	return EOK;
}




/*
 * We are called this function when io-net detached us
 * from its pipe and wants us to shutdown.
 * We need to stop all of our threads.
 */
static int ntc_shutdown2(int registrant_hdl, void *func_hdl)
{
	/* signal our logging thread to stop */
	return 0;
}












/*
 * Here we process packet queue - we register with resourse manager
 * and dispatch the resource.
 */
static void *ntc_pkt_process(void *arg)
{
    resmgr_attr_t        ntc_rm_attr;
    dispatch_t           *ntc_rm_dpp;
    dispatch_context_t   *ntc_rm_ctp;
    int                  id;
    
    /* initialize dispatch interface */
    if((ntc_rm_dpp = dispatch_create()) == NULL) {
        return NULL;
    }

    /* initialize resource manager attributes */
    memset(&ntc_rm_attr, 0, sizeof(ntc_rm_attr));
    ntc_rm_attr.nparts_max = 1;
    ntc_rm_attr.msg_max_size = MAX_PKT_READ*sizeof(pkt_info_t);	/* here we say how much data we'll transfer */

    /* initialize functions for handling messages */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &ntc_connect_funcs, 
                     _RESMGR_IO_NFUNCS, &ntc_io_funcs);

    ntc_io_funcs.read = ntc_rm_read;	/* the _IO_READ handler */
    ntc_io_funcs.devctl = ntc_rm_devctl; /* a handler to control /dev/ntc */

    /* initialize attribute structure used by the device */
    iofunc_attr_init(&ntc_attr, S_IFNAM | 0666, 0, 0);

    /* attach our device name */
    id = resmgr_attach(ntc_rm_dpp,			/* dispatch handle        */
                       &ntc_rm_attr,		/* resource manager attrs */
                       RM_DEVICE_NAME,			/* device name            */
                       _FTYPE_ANY,			/* open type              */
                       0,					/* flags                  */
                       &ntc_connect_funcs,	/* connect routines       */
                       &ntc_io_funcs,		/* I/O routines           */
                       &ntc_attr);			/* handle                 */
    if(id == -1) {
        return NULL;
    }

    /* allocate a context structure */
    ntc_rm_ctp = dispatch_context_alloc(ntc_rm_dpp);

    /* 
     * start the resource manager message loop
     */
    while(1) {
        if((ntc_rm_ctp = dispatch_block(ntc_rm_ctp)) == NULL) {
            return NULL;
        }
        dispatch_handler(ntc_rm_ctp);
    }
	return NULL;
}
