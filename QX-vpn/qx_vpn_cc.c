/*
 * -={ MikeP }=- (c) 2005
 */


/*
 * Here comes implementation of control connection
 */


#include	"qx_vpn_common.h"




uint32_t	CallID = 0;



/*
 * This function handles control connection messages.
 * Returns:
 *  0 - message handled correctly
 * -1 - message caused error - connection to be closed.
 */
int handle_ccm(client_t *client){
	cc_message_t	message;
	int				result = 0;
	int				sendsize = 0;
	
	/* try to get data from the socket */
	result = recv(client->cc_sock, &message, sizeof(cc_message_t), 0);
	
	/* in case of signal - return silently */
	if( (result<0) && (errno==EINTR) ) return 0;
	/* in case of error - clear connection */
	if(result <= 0) return -1;
	
	/* First sanity checks */
	if( result != ENDIAN_BE16(message.msglen) ){
		qx_vpn_log(QLL_ERROR,"Control message length and received length do not match\n");
		return -1;
	}
	if( MAGIC_COOKIE != message.cookie ){
		qx_vpn_log(QLL_ERROR,"TCP stream out of sync\n");
		return -1;
	}
	if( message.pptpm_type != PPTPMT_CM ){
		qx_vpn_log(QLL_ERROR,"Unknown PPTP control message\n");
		return -1;
	}
	if( message.reserved != 0 ){
		qx_vpn_log(QLL_ERROR,"RESERVED filed != 0\n");
		return -1;
	}
	
	
	/* 
	 * now switch types of messages and do the job 
	 */
	switch( message.ccm_type ){
		case CMT_STARTREQ:
			{
				ccm_startreq_t	*msgreq = (ccm_startreq_t*)message.data;
				ccm_startrep_t	*msgrep = (ccm_startrep_t*)message.data;
				
				qx_vpn_log(QLL_INFO,"start-control-connection-request\n");
				
				if(msgreq->reserved != 0) return -1;
				
				/* check if we are in the IDLE state - otherwise we reject */
				if( client->cc_state != CC_STATE_IDLE ){
					/* If the state is not idle - we send back CHANNEL ALREADY EXISTS */
					msgrep->reserc = STARTREP_RC_CE;
					result = 0;
				}else{
					
					/* if we meet unsupported protocol version */
					if( ENDIAN_BE16(msgreq->pver) < PPTP_PROTO_VERSION ){
						msgrep->reserc = STARTREP_RC_PE;
						result = -1;
					}else{
						msgrep->reserc = STARTREP_RC_OK;
						client->cc_state = CC_STATE_ESTABLISHED;
						client->cc_cookie = message.cookie;
						result = 0;
					}
				}
				
				/* fill in the rest of data in response */
				msgrep->pver = ENDIAN_BE16(PPTP_PROTO_VERSION);
				msgrep->framing = ENDIAN_BE32(3);
				msgrep->bearer = 0;
				msgrep->maxchan = 0;
				msgrep->fwver = ENDIAN_BE16(1);
				snprintf(msgrep->hname,64,"qnx.catnet.kaluga.org");
				snprintf(msgrep->vname,64,QX_VPN_VENDOR_STRING);
				message.ccm_type = CMT_STARTREP;
				
				/* send back start-control-connection-reply */
				sendsize = sizeof(ccm_startrep_t) + 12;
				break;
			}
		case CMT_STARTREP:
			{
				/* We should never get this type of message */
				qx_vpn_log(QLL_ERROR,"Unexpected STARTREP\n");
				return -1;
			}
		case CMT_STOPREQ:
			{
				ccm_stopreq_t	*msgreq = (ccm_stopreq_t*)message.data;
				ccm_stoprep_t	*msgrep = (ccm_stoprep_t*)message.data;
				
				qx_vpn_log(QLL_INFO,"stop-control-request\n");
				
				if(msgreq->reserved != 0) return -1;
				
				qx_vpn_log(QLL_INFO,"Reason for closing: %hu\n",msgreq->reason);
				
				msgrep->reserc = STOPREP_RC_OK;
				
				message.ccm_type = CMT_STOPREP;
				sendsize = sizeof(ccm_stoprep_t) + 12;
				result = -1; /* close connection */
				break;
			}
		case CMT_STOPREP:
			{
				/* We should never get this type of message */
				qx_vpn_log(QLL_ERROR,"Unexpected STOPREP\n");
				return -1;
			}
		case CMT_ECHOREQ:
			{
				qx_vpn_log(QLL_INFO,"echo-request\n");
				break;
			}
		case CMT_ECHOREP:
			{
				qx_vpn_log(QLL_INFO,"echo-reply\n");
				break;
			}
		case CMT_OCALLREQ:
			{
				ccm_ocallreq_t	*msgreq = (ccm_ocallreq_t*)message.data;
				ccm_ocallrep_t	*msgrep = (ccm_ocallrep_t*)message.data;
				
				if(msgreq->reserved != 0) return -1;
				
				qx_vpn_log(QLL_INFO,"outgoing-call-request...\n");
				qx_vpn_log(QLL_INFO,"call_id: %hu, call_sn: %hu\n",msgreq->call_id,msgreq->call_sn);
				qx_vpn_log(QLL_INFO,"min_bps: %u, max_bps: %u\n",ENDIAN_BE32(msgreq->min_bps),ENDIAN_BE32(msgreq->max_bps));
				
				result = 0;
				
				if(client->cc_state != CC_STATE_ESTABLISHED){
					/* we accept this kind of messages only when
					 * we are in ESTABLISHED STATE */
					msgrep->reserc = OCALLREP_RC_GE | CC_GE_NC;
					result = -1;
					qx_vpn_log(QLL_ERROR,"Not in established state\n");
					
				}else if((client->cc_call_id != 0) && (client->cc_call_sn != 0)){
					/* this means we already have outgoing call established */
					msgrep->reserc = OCALLREP_RC_NA;
					qx_vpn_log(QLL_ERROR,"Call already established\n");
					
				}else if( (client->raw_sock = socket(AF_INET,SOCK_RAW,IPPROTO_GRE)) <0 ){
					qx_vpn_log(QLL_ERROR,"GRE socket: %s\n",strerror(errno));
					msgrep->reserc = OCALLREP_RC_NC;
				}else{
					struct sockaddr_in raw_addr;
					raw_addr.sin_port = 0;
					raw_addr.sin_family = AF_INET;
					raw_addr.sin_addr = client->cli_addr.sin_addr;
					if(0>connect(client->raw_sock,(struct sockaddr*)&raw_addr,sizeof(raw_addr))){
						qx_vpn_log(QLL_ERROR,"GRE connect: %s\n",strerror(errno));
						msgrep->reserc = OCALLREP_RC_GE | CC_GE_NR;
						close(client->raw_sock);
						client->raw_sock = -1;
					}else if(0 > select_attach(dpp_tcp,&attr_tcp,client->raw_sock,SELECT_FLAG_READ|SELECT_FLAG_REARM,cc_action,client)){
						msgrep->reserc = OCALLREP_RC_GE | CC_GE_NR;
						close(client->raw_sock);
						client->raw_sock = -1;
					}else{
						msgrep->reserc = OCALLREP_RC_OK;
					}
				}
				
				client->cc_call_id = msgreq->call_id;
				client->cc_call_sn = msgreq->call_sn;
				client->call_id = 3;/*atomic_add_value(&CallID,3);*/
				
				msgrep->call_id = client->call_id;
				msgrep->call_id_peer = client->cc_call_id;
				msgrep->cause = 0;
				msgrep->speed = ENDIAN_BE32(100000000);
				msgrep->pkt_window_size = ENDIAN_BE16(8);
				msgrep->pkt_delay = ENDIAN_BE16(10);
				msgrep->pchan_id = 0;
				message.ccm_type = CMT_OCALLREP;
				sendsize = sizeof(ccm_ocallrep_t) + 12;
				message.msglen = ENDIAN_BE16(sendsize);
				
				break;
			}
		case CMT_OCALLREP:
			{
				/* We should never get this type of message */
				qx_vpn_log(QLL_ERROR,"Unexpected OCALLREP\n");
				return -1;
			}
		case CMT_ICALLREQ:
			{
				/* We should never get this type of message */
				qx_vpn_log(QLL_ERROR,"Unexpected ICALLREQ\n");
				return -1;
			}
		case CMT_ICALLREP:
			{
				/* We should never get this type of message */
				qx_vpn_log(QLL_ERROR,"Unexpected ICALLREP\n");
				return -1;
			}
		case CMT_ICALLCON:
			{
				/* We should never get this type of message */
				qx_vpn_log(QLL_ERROR,"Unexpected ICALLCON\n");
				return -1;
			}
		case CMT_CCLEAREQ:
			{
				ccm_ccleareq_t	*msgreq = (ccm_ccleareq_t*)message.data;
				ccm_cdiscnfy_t	*msgrep = (ccm_cdiscnfy_t*)message.data;
				
				qx_vpn_log(QLL_INFO,"call-clear-request\n");
				
				if(msgreq->reserved != 0) return -1;
				
				if(msgreq->call_id != client->cc_call_id){
					qx_vpn_log(QLL_WARNING,"Attempt to close non-existant call: %hu\n",msgreq->call_id);
					msgrep->reserc = CDISCNFY_RC_GE | CC_GE_BC;
				}else{
					msgrep->reserc = CDISCNFY_RC_RQ;
				}
				
				msgrep->cause = 0;
				msgrep->reserved = 0;
				msgrep->stats[0] = 0;
				
				sendsize = sizeof(ccm_cdiscnfy_t) + 12;
				message.msglen = ENDIAN_BE16(sendsize);
				message.ccm_type = CMT_CDISCNFY;
				
				/* clear call data */
				client->cc_call_id = 0;
				client->cc_call_sn = 0;
				client->call_id = 0;
				
				/* update call statistics in database */
				
				result = 0;
				
				break;
			}
		case CMT_CDISCNFY:
			{
				/* We should never get this type of message */
				qx_vpn_log(QLL_ERROR,"Unexpected CDISCNFY\n");
				return -1;
			}
		case CMT_WANERNFY:
			{
				/* We should never get this type of message */
				qx_vpn_log(QLL_ERROR,"Unexpected WANERNFY\n");
				return -1;
			}
		case CMT_SLINKNFO:
			{
				ccm_slinknfo_t	*msgreq = (ccm_slinknfo_t*)message.data;
				
				qx_vpn_log(QLL_INFO,"set-link-info\n");
				
				if(msgreq->reserved != 0){
					/* send control-connection-stop-request ? */
					return -1;
				}
				
				/* only allowed if state is ESTABLISHED and outgoing call was populated */
				if((client->cc_state == CC_STATE_ESTABLISHED) &&
					(client->cc_call_id != 0) && (client->cc_call_sn != 0) ){
					client->saccm = msgreq->send_accm;
					client->raccm = msgreq->recv_accm;
					result = 0;
				}else{
					printf("not established or no call\n");
					result = -1;
				}
				return result;
			}
	}
	
	send(client->cc_sock,&message,sendsize,0);
	return result;
}
