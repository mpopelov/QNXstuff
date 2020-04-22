/*
 * -=MikeP=- (c) 2005
 */
 
/*
 * This is a simple "nslookup" MX resolver
 */




#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <gulliver.h>





typedef struct {
	uint16_t	ID;
	uint16_t	Flags;
	uint16_t		QUEScount;
	uint16_t		ANSWcount;
	uint16_t		AUTHcount;
	uint16_t		ARECcount;
} dnshead_t;







char *	mxdomain = "yahoo.com";
char *	dnsip = "195.112.112.1";





int		sock;
struct sockaddr_in dns_srv;

unsigned char	MXmsg[512];





int main()
{
	int msglen;
	
	printf("Resolving MX for >>%s<<\n",mxdomain);
	printf("DNS server: %s\n",dnsip);
	
	inet_aton( dnsip, &dns_srv.sin_addr );
	dns_srv.sin_family = AF_INET;
	dns_srv.sin_port = htons(53);
	
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    	perror("sock:\n");
    	return (-1);
	}
	
	
	/* prepare DNS request */
	
	/* 1) Add header */
	{
		dnshead_t * MXh = (dnshead_t *) MXmsg;
		MXh->ID = ENDIAN_RET16(2);
		MXh->Flags = 1;
		MXh->QUEScount = ENDIAN_RET16(1);
		MXh->ANSWcount = 0;
		MXh->AUTHcount = 0;
		MXh->ARECcount = 0;
		
		msglen = sizeof(dnshead_t);
		printf("DNS header is %i bytes\n", msglen);
	}
	
	/* 2) Add question */
	MXmsg[msglen] = 0;
	memcpy( &MXmsg[msglen+1], mxdomain, strlen(mxdomain)+1 );
	{
		int i = msglen + strlen(mxdomain);
		unsigned char j = 0;
		msglen = i + 2;
		while(MXmsg[i]){
			if(MXmsg[i] == '.'){
				MXmsg[i] = j;
				j = 0;
			}else{
				j++;
			}
			i--;
		}
		MXmsg[i] = j;
	}
	printf("DNS query is %i bytes\n", msglen);
	
	/* 3) Add type and class */
	*((uint16_t *)&MXmsg[msglen]) = ENDIAN_RET16(15);
	msglen += 2;
	*((uint16_t *)&MXmsg[msglen]) = ENDIAN_RET16(1);
	msglen += 2;
	
	printf("Total query length: %i bytes\n", msglen);
	
	/* send it to server */
	sendto( sock, MXmsg, msglen, 0, (struct sockaddr *)&dns_srv, sizeof(struct sockaddr));
	msglen = recv( sock, MXmsg, 512, 0 );
	
	
	/* read answer from server */
	if(msglen<=0){
		printf("error reading reply from server or malformed reply\n");
	}
	
	{
		/* header related */
		char name[256];
		uint16_t id, flags, qcount, ancount, authcount, addcount;
		uint32_t ttl;
		uint16_t order;
		uint16_t class;
		uint16_t type;
		uint16_t length;
		unsigned char * here = MXmsg;
		
		printf("Got %i bytes from server\n",msglen);
		
		/* parse header */
		id = ENDIAN_RET16(*(uint16_t*)here); here +=2;
		flags = *(uint16_t*)here; here +=2;
		qcount = ENDIAN_RET16(*(uint16_t*)here); here +=2;
		ancount = ENDIAN_RET16(*(uint16_t*)here); here +=2;
		authcount = ENDIAN_RET16(*(uint16_t*)here); here +=2;
		addcount = ENDIAN_RET16(*(uint16_t*)here); here +=2;
		
		printf("ID = %hi\nFLAGS = %hi\nQUESTIONS = %hi\nANSWERS = %hi\nAUTHORITIES = %hi\nADDITIONAL = %hi\n",
		       id,flags,qcount,ancount,authcount,addcount);
		
		/* decode query */
		for(qcount;qcount>0;qcount--){
			uint8_t i = 0;
			uint8_t j = 0;
			uint8_t k = 0;
			
			i = here - MXmsg;
			
			while( MXmsg[i] ){
				j = MXmsg[i];
				/* query can not be compressed */
				for(j;j>0;j--){
					i++;
					name[k] = MXmsg[i];
					k++; here++;
				}
				name[k] = '.';
				k++;
				i++;
				here++;
			}
			name[k-1] = 0;
			here++;
			type = ENDIAN_RET16(*(uint16_t*)here); here += 2;
			class = ENDIAN_RET16(*(uint16_t*)here); here += 2;
			printf("Zone: %s (Type = %hi, Class = %hi)\n",name,type,class);
		}
		
		/* decode answers, if any */
		for(ancount;ancount>0;ancount--){
			
			/* "here" points to current location in message */
			uint16_t i = 0;
			uint16_t j = 0;
			uint8_t k = 0;
			
			/* move "here" pointer to the end of NAME section */
			while(1){
				if(*(uint8_t*)here == 0xc0){
					here += 2;
					break;
				}
				if(*here == 0x00){
					here++;
					break;
				}
				here++;
			}
			
			/* do we need name? if we asked one question we'll surely get
			 * answers to the only question */
			/* get answer parameters */
			type = ENDIAN_RET16(*(uint16_t*)here); here += 2;
			class = ENDIAN_RET16(*(uint16_t*)here); here += 2;
			ttl = ENDIAN_RET32(*(uint32_t*)here); here += 4;
			length = ENDIAN_RET16(*(uint16_t*)here); here += 2;
			
			order = ENDIAN_RET16(*(uint16_t*)here); here += 2;
			
			/* now scan the name in the answer */
			i = here - MXmsg;
			here += (length - 2);
			
			do{
				if((MXmsg[i] & 0xc0) == 0xc0){
					i = (0x3fff & ENDIAN_RET16(*((uint16_t*)&MXmsg[i])));
				}else{
					k = MXmsg[i];
					if(k==0) break;
					i++;
					for(k;k>0;k--){
						name[j] = MXmsg[i];
						i++;j++;
					}
					name[j] = '.'; j++;
				}
			}while(j<256);
			
			if(j) j--;
			name[j] = 0;
				
			printf("ANSWER:\n",name);
			printf("Type = %hi, Class = %hi\n",type,class);
			printf("TTL = %hi, Length = %hi, Order = %hi\n",ttl,length,order);
			printf("RDName = %s\n",name);
		}
		
		/* decode authority records, if any */
		for(authcount;authcount>0;authcount--){
			printf("Authority:\n");
		}
		
		/* decode additional records, if any */
		for(addcount;addcount>0;addcount--){
			printf("Additional:\n");
		}
		
	}
	
	printf("\nBye!\n");
	
	return 0;
}
