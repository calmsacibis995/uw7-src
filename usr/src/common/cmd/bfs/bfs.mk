#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bfs:bfs.mk	1.16.1.1"
#ident "$Header$"
#	bfs make file

include $(CMDRULES)
REL = current
LIST = lp
INSDIR = $(USRBIN)
OWN=bin
GRP=bin
LDLIBS =-lgen -lw
SOURCE = bfs.c
MAKE = make

compile all: bfs
	:

bfs:    bfs.c			\
		$(INC)/setjmp.h	\
		$(INC)/signal.h	\
		$(INC)/stdlib.h	\
		$(INC)/regexpr.h\
		$(INC)/limits.h	\
		$(INC)/sys/types.h	\
		$(INC)/unistd.h	\
		$(INC)/sys/stat.h
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o bfs bfs.c $(LDLIBS) $(SHLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) bfs

build:	bldmk
	get -p -r`gsid bfs $(REL)` s.bfs.c $(REWIRE) > $(RDIR)/bfs.c
bldmk:  ;  get -p -r`gsid bfs.mk $(REL)` s.bfs.mk > $(RDIR)/bfs.mk

listing:
	pr bfs.mk $(SOURCE) | $(LIST)
listmk: ;  pr bfs.mk | $(LIST)

edit:
	get -e s.bfs.c

delta:
	delta s.bfs.c
	rm -f $(SOURCE)

mkedit:  ;  get -e s.bfs.mk
mkdelta: ;  delta s.bfs.mk

clean:
	  :

clobber:  clean
	  rm -f bfs

delete:	clobber
	rm -f $(SOURCE)
