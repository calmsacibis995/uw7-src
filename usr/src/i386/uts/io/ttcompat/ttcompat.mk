#ident	"@(#)ttcompat.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	ttcompat.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/ttcompat

TTCOMPAT = ttcompat.cf/Driver.o
LFILE = $(LINTDIR)/ttcompat.ln

FILES = \
	ttcompat.o

CFILES = \
	ttcompat.c

SRCFILES = $(CFILES)

LFILES = \
	ttcompat.ln


all: $(TTCOMPAT)

install: all
	(cd ttcompat.cf; $(IDINSTALL) -R$(CONF) -M ttcompat)

$(TTCOMPAT): $(FILES)
	$(LD) -r -o $(TTCOMPAT) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(TTCOMPAT)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ttcompat

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
