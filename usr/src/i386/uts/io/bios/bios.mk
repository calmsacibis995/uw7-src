#ident	"@(#)kern-i386:io/bios/bios.mk	1.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	bios.mk
KBASE = ../..
DIR = io/bios
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/bios.ln
BIOS = bios.cf/Driver.o

FILES = bios.o
CFILES = bios.c
LFILES = bios.ln

SRCFILES = $(CFILES)


all:	$(BIOS)
install: all
	cd bios.cf; $(IDINSTALL) -R$(CONF) -M bios

$(BIOS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

clean:
	-rm -f $(FILES) $(BIOS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d bios
	-rm -f $(LFILE) $(LFILES) *.L

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


