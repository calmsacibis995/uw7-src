#ident	"@(#)lp.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	lp.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/lp

LP = lp.cf/Driver.o
LFILE = $(LINTDIR)/lp.ln

FILES = \
	lp.o

CFILES = \
	lp.c

LFILES = \
	lp.ln


all: $(LP)

install: all
	(cd lp.cf; $(IDINSTALL) -R$(CONF) -M lp)

$(LP): $(FILES)
	$(LD) -r -o $(LP) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(LP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e lp

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
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
	lp.h

headinstall: $(KBASE)/io/lp/lp.h
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	do \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	done

FRC:


include $(UTSDEPEND)

include $(MAKEFILE).dep
