#ident	"@(#)mac.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	mac.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/mac

MAC = mac.cf/Driver.o
LFILE = $(LINTDIR)/mac.ln

FILES = \
	covert.o \
	ipcmac.o \
	genmac.o \
	procmac.o \
	vnmac.o 

CFILES = \
	covert.c \
	ipcmac.c \
	genmac.c \
	procmac.c \
	vnmac.c 

SRCFILES = $(CFILES)

LFILES = \
	covert.ln \
	ipcmac.ln \
	genmac.ln \
	procmac.ln \
	vnmac.ln 

all:	$(MAC)

install: all
	(cd mac.cf; $(IDINSTALL) -R$(CONF) -M mac)

$(MAC): $(FILES)
	$(LD) -r -o $(MAC) $(FILES)
clean:
	-rm -f *.o $(LFILES) *.L $(MAC)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e mac

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
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	covert.h \
        mac_hier.h \
	mac.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
