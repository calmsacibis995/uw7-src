#ident	"@(#)in.snmpd.mk	1.5"
#ident	"$Header$"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

#
# Copyrighted as an unpublished work.
# (c) Copyright 1989 INTERACTIVE Systems Corporation
# All rights reserved.
#

#      @(#)in.snmpd.mk	1.5

#
# Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case
#

include ${CMDRULES}
LOCALINC=-I$(INC)/netmgt
LOCALDEF=-DNEW_MIB -DSVR4 -DSTRNET -D_KMEMUSER -DNETWARE

LDLIBS=-lnwutil -lgen -lsocket -lnsl -lsmux -lsnmp -lsnmpio

OBJS=snmpd.o gt_nxt_cls.o init_var.o response.o v_system.o \
	v_ifEntry.o v_icmp.o v_at.o v_ip.o v_ipRoute.o v_ipNet.o \
	v_udp.o v_udpTable.o v_tcp.o v_tcpConn.o v_snmp.o \
	sets.o general.o smuxd.o v_smux.o \
	v_pppIp.o v_pppLink.o v_ether_stats.o v_token_stats.o

LIBS =

LINTLIBS = ../lib/llib-lsnmp.ln 

INSDIR = ${USRSBIN}
ETCDIR = ${ETC}/netmgt
OWN= bin
GRP= sys

all: in.snmpd

in.snmpd: ${OBJS} ${LIBS}
	${CC} -o in.snmpd  ${OBJS} ${LIBS} ${LDLIBS} ${SHLIBS} 

snmpd.o: snmpd.c snmpd.h ${INCLUDES}
gt_nxt_cls.o: gt_nxt_cls.c snmpd.h ${INCLUDES}
init_var.o: init_var.c variables.h snmpd.h ${INCLUDES}
response.o: response.c snmpd.h ${INCLUDES}
v_system.o: v_system.c snmpd.h ${INCLUDES}
v_at.o: v_at.c snmpd.h ${INCLUDES}
v_icmp.o: v_icmp.c snmpd.h ${INCLUDES}
v_ip.o: v_ip.c snmpd.h ${INCLUDES}
v_ipRoute.o: v_ipRoute.c snmpd.h ${INCLUDES}
v_ipNet.o: v_ipNet.c snmpd.h ${INCLUDES}
v_udp.o: v_udp.c snmpd.h ${INCLUDES}
v_udpTable.o: v_udpTable.c snmpd.h ${INCLUDES}
v_tcp.o: v_tcp.c snmpd.h ${INCLUDES}
v_tcpConn.o: v_tcpConn.c snmpd.h ${INCLUDES}
v_ifEntry.o: v_ifEntry.c snmpd.h ${INCLUDES}
v_snmp.o: v_snmp.c snmpd.h ${INCLUDES}
sets.o: sets.c snmpd.h ${INCLUDES}
general.o: general.c peer.h snmpd.h ${INCLUDES}
smuxd.o: smuxd.c peer.h snmpd.h ${INCLUDES}
v_smux.o: v_smux.c peer.h snmpd.h ${INCLUDES}
v_pppIp.o: v_pppIp.c ${INCLUDES}
v_pppLink.o: v_pppLink.c ${INCLUDES}

lint:
	lint -h snmpd.c ${LINTLIBS}

install: all snmpd.conf snmpd.comm snmpd.trap
	@-[ -d $(ETCDIR) ] || mkdir -p $(ETCDIR)
	${INS} -f ${INSDIR} -m 02555 -u ${OWN} -g ${GRP} in.snmpd
	${INS} -f ${ETCDIR} -m 0444 -u ${OWN} -g bin snmpd.conf
	${INS} -f ${ETCDIR} -m 0444 -u ${OWN} -g bin snmpd.comm
	${INS} -f ${ETCDIR} -m 0444 -u ${OWN} -g bin snmpd.trap
	${INS} -f ${ETCDIR} -m 0444 -u ${OWN} -g bin snmpd.peers
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmsnmpd.str

clean clobber:
	rm -f ${OBJS} *~ in.snmpd
