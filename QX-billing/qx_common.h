/*
 * -={ MikeP }=- (c) 2005
 */



/*
 * Data definition common both for io-net module and logger client
 */


#ifndef	_QX_COMMON_H_
#define _QX_COMMON_H_




/* for better statistics we count all the data flowing
 * through the interface */
typedef struct if_counters{
	uint32_t	rx_bytes;	/* bytes came in on the interface (total) */
	uint32_t	tx_bytes;	/* bytes went out on the interface (total) */
	uint32_t	rx_pkts;	/* packets came in on the interface (total) */
	uint32_t	tx_pkts;	/* packets went out on the interface (total) */
	uint32_t	rx_pkt_blk;	/* blocked incoming packets */
	uint32_t	tx_pkt_blk;	/* blocked outgoing packets */
	uint32_t	rx_pkt_msd;	/* missed incoming packets */
	uint32_t	tx_pkt_msd;	/* missed outgoig packets */
} if_counters_t;



#define	QX_CONF_FILE	"/etc/QX-billing.cnf"






#endif /* _QX_COMMON_H_ */

