/*
 * 2005 (c) -={ MikeP }=-
 */


#include	"qxc_common.h"


/*
 * Handling devctl() messages takes place here
 */

extern qxc_if_info_t interface;


/*
 * handle _IO_DEVCTL message
 */
int qxc_rm_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *o)
{
	int		retval	= -1;
	int		nbytes	= 0;
	uint8_t	*data	= _DEVCTL_DATA(msg->i);
	
	switch(msg->i.dcmd){
		
		case QXC_ADD_TE:
		{
			/* check the size of buffer */
			if( msg->i.nbytes >= sizeof(qx_rule_exempt_t) ){
				/* add one more tax exemption rule */
				retval = rule_add_exempt( (qx_rule_exempt_t*)data );
			}
			/* now we return happily */
			break;
		}
			
		case QXC_ADD_NIC:
		{
			/* check the size of buffer */
			if( msg->i.nbytes >= sizeof(qx_rule_nics_t) ){
				/* add one more NIC */
				retval = rule_add_nic( (qx_rule_nics_t*)data );
			}
			/* now we return happily */
			break;
		}
		
		case QXC_GET_INFO:
		{
			if( msg->i.nbytes >= sizeof(qxc_if_info_t) ){
				/* we are asked for interface stats */
				nbytes = sizeof(qxc_if_info_t);
				retval = 1;
				memcpy( data, &interface, sizeof(qxc_if_info_t) );
			}
			/* now we return happily */
			break;
		}
			
		case QXC_SWAP_GET_LOG:
		{
			int	log_rgn	= 0;
			int	log_cnt	= 0;
			
			/* we are asked to swap rules and return logged packets */
			
			/* swap log buffers */
			log_swap_log(&log_rgn, &log_cnt);
			
			/* swap rule lists */
			rule_swap_lists();
			
			/* determine the ammount of memory in clients buffer
			 * available for data copyng. We will try to copy
			 * as many bytes as possible to store full log structures.
			 */
			*((int*)data) = log_rgn;
			retval = log_cnt;
			nbytes = sizeof(log_rgn);
			
			break;
		}
		
		default:
			return ENOSYS;
	}
	
	memset(&msg->o, 0, sizeof(msg->o) );
	msg->o.ret_val = retval;
	msg->o.nbytes = nbytes;
	return(_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o)+nbytes ));
}
