#ident	"@(#)OSRcmds:lib/libgen/libgen.mk	1.1"
#
#	      UNIX is a registered trademark of AT&T
#		Portions Copyright 1976-1989 AT&T
#	Portions Copyright 1980-1989 Microsoft Corporation
#    Portions Copyright 1983-1994 The Santa Cruz Operation, Inc
#		      All Rights Reserved
#	Copyright (c) 1984, 1986, 1987, 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

include ../../make.inc

OSRINC	= ../../include

CFLAGS	= -O -I$(INC) -DTELL -DBUFSIZ=1024 -DVPIX -DBSD
LIBS	= $(LDLIBS) $(NETLIB)
RM	= rm -f

OBJS	= \
	Err.o \
	erraction.o \
	errafter.o \
	errbefore.o \
	errstrtok.o \
	errtext.o \
	zchmod.o \
	zchown.o \
	zclose.o \
	zfopen.o \
	zmalloc.o \
	zopen.o \
	zrealloc.o \
	zstat.o

#
# Special massaging of C files for sharing of strings
#
.c.o:
	$(CC) -c $(CFLAGS) $*.c 

all:	$(OBJS)
	ar -ur libgen.a $(OBJS)

clean:
	$(RM) *.o
	$(RM) *.a

clobber: clean

install: all

Err.o:		../../include/errmsg.h

erraction.o:	$(INC)/stdio.h ../../include/errmsg.h

errbefore.o:	$(INC)/varargs.h ../../include/errmsg.h

errtext.o:	$(INC)/stdio.h $(INC)/varargs.h \
		../../include/errmsg.h

errtext.o:	$(INC)/stdio.h $(INC)/varargs.h \
		../../include/errmsg.h ../../include/strselect.h

zchmod.o:	../../include/errmsg.h

zchown.o:	$(INC)/stdio.h ../../include/errmsg.h

zclose.o:	$(INC)/stdio.h ../../include/errmsg.h

zfopen.o:	$(INC)/stdio.h ../../include/errmsg.h

zmalloc.o:	$(INC)/stdio.h ../../include/errmsg.h

zopen.o:	$(INC)/fcntl.h $(INC)/stdio.h ../../include/errmsg.h

zrealloc.o:	$(INC)/stdio.h ../../include/errmsg.h

zstat.o:	$(SYSINC)/stat.h ../../include/errmsg.h

