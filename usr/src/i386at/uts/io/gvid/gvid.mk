#ident	"@(#)gvid.mk	1.9"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	gvid.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/gvid

GVID = gvid.cf/Driver.o
LFILE = $(LINTDIR)/genvid.ln

FILES = \
	genvid.o

CFILES = \
	genvid.c

LFILES = \
	genvid.ln

all: $(GVID)

install: all
	(cd gvid.cf; $(IDINSTALL) -R$(CONF) -M gvid)

$(GVID): $(FILES)
	$(LD) -r -o $(GVID) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(GVID)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e gvid

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
	evc.h \
	genvid.h \
	vdc.h \
	vid.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
