#ident	"@(#)kern-i386at:io/autoconf/confmgr/confmgr.mk	1.4.1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = confmgr.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/autoconf/confmgr

CONFMGR = confmgr.cf/Driver.o
LFILE = $(LINTDIR)/confmgr.ln

FILES = \
	confmgr.o \
	confmgr_p.o

CFILES = \
	confmgr.c \
	confmgr_p.c

LFILES = \
	confmgr.ln \
	confmgr_p.ln

all: $(CONFMGR)

install: all
	(cd confmgr.cf; $(IDINSTALL) -R$(CONF) -M confmgr)

$(CONFMGR): $(FILES)
	$(LD) -r -o $(CONFMGR) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(CONFMGR)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e confmgr

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
	confmgr.h \
	cm_i386at.h

headinstall:	$(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:


include $(UTSDEPEND)

include $(MAKEFILE).dep
