#ident	"@(#)OSRcmds:lib/misc/misc.mk	1.1"
#	@(#) csh.mk 25.3 94/07/28 
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

CFLAGS	= -O -I$(INC) -DTELL -DBUFSIZ=1024 -DVPIX -DBSD
LIBS	= $(LDLIBS) $(NETLIB)
RM	= rm -f

OBJS	= \
	f_gethfdo.o \
	blockmode.o

#
# Special massaging of C files for sharing of strings
#
.c.o:
	$(CC) -c $(CFLAGS) $*.c 

all:	$(OBJS)

clean:
	$(RM) *.o

clobber: clean

lint:
	lint f_gethfdo.c

FRC:

$(OBJS):	$(FRC)

f_gethfdo.o:	f_gethfdo.c $(SYSINC)/time.h $(SYSINC)/resource.h \
			$(SYSINC)/types.h $(SYSINC)/fcntl.h $(INC)/unistd.h

blockmode.o:	blockmode.c $(SYSINC)/types.h $(SYSINC)/systm.h \
			$(SYSINC)/stat.h $(SYSINC)/param.h $(INC)/unistd.h \
			../../include/osr.h

install: all
