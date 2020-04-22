/*
 * -={ MikeP }=- (c) 2005
 */



/*
 * Main module routines go here
 */



#include	"qxc_common.h"
#include	"GetConf.h"






/* runtime global variables related to io-net */
void			*qxc_dll_hdl;	/* module handler as of io-net */
dispatch_t		*qxc_dpp;		/* dispatch handle */
io_net_self_t	*qxc_ion;		/* io-net callback entries */
int				qxc_reg_hdl;	/* registrant handler */



/* 
 * Forward definitions of functions availoble through module
 */
int qxc_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *ion, char *options);
int qxc_destroy(void *dll_hdl);
int qxc_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface);
int qxc_rx_down(npkt_t *npkt, void *func_hdl);
int qxc_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl);
int qxc_shutdown1(int registrant_hdl, void *func_hdl);
int qxc_shutdown2(int registrant_hdl, void *func_hdl);
void *qxc_rm_handle(void *arg);



/*
 * Entry that io-net searches for during load
 */
io_net_dll_entry_t io_net_dll_entry = {
	2,
	qxc_init,
	qxc_destroy
};


/*
 * The functions we export for io-net
 */
io_net_registrant_funcs_t qxc_funcs = {
	5,
	qxc_rx_up,
	qxc_rx_down,
	qxc_tx_done,
	qxc_shutdown1,
	qxc_shutdown2
};


/* 
 * Description of module capabilities
 */
static io_net_registrant_t qxc_reg = {
	_REG_FILTER_ABOVE | _REG_NO_REMOUNT | _REG_DEREG_ALL,
	"nfm-qxc.so",
	"en",
	"en",
	NULL,	/* we do not need to supply our handlers with extra info */
	&qxc_funcs,
	0
};





/*
 * Global variables we need during runtime
 */

qxc_if_info_t		interface;
char				*ifname		= "en0";


char				*devname	= "/dev/qxbil";
int					log_pkts	= 1000;
int					max_te		= 10;
int					max_nics	= 10;











/*
 * This function parses configuration file
 */
int qxc_parse_config(){
	
	char	*token;
	
	if( GetConf_open( QX_CONF_FILE ) == 0 ) return 0;
	
	while( (token = GetConf_string()) != NULL ){
		
		/* what interface are we going to handle */
		if( !stricmp("INTERFACE",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				ifname = strdup(token);
			}
			continue;
		}
		
		/* what device are we going to create */
		if( !stricmp("DEVICE",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				devname = strdup(token);
			}
			continue;
		}
		/* how many packets we should be able to hold in logging queue */
		if( !stricmp("LOGPKTS",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				log_pkts = atoi(token);
			}
			continue;
		}
		/* how many tax exemption rules are there expected */
		if( !stricmp("MAXTAXEXEMPT",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				max_te = atoi(token);
			}
			continue;
		}
		/* how many NICs will pass our filter */
		if( !stricmp("MAXNICS",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				max_nics = atoi(token);
			}
			continue;
		}
		
		/* we are here because we are not interested in this setting
		 * simply continue parsing file */
	}
	
	/* end parsing settings */
	GetConf_close();
	
	return 1;
}



























/*
 * Initialize our module; parse config file, attach to io-net, start thread.
 */
int qxc_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *ion, char *options){
	
	/* save supplied data for future use */
	qxc_dll_hdl	=	dll_hdl;
	qxc_ion		=	ion;
	qxc_dpp		=	dpp;
	
	memset( &interface, 0, sizeof(qxc_if_info_t) );
	
	/* read data from configuration file */
	if( !qxc_parse_config() ) return(-1);
	
	/* allocate buffers for logging and rules */
	if( !log_init(log_pkts) ){
		return (-1);
	}
	if( !rule_init_lists(max_te,max_nics) ){
		return (-1);
	}
	
	/* register our module with io-net */
	if(qxc_ion->reg(qxc_dll_hdl, &qxc_reg, &qxc_reg_hdl, NULL, NULL) == -1){
		return (-1);
	}
	
	/* say io-net we want all the data travelling across network */
	if(qxc_ion->reg_byte_pat(qxc_reg_hdl, 0, 0, NULL, _BYTE_PAT_ALL) == -1){
		qxc_ion->dereg(qxc_reg_hdl);
		return (-1);
	}
	
	/* do not need to advertise our capabilities - we are filter.
	 * Now start the resource manager thread with default parameters
	 * and no input to the thread
	 */
	if(EOK != pthread_create(NULL, NULL, qxc_rm_handle, NULL)){
		return (-1);
	}
	
	/* everything seems to be OK */
	return EOK;
}










