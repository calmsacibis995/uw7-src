#ident "@(#)dcxf.mk	10.3"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = dcxf.mk 
SCP_SRC = dcxf_main.c

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = dcxf_main.o dcxf_intr.o dcxf_ioctl.o dcxf_lli.o dcxf_mii.o dcxf_probe.o dcxf_srom.o dcxf_strcrap.o dcxf_utils.o dcxf_dbg_messages.o
LOBJS = dcxf_main.L dcxf_intr.L dcxf_ioctl.L dcxf_lli.L dcxf_mii.L dcxf_probe.L dcxf_srom.L dcxf_strcrap.L dcxf_utils.L dcxf_dbg_messages.L
DRV = dcxf.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): dcxf.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd dcxf.cf; $(IDINSTALL) -R$(CONF) -M dcxf)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd dcxf.cf; $(IDINSTALL) -R$(CONF) -d -e dcxf)

