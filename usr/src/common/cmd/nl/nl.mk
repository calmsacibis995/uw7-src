#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nl:nl.mk	1.4.4.3"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for nl

OWN = bin
GRP = bin

LDLIBS =

all: nl

nl: nl.o
	$(CC) -o nl nl.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

nl.o: nl.c \
	$(INC)/stdio.h \
	$(INC)/regexpr.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) nl

clean:
	rm -f nl.o

clobber: clean
	rm -f nl


lintit:
	$(LINT) $(LINTFLAGS) nl.c

