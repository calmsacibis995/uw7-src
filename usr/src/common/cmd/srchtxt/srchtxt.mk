#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)srchtxt.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for srchtxt

OWN = bin
GRP = bin

LDLIBS = -lgen -lw

all: srchtxt

srchtxt: srchtxt.o 
	$(CC) -o srchtxt srchtxt.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

srchtxt.o: srchtxt.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/dirent.h \
	$(INC)/regexpr.h \
	$(INC)/string.h \
	$(INC)/fcntl.h \
	$(INC)/locale.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/sys/types.h \
	$(INC)/sys/file.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/stat.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) srchtxt

clean:
	rm -f srchtxt.o

clobber: clean
	rm -f srchtxt

lintit:
	$(LINT) $(LINTFLAGS) srchtxt.c

#	These targets are useful but optional

partslist:
	@echo srchtxt.mk srchtxt.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo srchtxt | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit srchtxt.mk $(LOCALINCS) srchtxt.c -o srchtxt.o srchtxt
