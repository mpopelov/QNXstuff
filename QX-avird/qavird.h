/*
 * -=MikeP=- (c) 2004
 */
 
/*
 * Here are definitions needed only by server
 */
#ifndef _QAVIRD_H_
#define _QAVIRD_H_

#include	<stdio.h>
#include	<stdlib.h>
#include	<stddef.h>
#include	<unistd.h>

struct	Qavird_ocb;
#define	IOFUNC_OCB_T struct Qavird_ocb

#include	<sys/iofunc.h>
#include	<sys/dispatch.h>
#include	<devctl.h>
#include	<errno.h>
#include	<pthread.h>
#include	<fcntl.h>
#include	<atomic.h>
#include	<time.h>

#include	<clamav.h>


/*
 * Description of our own OCB
 */
struct Qavird_ocb{
	iofunc_ocb_t	hdr;		/* standart ocb - goes here because libc requires it to be this way */
	int				cl_errno;	/* this member stores the return code of the last operation performed by client */
	char *			stream;		/* this is for scanning streamed data */
	struct cl_limits limits;	/* these are limits of scaning */
	unsigned int	options;	/* these are options for scanning */
};


/* Information on current state of engine */
struct Qavird_info{
	int			signum;		/* number of signatures in loaded bases */
	uint32_t	virfound;	/* number of viruses found during work  */
	uint32_t	fdone;		/* number of files/streams processed	*/
	time_t		startup;	/* when we started up */
	time_t		base_time;	/* time when bases were last loaded		*/
	/* here follows info on loaded bases */
	int			m_ver;
	int			m_sig;
	int			m_fl;
	time_t		m_btime;
	int			d_ver;
	int			d_sig;
	int			d_fl;
	time_t		d_btime;
};




/* Error codes, returned to client */
#define	QAV_DATA		-1	/* this code is set when client supplies wrong buffers */
#define	QAV_CLEAN		0
#define	QAV_VIRUS		1
#define	QAV_INTERNAL	2





/* Definitions for commands to Qavird */
#define QAVIRD_CMD_BASE		    0

/* scan the file given its name */
#define QAVIRD_CMD_INFO				__DIOTF(_DCMD_MISC,  QAVIRD_CMD_BASE + 0, int)
/* reload current virus bases */
#define QAVIRD_CMD_AVBASERELOAD		__DIOTF(_DCMD_MISC,  QAVIRD_CMD_BASE + 1, int)


/*
 * Working functions
 */
/* scan the file given its name */
#define QAVIRD_CMD_SCANFILE			__DIOTF(_DCMD_MISC,  QAVIRD_CMD_BASE + 5, int)
/* start to scan the supplied byte stream */
#define QAVIRD_CMD_SCANSTREAM		__DIOTF(_DCMD_MISC,  QAVIRD_CMD_BASE + 6, int)
/* continue to scan the stream of bytes */
#define QAVIRD_CMD_SCANSTREAMNEXT	__DIOTF(_DCMD_MISC,  QAVIRD_CMD_BASE + 7, int)





#endif /* _QAVIRD_H_ */
