#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mp.cmds:common/cmd/lockstat/lockstat.mk	1.1"
#ident	"$Header$"

include $(CMDRULES)

OWN = bin
GRP = sys

LDLIBS = $(LIBELF)

all: lockstat

lockstat: lockstat.c
	$(CC) $(CFLAGS) $(DEFLIST) -D_KMEMUSER -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	-rm -f $(ETC)/lockstat
	 $(INS) -f $(USRSBIN) -m 02755 -u $(OWN) -g $(GRP) lockstat
	-$(SYMLINK) /usr/sbin/lockstat $(ETC)/lockstat

clean:
	-rm -f lockstat.o

clobber: clean
	-rm -f lockstat
	
lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
