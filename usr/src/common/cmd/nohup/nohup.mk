#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nohup:nohup.mk	1.5.4.1"
#ident "$Header$"

include $(CMDRULES)

#	nohup make file

OWN = bin
GRP = bin

all: nohup

nohup: nohup.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

nohup.o: nohup.c \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) nohup

clean:
	rm -f nohup.o

clobber: clean
	 rm -f nohup

lintit:
	$(LINT) $(LINTFLAGS) nohup.c
