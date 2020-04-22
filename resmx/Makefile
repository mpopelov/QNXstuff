#
#  resmx by -=MikeP=-
#  Copyright (C) 2005  -=MikeP=-
#
#  -=MikeP=- <mikep@kaluga.org>
#

O = o
A = a
CPP = c
CC = qcc
LD = qcc

CFLAGS = -O2 -I. -D_REENTRANT=1 -D_THREAD_SAFE=1
LDFLAGS = -lsocket


RESMX = resmx

SRCS = resmx.${CPP}

OBJS = resmx.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}

all: ${RESMX}

${RESMX}: ${OBJS}
	${LD} -o ${RESMX} ${OBJS} ${LDFLAGS}
	strip ${RESMX}

distclean: clean

clean:
	rm -f .depend a.out core ${RESMX}
	rm -f *.${O} *~