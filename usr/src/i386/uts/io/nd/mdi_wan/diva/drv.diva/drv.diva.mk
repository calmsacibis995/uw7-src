#ident "@(#)drv.diva.mk	29.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(UTSRULES)

MAKEFILE = drv.diva.mk
SCP_SRC = di_unix.c
KBASE = ../../../../..
# DEBUG = -DDEBUG
LOCALDEF = -v -DUNIX -DUNIXWARE -D_KERNEL -D_INKERNEL -DNEW_DRIVER $(DEBUG)
OBJ = di_unix.o 
LOBJS = di_unix.L 
DRVDIR = ../EtdD.cf
DRV = $(DRVDIR)/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd $(DRVDIR); $(IDINSTALL) -R$(CONF) -M EtdD)

lintit:	$(LOBJS)

clean: 
	rm -f $(OBJ) $(LOBJS) tags

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd $(DRVDIR); $(IDINSTALL) -R$(CONF) -d -e EtdD)
