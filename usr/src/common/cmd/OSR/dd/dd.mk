#ident	"@(#)OSRcmds:dd/dd.mk	1.1"
#	@(#) dd.mk 26.1 96/01/05 
#
#	      UNIX is a registered trademark of AT&T
#		Portions Copyright 1976-1989 AT&T
#	Portions Copyright 1980-1989 Microsoft Corporation
#    Portions Copyright 1983-1996 The Santa Cruz Operation, Inc
#		      All Rights Reserved
#	Copyright (c) 1984, 1986, 1987, 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#
#	Modification History
#	scol!anthonys	13 Aug 92
#	- Added message catalog and XPG/Posix rules and flags.
#	scol!donaldp	16 Feb 95
#	- Added support for multiple message catalogues.
#	scol!ashleyb	4 Jan 96
#	- Cleaned up.
#

include $(CMDRULES)
include ../make.inc

#	Makefile for dd

INTL=-DINTL

DIR = $(OSRDIR)

LDOBJS = ../lib/misc/blockmode.o ../lib/libos/errorl.o ../lib/libos/errorv.o \
	../lib/libos/psyserrorl.o ../lib/libos/psyserrorv.o \
	../lib/libos/sysmsgstr.o ../lib/libos/libos_intl.o \
	../lib/libos/catgets_sa.o

LFLAGS = $(LDOBJS)
# CFLAGS = -O $(XPGUTIL) -I$(INC)
CFLAGS = -O -I$(INC) -s

MAINS = dd

OBJECTS = dd.o

SOURCES = dd.c

all:	$(MAINS)

install: all
	$(DOCATS) -d NLS $@
	$(INS) -f $(DIR) dd

dd:	dd.o ../include/osr.h
	$(CC) $(CFLAGS) -o dd dd.o $(LDFLAGS) $(LFLAGS)


dd.o:	dd_msg.h

dd_msg.h:	NLS/en/dd.gen
	$(MKCATDEFS) dd $? >/dev/null
	cat dd_msg.h | sed 's/"dd.cat@Unix"/"OSRdd.cat@Unix"/g' > dd_msgt.h
	mv dd_msgt.h dd_msg.h

clean:
	rm -f $(OBJECTS) dd_msg.h

clobber: clean
	rm -f $(MAINS)
	$(DOCATS) -d NLS $@
