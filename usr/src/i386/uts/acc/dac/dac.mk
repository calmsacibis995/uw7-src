#ident	"@(#)dac.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	dac.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/dac

DAC = dac.cf/Driver.o
LFILE = $(LINTDIR)/dac.ln

FILES = \
	gendac.o \
	vndac.o \
	ipcdac.o

CFILES = \
	gendac.c \
	vndac.c \
	ipcdac.c

SRCFILES = $(CFILES)

LFILES = \
	gendac.ln \
	vndac.ln \
	ipcdac.ln

all:	$(DAC)

install: all
	(cd dac.cf; $(IDINSTALL) -R$(CONF) -M dac)

$(DAC): $(FILES)
	$(LD) -r -o $(DAC) $(FILES)
clean:
	-rm -f *.o $(LFILES) *.L $(DAC)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e dac

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

sysHeaders = \
	acl.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
