/*
 * -=MikeP=- (c) 2005
 */

/* $Date: Mon 13, Jun 2005 */

#include	"qxdns_common.h"




/*
 * Here comes the set of error descriptions.
 */

char * const QXDNSERR[] =
{
	"OK\n",

	/* Configuration time errors */
	"Unknown directive\n",

	/* Runtime errors */
	"Source query not found\n",
	
	/* End of error list */
	NULL
};









/*
 * Here comes the logging stuff.
 */

int				LogLevel = QLL_INFO;
const char *	log_names[] = {
	"system",
	"error",
	"warning",
	"info"
};


void qxdns_log(int log_level, const char * fmt, ...){
	va_list arg_list;
	char	ltimes[26];
	time_t	ltime;
	
	if(LogLevel<log_level) return;
	if(log_level != QLL_SYSTEM){
		/* print time: */
		time(&ltime);
		ctime_r(&ltime,ltimes);
		ltimes[24] = '\0';
		fprintf(stderr,"[%s] [%s] ",ltimes,log_names[log_level]);
	}
	
	/* print the message */
	va_start(arg_list,fmt);
	vfprintf(stderr,fmt,arg_list);
	va_end(arg_list);
}
