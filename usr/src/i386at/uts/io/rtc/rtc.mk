#ident	"@(#)kern-i386at:io/rtc/rtc.mk	1.7"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	rtc.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/rtc

RTC = rtc.cf/Driver.o
LFILE = $(LINTDIR)/rtc.ln

FILES = rtc.o

CFILES = rtc.c

SRCFILES = $(CFILES)

LFILES = rtc.ln


all: $(RTC)

install: all
	(cd rtc.cf; $(IDINSTALL) -R$(CONF) -M rtc)

$(RTC): $(FILES)
	$(LD) -r -o $(RTC) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(RTC)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e rtc

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
	rtc.h 

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
