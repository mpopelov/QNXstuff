/*
 * -=MikeP=- (c) 2004
 */


/*
 * Here placed functions, that are used both in SMTP and POP3 client/server
 */


#include	"qmail_common.h"
#include	"qmail_gen.h"

int			LogLevel = QLL_INFO;


FILE * qxm_stderr;



/*
 * Reads bytes from socket until CRLF is met or socket timeouts
 * Returns:
 * 
 * - number of bytes recieved, null-terminated string in buffer
 * - (-1) if socket timed out
 */
int ReadString( int sock, char * buffer, int bufsize )
{
	int	rcvb;
	int	i;
	int	index = 0;
	
	while(1){
		
		/* get the bytes from stream */
		rcvb = recv(sock, &buffer[index], (bufsize-index), 0);
		
		/* if socket timed out at any stage, return -1 */
		if( rcvb <= 0 ) return (-1);
		
		/* scan line for CRLF */
		for( i=0; i<rcvb; i++ ){
			if(buffer[index+i] == '\r') break;
			/* if(buffer[index+i] == '\n') break; /* would that help? */
		}
		
		/* if there was no '\r' byte - go forth accepting data */
		index += i;
		if(i < rcvb) break;		
	}
	
	/* we are here because the '\r' byte is found - return proper values */
	buffer[index] = '\0';
	return (index+1);
}







/*
 * This function returns the 16-byte buffer,
 * containing MD5 hash in BINARY form in LITTLE ENDIAN
 */
