#ident	"@(#)kern-i386:psm/abon/abon.mk	1.2.2.1"

LOCALDEF=-D_PSM=2 -DHIDE_NON_BOOT_CG_ADAPTECS
LOCALINC = -I.

include $(UTSRULES)

MAKEFILE = abon.mk
DIR = psm/abon
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/abon.ln

ABON = abon.cf/Driver.o
PROBEFILE = abon.c

MODULES = $(ABON)
PROBEFILE = abon.c

FILES = \
	abon_lock.o \
	abon.o

CFILES = \
	abon_lock.c \
	abon.c

SFILES = 



SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	abon.ln

all:
	-@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(MODULES) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(MODULES) ]; then \
			echo "ERROR: $(MODULES) is missing" 1>&2; \
			false; \
		fi \
	fi



install: all
	cd abon.cf; $(IDINSTALL) -R$(CONF) -M abon

$(ABON): $(FILES) 
	$(LD) -r -o $(ABON) $(FILES) 


clean:
	-rm -f *.o $(LFILES) *.L $(ABON)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e abon 

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

headinstall:

#include $(UTSDEPEND)

#include $(MAKEFILE).dep
