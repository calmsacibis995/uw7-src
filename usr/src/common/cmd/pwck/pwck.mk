#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pwck.mk	1.3"
#ident  "$Header$"

include $(CMDRULES)

#	Makefile for pwck

OWN = bin
GRP = bin

all: pwck

pwck: pwck.o
	$(CC) -o pwck pwck.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pwck.o: pwck.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	msg.h

install: all
	-rm -f $(ETC)/pwck
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) pwck 
	-$(SYMLINK) /usr/sbin/pwck $(ETC)/pwck

clean:
	rm -f pwck.o

clobber: clean
	rm -f pwck

lintit:
	$(LINT) $(LINTFLAGS) pwck.c

#	These targets are useful but optional

partslist:
	@echo pwck.mk pwck.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo pwck | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit pwck.mk $(LOCALINCS) pwck.c -o pwck.o pwck

