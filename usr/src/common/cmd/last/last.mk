#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)last:last.mk	1.4.4.1"
#ident "$Header$"

include $(CMDRULES)

#		PROPRIETARY NOTICE (Combined)
#

#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

OWN = bin
GRP = bin

all: last

last: last.o 
	$(CC) -o last last.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

last.o: last.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/utmpx.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) last

clean:
	rm -f last.o

clobber: clean
	rm -f last

lintit:
	$(LINT) $(LINTFLAGS) last.c

#	These targets are useful but optional

partslist:
	@echo last.mk last.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo last | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit last.mk $(LOCALINCS) last.c -o last.o last
