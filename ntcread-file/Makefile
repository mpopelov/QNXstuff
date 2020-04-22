#
#  NFM-NTC.SO reader by -=MikeP=-
#  Copyright (C) 2003, 2004  Mike Anatoly Popelov ( AKA -=MikeP=- )
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

CFLAGS = -O -I ../ntc-debug/
LDFLAGS =

NTCREAD = ntcread

RDSRCS = ntcread.${CPP}

RDOBJS = ntcread.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}










all: ${NTCREAD}

${NTCREAD}: ${RDOBJS}
	${LD} -o ${NTCREAD} ${RDOBJS} -lsocket
	strip ${NTCREAD}

distclean: clean

clean:
	rm -f .depend a.out core ${NTCREAD}
	rm -f *.${O} *~