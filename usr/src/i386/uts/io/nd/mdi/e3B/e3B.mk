#ident "@(#)e3B.mk	5.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = e3B.mk

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = e3Bli.o e3Bmac.o e3Bio.o
LOBJS = e3Bli.L e3Bmac.L
DRV = e3B.cf/Driver.o

.c.L:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all: $(DRV)

e3Bio.o: e3Bio.s
	$(CC) -c e3Bio.s

$(OBJ): e3B.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd e3B.cf; $(IDINSTALL) -R$(CONF) -M e3B)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	rm -f $(DRV)
	(cd e3B.cf; $(IDINSTALL) -R$(CONF) -d -e e3B)
