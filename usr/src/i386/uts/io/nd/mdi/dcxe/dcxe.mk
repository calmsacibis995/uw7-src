#ident "@(#)dcxe.mk	10.3"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = dcxe.mk 
SCP_SRC = dcxe_main.c

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = dcxe_main.o dcxe_intr.o dcxe_ioctl.o dcxe_lli.o dcxe_probe.o dcxe_srom.o dcxe_strcrap.o dcxe_utils.o dcxe_dbg_messages.o
LOBJS = dcxe_main.L dcxe_intr.L dcxe_ioctl.L dcxe_lli.L dcxe_probe.L dcxe_srom.L dcxe_strcrap.L dcxe_utils.L dcxe_dbg_messages.L
DRV = dcxe.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): dcxe.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd dcxe.cf; $(IDINSTALL) -R$(CONF) -M dcxe)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd dcxe.cf; $(IDINSTALL) -R$(CONF) -d -e dcxe)

