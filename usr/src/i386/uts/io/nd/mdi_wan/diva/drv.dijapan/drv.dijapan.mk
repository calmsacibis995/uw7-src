#ident "@(#)drv.dijapan.mk	29.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(UTSRULES)

MAKEFILE = drv.dijapan.mk
SCP_SRC = signal.c
KBASE = ../../../../..
# DEBUG = -DDEBUG
LOCALDEF =  -DUNIX -DUNIXWARE -D_KERNEL -D_INKERNEL -DNEW_DRIVER $(DEBUG)
OBJ = signal.o te_tbl.o
LOBJS = signal.L te_tbl.L
DRVDIR = ../japan.cf
DRV = $(DRVDIR)/Driver.o
Q931DRV = ../drv.diq931/Driver.japan.o 
CTABLE = ../ditools/ctable

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(DRV): $(Q931DRV) $(OBJ)
	$(LD) -r -o $@ $(OBJ) $(Q931DRV)

$(Q931DRV):
	(cd ../drv.diq931; make -f drv.diq931.mk $(MAKEARGS))

te_tbl.o:	te_tbl.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c te_tbl.c

te_tbl.c:	$(CTABLE)
	$(CTABLE) te_tbl.asm te_tbl.c
	
$(CTABLE):
	(cd ../ditools; make -f ditools.mk $(MAKEARGS))

install: all
	(cd $(DRVDIR); $(IDINSTALL) -R$(CONF) -M japan)

lintit:	$(LOBJS)

clean: 
	rm -f $(OBJ) $(LOBJS) te_tbl.c tags

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd $(DRVDIR); $(IDINSTALL) -R$(CONF) -d -e japan)
