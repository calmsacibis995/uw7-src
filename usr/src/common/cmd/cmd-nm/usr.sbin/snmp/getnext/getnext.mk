#ident	"@(#)getnext.mk	1.2"
#ident "$Header$"
# Copyrighted as an unpublished work.
# (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
# All rights reserved.
#

#      @(#)getnext.mk	1.5 INTERACTIVE SNMP  source

# Copyright (c) 1987, 1988 Kenneth W. Key and Jeffrey D. Case


include ${CMDRULES}

LOCALDEF=-DSVR4
LOCALINC=-I$(INC)/netmgt

LDLIBS = -lsocket -lnsl -lsnmp -lsnmpio

INCLUDES = $(INC)/netmgt/snmp.h $(INC)/netmgt/snmpuser.h

INSDIR = ${USRSBIN}
OWN = bin
GRP = bin

all: getnext

getnext: getnext.o ${LIBS}
	${CC} -o getnext ${LDFLAGS} getnext.o ${LIBS} ${LDLIBS} ${SHLIBS}

getnext.o: getnext.c ${INCLUDES}

install: all
	${INS} -f ${INSDIR} -m 0555 -u ${OWN} -g ${GRP} getnext
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmgetnext.str

clean:
	rm -f getnext.o

clobber: clean
	rm -f getnext
