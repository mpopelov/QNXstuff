/*
 * -= MikeP =- (c) 2004
 */


#ifndef _QMAIL_SMTP_H_
#define _QMAIL_SMTP_H_


#define		SMTP_PORT		25

#define		SMTP_EOM		".\r\n";


/* SMTP commands in UPPER case */
#define	SMTP_HELO	0x4f4c4548	/* standart HELO */
#define	SMTP_EHLO	0x4f4c4845	/* extended HELO */
#define	SMTP_AUTH	0x48545541	/* authenticate user */
#define	SMTP_MAIL	0x4c49414d	/* MAIL command */
#define	SMTP_RCPT	0x54504352	/* RCPT command */
#define	SMTP_DATA	0x41544144	/* DATA command */
#define	SMTP_RSET	0x54455352	/* RSET command */
#define	SMTP_NOOP	0x504f4f4e	/* NO OPeration command */
#define	SMTP_QUIT	0x54495551	/* Bye Bye! */

typedef enum{
	SS_NONE,	/* newly connected		*/
	SS_HELO,	/* GREETING RECEIVED	*/
	SS_RCPT,	/* waiting for mail		*/
	SS_DATA		/* TRANSACTION state	*/
} smtp_state_t;



/* structure, describing recipients of the mail */
typedef struct _recipt{
	char *		to;
	struct _recipt * next;
} recipt_t;

/* structure, describing message */
typedef struct _smtpmsg{
	char *		from;		/* from who is the message */
	recipt_t *	to;			/* list of recipients */
	int			nrecips;	/* number of recipients on the list */
	int			spoolfd;	/* file descriptor of the spooler (temporary) file */
	char *		spoolfn;	/* name of the file in spooler */
	/* char *		uid;	/* maybe it'll be the filename for UIDL pop3 command */
	struct _smtpmsg * next;
} smtpmsg_t;



#endif /* _QMAIL_SMTP_H_ */

