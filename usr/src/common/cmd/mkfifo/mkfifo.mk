#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mkfifo:mkfifo.mk	1.1.3.2"

include $(CMDRULES)

#	Makefile for mkfifo

OWN = bin
GRP = bin

all: mkfifo

mkfifo: mkfifo.o 
	$(CC) -o mkfifo mkfifo.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mkfifo.o: mkfifo.c \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h	\
	$(INC)/stdlib.h	\
	$(INC)/locale.h	\
	$(INC)/pfmt.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) mkfifo

clean:
	rm -f mkfifo.o

clobber: clean
	rm -f mkfifo

lintit:
	$(LINT) $(LINTFLAGS) mkfifo.c

#	These targets are useful but optional

partslist:
	@echo mkfifo.mk mkfifo.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo mkfifo | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit mkfifo.mk $(LOCALINCS) mkfifo.c -o mkfifo.o mkfifo
