O = o
A = a
CPP = c
CC = qcc
LD = qcc

CFLAGS = -O -I ../getconf
LDFLAGS = -L

QBASE = conftest

RDSRCS = conftest.${CPP}

RDOBJS = conftest.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}










all: ${QBASE}

${QBASE}: ${RDOBJS}
	${LD} -o ${QBASE} ../getconf/GetConf.o ${RDOBJS}
	strip ${QBASE}

distclean: clean

clean:
	rm -f .depend a.out core ${QBASE}
	rm -f *.${O} *~