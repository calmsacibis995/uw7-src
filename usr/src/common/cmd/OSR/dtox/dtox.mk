#ident	"@(#)OSRcmds:dtox/dtox.mk	1.2"
#	@(#) dtox.mk 26.1 95/08/07 
#
#	Copyright (C) The Santa Cruz Operation, 1987-1995.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, Microsoft Corporation
#	and AT&T, and should be treated as Confidential.
#
#	Modification History
#
#	sco!belal	02 Feb 94
#	- build xtod as a link to dtox
#	scol!ianw	02 Aug 95
#	- cleaned up and added lint rule

include $(CMDRULES)
include ../make.inc

MAINS	= dtox
DIR	= $(OSRDIR)
CFLAGS	= -O

all:	$(MAINS)

install:	all
	$(INS) -f $(DIR) dtox

clean:
	rm -f *.o

clobber:	clean
	rm -f $(MAINS)

lint:
	$(LINT) dtox.c
