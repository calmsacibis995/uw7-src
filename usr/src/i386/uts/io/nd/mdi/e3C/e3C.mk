#ident "@(#)e3B.mk	4.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)

MAKEFILE = wdn.mk
KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = e3C.mk
SCP_SRC = e3Cmac.c

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = e3Cli.o e3Cmac.o
LOBJS = e3Cli.L e3Cmac.L
DRV = e3C.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): e3C.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd e3C.cf; $(IDINSTALL) -R$(CONF) -M e3C)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd e3C.cf; $(IDINSTALL) -R$(CONF) -d -e e3C)
