#ident "@(#)wdn.mk	10.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(UTSRULES)

MAKEFILE = wdn.mk
KBASE = ../../../..
# DEBUG = -DDEBUG
LOCALDEF = -DSCO -UM_XENIX -DUNIXWARE -D_KERNEL $(DEBUG)
SCP_SRC = wd.c
OBJ = wd.o
IDLIB = lib/board_id.o
LOBJS = wd.L lib/board_id.L
DRV = wdn.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(DRV): $(OBJ) $(IDLIB)
	$(LD) -r -o $@ $(OBJ) $(IDLIB)

$(IDLIB):
	(cd lib; $(MAKE) -f lib.mk $(MAKEARGS))

install: all
	(cd wdn.cf; $(IDINSTALL) -R$(CONF) -M wdn)

lintit:	$(LOBJS)

clean: 
	rm -f $(OBJ) $(LOBJS) tags
	if [ -f $(SCP_SRC) ]; then \
		(cd lib; $(MAKE) -f lib.mk $@ $(MAKEARGS)); \
	fi

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
		(cd lib; $(MAKE) -f lib.mk $@ $(MAKEARGS)); \
	fi
	(cd wdn.cf; $(IDINSTALL) -R$(CONF) -d -e wdn)
