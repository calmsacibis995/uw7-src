#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pwd:pwd.mk	1.9.2.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for pwd

OWN = bin
GRP = bin

all: pwd

pwd: pwd.o 
	$(CC) -o pwd pwd.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

pwd.o: pwd.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

clean:
	rm -f pwd.o

clobber: clean
	rm -f pwd

lintit:
	$(LINT) $(LINTFLAGS) pwd.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pwd

#	These targets are useful but optional

partslist:
	@echo pwd.mk pwd.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo pwd | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit pwd.mk $(LOCALINCS) pwd.c -o pwd.o pwd
