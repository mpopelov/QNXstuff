/*
 * 2006 (c) -={ MikeP }=-
 * 
 * QX-httpd server
 * 
 * Logging facilities
 */


#ifndef _QXH_LOG_H_
#define _QXH_LOG_H_


/* define log levels */
#define	QXH_LOG_SYSTEM	0
#define	QXH_LOG_ERROR	1
#define	QXH_LOG_WARNING	2
#define	QXH_LOG_INFO	3


void qxh_log_vlevel( int nLevel );
void qxh_log_rdr( const char *cszName );
void qxh_log( int nLevel, const char *cszFmt, ... );

#endif /* _QXH_LOG_H_  */
