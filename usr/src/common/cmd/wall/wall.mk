#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)wall:common/cmd/wall/wall.mk	1.9.4.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for wall 

OWN = bin
GRP = tty

LDLIBS = -lw

all: wall

wall: wall.o 
	$(CC) -o wall wall.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

wall.o:  $(INC)/signal.h $(INC)/sys/signal.h \
	 $(INC)/stdio.h $(INC)/grp.h \
	 $(INC)/sys/types.h $(INC)/unistd.h \
	 $(INC)/stdlib.h $(INC)/string.h \
	 $(INC)/sys/stat.h $(INC)/utmp.h \
	 $(INC)/sys/utsname.h $(INC)/sys/dirent.h \
	 $(INC)/pwd.h $(INC)/fcntl.h \
	 $(INC)/time.h $(INC)/errno.h \
	 $(INC)/locale.h $(INC)/priv.h \
	 $(INC)/sys/errno.h $(INC)/sys/euc.h \
	 $(INC)/getwidth.h $(INC)/pfmt.h \
	 wall.h

clean:
	rm -f wall.o

clobber: clean
	rm -f wall

lintit:
	$(LINT) $(LINTFLAGS) wall.c

install: all
	-rm -f $(ETC)/wall
	 $(INS) -f $(USRSBIN) -m 02555 -u $(OWN) -g $(GRP) wall
	-$(SYMLINK) /usr/sbin/wall $(ETC)/wall

#	These targets are useful but optional

partslist:
	@echo wall.mk wall.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo wall | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit wall.mk $(LOCALINCS) wall.c -o wall.o wall
