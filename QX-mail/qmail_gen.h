/*
 * -=MikeP=- (c) 2004
 */

#ifndef _QMAIL_GEN_H_
#define	_QMAIL_GEN_H_





/* these are defined to use by MD5 */
#define _FF(b, c, d)	(d ^ (b & (c ^ d)))
#define _FG(b, c, d)	_FF(d, b, c)
#define _FH(b, c, d)	(b ^ c ^ d)
#define _FI(b, c, d)	(c ^ (b | ~d))

#define CYCLIC(w, s) (w = (w << s) | (w >> (32 - s)))

#define OP(f,a, b, c, d, s, M, T)\
		do{						\
		a += f(b,c,d) + M + T;	\
        CYCLIC (a, s);			\
        a += b;					\
		}while (0)

char * MD5bin( char * buf, uint32_t buflen );
char * GetMD5( char * buf, uint32_t buflen );
/* ----- MD5 ------ */



/* these are for Base64 encoding/decoding */
#define B64_OK                  (0)
#define B64_FAIL                (-1)
#define B64_BUFOVER             (-2)


int decode64(const char *in, unsigned inlen, char *out, unsigned *outlen);
int encode64(const char *_in, unsigned inlen, char *_out, unsigned outmax, unsigned *outlen);
/* ---- Base64 ---- */






/* define log levels */
#define	QLL_SYSTEM	0
#define	QLL_ERROR	1
#define	QLL_WARNING	2
#define	QLL_INFO	3



int ReadString( int sock, char * buffer, int bufsize );
void qmail_log(int log_level, const char * fmt, ...);

#define qserr(x) sys_errlist[x]

#endif /* _QMAIL_GEN_H_ */
