#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)factor:factor.mk	1.9.1.1"
#ident "$Header$"
#	factor make file

include $(CMDRULES)
INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LDLIBS = -lm

SOURCE = factor.c

all: factor

factor:	$(SOURCE)
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o $@ factor.c $(LDLIBS) $(SHLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) factor

clean:
	rm -f factor.o

clobber:	clean
	  rm -f factor
