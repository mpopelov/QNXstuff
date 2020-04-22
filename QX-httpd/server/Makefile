#
#  QX-httpd by -=MikeP=-
#  Copyright (C) 2006  -=MikeP=-
#
#  -=MikeP=- <mikep@qnx.org.ru>
#

O = o
A = a
CPP = c
CC = qcc
LD = qcc

CFLAGS = -O2 -I. -I../include -I../../getconf
LDFLAGS = -lsocket

# These lines replace the ones above in case of debug build
#CFLAGS = -g -I. -I../getconf -I../Qavird
#LDFLAGS = -g -lsocket




QXHTTPD = QX-httpd

SRCS = log.${CPP} main.${CPP}

OBJS = log.${O} main.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}

all: ${QXHTTPD}

${QXHTTPD}: ${OBJS}
	${LD} -o ../out/${QXHTTPD} ../../getconf/GetConf.o ${OBJS} ${LDFLAGS}
#	strip ${QMAIL}

distclean: clean

clean:
	rm -f .depend a.out core ${QXHTTPD}
	rm -f *.${O} *~