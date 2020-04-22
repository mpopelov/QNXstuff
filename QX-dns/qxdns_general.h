/*
 *  -=MikeP=- (c) 2005
 */

/* $Date: Mon 13, Jun 2005 */

#ifndef _QXDNS_GENERAL_H_
#define	_QXDNS_GENERAL_H_



/* define log levels */
#define	QLL_SYSTEM	0
#define	QLL_ERROR	1
#define	QLL_WARNING	2
#define	QLL_INFO	3
extern int			LogLevel;




/* Related to error handling */
extern char * const	QXDNSERR[];

#define	QXDNS_E_OK			0

#define	QXDNS_E_UNKNOWN		1

#define	QXDNS_E_LOSTQUERY	2








/* function to log events of the server */

void qxdns_log(int log_level, const char * fmt, ...);


#endif	/* _QXDNS_GENERAL_H_ */
