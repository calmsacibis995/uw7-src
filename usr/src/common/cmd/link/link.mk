#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)link:link.mk	1.2.5.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for link

OWN = root
GRP = bin

all: link

link: link.o
	$(CC) -o link link.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

link.o: link.c

install: all
	-rm -f $(ETC)/link
	 $(INS) -f $(USRSBIN) -m 0500 -u $(OWN) -g $(GRP) link
	-$(SYMLINK) /usr/sbin/link $(ETC)/link

clean:
	rm -f link.o

clobber: clean
	rm -f link

lintit:
	$(LINT) $(LINTFLAGS) link.c