char * MD5bin( char * buf, uint32_t buflen )
{
	/* apply MD5 */
	uint32_t	length;
	uint32_t *	msg_buffer, *msg;
	uint32_t	AA,BB,CC,DD;
	uint32_t	i,times;
	uint32_t *	result;
	uint32_t	A = 0x67452301;
	uint32_t	B = 0xefcdab89;
	uint32_t	C = 0x98badcfe;
	uint32_t	D = 0x10325476;

	/* apply padding */
	length = (buflen/64 + 1)*16;
	if((buflen%64) >= 56) length+=16;
	msg = (uint32_t*)malloc(length*sizeof(uint32_t));
	memset(msg,0,length*4);
	memcpy(msg, buf, buflen);
	((char*)(msg))[buflen] = 0x80; /* '8' == 56 == 0x80 */
	msg[length-2] = (buflen*8);


	/* calculate how many times we should iterate */
	times = length/16;


	for(i=0;i<times;i++){
		AA=A, BB=B, CC=C, DD=D;
		msg_buffer = msg + i*16;
		
		/* phase1 */
		OP(_FF,A,B,C,D, 7,msg_buffer[0],0xd76aa478);
		OP(_FF,D,A,B,C,12,msg_buffer[1],0xe8c7b756);
		OP(_FF,C,D,A,B,17,msg_buffer[2],0x242070db);
		OP(_FF,B,C,D,A,22,msg_buffer[3],0xc1bdceee);
		OP(_FF,A,B,C,D, 7,msg_buffer[4],0xf57c0faf);
		OP(_FF,D,A,B,C,12,msg_buffer[5],0x4787c62a);
		OP(_FF,C,D,A,B,17,msg_buffer[6],0xa8304613);
		OP(_FF,B,C,D,A,22,msg_buffer[7],0xfd469501);
		OP(_FF,A,B,C,D, 7,msg_buffer[8],0x698098d8);
		OP(_FF,D,A,B,C,12,msg_buffer[9],0x8b44f7af);
		OP(_FF,C,D,A,B,17,msg_buffer[10],0xffff5bb1);
		OP(_FF,B,C,D,A,22,msg_buffer[11],0x895cd7be);
		OP(_FF,A,B,C,D, 7,msg_buffer[12],0x6b901122);
		OP(_FF,D,A,B,C,12,msg_buffer[13],0xfd987193);
		OP(_FF,C,D,A,B,17,msg_buffer[14],0xa679438e);
		OP(_FF,B,C,D,A,22,msg_buffer[15],0x49b40821);
		/* ------ */

		/* phase2 */
		OP(_FG,A,B,C,D, 5,msg_buffer[1],0xf61e2562);
        OP(_FG,D,A,B,C, 9,msg_buffer[6],0xc040b340);
        OP(_FG,C,D,A,B,14,msg_buffer[11],0x265e5a51);
        OP(_FG,B,C,D,A,20,msg_buffer[0],0xe9b6c7aa);
        OP(_FG,A,B,C,D, 5,msg_buffer[5],0xd62f105d);
        OP(_FG,D,A,B,C, 9,msg_buffer[10],0x02441453);
        OP(_FG,C,D,A,B,14,msg_buffer[15],0xd8a1e681);
        OP(_FG,B,C,D,A,20,msg_buffer[4],0xe7d3fbc8);
        OP(_FG,A,B,C,D, 5,msg_buffer[9],0x21e1cde6);
        OP(_FG,D,A,B,C, 9,msg_buffer[14],0xc33707d6);
        OP(_FG,C,D,A,B,14,msg_buffer[3],0xf4d50d87);
        OP(_FG,B,C,D,A,20,msg_buffer[8],0x455a14ed);
        OP(_FG,A,B,C,D, 5,msg_buffer[13],0xa9e3e905);
        OP(_FG,D,A,B,C, 9,msg_buffer[2],0xfcefa3f8);
        OP(_FG,C,D,A,B,14,msg_buffer[7],0x676f02d9);
        OP(_FG,B,C,D,A,20,msg_buffer[12],0x8d2a4c8a);
		/* ------ */

		/* phase3 */
		OP(_FH,A,B,C,D, 4,msg_buffer[5],0xfffa3942);
        OP(_FH,D,A,B,C,11,msg_buffer[8],0x8771f681);
        OP(_FH,C,D,A,B,16,msg_buffer[11],0x6d9d6122);
        OP(_FH,B,C,D,A,23,msg_buffer[14],0xfde5380c);
        OP(_FH,A,B,C,D, 4,msg_buffer[1],0xa4beea44);
        OP(_FH,D,A,B,C,11,msg_buffer[4],0x4bdecfa9);
        OP(_FH,C,D,A,B,16,msg_buffer[7],0xf6bb4b60);
        OP(_FH,B,C,D,A,23,msg_buffer[10],0xbebfbc70);
        OP(_FH,A,B,C,D, 4,msg_buffer[13],0x289b7ec6);
        OP(_FH,D,A,B,C,11,msg_buffer[0],0xeaa127fa);
        OP(_FH,C,D,A,B,16,msg_buffer[3],0xd4ef3085);
        OP(_FH,B,C,D,A,23,msg_buffer[6],0x04881d05);
        OP(_FH,A,B,C,D, 4,msg_buffer[9],0xd9d4d039);
        OP(_FH,D,A,B,C,11,msg_buffer[12],0xe6db99e5);
        OP(_FH,C,D,A,B,16,msg_buffer[15],0x1fa27cf8);
        OP(_FH,B,C,D,A,23,msg_buffer[2],0xc4ac5665);
		/* ------ */

		/* phase4 */
		OP(_FI,A,B,C,D, 6,msg_buffer[0],0xf4292244);
        OP(_FI,D,A,B,C,10,msg_buffer[7],0x432aff97);
        OP(_FI,C,D,A,B,15,msg_buffer[14],0xab9423a7);
        OP(_FI,B,C,D,A,21,msg_buffer[5],0xfc93a039);
        OP(_FI,A,B,C,D, 6,msg_buffer[12],0x655b59c3);
        OP(_FI,D,A,B,C,10,msg_buffer[3],0x8f0ccc92);
        OP(_FI,C,D,A,B,15,msg_buffer[10],0xffeff47d);
        OP(_FI,B,C,D,A,21,msg_buffer[1],0x85845dd1);
        OP(_FI,A,B,C,D, 6,msg_buffer[8],0x6fa87e4f);
        OP(_FI,D,A,B,C,10,msg_buffer[15],0xfe2ce6e0);
        OP(_FI,C,D,A,B,15,msg_buffer[6],0xa3014314);
        OP(_FI,B,C,D,A,21,msg_buffer[13],0x4e0811a1);
        OP(_FI,A,B,C,D, 6,msg_buffer[4],0xf7537e82);
        OP(_FI,D,A,B,C,10,msg_buffer[11],0xbd3af235);
        OP(_FI,C,D,A,B,15,msg_buffer[2],0x2ad7d2bb);
        OP(_FI,B,C,D,A,21,msg_buffer[9],0xeb86d391);
		/* ------ */

		A+=AA, B+=BB, C+=CC, D+=DD;
	}

	free(msg);

	/* create and fill in the md5_hash buffer */
	result = (uint32_t*)malloc(16);
	result[0] = A; result[1] = B; result[2] = C; result[3] = D;
	return (char*)result;
}



