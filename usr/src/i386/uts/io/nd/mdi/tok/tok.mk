#ident "@(#)tok.mk	10.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
#	IBM Token Ring Driver Makefile
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DINIT_ACA
MAKEFILE = tok.mk

# DEBUG= -DDEBUG
# CFLAGS= -O -I../.. -D_KERNEL -DUNIXWARE $(DEBUG)

OBJ = tokli.o tokmac.o
LOBJS = tokli.L tokmac.L
DRV = tok.cf/Driver.o

.SUFFIXES: .c .i .L

.c.i:
	$(CC) $(CFLAGS) -L $<

.c.L:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all: $(DRV)

$(OBJ): tok.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd tok.cf; $(IDINSTALL) -R$(CONF) -M tok)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	rm -f $(DRV)
	(cd tok.cf; $(IDINSTALL) -R$(CONF) -d -e tok)
