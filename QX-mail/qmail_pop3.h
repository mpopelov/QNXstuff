/*
 * -= MikeP =- (c) 2004
 */


#ifndef _QMAIL_POP3_H_
#define _QMAIL_POP3_H_



#define		POP3_PORT		110


/* POP3 commands in UPPER case */
#define	POP3_APOP	0x504f5041
#define	POP3_DELE	0x454c4544
#define	POP3_LIST	0x5453494c
#define	POP3_NOOP	0x504f4f4e
#define	POP3_PASS	0x53534150
#define	POP3_QUIT	0x54495551
#define	POP3_RETR	0x52544552
#define	POP3_RSET	0x54455352
#define	POP3_STAT	0x54415453
#define	POP3_TOP	0x20504f54
#define	POP3_UIDL	0x4c444955
#define	POP3_USER	0x52455355

/* POP3 responses */
#define	POP3_POS	"+OK\r\n"
#define	POP3_NEG	"-ERR\r\n"
#define	POP3_EOM	"\r\n.\r\n"



typedef enum{
	MBX_NONE,	/* newly connected		*/
	MBX_AUTH,	/* AUTHORIZATION state	*/
	MBX_TRAN	/* TRANSACTION state	*/
} pop_state_t;



/* structure, describing the list of messages, available during this session
 * to client */
typedef struct _msgent{
	uint32_t	del;	/* checked if the message is to be deleted upon QUIT */
	uint32_t	size;	/* the size of message in octets */
	char *		name;
	struct _msgent * next;
} msgent_t;




#endif /* _QMAIL_POP3_H_ */

