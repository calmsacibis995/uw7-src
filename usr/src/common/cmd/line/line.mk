#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)line:line.mk	1.5.5.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for line

OWN = bin
GRP = bin

all: line

line: line.o
	$(CC) -o line line.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

line.o: line.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) line

clean:
	rm -f line.o

clobber: clean
	rm -f line

lintit:
	$(LINT) $(LINTFLAGS) line.c

#	These targets are useful but optional

partslist:
	@echo line.mk line.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo line | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit line.mk $(LOCALINCS) line.c -o line.o line
