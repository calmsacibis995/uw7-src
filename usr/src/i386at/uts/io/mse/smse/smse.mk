#ident	"@(#)smse.mk	1.5"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	smse.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/mse/smse

LOCALDEF = -DESMP

SMSE = smse.cf/Driver.o
LFILE = $(LINTDIR)/smse.ln

FILES = \
	smse.o

CFILES = \
	smse.c

LFILES = \
	smse.ln


all: $(SMSE)

install: all
	(cd smse.cf; $(IDINSTALL) -R$(CONF) -M smse)

$(SMSE): $(FILES)
	$(LD) -r -o $(SMSE) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SMSE)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e smse

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
