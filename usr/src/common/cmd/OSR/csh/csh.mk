#ident	"@(#)OSRcmds:csh/csh.mk	1.1"
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

include $(CMDRULES)
include ../make.inc

#	wfs 82/3/3 changed BUFSIZ from 1024 to 512
#	scol!chrisu 16 Apr 91 changed BUFSIZ back to 1024
#	scol!gregw   6 Jan 94 added -lrpc and -lsocket to LIBS.
#	scol!ianw   18 Feb 94 added -lbsd to LIBS.
#	scol!ianw   28 Jul 94 replaced -lrpc -lsocket -lbsd with $(NETLIB).

OSRINC	= ../include
CFLAGS	= -O -I$(INC) -DTELL -DBUFSIZ=1024 -DVPIX -DBSD
LIBS	= $(LDLIBS) $(NETLIB)
RM	= rm -f

DBIN	= $(OSRDIR)

ED	= -ed

#
# strings.o must be last in OBJS since it can change when previous files compile
#
OBJS	= \
	sh.o sh.dol.o sh.err.o sh.exec.o sh.exp.o sh.func.o sh.glob.o \
	sh.hist.o \
	sh.lex.o sh.misc.o sh.parse.o sh.print.o sh.sem.o sh.set.o \
	spname.o sh.wait.o alloc.o sh.init.o printf.o \
	../lib/libos/sym_mode.o ../lib/misc/f_gethfdo.o

#
# Special massaging of C files for sharing of strings
#
.c.o:	sh.local.h $(OSRINC)/osr.h $(OSRINC)/sym_mode.h
	$(CC) -c $(CFLAGS) $*.c $(LDFLAGS)

all:	csh

install: all
	$(INS) -f $(DBIN) csh

clean:
	$(RM) strings x.c xs.c *.o

clobber: clean
	$(RM) csh

cmp:	all
	cmp csh $(DBIN)/csh

lint:
	lint sh*.h sh*.c

FRC:

$(OBJS):	$(FRC)

# We are running csh split I/D because of a bug (probably in the system)
# where it only works right split I/D if you create a 1 line file as
#	set date = `date`
# and source the file.  If this works OK for you can probably run it
# shared but not I/D.  It should thrash less if you do.
#	jjd	On 68000 compile -n
csh:	$(OBJS) sh.local.h $(OSRINC)/osr.h $(OSRINC)/sym_mode.h
	$(CC) -s $(IFLAG) $(LDFLAGS) -o csh $(OBJS) $(LIBS) 
	$(PFX)size csh

sh.o:	sh.c sh.h sh.local.h $(OSRINC)/osr.h
