#ident	"@(#)kern-i386at:io/xpoll/xpoll.mk	1.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = xpoll.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/xpoll

XPOLL = xpoll.cf/Driver.o
LFILE = $(LINTDIR)/xpoll.ln

FILES = \
	xpoll.o

CFILES = \
	xpoll.c

SRCFILES = $(CFILES)

LFILES = \
	xpoll.ln


all: $(XPOLL)

install: all
	(cd xpoll.cf; $(IDINSTALL) -R$(CONF) -M xpoll)

$(XPOLL): $(FILES)
	$(LD) -r -o $(XPOLL) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(XPOLL)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e xpoll

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

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
