#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)alpq.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

DIR = $(USRBIN)

OFILES=	alpq.o

MAINS= alpq

all:	$(MAINS)

alpq:	$(OFILES)
	$(CC) $(OFILES) -o $(MAINS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install:	all
	$(INS) -f $(DIR) $(MAINS)

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(MAINS)
