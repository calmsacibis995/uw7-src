#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sort:sort.mk	1.4.8.1"

include $(CMDRULES)

#	Makefile for sort 

OWN = bin
GRP = bin

all: sort

sort: sort.o 
	$(CC) -o sort sort.o  $(LDFLAGS) -Kosrcrt $(LDLIBS) $(PERFLIBS)

sort.o: sort.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/time.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/values.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/sys/euc.h \
	$(INC)/limits.h \
	$(INC)/stdlib.h \
	$(INC)/ulimit.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) sort

clean:
	rm -f sort.o

clobber: clean
	rm -f sort

lintit:
	$(LINT) $(LINTFLAGS) sort.c

#	These targets are useful but optional

partslist:
	@echo sort.mk sort.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo sort | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit sort.mk $(LOCALINCS) sort.c -o sort.o sort
