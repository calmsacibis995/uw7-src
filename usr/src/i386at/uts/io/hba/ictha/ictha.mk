#ident	"@(#)kern-i386at:io/hba/ictha/ictha.mk	1.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	ictha.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/ictha

ICTHA = ictha.cf/Driver.o
LFILE = $(LINTDIR)/ictha.ln

FILES = ictha.o
CFILES = ictha.c
LFILES = ictha.ln

SRCFILES = $(CFILES)

all:	$(ICTHA)

install:	all
		(cd ictha.cf ; $(IDINSTALL) -R$(CONF) -M ictha; \
		rm -f $(CONF)/pack.d/ictha/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/ictha	)

$(ICTHA):	$(FILES)
		$(LD) -r -o $(ICTHA) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ICTHA)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e ictha

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
	ictha.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
