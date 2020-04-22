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
 * This header defines commands supported and understood by
 * devctl handler of resource;
 * Here implemented interface entries of resource manager are also defined
 * ( forward definitions )
 */


#ifndef _NTC_DCTL_H_
#define _NTC_DCTL_H_

/* Forward definitions of used functions */
#define	RM_DEVICE_NAME	"/dev/ntc"

int ntc_rm_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb);
int ntc_rm_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *o);

#define NTC_BASE_CODE      1

#define NTC_FLUSH __DIOT(_DCMD_MISC,  NTC_BASE_CODE + 0, int)		/* flush counters */
#define NTC_NUMMISSED __DIOF(_DCMD_MISC,  NTC_BASE_CODE + 1, int)	/* get number of missed packets */
#define NTC_QUEUELEN __DIOF(_DCMD_MISC,  NTC_BASE_CODE + 2, int)	/* get the length of pending queue */
#define NTC_EXIT __DIOT(_DCMD_MISC,  NTC_BASE_CODE + 3, int)		/* shut down the filter */
#define	NTC_ADDBLOCK __DIOT(_DCMD_MISC,  NTC_BASE_CODE + 4, int)	/* add the MAC to block */
#define	NTC_DELBLOCK __DIOT(_DCMD_MISC,  NTC_BASE_CODE + 5, int)	/* delete the MAC from blocking list */
#define NTC_NUMBLOCKED __DIOF(_DCMD_MISC,  NTC_BASE_CODE + 6, int)	/* get the number of blocked packets */

#define NTC_IFPRESENT __DIOF(_DCMD_MISC,  NTC_BASE_CODE + 7, int)	/* get info of present ifaces */
#define	NTC_ADDIF __DIOT(_DCMD_MISC,  NTC_BASE_CODE + 8, int)		/* say we are interested in iface */
#define	NTC_DELIF __DIOT(_DCMD_MISC,  NTC_BASE_CODE + 9, int)		/* say we are NOT interested in iface */






/* The folllowing is placed herein as an example for adding different kinds
 * of devctl command codes:
 *  - with no input and some output
 *  - with some input and no output
 *  - with some input and some output
 * 
 * For more information on this topic refer to QNX Neutrino manual
 * ( reference on how to write Resource Manager )
 */

/*
#define MY_DEVCTL_GETVAL __DIOF(_DCMD_MISC,  NTC_BASE_CODE + 0, int)
#define MY_DEVCTL_SETVAL __DIOT(_DCMD_MISC,  NTC_BASE_CODE + 1, int)
#define MY_DEVCTL_SETGET __DIOTF(_DCMD_MISC, NTC_BASE_CODE + 2, union _my_devctl_msg)
*/


#endif /* _NTC_DCTL_H_ */
