#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bdiff:bdiff.mk	1.12.3.1"
#ident "$Header$"
#
#

include $(CMDRULES)
OWN=bin
GRP=bin
INSDIR=$(USRBIN)

all:	bdiff

bdiff:	bdiff.o
	$(CC) $(LDFLAGS) -o bdiff bdiff.o $(SHLIBS) $(LDLIBS)

bdiff.o:	bdiff.c			\
			$(INC)/fatal.h	\
			$(INC)/signal.h	\
			$(INC)/sys/types.h	\
			$(INC)/unistd.h	\
			$(INC)/stdio.h	\
			$(INC)/ctype.h

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) bdiff

clean:
	-rm -f bdiff.o

clobber:	clean
	-rm -f bdiff
