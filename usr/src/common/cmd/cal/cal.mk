#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cal:cal.mk	1.4.3.2"
#	cal make file

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN=bin
GRP=bin
SOURCE = cal.c
LDLIBS=-lm

all:	cal

cal:	$(SOURCE)
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o cal cal.c $(LDLIBS) $(SHLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) cal

clean:
	rm -f cal.o

clobber:	clean
	  rm -f cal
