#ident	"@(#)kern-i386at:psm/toolkits/at_toolkit/at_toolkit.mk	1.1.1.1"
#ident	"$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = at_toolkit.mk
DIR = psm/toolkits/at_toolkit
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/at_toolkit.ln

AT_TOOLKIT = at_toolkit.cf/Driver.o

MODULES = $(AT_TOOLKIT)

FILES = \
	at_toolkit.o

CFILES = \
	at_toolkit.c

SRCFILES = $(CFILES)

LFILES = \
	at_toolkit.ln

all:	$(MODULES)

install: all
	cd at_toolkit.cf; $(IDINSTALL) -R$(CONF) -M at_toolkit 

$(AT_TOOLKIT): $(FILES)
	$(LD) -r -o $(AT_TOOLKIT) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(AT_TOOLKIT)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e at_toolkit 

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
