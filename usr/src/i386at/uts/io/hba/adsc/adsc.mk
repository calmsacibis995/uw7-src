#ident	"@(#)kern-i386at:io/hba/adsc/adsc.mk	1.8"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	adsc.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/adsc

ATSC = adsc.cf/Driver.o
LFILE = $(LINTDIR)/adsc.ln

FILES = adsc.o
CFILES = adsc.c
LFILES = adsc.ln

SRCFILES = $(CFILES)

all:	$(ATSC)

install:	all
		(cd adsc.cf ; $(IDINSTALL) -R$(CONF) -M adsc; \
		rm -f $(CONF)/pack.d/adsc/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/adsc	)

$(ATSC):	$(FILES)
		$(LD) -r -o $(ATSC) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ATSC)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e adsc

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
	adsc.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
