#ident	"@(#)osocket.mk	1.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	osocket.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/osocket
LOCALDEF = -D__SOCKADDR_CONVERSION__

OSOCKET = osocket.cf/Driver.o
LFILE = $(LINTDIR)/osocket.ln

FILES = \
	osocket.o

CFILES = \
	osocket.c

SRCFILES = $(CFILES)

LFILES = \
	osocket.ln

all: $(OSOCKET)

install: all
	(cd osocket.cf; $(IDINSTALL) -R$(CONF) -M osocket)

$(OSOCKET): $(FILES)
	$(LD) -r -o $(OSOCKET) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(OSOCKET)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e osocket

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
	osocket.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
