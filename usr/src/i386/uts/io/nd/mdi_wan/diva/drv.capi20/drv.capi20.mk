#ident "@(#)drv.capi20.mk	29.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(UTSRULES)

MAKEFILE = drv.capi20.mk
SCP_SRC = capiux.c
KBASE = ../../../../..
# DEBUG = -DDEBUG
LOCALDEF =  -DUNIX -DUNIXWARE -D_KERNEL -D_INKERNEL $(DEBUG) 
OBJ = capiux.o message.o check.o mdi.o
LOBJS = capiux.L message.L check.L mdi.L
DRVDIR = ../EtdC.cf
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
	(cd $(DRVDIR); $(IDINSTALL) -R$(CONF) -M EtdC)

lintit:	$(LOBJS)

clean: 
	rm -f $(OBJ) $(LOBJS) tags

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd $(DRVDIR); $(IDINSTALL) -R$(CONF) -d -e EtdC)