/*
 * Function calculates MD5 hash of a given string and returns
 * a sring representation of the hash or NULL in case of error
 */
char * GetMD5( char * buf, uint32_t buflen )
{
	char * result;
	uint32_t * MD5hash = (uint32_t*)MD5bin( buf, buflen );

	/* create and fill in the md5_hash buffer */
	result = (char*)malloc(32);
	sprintf(result,"%08x%08x%08x%08x",ENDIAN_RET32(MD5hash[0]),ENDIAN_RET32(MD5hash[1]),ENDIAN_RET32(MD5hash[2]),ENDIAN_RET32(MD5hash[3]));
	free(MD5hash);
	return result;
}





























/*
 * These functions deal with Base64 encoding/decoding
 */

static char     basis_64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";

static char     index_64[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};

#define CHAR64(c)           (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])


int             encode64(const char *_in, unsigned inlen,
                         char *_out, unsigned outmax, unsigned *outlen)
{

    const unsigned char *in = (const unsigned char *) _in;
    unsigned char  *out = (unsigned char *) _out;
    unsigned char   oval;
    char           *blah;
    unsigned        olen;

    olen = (inlen + 2) / 3 * 4;
    if (outlen)
        *outlen = olen;
    if (outmax < olen)
        return B64_BUFOVER;

    blah = (char *) out;
    while (inlen >= 3)
    {
/* user provided max buffer size; make sure we don't go over it */
        *out++ = basis_64[in[0] >> 2];
        *out++ = basis_64[((in[0] << 4) & 0x30) | (in[1] >> 4)];
        *out++ = basis_64[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
        *out++ = basis_64[in[2] & 0x3f];
        in += 3;
        inlen -= 3;
    }
    if (inlen > 0)
    {
/* user provided max buffer size; make sure we don't go over it */
        *out++ = basis_64[in[0] >> 2];
        oval = (in[0] << 4) & 0x30;
        if (inlen > 1)
            oval |= in[1] >> 4;
        *out++ = basis_64[oval];
        *out++ = (inlen < 2) ? '=' : basis_64[(in[1] << 2) & 0x3c];
        *out++ = '=';
    }

    if (olen < outmax)
        *out = '\0';

    return B64_OK;

}

int decode64(const char *in, unsigned inlen, char *out, unsigned *outlen)
{

    unsigned        len = 0;
    unsigned        lup;
    int             c1;
    int             c2;
    int             c3;
    int             c4;

    if (inlen >= 2 && in[0] == '+' && in[1] == ' ')
        in += 2, inlen -= 2;

    if (*in == '\0')
        return B64_FAIL;

    for (lup = 0; lup < inlen / 4; lup++)
    {
        c1 = in[0];
        if (CHAR64(c1) == -1)
            return B64_FAIL;
        c2 = in[1];
        if (CHAR64(c2) == -1)
            return B64_FAIL;
        c3 = in[2];
        if (c3 != '=' && CHAR64(c3) == -1)
            return B64_FAIL;
        c4 = in[3];
        if (c4 != '=' && CHAR64(c4) == -1)
            return B64_FAIL;
        in += 4;
        *out++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);
        ++len;
        if (c3 != '=')
        {
            *out++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);
            ++len;
            if (c4 != '=')
            {
                *out++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);
                ++len;
            }
        }
    }

    *out = 0;
    *outlen = len;

    return B64_OK;

}




const char * log_names[] ={
	"system",
	"error",
	"warning",
	"info"
};



/*
 * Outputs all log to qxm_stderr.
 * Messages of log_level QLL_SYSTEM are always AS IS.
 * All other message types are printed in the following way:
 * 
 * [timestamp of message] [log level of message] message itself <LF>
 * 
 * If message log_level exceeds LogLevel set for entire server,
 * it is not printed.
 */

void qmail_log(int log_level, const char * fmt, ...){
	va_list	arg_list;
	char	ltimes[26];
	time_t	ltime;
	
	if(LogLevel<log_level) return;
	if(log_level != QLL_SYSTEM){
		/* print time: */
		time(&ltime);
		ctime_r(&ltime,ltimes);
		ltimes[24] = '\0';
		fprintf(qxm_stderr,"[%s] [%s] ",ltimes,log_names[log_level]);
	}
	
	/* print the message */
	va_start(arg_list,fmt);
	vfprintf(qxm_stderr,fmt,arg_list);
	va_end(arg_list);
}
