#ident	"@(#)OSRcmds:lib/libos/libos.mk	1.3"
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
	sym_mode.o \
	catgets_sa.o \
	errorl.o \
	errorv.o \
	libos_intl.o \
	nl_confirm.o \
	psyserrorl.o \
	psyserrorv.o \
	errmsg_fp.o \
	sysmsgstr.o

#
# Special massaging of C files for sharing of strings
#
.c.o:
	$(CC) -c $(CFLAGS) $*.c 

all:	$(OBJS)

clean:
	$(RM) *.o

clobber: clean

install: all

lint:
	lint sym_mode.c

sym_mode.o:	sym_mode.c $(OSRINC)/sym_mode.h $(OSRINC)/osr.h \
		$(INC)/stdio.h $(INC)/stdlib.h $(INC)/string.h \
		$(INC)/ctype.h $(SYSINC)/types.h $(SYSINC)/stat.h \
		$(SYSINC)/errno.h ../../include/sym_mode.h

catgets_sa.o:	$(INC)/nl_types.h $(INC)/errno.h

errorl.o:	$(INC)/unistd.h $(INC)/stdarg.h $(INC)/stdio.h

errorv.o:	$(INC)/stdio.h $(INC)/stdarg.h

libos_intl.o:	$(INC)/stdio.h libos_msg.h

nl_confirm.o:	$(INC)/nl_types.h $(INC)/langinfo.h $(INC)/string.h

psyserrorl.o:	$(INC)/unistd.h $(INC)/stdarg.h $(INC)/stdio.h

psyserrorv.o:	$(INC)/stdio.h $(INC)/stdarg.h

errmsg_fp.o:	$(INC)/unistd.h $(INC)/stdio.h

sysmsgstr.o:	$(INC)/string.h libos_intl.h libos_msg.h

