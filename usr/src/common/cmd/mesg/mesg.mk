#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mesg:mesg.mk	1.5.5.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for mesg 

OWN = bin
GRP = bin

all: mesg

mesg: mesg.o 
	$(CC) -o mesg mesg.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mesg.o: mesg.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) mesg

clean:
	rm -f mesg.o

clobber: clean
	rm -f mesg

lintit:
	$(LINT) $(LINTFLAGS) mesg.c

#	These targets are useful but optional

partslist:
	@echo mesg.mk mesg.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo mesg | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit mesg.mk $(LOCALINCS) mesg.c -o mesg.o mesg
