#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)chroot:chroot.mk	1.9.7.1"
#ident "$Header$"
#	Makefile for chroot

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN=bin
GRP=bin

#top#

MAKEFILE = chroot.mk

MAINS = chroot

OBJECTS =  chroot.o

SOURCES =  chroot.c

all:		$(MAINS)

$(MAINS):	chroot.o
	$(CC) -o chroot chroot.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

chroot.o:	 $(INC)/stdio.h $(INC)/errno.h \
		 $(INC)/sys/errno.h $(INC)/unistd.h $(INC)/priv.h

GLOBALINCS =	 $(INC)/stdio.h  $(INC)/errno.h \
		 $(INC)/sys/errno.h $(INC)/unistd.h $(INC)/priv.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all 
	-rm -f $(ETC)/chroot
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)
	-$(SYMLINK) /usr/sbin/chroot $(ETC)/chroot

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
