#ident	"@(#)kern-i386at:io/autoconf/ca/mca/mca.mk	1.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = mca.mk
KBASE = ../../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/autoconf/ca/mca 

MCA = mca.cf/Driver.o
LFILE = $(LINTDIR)/mca.ln

MODULES = \
	$(MCA)

FILES = \
	mca.o

CFILES = \
	mca.c

SRCFILES = $(CFILES)

LFILES = \
	mca.ln

all:	$(MODULES)

install: all
	cd mca.cf; $(IDINSTALL) -R$(CONF) -M mca

$(MCA): $(FILES)
	$(LD) -r -o $(MCA) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(MCA)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e mca 

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
	mca.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)
 
include $(MAKEFILE).dep

