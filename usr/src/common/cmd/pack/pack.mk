#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pack:pack.mk	1.10.5.2"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for pack

OWN = bin
GRP = bin

LOCAL_LDLIBS = $(LDLIBS) -lgen
all: pack

pack: pack.o 
	$(CC) -o pack pack.o  $(LDFLAGS) $(LOCAL_LDLIBS) $(PERFLIBS)

pack.o: pack.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pack

clean:
	rm -f pack.o

clobber: clean
	rm -f pack

lintit:
	$(LINT) $(LINTFLAGS) pack.c

#	These targets are useful but optional

partslist:
	@echo pack.mk pack.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo pack | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit pack.mk $(GLOBALINCS) pack.c -o pack.o pack
