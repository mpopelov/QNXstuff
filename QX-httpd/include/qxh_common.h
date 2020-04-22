/*
 * 2006 (c) -={ MikeP }=-
 * 
 * A small and scalable http server.
 * 
 * All includes go here.
 */

#ifndef _QXH_COMMON_H_
#define _QXH_COMMON_H_


#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>

#include	<string.h>
#include	<errno.h>
#include	<time.h>

#include	<pthread.h>
#include	<gulliver.h>

#include	<sys/iofunc.h>
#include	<sys/dispatch.h>

#include	<sys/socket.h>
#include	<netinet/in.h>



/* now include local definitions */
#include	"qxh_log.h"			/* need for logging */
#include	"qxh_http.h"		/* http protocol definitions */
#include	"qxh_vhost.h"		/* virtual hosts definitions */





/* define everything here */





#endif /* _QXH_COMMON_H_ */
