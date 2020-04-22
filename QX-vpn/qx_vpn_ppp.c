/*
 * -={ MikeP }=-
 */


#include	"qx_vpn_common.h"



/*
 * PPP processing goes here
 */
int handle_ppp(client_t *client){
	int		result = 0;
	int		datalen = 0;
	uint8_t	*ppp_in = client->pkt_in;
	uint8_t	*ppp_out = client->pkt_out;
	gre_packet_t	*gp = NULL;
	uint16_t	*PPP_proto = NULL;
	
	/* try to get data from the socket */
	result = recv(client->raw_sock, ppp_in, 1514, 0);
	
	/* in case of error - return silently */
	if( result<0 ) return 0;
	
	/* we get the full packet with IP header from RAW sockets */
	/* 1) find GRE header in the packet: we look into the IP version/length
	 * field and determine the offset to GRE */
	{
		uint8_t IPvl = ppp_in[0];
		if( (IPvl & 0xF0) != 0x40 ){
			qx_vpn_log(QLL_ERROR,"IP protocol version != 4\n");
			return 0;
		}
		gp = (gre_packet_t*)&ppp_in[(IPvl&0x0F)<<2];
	}
	
	/* perform sanity check */
	if( (gp->flags & GRE_SANITY_MASK) != GRE_SANITY_VALUE ){
		qx_vpn_log(QLL_ERROR,"Sanity check failed. GRE Flags: %hx & %hx != %hx\n",gp->flags,GRE_SANITY_MASK,GRE_SANITY_VALUE);
		/* silently discard packet */
		return 0;
	}
	if(gp->protocol != GRE_PROTOCOL){
		qx_vpn_log(QLL_ERROR,"Wrong proto in GRE packet\n");
		/* silently discard packet */
		return 0;
	}
	if(gp->key_lw != client->call_id){
		qx_vpn_log(QLL_ERROR,"Wrong call ID for the session: %hu\n",gp->key_lw);
		return 0;
	}
	datalen = ENDIAN_BE16(gp->key_hw);
	qx_vpn_log(QLL_INFO,"GRE call ID: %hu\n",client->call_id);
	qx_vpn_log(QLL_INFO,"GRE bytes: %i effective, %i total\n",datalen,result);
	
	if(gp->flags & GRE_FL_A){
		uint32_t	snd_ack;
		snd_ack = ENDIAN_BE32( (gp->flags & GRE_FL_S ? gp->dat2 : gp->dat1) );
		if((snd_ack<client->snd_ack) || (snd_ack>=client->snd_num)){
			qx_vpn_log(QLL_WARNING,"Peer sent wrong ACK number\n");
			return 0;
		}
		qx_vpn_log(QLL_INFO,"Peer ACK-ed packets up to %u\n",snd_ack);
		client->snd_ack = snd_ack;
	}
	
	/* Now look into data within the packet and say what we are supposed to do */
	if(!(gp->flags & GRE_FL_S)){
		/* no data present */
		return 0;
	}else{
		uint32_t	rcv_num = ENDIAN_BE32(gp->dat1);
		qx_vpn_log(QLL_INFO,"Sequence number present: %u\n",rcv_num);
		if( rcv_num < client->rcv_num ){
			qx_vpn_log(QLL_WARNING,"GRE: Duplicate or out of order packet - ignoring\n");
			return 0;
		}else{
			client->rcv_num = rcv_num;
		}
	}
	
	/* now process packet */
	if(gp->flags & GRE_FL_A){
		ppp_in = (uint8_t*)&gp->dat3;
	}else{
		ppp_in = (uint8_t*)&gp->dat2;
	}
	
	
	/* Sanity chek - is this really PPP packet? */
	if((ppp_in[0] != 0xFF) || (ppp_in[1] != 0x03)){
		qx_vpn_log(QLL_ERROR,"Wrong formatted or not a PPP packet\n");
		return 0;
	}
	
	PPP_proto = (uint16_t*)&ppp_in[2];
	ppp_in += 4;
	/* calculate the length of remaining data in the packet */
	result = result - (ppp_in - client->pkt_in);
	
	/* Prepare the packet that we are going to send back to the client.
	 * 
	 * We supply processing function with the following info:
	 * - pointer to the data of appropriate packet.
	 * - length of incoming data
	 * - pointer to where we may store outgoing buffer
	 * - remaining length in output buffer
	 * - client descriptor
	 * 
	 * We expect handling function to return number of bytes
	 * that it placed in the buffer.
	 * If it placed 0 bytes - we simply send back ACK to the packet.
	 * 
	 * So first we place GRE header into the output buffer.
	 */
	
	/* put GRE header into the output buffer */
	gp = (gre_packet_t*)&ppp_out;
	gp->protocol = GRE_PROTOCOL;
	gp->key_lw = client->call_id; /* ? */
	gp->flags = GRE_RESPONSE_FLAGS;	/* we assume there will be ACK number */
	ppp_out = (uint8_t*)&gp->dat3;
	ppp_out[0] = 0xFF; ppp_out[1] = 0x03;
	ppp_out += 2;
	
	switch(*PPP_proto){
		case PPPPROTO_IP:
			qx_vpn_log(QLL_INFO,"IP packet\n");
			/* call function to handle the IP message */
			break;
		case PPPPROTO_LCP:
			qx_vpn_log(QLL_INFO,"LCP packet\n");
			/* handle LCP message */
			/* datalen = handle_LCP(ppp_in, bytes_in, ppp_out, bytes_out, client ); */
			break;
		case PPPPROTO_IPCP:
			qx_vpn_log(QLL_INFO,"IPCP packet\n");
			break;
		case PPPPROTO_ECP:
			qx_vpn_log(QLL_INFO,"ECP packet\n");
			break;
		case PPPPROTO_CCP:
			qx_vpn_log(QLL_INFO,"CCP packet\n");
			break;
		case PPPPROTO_PAP:
			qx_vpn_log(QLL_INFO,"PAP packet\n");
			break;
		case PPPPROTO_CHAP:
			qx_vpn_log(QLL_INFO,"CHAP packet\n");
			break;
		default:
			qx_vpn_log(QLL_WARNING,"Unsupported protocol: %hx\n",*PPP_proto);
			/* send protocol reject */
			break;
	}
	
	
	/* After all we check if function placed something into the output buffer.
	 * If so - we add the length of payload and ACK number to packet. Add
	 * sequence number and send response back */
	
	if(datalen != 0){
		/* they placed something to the buffer */
		gp->key_hw = ENDIAN_BE16(datalen + 2);
		gp->flags |= GRE_FL_S;	/* we also add sequence number */
		gp->dat1 = ENDIAN_BE32(client->snd_num);
		gp->dat2 = ENDIAN_BE32(client->rcv_num);
		client->snd_num++;
		datalen += 18;
		
	}else{
		/* nothing was placed into the buffer - send simple ACK */
		gp->key_hw = 0;
		/* there was no data - so no sequence number - just add ACK */
		gp->dat1 = ENDIAN_BE32(client->rcv_num);
		datalen = 12;
	}
	
	/* we are done - return */
	send(client->raw_sock,client->pkt_out,datalen,0);
	
	return 0;
}
