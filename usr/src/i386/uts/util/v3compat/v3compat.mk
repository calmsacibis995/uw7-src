#ident	"@(#)kern-i386:util/v3compat/v3compat.mk	1.5.2.1"
#ident 	"$Header$"

include $(UTSRULES)

MAKEFILE=	v3compat.mk
KBASE   = ../..
LINTDIR = $(KBASE)/lintdir
MOD	= v3compat.cf/Driver.o

DFILE	= v3compat.o
CFILES	= $(DFILE:.o=.c)
LFILES	= $(DFILE:.o=.ln)
LFILE	= $(LINTDIR)/v3compat.ln
SRCFILES = $(CFILES)

all:	$(MOD)

install: all
	(cd v3compat.cf; $(IDINSTALL) -R$(CONF) -M v3compat)

$(MOD):	$(DFILE)
	$(LD) -r -o $@ $(DFILE)
	$(FUR) -W -o v3compat.order $@

clean:
	-rm -f $(DFILE) $(MOD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d v3compat
	-rm -f $(LFILES) *.L

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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
