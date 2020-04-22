O = o
A = a
CPP = c
CC = qcc
LD = qcc

CFLAGS = -O -I ../
LDFLAGS =

NTCREAD = qtest

RDSRCS = qb_test.${CPP}

RDOBJS = qb_test.${O}


%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}





all: ${NTCREAD}

${NTCREAD}: ${RDOBJS}
	${LD} -o ${NTCREAD} ../qbase_client.o ${RDOBJS}
	strip ${NTCREAD}

distclean: clean

clean:
	rm -f .depend a.out core ${NTCREAD}
	rm -f *.${O} *~