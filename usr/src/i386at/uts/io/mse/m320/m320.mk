#ident	"@(#)m320.mk	1.5"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	m320.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/mse/m320

LOCALDEF = -DESMP

M320 = m320.cf/Driver.o
LFILE = $(LINTDIR)/m320.ln

FILES = \
	m320.o

CFILES = \
	m320.c

LFILES = \
	m320.ln


all: $(M320)

install: all
	(cd m320.cf; $(IDINSTALL) -R$(CONF) -M m320)

$(M320): $(FILES)
	$(LD) -r -o $(M320) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(M320)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e m320

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
