#ident	"@(#)kern-i386at:io/cpyrt/cpyrt.mk	1.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = cpyrt.mk
KBASE    = ../..
LINTDIR  = $(KBASE)/lintdir
DIR      = io/cpyrt

CPYRT = cpyrt.cf/Driver.o
LFILE = $(LINTDIR)/cpyrt.ln

DFILES = cpyrt.o

CFILES = $(DFILES:.o=.c)

LFILES = $(DFILES:.o=.ln)


all:	$(CPYRT)

install: all
	cd cpyrt.cf; $(IDINSTALL) -R$(CONF) -M cpyrt 

$(CPYRT): $(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Cleanup Section
#
clean:
	-rm -f $(DFILES) $(LFILES) *.L $(CPYRT)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d cpyrt

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
	@for i in $(CFILES); do \
		echo $$i; \
	done

headinstall:


include $(UTSDEPEND)

include $(MAKEFILE).dep
