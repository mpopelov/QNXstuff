O = o
A = a
CPP = c
CC = qcc
LD = qcc

CFLAGS = -O
LDFLAGS =

%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}

all: GetConf.o

distclean: clean
clean:
	rm -f .depend a.out core
	rm -f *.${O} *~