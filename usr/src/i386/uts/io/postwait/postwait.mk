#ident	"@(#)kern-i386:io/postwait/postwait.mk	1.6.2.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	postwait.mk
KBASE = ../..
DIR = io/postwait
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/postwait.ln
POSTWAIT  = postwait.cf/Driver.o

FILES = postwait.o
CFILES = postwait.c
LFILES = postwait.ln
HEADERS = postwait.h

SRCFILES = $(CFILES)


all:	$(POSTWAIT)
install: all
	cd postwait.cf; $(IDINSTALL) -R$(CONF) -M postwait

$(POSTWAIT):	$(FILES)
	$(LD) -r -o $@ $(FILES)
	$(FUR) -W -o postwait.order $@

clean:
	-rm -f $(FILES) $(POSTWAIT)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d postwait
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

FRC:

headinstall:
	@for i in $(HEADERS); \
	do \
		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
	done


include $(UTSDEPEND)

include $(MAKEFILE).dep
