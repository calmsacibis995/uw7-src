#ident	"@(#)kern-i386at:psm/toolkits/psm_i8259/psm_i8259.mk	1.3"
#ident	"$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = psm_i8259.mk
DIR = psm/toolkits/psm_i8259
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/psm_i8259.ln

PSM_I8259 = psm_i8259.cf/Driver.o

MODULES = $(PSM_I8259)

FILES = \
	psm_i8259.o

CFILES = \
	psm_i8259.c

SRCFILES = $(CFILES)

LFILES = \
	psm_i8259.ln

all:	$(MODULES)

install: all
	cd psm_i8259.cf; $(IDINSTALL) -R$(CONF) -M psm_i8259 

$(PSM_I8259): $(FILES)
	$(LD) -r -o $(PSM_I8259) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(PSM_I8259)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e psm_i8259 

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
