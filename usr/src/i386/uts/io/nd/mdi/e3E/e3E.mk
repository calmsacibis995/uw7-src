#
#	Copyright (C) The Windward Group, 1992.
#	All Rights Reserved.
#
#	Copyright (C) 3Com Corporation, 1992, 1993.
#	All Rights Reserved.
#
#	3Com Corporation Proprietary and Confidential
#
#	3Com EtherLink III SCO UNIX MDI Driver
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = e3E.mk
SCP_SRC = e3elli.c

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = e3elli.o e3emac.o e3eio.o
LOBJS = e3elli.L e3Emac.L
DRV = e3E.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

e3elli.o: e3E.h
e3emac.o: e3E.h
e3eio.o: e3E.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd e3E.cf; $(IDINSTALL) -R$(CONF) -M e3E)

lintit: $(LOBJS)

clean:
	rm -f $(OBJ) *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	rm -f tags
	(cd e3E.cf; $(IDINSTALL) -R$(CONF) -d -e e3E)

