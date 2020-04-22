#
#  QX-counter.so by -=MikeP=-
#  Copyright (C) 2003-2005  Mike Anatoly Popelov ( AKA -=MikeP=- )
#
#  -=MikeP=- <mikep@kaluga.org>
#
#  You are free to use this code in the way you like and for any purpose,
#  not deprecated by your local law. I will have now claim for using this code
#  in any project, provided that the reference to me as the author provided.
#  I also DO NOT TAKE ANY RESPONSIBILITY if this code does not suite you in your
#  particular case. Though this code was written to be stable and working,
#  I will take NO RESPONSIBILITY if it does not work for you, on your particular
#  equipment, in your particular configuration and environment, if it brings any
#  kind of damage to your equipment.
#

O = o
A = a
CPP = c
CC = qcc
LD = qcc

CFLAGS = -V3.3.5,gcc_ntox86 -O2 -shared -I. -I../getconf -I../QX-base
LDFLAGS = -V3.3.5,gcc_ntox86 -shared

COUNTER = nfm-qxc.so
LOGGER = QX-billing
CHECKER = QX-consistency

QXCSRCS = qxc_main.${CPP} qxc_rules.${CPP} qxc_logging.${CPP} qxc_rm.${CPP}
QXCOBJS = qxc_main.${O} qxc_rules.${O} qxc_logging.${O} qxc_rm.${O}

QXLSRCS = qxl_eod.${CPP} qxl_main.${CPP}
QXLOBJS = qxl_eod.${O} qxl_main.${O}

QXCSSRCS = qxl_consistency.${CPP}
QXCSOBJS = qxl_consistency.${O}

%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}




all: ${COUNTER} ${LOGGER} ${CHECKER}

${COUNTER}: ${QXCOBJS}
	${LD} -o ${COUNTER} ../getconf/GetConf.o ${QXCOBJS} ${LDFLAGS}
	strip ${COUNTER}

${LOGGER}: ${QXLOBJS}
	${LD} -o ${LOGGER} ../getconf/GetConf.o ../QX-base/qbase_client.o ${QXLOBJS} -lsocket
	strip ${LOGGER}

${CHECKER}: ${QXCSOBJS}
	${LD} -o ${CHECKER} ../getconf/GetConf.o ../QX-base/qbase_client.o ${QXCSOBJS}
	strip ${CHECKER}
	
distclean: clean

clean:
	rm -f .depend a.out core ${COUNTER} ${LOGGER} ${CHECKER}
	rm -f *.${O} *~