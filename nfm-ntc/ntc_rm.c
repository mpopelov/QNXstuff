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
 * Resource manager interfaces for /dev/ntc
 * are described here
 */

#include	"ntc_common.h"

/* queue mutex */
extern pthread_mutex_t	pkt_mutex;
extern pthread_rwlock_t	pkt_rwlock;
extern pthread_rwlock_t	if_rwlock;

/* the descriptor of our packet logging queue */
extern pkt_info_t * pktq;
extern if_info_t  *	ntc_ifs;
extern block_list_t * blocklist;
extern int	num_packets;				/* number of packets in the queue */
extern int	num_pkt_missed;				/* number of missed packets       */
extern blkinfo_t	num_pkt_blocked;	/* number of blocked packets      */

/*#define MAX_PACKETS_LOGGED 100*/
extern int	pkt_log_limit; /* how many packets to log */
extern io_net_self_t * ntc_ion;	/* io-net callback entries */




/* handle _IO_READ message */
int ntc_rm_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
	int		nparts;

	/* we support no special types! */
	if (msg->i.xtype & _IO_XTYPE_MASK != _IO_XTYPE_NONE)
		return (_RESMGR_ERRNO(ENOSYS));
	
     /* now return the packet descriptor from the queue */
     pthread_mutex_lock(&pkt_mutex);

	/* check if client has enough space to read from /dev/ntc */
	if( (msg->i.nbytes < sizeof(pkt_info_t)) || (num_packets == 0) ){
		
		pthread_mutex_unlock(&pkt_mutex);

		/* return EOF */
		_IO_SET_READ_NBYTES(ctp, 0);
		nparts = 0;
	}else{

		/* get how many packets we are supposed to copy to buffer */
		int		npkts = min(MAX_PKT_READ , msg->i.nbytes/sizeof(pkt_info_t));
		npkts = min( npkts, num_packets);
		
		/* decrease the number of packets by number of copied */
		num_packets -= npkts;

		/* now copy data to client's buffer  */
		SETIOV(ctp->iov, &pktq[num_packets], npkts*sizeof(pkt_info_t));
		_IO_SET_READ_NBYTES(ctp, npkts*sizeof(pkt_info_t));

		pthread_mutex_unlock(&pkt_mutex);
		nparts = 1;
	}
	
	if(msg->i.nbytes >0 ){
		ocb->attr->flags |= IOFUNC_ATTR_ATIME;
	}
	
	return _RESMGR_NPARTS(nparts);
}





/* handle _IO_DEVCTL message 
 * NTC_FLUSH - emty logging queue
 * NTC_NUMMISSED - get the number of packets we  missed
 * NTC_QUEUELEN - get the number of packets in queue
 */
