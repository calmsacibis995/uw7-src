#ident	"@(#)xque.mk	1.7"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	xque.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/xque

XQUE = xque.cf/Driver.o
LFILE = $(LINTDIR)/xque.ln

FILES = \
	xque.o

CFILES = \
	xque.c

LFILES = \
	xque.ln

all: $(XQUE)

install: all
	(cd xque.cf; $(IDINSTALL) -R$(CONF) -M xque)

$(XQUE): $(FILES)
	$(LD) -r -o $(XQUE) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(XQUE)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e xque

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

#
# Header Install Section
#

sysHeaders = \
	xque.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
