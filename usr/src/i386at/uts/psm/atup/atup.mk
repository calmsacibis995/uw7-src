#ident	"@(#)kern-i386at:psm/atup/atup.mk	1.3.3.2"
#ident	"$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = atup.mk
DIR = psm/atup
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/atup.ln

ATUP = atup.cf/Driver.o

MODULES = $(ATUP)

FILES = \
	atup.o

CFILES = \
	atup.c

SRCFILES = $(CFILES)

LFILES = \
	atup.ln

all:	$(MODULES)

install: all
	cd atup.cf; $(IDINSTALL) -R$(CONF) -M atup 

$(ATUP): $(FILES)
	$(LD) -r -o $(ATUP) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ATUP)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e atup 

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

include $(UTSDEPEND)

include $(MAKEFILE).dep
