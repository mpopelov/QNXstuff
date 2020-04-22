#
#  Copyright (C) 2005  Mike Anatoly Popelov ( AKA -=MikeP=- )
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

#CFLAGS = -O -I ../getconf -I .
#LDFLAGS = -l/lib/libclamav.so.1

CFLAGS = -V3.3.5,gcc_ntox86 -O -I ../getconf -I .
LDFLAGS = -V3.3.5,gcc_ntox86 -l/lib/libclamav.so

QBASE = QX-avird
TESTAV = testav
QBRLD = qbreload

RDSRCS = qavird.${CPP}
RDOBJS = qavird.${O}

TSRC = testav.${CPP}
TOBJ = testav.${O}

QBRLDC = qbreload.${CPP}
QBRLDO = qbreload.${O}

%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}










all: ${QBASE} ${QBRLD} ${TESTAV}

${QBASE}: ${RDOBJS}
	${LD} -o ${QBASE} ../getconf/GetConf.o ${RDOBJS} ${LDFLAGS}
	strip ${QBASE}
	
${TESTAV}: ${TOBJ}
	${LD} -o ${TESTAV} ${TOBJ}
	strip ${TESTAV}

${QBRLD}: ${QBRLDO}
	${LD} -o ${QBRLD} ${QBRLDO}
	strip ${QBRLD}

distclean: clean

clean:
	rm -f .depend a.out core ${QBASE} ${TESTAV} ${QBRLD}
	rm -f *.${O} *~