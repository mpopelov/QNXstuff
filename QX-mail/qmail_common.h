/*
 * -=MikeP=- (c) 2004
 */

#ifndef _QMAIL_COMMON_H_ 
#define _QMAIL_COMMON_H_


/* common includes and defines */

#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<pthread.h>
#include	<errno.h>
#include	<string.h>
#include	<fcntl.h>
#include	<time.h>
#include	<signal.h>

#include	<sys/socket.h>
#include	<sys/types.h>
#include	<sys/dispatch.h>
#include	<sys/dir.h>
#include	<sys/select.h>

#include	<netinet/in.h>
#include	<netinet/tcp.h>

#include	<gulliver.h>




/* define this to get debug output */
/*#define QMAIL_DEBUG_ON 1*/

/* this defines the default send/recv buffer size */
#define DEF_BUF_SIZE 8192

/* this defines default timeout to wait for data input in seconds */
#define DEF_RCVTIMEOUT 10





/* this template is used to generate temp files */
#define SPOOLER_TMPF "spool/msg.XXXXXXXXXXXX"





/* this structure describes a list of listeners */
#define	QMAIL_LISTEN_POP	1
#define QMAIL_LISTEN_SMTP	2

typedef struct q_listener{
	struct sockaddr_in	bind_addr;	/* the address the listener is bound to */
	int			sock;		/* the socket itself */
	uint32_t	type;		/* the type of communication: SMTP/POP3 */
	struct q_listener * next;
} listener_t;



/* this is the structure, describing available mailboxes
 * it is common for all parts of the server */
typedef struct _mailbox{
	/* mailbox specific properties */
	char *			user;		/* mailbox/user name: Some.Person@some.domai*/
	char *			password;	/* a password to mailbox 					*/
	/* related to mailbox treatment */
	pthread_mutex_t	mtx;		/* a mutex to lock the mailbox				*/
	struct _mailbox *	next;	/* to maintain list of mailboxes			*/
} mailbox_t;



/* local headers */
#include	"qmail_gen.h"
#include	"qmail_pop3.h"
#include	"qmail_smtp.h"
#include	"qmail_spooler.h"





/* global connection descriptor type */


typedef struct _conn_info{
	struct sockaddr_in	from;		/* the remote side */
	socklen_t			fromlen;	/* length of structure */
	int					tcp_sock;	/* connected the socket itself */
	char *				user;		/* connected user */
	char *				greeting;	/* greeting, sent to client */
	uint32_t			type;
	struct _conn_info *	next;
	
	/* pop3 related stuff */
	pop_state_t		pstate;		/* current state of connection	*/
	msgent_t *		msgs;

	/* smtp related stuff */
	smtp_state_t	sstate;		/* current state of connection	*/
	uint32_t		auth;		/* if user is authorised */
	smtpmsg_t *		msg;
} conn_info_t;



typedef struct _thread_storage{
	char *	buffer;
	conn_info_t * conn;
} QTS;




/* handling functions */
int thread_handler_pop( QTS * ts );
int thread_handler_smtp( QTS * ts );




#endif /* _QMAIL_COMMON_H_ */
