#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)du:du.mk	1.6.7.1"
#ident "$Header$"

#	Makefile for du 

include $(CMDRULES)

LOCALDEF = -D_FILE_OFFSET_BITS=64

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

MAKEFILE = du.mk


MAINS = du

OBJECTS =  du.o

SOURCES =  du.c

all:		$(MAINS)

du:		du.o 
	$(CC) -o $@  du.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)


du.o:	 $(INC)/sys/types.h \
	 $(INC)/sys/stat.h \
	 $(INC)/dirent.h \
	 $(INC)/limits.h \
	 $(INC)/stdio.h \
	 $(INC)/stdlib.h \
	 $(INC)/string.h \
	 $(INC)/unistd.h \
	 $(INC)/locale.h \
	 $(INC)/pfmt.h \
	 $(INC)/errno.h

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
