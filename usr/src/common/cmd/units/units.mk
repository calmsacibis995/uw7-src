#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)units:units.mk	1.10.3.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for units

OWN = bin
GRP = bin

LIBDIR  = $(USRSHARE)/lib
SOURCES = units.c unittab

all: units unittab

units: units.o 
	$(CC) -o units units.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS) 

units.o: units.c \
	$(INC)/stdio.h 

clean:
	rm -f units.o

clobber: clean
	rm -f units

lintit:
	$(LINT) $(LINTFLAGS) units.c

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) units
	$(INS) -f $(LIBDIR) -m 0444 -u $(OWN) -g $(GRP) unittab

#	These targets are useful but optional

partslist:
	@echo units.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort
	@echo $(LIBDIR) | tr ' ' '\012' | sort

product:
	@echo units | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'
	@echo unittab | tr ' ' '\012' | \
	sed 's;^;$(LIBDIR)/;'

srcaudit:
	@fileaudit units.mk $(LOCALINCS) $(SOURCES) -o units.o units unittab
