#ident	"@(#)OSRcmds:i486/i486.mk	1.1"
#	@(#) i486.mk 23.2 91/02/26 
#
#	Copyright (C) The Santa Cruz Operation, 1991.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#	All rights reserved.

include $(CMDRULES)
include ../make.inc

#	Makefile for i486

INSDIR = $(OSRDIR)

# CFLAGS = -O -I$(SYSINC) -I$(INC)
CFLAGS = -g -I$(SYSINC) -I$(INC)

MAKEFILE = i486.mk

MAINS = i486

OBJECTS =  i486.o

SOURCES =  i486.c

GLOBALINCS =

all:		$(MAINS)

clean:
	rm -f $(OBJECTS)

clobber:	clean
	rm -f $(MAINS)

newmakefile:
	$(MAKE) -m -f $(MAKEFILE) -s INC $(INC)

install: all
	$(INS) -f $(INSDIR) i486

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
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
