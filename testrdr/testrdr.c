/* testrdr.c */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <ctype.h>
#include <nlist.h>
/*#include "netinet/ip_compat.h"*/
/*#include "netinet/ip_fil.h"*/
/*#include "netinet/ip_nat.h"*/
/*#include "netinet/ip_state.h"*/
/*#include "netinet/ip_proxy.h"*/
/*#include "ipf.h"
#include "kmem.h"*/

#include	<gulliver.h>

/* For IPFILTER SUPPORT */
#define	u_int32_t	uint32_t
#define	u_int16_t	uint16_t
#define	u_int8_t	uint8_t

#include	<sys/ioctl.h>
#include	<net/if.h>
#include	<netinet/ip_compat.h>
#include	<netinet/ip_fil.h>
#include	<netinet/ip_nat.h>







struct sockaddr_in	bind_addr;
struct sockaddr_in	from_addr;
int	l_sock;
int p_sock;
int natfd;


int main(){
	
	struct natlookup natLookup, *natLookupP = &natLookup;
	int opt=1;
	
	
	
	
	printf("Starting test listener\n");
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(180);
	inet_aton( "10.0.1.5", &(bind_addr.sin_addr) );
	
	if((l_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("socket: %s\n",strerror(errno));
		return 0;
	}
	setsockopt(l_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(l_sock, (struct sockaddr *)&(bind_addr), sizeof(struct sockaddr_in)) < 0){
		printf("bind: %s\n",strerror(errno));
		return 0;
	}
	if(listen(l_sock, 5) != 0){
		printf("listen: %s\n",strerror(errno));
		return 0;
	}
	
	{
		int snt = SIOCGNATL & 0xff;
		printf("NAT Command version: %d\n",snt);
	}
	
	
	/* now accept the connection */
	printf("Waiting for connection\n");
	opt = sizeof(struct sockaddr);
	p_sock = accept(l_sock,(struct sockaddr*)&from_addr,&opt);
	
	
	printf("Accepted connection:\n");
	printf("Host: %s @ %hu\n", inet_ntoa(bind_addr.sin_addr), ntohs(bind_addr.sin_port));
	printf("Peer: %s @ %hu\n", inet_ntoa(from_addr.sin_addr), ntohs(from_addr.sin_port));
	printf("Try to determine real destination of peer\n");
	
	
	/* ask IPFilter about redirections */
   	natLookup.nl_inport = bind_addr.sin_port;
   	natLookup.nl_outport = from_addr.sin_port;
   	natLookup.nl_inip = bind_addr.sin_addr;
   	natLookup.nl_outip = from_addr.sin_addr;
   	natLookup.nl_flags = IPN_TCP; /*IPN_TCPUDP*/
   	
   	natLookup.nl_realip.s_addr = 0;
   	natLookup.nl_realport = 0;
	
	natfd = open(IPL_NAT, O_RDONLY, 0);
	if (natfd < 0) {
		printf("Error opening NAT: %s\n", strerror(errno));
		return 0;
	}
	
	/*opt = ioctl(natfd, SIOCGNATL, &natLookupP); /* natLookupP */
	opt = devctl(natfd, SIOCGNATL, natLookupP, sizeof(natLookup), NULL);
	if( opt!=0 ){
		printf("NAT Query failed: %s (%d)\n", strerror(errno), opt);
		return 0;
	}
	
	printf("NAT Lookup resulted in (%i)\n",opt);
	printf("Dest: %s @ %hu\n", inet_ntoa(natLookup.nl_realip), ntohs(natLookup.nl_realport));
	
	return 1;
}
