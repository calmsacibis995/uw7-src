#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)setpgrp:setpgrp.mk	1.4.4.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for setpgrp

OWN = root
GRP = sys

all: setpgrp

setpgrp: setpgrp.o 
	$(CC) -o setpgrp setpgrp.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

setpgrp.o: setpgrp.c \
	$(INC)/stdio.h

clean:
	rm -f setpgrp.o

clobber: clean
	rm -f setpgrp

lintit:
	$(LINT) $(LINTFLAGS) setpgrp.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) setpgrp 

#	These targets are useful but optional

partslist:
	@echo setpgrp.mk setpgrp.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo setpgrp | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit setpgrp.mk $(LOCALINCS) setpgrp.c -o setpgrp.o setpgrp
