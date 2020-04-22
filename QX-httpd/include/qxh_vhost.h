/*
 * 2006 (c) -={ MikeP }=-
 * 
 * QX-httpd server.
 * 
 * Everything concerning virtual hosts goes here.
 * Every virtual host maintained by QX-httpd is described
 * in QX-httpd.conf file within the <vhost></vhost> tags.
 * 
 * NO NAME-BASED VIRTUAL HOSTING OR etc. Use more appropriate
 * solution like Apache on a different platform.
 * 
 */

#ifndef _QXH_VHOST_H_
#define _QXH_VHOST_H_


/* Description of virtual host */
typedef struct _qxh_vhost_{
	/* linking servers together */
	struct _qxh_vhost_	*next;
	
	/* information on binding socket */
	struct sockaddr_in	vhost_addr;		/* bound to this address */
	int					sock;			/* listening socket */
	
	/* information on server itself */
	char				*szFQDN;		/* domain name of the server */
	char				*szDir;			/* root directory of the server */
	char				*szIndex;		/* index of directories */
} qxh_vhost_t;




#endif /* _QXH_VHOST_H_ */