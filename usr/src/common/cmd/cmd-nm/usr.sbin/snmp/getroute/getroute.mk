#ident	"@(#)getroute.mk	1.2"
#ident "$Header$"
# Copyrighted as an unpublished work.
# (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
# All rights reserved.
#

#      @(#)getroute.mk	1.5 INTERACTIVE SNMP  source

# Copyright (c) 1987, 1988 Kenneth W. Key and Jeffrey D. Case


include ${CMDRULES}

LOCALDEF=-DSVR4
LOCALINC=-I$(INC)/netmgt

LDLIBS = -lsocket -lnsl -lsnmp -lsnmpio

INCLUDES = $(INC)/netmgt/snmp.h $(INC)/netmgt/snmpuser.h

INSDIR = ${USRSBIN}
OWN = bin
GRP = bin

all: getroute

getroute: getroute.o ${LIBS}
	${CC} -o getroute ${LDFLAGS} getroute.o ${LIBS} ${LDLIBS} ${SHLIBS}

getroute.o: getroute.c ${INCLUDES}

install: all
	${INS} -f ${INSDIR} -m 0555 -u ${OWN} -g ${GRP} getroute
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmgetroute.str

clean:
	rm -f getroute.o

clobber: clean
	rm -f getroute
