#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)printf:printf.mk	1.3.6.1"

include $(CMDRULES)

#	Makefile for printf

OWN = bin
GRP = bin

LDLIBS = -lgen

all: printf

printf: printf.o 
	$(CC) -o printf printf.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

printf.o: printf.c \
	$(INC)/stdio.h	\
	$(INC)/stdlib.h	\
	$(INC)/errno.h	\
	$(INC)/locale.h	\
	$(INC)/string.h	\
	$(INC)/pfmt.h


install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) printf

clean:
	rm -f printf.o

clobber: clean
	rm -f printf

lintit:
	$(LINT) $(LINTFLAGS) printf.c

#	These targets are useful but optional

partslist:
	@echo printf.mk printf.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo printf | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit printf.mk $(LOCALINCS) printf.c -o printf.o printf