int ntc_rm_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *o)
{
	int ret;
	int retval = 0;
	int nbytes = 0;
	char * data = _DEVCTL_DATA(msg->i);
	
	ret = EOK;
	
	switch(msg->i.dcmd){
		
		case NTC_FLUSH:
		{
			/*we are asked to flush the queue - get it */
			pthread_mutex_lock(&pkt_mutex);
			num_packets = 0;
			pthread_mutex_unlock(&pkt_mutex);
			
			/* now we return happily */
			nbytes = 0;
			break;
		}
			
		case NTC_NUMMISSED:
		{
			/* we are asked for ammount of missed packets */
			*(int *)data = num_pkt_missed;
			
			/* say we returned sizeof(int) */
			nbytes = sizeof(int);
			break;
		}
		
		case NTC_NUMBLOCKED:
		{
			/* we are asked for ammount of blocked packets */
			((blkinfo_t *)data)->in_pkt = num_pkt_blocked.in_pkt;
			((blkinfo_t *)data)->out_pkt = num_pkt_blocked.out_pkt;

			
			/* say we returned sizeof(int) */
			nbytes = sizeof(blkinfo_t);
			break;
		}
			
		case NTC_QUEUELEN:
		{
			/* we are asked for ammount packets in queue */
			*(int *)data = num_packets;
			
			/* say we returned sizeof(int) */
			nbytes = sizeof(int);
			break;
		}
		
		case NTC_ADDBLOCK:
		{
			/* make a new entry */
			block_list_t * blist = (block_list_t *) malloc( sizeof(block_list_t) );
			blist->next = NULL;
			blist->prev = NULL;
			
			/* add some mac address on top of block list */
			blist->block_addr.NID = ((mac_cmp_t *)data)->NID;
			blist->block_addr.VID = ((mac_cmp_t *)data)->VID;
			
			/* now add the rule on top of the rules list:
			 * We should minimize the time of write-locking
			 * of blocklist as this will prevent ALL the threads from processing
			 * their packets - so we know that the only place where the queue is
			 * modified - it is here!
			 * We do some checking and tuning of new HEAD without locking and
			 * lock resource only when adding new HEAD
			 * Can't see why it will not work :+)
			 */
			if( blocklist != NULL ) blist->next = blocklist;
			
			pthread_rwlock_wrlock( &pkt_rwlock );
			blocklist = blist;			
			pthread_rwlock_unlock( &pkt_rwlock );
			
			nbytes = 0;
			
			break;
		}
		
		case NTC_DELBLOCK:
		{
			block_list_t * blist = blocklist;
			int	res = 0;
			
			/* remove some mac address from block list
			 * first - find that very rule 
			 */
			while( blist != NULL ){
				
				/* first we compare node ID */
				if( ((mac_cmp_t *)data)->NID == blist->block_addr.NID ){
					/* we MUST be sure that vendor ID's are also equal */
					if( ((mac_cmp_t *)data)->VID == blist->block_addr.VID ){
						/* YES - that's it! */
						res = 1;
						break;
					}
				}
				
				/* if this rule didn't match we go forth */
				blist = blist->next;
			}
			
			/* now we need to delete the node found */
			if( res ==1 ){
				
				/* to see why we do something without blocking
				 * see comments in adding rules procedure.
				 * take in consideration that the block list is
				 * one-way list and we need "prev" pointer only here
				 * so we may freely change it without locking
				 */
				if( blist->next != NULL ) blist->next->prev = blist->prev;
				
				pthread_rwlock_wrlock( &pkt_rwlock );
				
				/* if we delete the first item in the list */
				if( blist->prev == NULL ){
					blocklist = blist->next;
				}else{
					/* we delete not the head item */
					blist->prev->next = blist->next;
				}
				
				pthread_rwlock_unlock( &pkt_rwlock );
				
				/* now free list item */
				free( blist );
			}

			nbytes = 0;
			
			break;
		}
		
		case NTC_IFPRESENT:
		{
			if_info_t * ifs = ntc_ifs;
			int num_ifs = 0;
			
			/* count the number of interfaces in our list */
			pthread_rwlock_rdlock( &if_rwlock );
			while( ifs != NULL ){
				num_ifs++;
				ifs = ifs->next;
			}
						
			retval = num_ifs;	/* in return value we show how many interfaces are present in the system */
			
			
			num_ifs = 0;
			ifs = ntc_ifs;
			{
				int siz = msg->i.nbytes;
				
				/* copy as many descriptors as we can */
				while( (num_ifs < retval) && ( siz >= sizeof(if_info_t) ) ){
					
					memcpy( (data + num_ifs*sizeof(if_info_t) ), ifs, sizeof(if_info_t) );
					
					num_ifs++;
					ifs = ifs->next;
					siz -= sizeof(if_info_t);
				}
			}
			
			pthread_rwlock_unlock( &if_rwlock );
			nbytes = num_ifs * sizeof(if_info_t);
			
			break;
		}
		
		case NTC_ADDIF:
		{
			if_info_t * ifs = ntc_ifs;
			retval = 0;
			
			pthread_rwlock_rdlock( &if_rwlock );
			while( ifs != NULL ){
				if( stricmp( ifs->ifname, data ) == 0 ){
					ifs->ofinterest = 1;
					retval = 1;
					break;
				}
				ifs = ifs->next;
			}
			pthread_rwlock_unlock( &if_rwlock );
			
			nbytes = 0;
			break;
		}
		
		case NTC_DELIF:
		{
			if_info_t * ifs = ntc_ifs;
			retval = 0;
			
			pthread_rwlock_rdlock( &if_rwlock );
			while( ifs != NULL ){
				if( stricmp( ifs->ifname, data ) == 0 ){
					ifs->ofinterest = 0;
					retval = 1;
					break;
				}
				ifs = ifs->next;
			}
			pthread_rwlock_unlock( &if_rwlock );
			
			nbytes = 0;
			break;
		}
		
		case NTC_EXIT:
		default:
			return ENOSYS;
	}
	
	memset(&msg->o, 0, sizeof(msg->o) );
	msg->o.ret_val = retval;
	msg->o.nbytes = nbytes;
	return(_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o)+nbytes ));
}
