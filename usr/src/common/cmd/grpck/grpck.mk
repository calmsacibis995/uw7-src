#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)grpck.mk	1.2"
#ident "$Header$"

#	Makefile for grpck

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
LDLIBS = -lgen

#top#

MAKEFILE = grpck.mk

MAINS = grpck

OBJECTS =  grpck.o

SOURCES =  grpck.c

all:		$(MAINS)

$(MAINS):	$(OBJECTS)
		$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

grpck.o:	 $(INC)/stdio.h \
		 $(INC)/sys/types.h \
		 $(INC)/ctype.h \
		 $(INC)/pwd.h 

GLOBALINCS = $(INC)/ctype.h \
	$(INC)/pwd.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	-rm -f $(ETC)/grpck
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(MAINS) 
	-$(SYMLINK) /usr/sbin/grpck $(ETC)/grpck

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
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)



