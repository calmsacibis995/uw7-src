# Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#  All Rights Reserved

# THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
# The copyright notice above does not evidence any
# actual or intended publication of such source code.

#ident	"@(#)xargs:xargs.mk	1.7.3.3"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for xargs

OWN = bin
GRP = bin

all: xargs

xargs: xargs.o 
	$(CC) -o xargs xargs.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

xargs.o: $(INC)/sys/types.h \
	 $(INC)/unistd.h \
	 $(INC)/fcntl.h \
	 $(INC)/string.h \
	 $(INC)/sys/wait.h \
	 $(INC)/stdlib.h \
	 $(INC)/ctype.h \
	 $(INC)/pfmt.h \
	 $(INC)/locale.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) xargs

clean:
	rm -f xargs.o

clobber: clean
	rm -f xargs

lintit:
	$(LINT) $(LINTFLAGS) xargs.c

# These targets are useful but optional

partslist:
	@echo xargs.mk xargs.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo xargs | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit xargs.mk $(LOCALINCS) xargs.c -o xargs.o xargs
