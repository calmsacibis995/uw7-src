#ident "@(#)dex.mk	10.3"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = dex.mk 
SCP_SRC = main.c

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = main.o ctl.o ctl_subrs.o isr.o subrs.o xmt.o
LOBJS = main.L ctl.L ctl_subrs.L isr.L subrs.L xmt.L
DRV = dex.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): defs.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd dex.cf; $(IDINSTALL) -R$(CONF) -M dex)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd dex.cf; $(IDINSTALL) -R$(CONF) -d -e dex)
