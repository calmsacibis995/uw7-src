#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fgrep:fgrep.mk	1.8.4.2"

#	Makefile for fgrep

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LDLIBS=

#top#
# Generated by makefile 1.47

MAKEFILE = fgrep.mk


MAINS = fgrep

OBJECTS =  fgrep.o

SOURCES =  fgrep.c

all:		$(MAINS)

fgrep:		fgrep.o	
	$(CC) -o $@ fgrep.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)


fgrep.o:	 $(INC)/stdio.h $(INC)/sys/euc.h \
		 $(INC)/pfmt.h $(INC)/errno.h $(INC)/limits.h

GLOBALINCS = $(INC)/stdio.h $(INC)/sys/euc.h $(INC)/getwidth.h \
		 $(INC)/pfmt.h $(INC)/errno.h $(INC)/limits.h


clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

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