#ident "@(#)dfx.mk	26.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1997
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE -DDEFXX
MAKEFILE = dfx.mk 
SCP_SRC = main.c

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = bus_x.o ctl.o hw.o int.o main.o rcv.o smt.o xmt.o
LOBJS = bus_x.L ctl.L hw.L int.L main.L rcv.L smt.L xmt.L
DRV = dfx.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): bus_x.h ctl.h gen.h hw.h inc_all.h int.h port.h porteisa.h portpci.h rcv.h smt.h string.h version.h xmt.h


$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd dfx.cf; $(IDINSTALL) -R$(CONF) -M dfx)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd dfx.cf; $(IDINSTALL) -R$(CONF) -d -e dfx)

