O = o
A = a
CPP = c
CC = qcc
LD = qcc

CFLAGS = -V3.3.5,gcc_ntox86 -O -I ../
LDFLAGS = -V3.3.5,gcc_ntox86

NTCREAD = qb_client

RDSRCS = qb_client.${CPP}

RDOBJS = qb_client.${O}


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