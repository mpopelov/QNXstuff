#
#  Qbase reader by -=MikeP=-
#  Copyright (C) 2004  Mike Anatoly Popelov ( AKA -=MikeP=- )
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

# for gcc 2.95.3
#CFLAGS = -O -I ../getconf
#LDFLAGS =

# for gcc 3.3.5
CFLAGS = -V3.3.5,gcc_ntox86 -O -I. -I../getconf
LDFLAGS = -V3.3.5,gcc_ntox86



QBASE = QX-base

RDSRCS = qbase_rm.${CPP} qbase_filefunc.${CPP} qbase_filterfunc.${CPP} qbase_iofunc.${CPP}

RDOBJS = qbase_rm.${O} qbase_filefunc.${O} qbase_filterfunc.${O} qbase_iofunc.${O}

CLISRC = qbase_client.${CPP}

CLIOBJ = qbase_client.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}










all: ${QBASE} client resize

${QBASE}: ${RDOBJS}
	${LD} -o ${QBASE} ../getconf/GetConf.o ${RDOBJS} ${LDFLAGS}
	strip ${QBASE}

client: qbase_client.${O}

resize: qxb_resize.${O}
	${LD} -o QXB-resize ../getconf/GetConf.o qbase_client.${O} qxb_resize.${O} ${LDFLAGS}
	strip QXB-resize

distclean: clean

clean:
	rm -f .depend a.out core ${QBASE}
	rm -f *.${O} *~