/*
 * Delete all the data we do not need anymore.
 * As we are going to die only with io-net and not going to be remounted
 * we probably should do nothing here.
 */
int qxc_destroy(void *dll_hdl){
	return EOK;
}




































/*
 * The following are the two main functions that process flowing packets
 */

/*
 * All incoming packets are processed here
 */

int qxc_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface){
	
	uint16_t		*special;
	
	uint8_t			pkt_raw[PKT_LOG_LEN];
	int				pkt_raw_len = 0;
	int				pkt_exmpt;
	iov_t			iov;
	
	qx_log_t		pkt_parsed;
	
	
	
	
	
	/* if this is not a system message - process the packet */
	if( (npkt->flags & _NPKT_MSG) == 0 ){
		
		/* 1) Check if this interface is of interest */
		if( (interface.cell != cell) || (interface.endpoint != endpoint) || (interface.iface != iface) ){
			goto rx_up_pass;
		}
		
		/* 2) Do our best to parse the packet */
		/* 2.1) copy its contents to the buffer */
		pkt_raw_len = min(npkt->framelen, PKT_LOG_LEN);
		SETIOV( &iov, pkt_raw, pkt_raw_len );
		qxc_ion->memcpy_from_npkt(&iov,1,0,npkt,0,pkt_raw_len);
		
		
		/* 2.2) parse packet data - if error occured - block packet */
		if( !log_parse_pkt( &pkt_parsed, pkt_raw, pkt_raw_len, QXC_PKT_FROMCLI ) )
			goto rx_up_block;
		/* 2.3) add bytes counter */
		pkt_parsed.bytes_to_host = npkt->framelen;
		
		
		/* 3) Check if this packet is allowed to pass without taxes */
		pkt_exmpt = rule_match_exempt( &pkt_parsed );
		
		/* if everyone is allowed to use tax-exempted traffic - skip logging */
		if( pkt_exmpt == RET_RME_EVERYONE ){
			goto rx_up_pass;
		}
		
		/* 4) Check if this client may pass the filter */
		if( !rule_match_client( &pkt_parsed ) ){
			goto rx_up_block;
		}
		
		/* 5) If this was tax-exempted traffic for active users
		 * we also do not need logging */
		if( pkt_exmpt == RET_RME_ACTIVEONLY ){
			goto rx_up_pass;
		}
		
		/* 6) log packet to queue. If there is no room - block the packet. */
		if( !log_store_pkt( &pkt_parsed ) ){
			goto rx_up_block;
		}
		
		goto rx_up_pass;
	}
	
	
	
	/* if some interface is coming up - add it to the list */
	special = (uint16_t *)(npkt->buffers.tqh_first->net_iov->iov_base);
	if( *special == _IO_NET_MSG_DL_ADVERT ){
		/* we got some new interface - if it is of interest, store its IDs */
		io_net_msg_dl_advert_t *ad = (io_net_msg_dl_advert_t *)special;
		
		if( stricmp(ifname, ad->dl.sdl_data) == 0 ){
			interface.cell = cell;
			interface.endpoint = endpoint;
			interface.iface = iface;
		}
	}
	
	
	
	
	
	/* pass the packet upstairs */
rx_up_pass:
	if( qxc_ion->tx_up(qxc_reg_hdl,npkt,off,len_sub,cell,endpoint,iface)==0 )
		qxc_ion->tx_done(qxc_reg_hdl, npkt);
	return 0;
	
	/* block the packet fooling everybody it has passed */
rx_up_block:
	qxc_ion->tx_done(qxc_reg_hdl, npkt);
	return 0;
}













/*
 * Packets that try to leave our host are processed here
 */

