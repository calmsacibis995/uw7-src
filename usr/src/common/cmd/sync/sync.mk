#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sync:sync.mk	1.5.4.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for sync 

OWN = bin
GRP = bin

all: sync

sync: sync.o 
	$(CC) -o sync sync.o  $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

sync.o: sync.c

install: all
	-rm -f $(USRSBIN)/sync
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) sync
	-$(SYMLINK) /sbin/sync $(USRSBIN)/sync

clean:
	rm -f sync.o

clobber: clean
	rm -f sync

lintit:
	$(LINT) $(LINTFLAGS) sync.c

#	These targets are useful but optional

partslist:
	@echo sync.mk sync.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(SBIN) | tr ' ' '\012' | sort

product:
	@echo sync | tr ' ' '\012' | \
	sed 's;^;$(SBIN)/;'

srcaudit:
	@fileaudit sync.mk $(LOCALINCS) sync.c -o sync.o sync
