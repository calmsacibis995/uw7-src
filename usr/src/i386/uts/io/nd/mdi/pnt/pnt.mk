#ident "@(#)pnt.mk	5.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE -DENGLISH
MAKEFILE = pnt.mk
SCP_SRC = pntmac.c

UNIXWARE = -DUNIXWARE -D_KERNEL 
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = pntli.o pntmac.o 
LOBJS = pntli.L pntmac.L
DRV = pnt.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): pnt.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd pnt.cf; $(IDINSTALL) -R$(CONF) -M pnt)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd pnt.cf; $(IDINSTALL) -R$(CONF) -d -e pnt)
