#
#	Copyright (C) The Windward Group, 1992.
#	All Rights Reserved.
#
#	Copyright (C) 3Com Corporation, 1992, 1993, 1994, 1995.
#	All Rights Reserved.
#
#	3Com Corporation Proprietary and Confidential
#
#	3Com 3C59x SCO UNIX MDI Driver
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE -DBUS_MASTER
MAKEFILE = e3G.mk
SCP_SRC = e3Glli.c

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = e3Glli.o e3Gmac.o e3Gio.o e3Gpcieisa.o
DRV = e3G.cf/Driver.o

.C.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

e3Glli.o: e3G.h
e3Gmac.o: e3G.h
e3Gio.o:  e3G.h
e3Gpcieisa.o: e3G.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd e3G.cf; $(IDINSTALL) -R$(CONF) -M e3G)

clean:
	rm -f $(OBJ)

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	rm -f tags
	(cd e3G.cf; $(IDINSTALL) -R$(CONF) -d -e e3G)
