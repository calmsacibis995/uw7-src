#ident	"@(#)kern-i386at:io/pccard/pcic/pcic.mk	1.1"


include $(UTSRULES)

MAKEFILE=	pcic.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/pccard/pcic

PCIC = pcic.cf/Driver.o

LFILE = $(LINTDIR)/pcic.ln

FILES =  pcic.o

CFILES = pcic.c

LFILES =  pcic.ln

all:	$(PCIC) 
	
install: all
	cd pcic.cf; $(IDINSTALL) -R$(CONF) -M pcic

$(PCIC):	pcic.o
	$(LD) -r -o $@ pcic.o

#
# Configuration Section
#


clean:
	-rm -f $(FILES) $(PCIC)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d pcic
	-rm -f $(LFILES) *.L

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done


FRC:

#
# Header Install Section
#

sysHeaders = 

headinstall: $(sysHeaders)
	:

include $(UTSDEPEND)

include $(MAKEFILE).dep
