#ident	"@(#)mfpd.mk	1.1"

include $(UTSRULES)

MAKEFILE=mfpd.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/mfpd

MFPD = mfpd.cf/Driver.o
LFILE = $(LINTDIR)/mfpd.ln

ASFLAGS = -m

FILES = mfpd.o \
	mfpdhwrtns.o
CFILES = mfpd.c \
	 mfpdhwrtns.c
LFILES = mfpd.ln \
	 mfpdhwrtns.ln


SRCFILES = $(CFILES)

all:	$(MFPD)

install:	all
		( \
		cd mfpd.cf ; $(IDINSTALL) -R$(CONF) -M mfpd; \
		)


$(MFPD):	$(FILES)
		$(LD) -r -o $(MFPD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(MFPD)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e mfpd

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


sysHeaders = mfpd.h \
	     mfpdhw.h


headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:


include $(UTSDEPEND)

include $(MAKEFILE).dep
