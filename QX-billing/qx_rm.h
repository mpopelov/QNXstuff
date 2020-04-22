/*
 * 2005 (c) -={ MikeP }=-
 */




/*
 * Definitions needed for communication between io-net
 * module and logger go here
 */


#ifndef _QX_RM_H_
#define _QX_RM_H_


#define QXC_BASE_CODE      1

#define QXC_ADD_TE			__DIOT(_DCMD_MISC,  QXC_BASE_CODE + 0, int)	/* add tax exempt rule */
#define QXC_ADD_NIC			__DIOT(_DCMD_MISC,  QXC_BASE_CODE + 1, int)	/* add NIC allowed to pass */
#define QXC_SWAP_GET_LOG	__DIOF(_DCMD_MISC,  QXC_BASE_CODE + 2, int)	/* swap internal rules and get last active log */
#define QXC_GET_INFO		__DIOF(_DCMD_MISC,  QXC_BASE_CODE + 3, int)	/* get interface stats */




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






#endif /* _QX_RM_H_ */
