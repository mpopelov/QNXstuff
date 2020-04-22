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
 * This is the header, containing definitions of commonly used structures\
 * and values
 */

#ifndef _NTC_COMMON_H_
#define _NTC_COMMON_H_

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
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

#include	"ntc_rm_dctl.h"






/* 
 * define type for MAC address
 */

#define MAC_ADDR_LENGTH 6

typedef uchar_t mac_addr_t[MAC_ADDR_LENGTH];





/* 
 * Here we describe the majority of headers 
 * we woul be interested in. As the most complete
 * description of packet we may need DLC, IP,
 * TCP, UDP, ICMP headers and some extra data:
 * the time the packet was captured and the size of packet
 * 
 * As maximum we may need to copy 54 bytes of
 * packet data - so our structure will be approximately
 * this long
 */

typedef struct _DLC_H{	/* DLC header of the packet */
	mac_addr_t	mac_dst;
	mac_addr_t	mac_src;
	uint16_t	EtherType;
} __attribute__((__packed__)) DLC_t;





/* 
 * we need the full IP header for logging and
 * data alignment.
 */

typedef struct _IP_H{	/* IP header of the packet */
	uint8_t		h_ver;		/* 4bits = version + 4bits = length of header */
	uint8_t		TOS;		/* Type of service */
	uint16_t	len;		/* (IP header + data) length */
	uint16_t	ID;
	uint16_t	f_offset;	/* fragment flags and offset */
	uint8_t		TTL;		/* time to live hops/seconds */
	uint8_t		proto;		/* protocol type of IP family */
	uint16_t	CRC;		/* checksum */	
	struct in_addr	ip_src;		/* source IP address */
	struct in_addr	ip_dst;		/* destination IP address */
} __attribute__((__packed__)) IP_t;





/* 
 * we need only the first 8 bytes - that's enough
 */

typedef struct _ICMP_H{
	uint8_t		type;
	uint8_t		code;
	uint16_t	CRC;
	uint16_t	ID;
	uint16_t	sn;			/* sequence number for type 8: ping */
} __attribute__((__packed__)) ICMP_t;





/* 
 * for data alignment we will copy only the first eight
 * bytes of TCP header - there's enough information
 * for us.
 */

typedef struct _TCP_H{
	uint16_t	src_port;
	uint16_t	dst_port;
	uint32_t	init_seq;	/* initial sequense number */
} __attribute__((__packed__)) TCP_t;





/* 
 * we will copy the whole UDP header - we have enough space
 */

typedef struct _UDP_H{
	uint16_t	src_port;
	uint16_t	dst_port;
	uint16_t	len;
	uint16_t	CRC;
} __attribute__((__packed__)) UDP_t;





/* 
 * here follows the packet information placeholder.
 * actually there is 54 bytes, but take in mind
 * alignment on DWORD boundry - you get 56 :+(
 */

typedef struct pkt_info{
	uint32_t	len;			/* total length of the packet	*/
	DLC_t		DLC;			/* DLC header					*/
	IP_t		IP;				/* IP header					*/
	union {						/* this union describes those	*/
		TCP_t	TCP;			/* parts of application layer	*/
		UDP_t	UDP;			/* protocols that would be of	*/
		ICMP_t	ICMP;			/* interest to us				*/
	} app;
	uint16_t	dir;			/* this shows the direction of packet: 0 = incoming, 1 = outgoing */
} __attribute__((__packed__)) pkt_info_t;

#define	PKT_LOG_SIZE 42
#define	PKT_DIR_IN	 0
#define PKT_DIR_OUT	 1


/* 
 * This defines how large the logging queue shoul be
 * (in pkt_info_t units)
 */

#define MAX_PACKETS_LOGGED 500



/*
 * This defines how many packets at once
 * we are allowed to read from our
 * pseudo-device ( resource )
 */

#define MAX_PKT_READ 50






/*
 * To add blocking functionality we should maintain
 * queue of MAC addresses we are about to block.
 * 
 * For the purpose of fast comparison we would
 * define MAC address in slightly other way:
 * 
 * instead of comparing the whole MAC in packet with the one, defined
 * in "block list" we will use the following assumption to make search faster:
 * 
 * as far as first 3 bytes of MAC are (usually) unique for each vendor
 * and last 3 uniqly dfine device node on the network we should take more
 * care about the LAST bytes when comparing and compare the FIRST bytes
 * only on case we have found similar LAST bytes.
 * 
 * It should be evident that in packet everything is
 * stored in network byte order and we will have LAST and FIRST bytes
 * changed their places respectively: LAST -> FIRST
 *                                    FIRST -> LAST
 */
 
typedef struct _mac_cmp{
	uint32_t	NID;	/* node ID - in norwal way these are the LAST bytes */
	uint16_t	VID;	/* vendor ID - these are FIRST bytes in normal way  */
	uint16_t	res;	/* this is for memory alignment purpose only but it makes dereferencing faster */
} __attribute__((__packed__)) mac_cmp_t;

typedef struct block_list{
	mac_cmp_t			block_addr;
	struct block_list *	next;
	struct block_list *	prev;
} block_list_t;

typedef struct block_info_struct{
	uint32_t	in_pkt;
	uint32_t	out_pkt;
} blkinfo_t;






/*
 * The following is a bit tricky.
 * We use the following structure to reflect interfaces
 * present in the system thus makinig it it possible to bypass
 * packet capturing on some cards:
 * I don't need to count traffic on my "external" interface en1
 * but it is obviously required on "internal" en0
 * 
 * Within io-net each "physical" interface is described
 * by three numbers: cell, endpoint, iface - to understand their meaning
 * one should be familiar with io-net's architecture.
 * See the "Network DDK" programmer's guide.
 */
typedef struct if_info{
	struct if_info * next;
	char		ifname[IFNAMSIZ];
	uint16_t	cell;
	uint16_t	endpoint;
	uint16_t	iface;
	uint8_t		ofinterest;
	uint8_t		dead;
} if_info_t;




#endif /* _NTC_COMMON_H_*/
