#ident "@(#)lib.mk	4.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(UTSRULES)
MAKEFILE = lib.mk
KBASE = ../../../../..
# DEBUG = -DDEBUG
LOCALDEF = -DUNIXWARE $(DEBUG)
LOBJS = board_id.L

.c.L:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all: board_id.o

lintit:	$(LOBJS)

clean clobber:
	rm -f *.L
	rm -f board_id.o
