#ident	"@(#)kern-i386at:io/target/sp01/sp01.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	sp01.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/sp01

SP01 = sp01.cf/Driver.o
LFILE = $(LINTDIR)/sp01.ln

FILES = sp01.o
CFILES = sp01.c
LFILES = sp01.ln

SRCFILES = $(CFILES)

all:	$(SP01)

install:	all
		(cd sp01.cf ; $(IDINSTALL) -R$(CONF) -M sp01)

$(SP01):	$(FILES)
		$(LD) -r -o $(SP01) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SP01)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e sp01

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
	sp01.h 

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
