# Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#  All Rights Reserved

# THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
# The copyright notice above does not evidence any
# actual or intended publication of such source code.

#ident	"@(#)write:write.mk	1.8.2.3"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for write 

OWN = bin
GRP = tty

LDLIBS = -lw -lgen

all: write

write: write.o 
	$(CC) -o write write.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

write.o: $(INC)/stdio.h $(INC)/signal.h \
	 $(INC)/sys/signal.h $(INC)/sys/types.h \
	 $(INC)/sys/stat.h $(INC)/sys/utsname.h \
	 $(INC)/stdlib.h $(INC)/unistd.h $(INC)/time.h \
	 $(INC)/utmp.h $(INC)/pwd.h $(INC)/fcntl.h \
	 $(INC)/locale.h $(INC)/sys/euc.h $(INC)/getwidth.h \
	 $(INC)/pfmt.h $(INC)/errno.h $(INC)/string.h \
	 $(INC)/priv.h $(INC)/sys/secsys.h

clean:
	rm -f write.o

clobber: clean
	rm -f write

lintit:
	$(LINT) $(LINTFLAGS) write.c

install: all
	 $(INS) -f $(USRBIN) -m 02555 -u $(OWN) -g $(GRP) write

# These targets are useful but optional

partslist:
	@echo write.mk write.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo write | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit write.mk $(LOCALINCS) write.c -o write.o write
