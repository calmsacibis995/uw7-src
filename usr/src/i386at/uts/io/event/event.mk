#ident	"@(#)event.mk	1.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	event.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/event

EVENT = event.cf/Driver.o
LFILE = $(LINTDIR)/event.ln

FILES = \
	event.o

CFILES = \
	event.c

LFILES = \
	event.ln

all: $(EVENT)

install: all
	(cd event.cf; $(IDINSTALL) -R$(CONF) -M event)

$(EVENT): $(FILES)
	$(LD) -r -o $(EVENT) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(EVENT)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e event 

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

sysHeaders = \
	event.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
