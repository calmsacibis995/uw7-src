#ident	"@(#)kern-i386at:io/target/st01/st01.mk	1.6"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	st01.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/st01

ST01 = st01.cf/Driver.o
LFILE = $(LINTDIR)/st01.ln

FILES = st01.o
CFILES = st01.c
LFILES = st01.ln

SRCFILES = $(CFILES)

all:	$(ST01)

install:	all
		(cd st01.cf ; $(IDINSTALL) -R$(CONF) -M st01)

$(ST01):	$(FILES)
		$(LD) -r -o $(ST01) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ST01)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e st01

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
	st01.h \
	tape.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
