#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)banner:banner.mk	1.4.3.1"
#ident "$Header$"
#	banner make file

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN=bin
GRP=bin
LDLIBS=-lm

SOURCE = banner.c

all:	$(SOURCE)
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o banner banner.c $(LDLIBS) $(SHLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) banner

clean:
	rm -f banner.o

clobber:	clean
	  rm -f banner
