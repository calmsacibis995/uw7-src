#ident	"@(#)consem.mk	1.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	consem.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/consem

CONSEM = consem.cf/Driver.o
LFILE = $(LINTDIR)/consem.ln

FILES = \
	consem.o

CFILES = \
	consem.c

SRCFILES = $(CFILES)

LFILES = \
	consem.ln

all: $(CONSEM)

install: all
	(cd consem.cf; $(IDINSTALL) -R$(CONF) -M consem)

$(CONSEM): $(FILES)
	$(LD) -r -o $(CONSEM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(CONSEM)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e consem

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
	consem.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
