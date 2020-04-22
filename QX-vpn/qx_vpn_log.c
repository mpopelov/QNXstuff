/*
 * -={ MikeP }=- (c) 2005
 */

#include	"qx_vpn_common.h"



int	LogLevel = QLL_INFO;
const char * log_names[] ={
	"system",
	"error",
	"warning",
	"info"
};



/*
 * Outputs all log to stderr.
 * Messages of log_level QLL_SYSTEM are always AS IS.
 * All other message types are printed in the following way:
 * 
 * [timestamp of message] [log level of message] message itself <LF>
 * 
 * If message log_level exceeds LogLevel set for entire server,
 * it is not printed.
 */

void qx_vpn_log(int log_level, const char * fmt, ...){
	va_list	arg_list;
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
