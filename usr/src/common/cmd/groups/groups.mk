#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)groups:groups.mk	1.6.4.2"
#ident "$Header$"

#	Makefile for groups 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin
LDLIBS = -lgen

MAKEFILE = groups.mk

MAINS = groups

OBJECTS =  groups.o

SOURCES =  groups.c

all:		$(MAINS)

groups:		groups.o	
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

groups.o:	 $(INC)/stdio.h $(INC)/grp.h $(INC)/pwd.h $(INC)/sys/sysconfig.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

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
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(SOURCES) -o $(OBJECTS) $(MAINS)
