#ident	"@(#)kern-i386:io/prf/prf.mk	1.4"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	prf.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/prf

PRF = prf.cf/Driver.o
LFILE = $(LINTDIR)/prf.ln

FILES = \
	prf.o modprf.o

CFILES = \
	prf.c modprf.c

SRCFILES = $(CFILES)

LFILES = \
	prf.ln modprf.ln

all: $(PRF)

install: all
	(cd prf.cf; $(IDINSTALL) -R$(CONF) -M prf)

$(PRF): $(FILES)
	$(LD) -r -o $(PRF) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(PRF)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e prf

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

sysHeaders = \
	prf.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
