#ident	"@(#)cmux.mk	1.7"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	cmux.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/cmux

CMUX = cmux.cf/Driver.o
LFILE = $(LINTDIR)/cmux.ln

FILES = \
	chanmux.o

CFILES = \
	chanmux.c

LFILES = \
	chanmux.ln

all:	$(CMUX)

install: all
	(cd cmux.cf; $(IDINSTALL) -R$(CONF) -M cmux)

$(CMUX): $(FILES)
	$(LD) -r -o $(CMUX) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(CMUX)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e cmux

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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

#
# Header Install Section
#
sysHeaders = \
	chanmux.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep
