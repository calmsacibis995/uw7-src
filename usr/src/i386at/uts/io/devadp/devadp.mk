#ident	"@(#)devadp.mk	1.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	devadp.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/devadp

DEVADP = devadp.cf/Driver.o
LFILE = $(LINTDIR)/kdvm.ln

FILES = \
	devadp.o

CFILES = \
	devadp.c

LFILES = \
	devadp.ln

all: $(DEVADP)

install: all
	(cd devadp.cf; $(IDINSTALL) -R$(CONF) -M devadp)

$(DEVADP): $(FILES)
	$(LD) -r -o $(DEVADP) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(DEVADP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e devadp 

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

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
