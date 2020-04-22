#
#  QDNS by -=MikeP=-
#  Copyright (C) 2003  -=MikeP=-
#
#  -=MikeP=- <mikep@kaluga.org>
#

O = o
A = a
CPP = c
CC = qcc
LD = qcc

# for gcc 2.95.3
CFLAGS = -g -I. -I../getconf
LDFLAGS = -g -lsocket

# for gcc 3.3.1
#CFLAGS = -V3.3.1,gcc_ntox86 -O2 -I. -I../getconf
#LDFLAGS = -V3.3.1,gcc_ntox86 -lsocket



QXDNS = QX-dns

SRCS = general.${CPP} cache.${CPP} master.${CPP} main.${CPP}
OBJS = general.${O} cache.${O} master.${O} main.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}

all: ${QXDNS}

${QXDNS}: ${OBJS}
	${LD} -o ${QXDNS} ../getconf/GetConf.o ${OBJS} ${LDFLAGS}
#	strip ${QXDNS}

distclean: clean

clean:
	rm -f .depend a.out core ${QXDNS}
	rm -f *.${O} *~