#ident	"@(#)kern-i386at:io/target/sw01/sw01.mk	1.4"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	sw01.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/sw01

SW01 = sw01.cf/Driver.o
LFILE = $(LINTDIR)/sw01.ln

FILES = sw01.o
CFILES = sw01.c
LFILES = sw01.ln

SRCFILES = $(CFILES)

all:	$(SW01)

install:	all
		(cd sw01.cf ; $(IDINSTALL) -R$(CONF) -M sw01)

$(SW01):	$(FILES)
		$(LD) -r -o $(SW01) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SW01)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e sw01

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
	worm.h \
	sw01.h 

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
