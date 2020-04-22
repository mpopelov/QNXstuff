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

CFLAGS = -g -I ../getconf -I .
LDFLAGS = -g -lsocket

QXVPN = QX-vpn
DEVNVPN = devn-vpn.so

VPNSRCS = qx_vpn_log.${CPP} qx_vpn_ppp.${CPP} qx_vpn_cc.${CPP} qx_vpn_main.${CPP}
VPNOBJS = qx_vpn_log.${O} qx_vpn_ppp.${O} qx_vpn_cc.${O} qx_vpn_main.${O}

DEVNSRC = devn_vpn.${CPP}
DEVNOBJ = devn_vpn.${O}

%.${O} : %.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}










#all: ${QXVPN} ${DEVNVPN}
all: ${QXVPN}

${QXVPN}: ${VPNOBJS}
	${LD} -o ${QXVPN} ../getconf/GetConf.o ${VPNOBJS} ${LDFLAGS}
#	strip ${QXVPN}
	
#${DEVNVPN}: ${DEVNOBJ}
#	${LD} -o ${DEVNVPN} ${DEVNOBJ}
#	strip ${DEVNVPN}

distclean: clean

clean:
	rm -f .depend a.out core ${QXVPN} ${DEVNVPN}
	rm -f *.${O} *~