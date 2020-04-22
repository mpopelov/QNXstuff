/*
 * 2006 (c) -={ MikeP }=-
 * 
 * QX-httpd server.
 * 
 * HTTP protocol description goes here
 */

#ifndef _QXH_HTTP_H_
#define _QXH_HTTP_H_



/* HTTP protocol methods */
typedef enum{
	M_OPTIONS,
	M_GET,
	M_HEAD,
	M_POST,
	M_PUT,
	M_DELETE,
	M_TRACE,
	M_CONNECT	
} http_meth_t;


/* HTTP protocol errors */
typedef enum{
	e_101,
	e_102,
	e_103,
	e_200,
	e_400,
	e_405,
	e_406,
	e_500
} http_err_t;








#endif /* _QXH_HTTP_H_ */