int qxc_rx_down(npkt_t *npkt, void *func_hdl){
	
	uint8_t			pkt_raw[PKT_LOG_LEN];
	int				pkt_raw_len = 0;
	int				pkt_exmpt = 0;
	iov_t			iov;
	
	qx_log_t		pkt_parsed;
	
	/* we pass all internal messages without touching them */
	if( (npkt->flags & _NPKT_MSG) != 0 ) goto rx_down_pass;
	
	/* 1) Check if this interface is of interest */
	if( (interface.cell != npkt->cell) || (interface.endpoint != npkt->endpoint) || (interface.iface != npkt->iface) ){
		goto rx_down_pass;
	}
	
	/* 2) Do our best to parse the packet */
	/* 2.1) copy its contents to the buffer */
	pkt_raw_len = min(npkt->framelen, PKT_LOG_LEN);
	SETIOV( &iov, pkt_raw, pkt_raw_len );
	qxc_ion->memcpy_from_npkt(&iov,1,0,npkt,0,pkt_raw_len);
	
	/* 2.2) parse packet data - if error occured - block packet */
	if( !log_parse_pkt( &pkt_parsed, pkt_raw, pkt_raw_len, QXC_PKT_TOCLI ) )
		goto rx_down_block;
	/* 2.3) add bytes counter */
	pkt_parsed.bytes_to_cli = npkt->framelen;
	
	
	/* 3) Check if this packet is allowed to pass without taxes */
	pkt_exmpt = rule_match_exempt( &pkt_parsed );
		
	/* if everyone is allowed to use tax-exempted traffic - skip logging */
	if( pkt_exmpt == RET_RME_EVERYONE ){
		goto rx_down_pass;
	}
	
	/* 4) Check if this client may pass the filter */
	if( !rule_match_client( &pkt_parsed ) ){
		goto rx_down_block;
	}
	
	/* 5) If this was tax-exempted traffic for active users
	 * we also do not need logging */
	if( pkt_exmpt == RET_RME_ACTIVEONLY ){
		goto rx_down_pass;
	}
	
	/* 6) log packet to queue. If there is no room - block the packet. */
	if( !log_store_pkt( &pkt_parsed ) ){
		goto rx_down_block;
	}
	
	
	
	
	
	/* happily pass packet to its destination */
rx_down_pass:
	return qxc_ion->tx_down(qxc_reg_hdl, npkt);
	
	/* block the packet fooling everybody it has passed */
rx_down_block:
	qxc_ion->tx_done(qxc_reg_hdl, npkt );
	return TX_DOWN_OK;
}














































/*
 * We hold no packets - pass them all through
 */
int qxc_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl){
	return 0;
}





/*
 * We are called this function when io-net wants to unmount
 * our filter. We are still connected into io-net's pipe
 * and may prevent shutdown of filter.
 * But here we need to close all file handlers and release any
 * memory we allocated for work.
 */
int qxc_shutdown1(int registrant_hdl, void *func_hdl)
{
	/* we will clean up everything in qxc_shutdown2 as we do not hold
	 * any packets or foreign buffers for our own.
	 * It shoud be safe to clean everything later. */
	return EOK;
}





/*
 * We are called this function when io-net detached us
 * from its pipe and wants us to shutdown.
 * We need to stop all of our threads and free all our buffers.
 */
int qxc_shutdown2(int registrant_hdl, void *func_hdl)
{
	/* free logging queues */
	log_destroy();
		
	/* for each of the interfaces known - free active rule sets 
	 * and free interface information structures */
	
	return 0;
}





/* 
 * qxc resource manager thread
 */
void *qxc_rm_handle(void *arg){
	
	resmgr_connect_funcs_t	qxc_connect_funcs;
	resmgr_io_funcs_t		qxc_io_funcs;
	iofunc_attr_t			qxc_attr;
	
	resmgr_attr_t			qxc_rm_attr;
    dispatch_t				*qxc_rm_dpp;
    dispatch_context_t		*qxc_rm_ctp;
    int						id;
    
    /* initialize dispatch interface */
    if((qxc_rm_dpp = dispatch_create()) == NULL) {
        return NULL;
    }

    /* initialize resource manager attributes */
    memset(&qxc_rm_attr, 0, sizeof(qxc_rm_attr));
    qxc_rm_attr.nparts_max = 1;
    qxc_rm_attr.msg_max_size = QX_LOG_MSG_SIZE*log_pkts;	/* here we say how much data we'll transfer */

    /* initialize functions for handling messages */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &qxc_connect_funcs, 
                     _RESMGR_IO_NFUNCS, &qxc_io_funcs);

    qxc_io_funcs.devctl = qxc_rm_devctl; /* a handler to control device */

    /* initialize attribute structure used by the device */
    iofunc_attr_init(&qxc_attr, S_IFNAM | 0666, 0, 0);

    /* attach our device name */
    id = resmgr_attach(qxc_rm_dpp,			/* dispatch handle        */
                       &qxc_rm_attr,		/* resource manager attrs */
                       devname,				/* device name            */
                       _FTYPE_ANY,			/* open type              */
                       0,					/* flags                  */
                       &qxc_connect_funcs,	/* connect routines       */
                       &qxc_io_funcs,		/* I/O routines           */
                       &qxc_attr);			/* handle                 */
    if(id == -1) {
        return NULL;
    }

    /* allocate a context structure */
    qxc_rm_ctp = dispatch_context_alloc(qxc_rm_dpp);

    /* 
     * start the resource manager message loop
     */
    while(1) {
        if((qxc_rm_ctp = dispatch_block(qxc_rm_ctp)) == NULL) {
            return NULL;
        }
        dispatch_handler(qxc_rm_ctp);
    }
	return NULL;
}



/* End Of File */

