#ident	"@(#)sp.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	sp.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/sp

SP = sp.cf/Driver.o
LFILE = $(LINTDIR)/sp.ln

FILES = \
	sp.o

CFILES = \
	sp.c

SRCFILES = $(CFILES)

LFILES = \
	sp.ln


all: $(SP)

install: all
	(cd sp.cf; $(IDINSTALL) -R$(CONF) -M sp)

$(SP): $(FILES)
	$(LD) -r -o $(SP) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e sp

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

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
