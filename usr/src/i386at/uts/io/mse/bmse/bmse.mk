#ident	"@(#)bmse.mk	1.5"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	bmse.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/mse/bmse

LOCALDEF = -DESMP

BMSE = bmse.cf/Driver.o
LFILE = $(LINTDIR)/bmse.ln

FILES = \
	bmse.o

CFILES = \
	bmse.c

LFILES = \
	bmse.ln


all: $(BMSE)

install: all
	(cd bmse.cf; $(IDINSTALL) -R$(CONF) -M bmse)

$(BMSE): $(FILES)
	$(LD) -r -o $(BMSE) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(BMSE)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e bmse

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
