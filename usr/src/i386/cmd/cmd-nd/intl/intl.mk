#ident "@(#)intl.mk	4.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(CMDRULES)

MAKEFILE = intl.mk
OBJS =	intl.o
LOBJS =	intl.L

.c.L:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all: $(OBJS)

install: all

lintit:	$(LOBJS)

clean:
	rm -f $(OBJS) $(LOBJS) tags

clobber: clean
