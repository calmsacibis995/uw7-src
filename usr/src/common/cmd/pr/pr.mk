#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pr:pr.mk	1.1.10.1"

include $(CMDRULES)

#	Makefile for pr 

OWN = bin
GRP = bin

LDLIBS = -lw -lgen

all: pr

pr: pr.o 
	$(CC) -o pr pr.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

pr.o: pr.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/locale.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/limits.h $(INC)/wctype.h $(INC)/widec.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pr

clean:
	rm -f pr.o

clobber: clean
	rm -f pr

lintit:
	$(LINT) $(LINTFLAGS) pr.c

#	These targets are useful but optional

partslist:
	@echo pr.mk pr.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo pr | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit pr.mk $(LOCALINCS) pr.c -o pr.o pr
