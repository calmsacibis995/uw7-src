#ident "@(#).mk	3.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)

KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = nat.mk
SCP_SRC = natli.c

UNIXWARE = -DUNIXWARE -D_KERNEL
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = natli.o natmac.o
LOBJS = natli.L natmac.L
DRV = nat.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd nat.cf; $(IDINSTALL) -R$(CONF) -M nat)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd nat.cf; $(IDINSTALL) -R$(CONF) -d -e nat)
