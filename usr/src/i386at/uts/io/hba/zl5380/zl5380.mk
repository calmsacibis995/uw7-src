#ident	"@(#)kern-pdi:io/hba/zl5380/zl5380.mk	1.1.2.1"

#
# Makefile for ZL5380
#

include $(UTSRULES)

MAKEFILE=zl5380.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/zl5380

ZL5380 = zl5380.cf/Driver.o
LFILE = $(LINTDIR)/zl5380.ln

FILES = hba.o zl5380.o t160.o mvpas16.o 

CFILES = hba.c zl5380.c t160.c mvpas16.c 

LFILES = hba.ln zl5380.ln t160.ln mvpas16.ln

SRCFILES = $(CFILES)

all:	$(ZL5380) 

install:	all
		(cd zl5380.cf ; $(IDINSTALL) -R$(CONF) -M zl5380; \
		rm -f $(CONF)/pack.d/zl5380/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/zl5380	)

$(ZL5380):	$(FILES)
		$(LD) -r -o $(ZL5380) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ZL5380)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e zl5380

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
	zl5380.h t160.h mvpas16.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)
