#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)time:time.mk	1.7.5.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for time

OWN = bin
GRP = bin

all: time

time: time.o
	$(CC) -o time time.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

time.o: time.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/times.h \
	$(INC)/sys/param.h \
	$(INC)/unistd.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) time

clean:
	rm -f time.o

clobber: clean
	rm -f time

lintit:
	$(LINT) $(LINTFLAGS) time.c

#	These targets are useful but optional

partslist:
	@echo time.mk time.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo time | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit time.mk $(LOCALINCS) time.c -o time.o time
