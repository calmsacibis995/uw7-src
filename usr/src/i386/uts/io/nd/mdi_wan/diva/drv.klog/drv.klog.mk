#ident "@(#)drv.klog.mk	29.1"

include $(UTSRULES)

MAKEFILE = drv.klog.mk
SCP_SRC = klog.c

KBASE = ../../../../..
LOCALDEF = -v -DUNIX -DUNIXWARE -D_INKERNEL -DKERNEL -DNEW_DRIVER $(DEBUG)
# DEBUG = -DDEBUG
OBJ = klog.o kprintf.o 
LOBJS = klog.L kprintf.L
DRVDIR = ../EtdK.cf
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
	(cd $(DRVDIR); $(IDINSTALL) -R$(CONF) -M EtdK)

lintit: $(LOBJS)

clean:
	rm -f $(OBJ) 

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd $(DRVDIR); $(IDINSTALL) -R$(CONF) -d -e EtdK)
