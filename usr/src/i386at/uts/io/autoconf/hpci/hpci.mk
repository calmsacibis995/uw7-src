#ident	"@(#)kern-i386at:io/autoconf/hpci/hpci.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	hpci.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hpci

HPCI = hpci.cf/Driver.o
LFILE = $(LINTDIR)/hpci.ln

FILES = \
	hpci.o hpcipci.o hpci_bios.o

CFILES = \
	hpci.c \
	hpcipci.c \
	hpci_bios.c

LFILES = \
	hpci.ln \
	hpcipci.ln \
	hpci_bios.ln

all: $(HPCI)

install: all
	(cd hpci.cf; $(IDINSTALL) -R$(CONF) -M hpci)

$(HPCI): $(FILES)
	$(LD) -r -o $(HPCI) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(HPCI)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e hpci 

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
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
	hpci.h \
	hpci_bios.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
