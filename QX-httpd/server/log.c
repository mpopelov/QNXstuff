/*
 * 2006 (c) -={ MikeP }=-
 * 
 * QX-httpd server
 * 
 * Logging facilities
 */


#include	"qxh_common.h"


/* the level of log verbosity */
static int	nLogVerbose	= QXH_LOG_INFO;

/* FILE* of reopened stderr */
FILE*	efd = NULL;

/* names to show in log */
const char*	cszLogLevels[] ={
	"system",
	"error",
	"warning",
	"info"
};





/* set verbosity of the log file */
void qxh_log_vlevel( int nLevel ){
	nLogVerbose = nLevel;
}





/* redirect stderr to some file */
void qxh_log_rdr( const char *cszName ){
	
	efd = freopen( cszName, "a", stderr );
	
	if( efd == NULL ){
		printf("LogFile: %s\n", strerror(errno));
		exit(-1);
	}
	
	setvbuf( stderr, NULL, _IOLBF, 0 );
}





/*
 * Print the error message to redirected stderr.
 * Add time and severity of the message.
 */
void qxh_log( int nLevel, const char *cszFmt, ... ){
	va_list	arg_list;
	char	sLogTime[26];
	time_t	nLogTime;
	
	/* limit verbosity of log to configured one */
	if( nLogVerbose < nLevel ) return;
	
	/* do not print log time before system messages */
	if( nLevel != QXH_LOG_SYSTEM ){
		time( &nLogTime );
		ctime_r( &nLogTime, sLogTime );
		sLogTime[24] = '\0';
		fprintf(stderr,"[%s] [%s] ", sLogTime, cszLogLevels[nLevel] );
	}
	
	/* print variable length message */
	va_start( arg_list, cszFmt );
	vfprintf( stderr, cszFmt, arg_list );
	va_end( arg_list );
}
