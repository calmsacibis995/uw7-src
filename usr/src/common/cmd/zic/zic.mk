#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)zic:zic.mk	1.1.6.7"

include $(CMDRULES)

# PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
# Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
# (c) 1986,1987,1988,1989 Sun Microsystems, Inc
# (c) 1983,1984,1985,1986,1987,1988,1989 AT&T.
# All rights reserved.

# Makefile for zic

# If you want something other than Eastern United States time used on your
# system, change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
# zic -l rightzone

################################################################################

OWN = bin
GRP = bin

OBJECTS = zic.o scheck.o ialloc.o
SOURCES = zic.c scheck.c ialloc.c

FILES = asia australasia europe etcetera northamerica pacificnew \
	southamerica

all: zic yearistype

zic: $(OBJECTS)
	$(CC) -o zic $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ialloc.o: ialloc.c \
	$(INC)/stdio.h \
	$(INC)/string.h

scheck.o: scheck.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h

zic.o: zic.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/time.h \
	$(INC)/tzfile.h \
	$(INC)/string.h

clean:
	rm -f $(OBJECTS)
	(cd build_data; make clean)

clobber: clean
	rm -f zic
	(cd build_data; make clobber)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

install: all uxzic.str $(FILES)
	if [ ! -d $(ETC)/TZ ] ; then mkdir -p $(ETC)/TZ ; fi
	$(CH)-chmod 755 $(ETC)/TZ
	$(CH)-chgrp bin $(ETC)/TZ
	$(CH)-chown bin $(ETC)/TZ
	[ -d $(USRLIB)/locale/C/MSGFILES ]  || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 uxzic.str
	for i in $(FILES); do \
	($(INS) -f $(ETC)/TZ -m 644 $$i); done
	 $(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 555 zic
	 $(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 555 yearistype
	(cd build_data; make install)

# These targets are useful but optional

partslist:
	@echo zic.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo zic | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit zic.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) zic
