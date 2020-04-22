#
#  devn-ser.so by -=MikeP=-
#  Copyright (c) 2004, 2005  Mike Anatoly Popelov ( AKA -=MikeP=- )
#
#  -=MikeP=- <mikep@qnx.org.ru>
#            <mikep@kaluga.org>
#
#            WWW:	http://mikep.qnx.org.ru
#            ICQ:	104287300
#
#  You are free to use this code in the way you like and for any purpose,
#  not deprecated by your local law. I will have now claim for using this code
#  in any project, provided that the reference to me as the author is given in
#  in this file or any code derived from this one.
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

CFLAGS = -O -shared
LDFLAGS = -shared


DEVNSER = devn-ser.so

DSSRC = main.${CPP}

DSOBJS = main.${O}

%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}




all: ${DEVNSER}

${DEVNSER}: ${DSOBJS}
	${LD} -o ${DEVNSER} ${DSOBJS} ${LDFLAGS}
	strip ${DEVNSER}
	
distclean: clean

clean:
	rm -f .depend a.out core ${DEVNSER}
	rm -f *.${O} *~