#ident	"@(#)kern-i386at:io/autoconf/ca/pci/pci.mk	1.5.2.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = pci.mk
KBASE = ../../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/autoconf/ca/pci 

LOCALDEF = -D_DDI_C

PCI = pci.cf/Driver.o
LFILE = $(LINTDIR)/pci.ln

MODULES = \
	$(PCI)

FILES = \
	pci_bios.o \
	pci_ca.o \
	pci_rom.o

PROBEFILE = pci_ca.c

CFILES = \
	pci_bios.c \
	pci_ca.c

SFILES = \
	pci_rom.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	pci_bios.ln \
	pci_ca.ln

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(PCI) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for x in $(PCI); do \
			if [ ! -r $$x ]; then \
				echo "ERROR: $$x is missing " 1>&2;\
				false ;\
				break ;\
			fi \
		done \
	fi

install: all
	cd pci.cf; $(IDINSTALL) -R$(CONF) -M pci

$(PCI): $(FILES)
	$(LD) -r -o $(PCI) $(FILES)



clean:
	-rm -f *.o $(LFILES) *.L 

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e pci 
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(PCI)" ;\
		rm -f $(PCI) ;\
        fi


$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done



sysHeaders = \
	pci.h

headinstall:
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	if [ -f $(PROBEFILE) ]; then \
		for f in $(sysHeaders); \
		do \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
		done; \
	fi

FRC:

include $(UTSDEPEND)
 
include $(MAKEFILE).dep

