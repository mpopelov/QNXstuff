#
#  Qmail by -=MikeP=-
#  Copyright (C) 2004  -=MikeP=-
#
#  -=MikeP=- <mikep@qnx.org.ru>
#

O = o
A = a
CPP = c
CC = qcc
LD = qcc

CFLAGS = -O2 -I.
LDFLAGS = -lsocket

# These lines replace the ones above in case of debug build
#CFLAGS = -g -I. -I../getconf -I../Qavird
#LDFLAGS = -g -lsocket




QMAIL = testrdr

SRCS = testrdr.${CPP}

OBJS = testrdr.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}

all: ${QMAIL}

${QMAIL}: ${OBJS}
	${LD} -o ${QMAIL} ${OBJS} ${LDFLAGS}
#	strip ${QMAIL}

distclean: clean

clean:
	rm -f .depend a.out core ${QMAIL}
	rm -f *.${O} *~