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

#CFLAGS = -O2 -I. -I../getconf -I../QX-avird
#LDFLAGS = -lsocket

# for gcc 3.3.5
CFLAGS = -V3.3.5,gcc_ntox86 -g -O2 -I. -I../getconf -I../QX-avird
LDFLAGS = -V3.3.5,gcc_ntox86 -g -lsocket



# These lines replace the ones above in case of debug build
#CFLAGS = -g -I. -I../getconf -I../Qavird
#LDFLAGS = -g -lsocket




QMAIL = QX-mail

SRCS = general.${CPP} spooler.${CPP} smtp.${CPP} pop3.${CPP} main.${CPP}

OBJS = general.${O} spooler.${O} smtp.${O} pop3.${O} main.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}

all: ${QMAIL}

${QMAIL}: ${OBJS}
	${LD} -o ${QMAIL} ../getconf/GetConf.o ${OBJS} ${LDFLAGS}
#	strip ${QMAIL}

distclean: clean

clean:
	rm -f .depend a.out core ${QMAIL}
	rm -f *.${O} *~