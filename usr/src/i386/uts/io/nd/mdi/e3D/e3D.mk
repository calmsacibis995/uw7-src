#ident "@(#)e3D.mk	3.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)

KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = e3D.mk

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
#CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = e3Dli.o e3Dmac.o
LOBJS = e3Dli.L e3Dmac.L
DRV = e3D.cf/Driver.o

.c.L:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all: $(DRV)

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd e3D.cf; $(IDINSTALL) -R$(CONF) -M e3D)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	rm -f $(DRV)
	(cd e3D.cf; $(IDINSTALL) -R$(CONF) -d -e e3D)
