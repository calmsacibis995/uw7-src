#ident	"@(#)kern-pdi:io/target/mc01/mc01.mk	1.1"
#ident	"$Header$"

include $(UTSRULES)

DEBUG=
MAKEFILE=	mc01.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/mc01

MC01 = mc01.cf/Driver.o
LFILE = $(LINTDIR)/mc01.ln

FILES = mc01.o
CFILES = mc01.c
LFILES = mc01.ln

SRCFILES = $(CFILES)

all:	$(MC01)

install:	all
		(cd mc01.cf ; $(IDINSTALL) -R$(CONF) -M mc01)

$(MC01):	$(FILES)
		$(LD) -r -o $(MC01) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(MC01)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e mc01

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
	mc01.h 

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
