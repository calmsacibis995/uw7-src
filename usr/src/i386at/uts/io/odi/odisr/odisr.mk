#ident	"@(#)odisr.mk	26.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= odisr.mk
KBASE   = ../../..
LINTDIR = $(KBASE)/lintdir
DIR     = io/odi/odisr
MOD     = odisr.cf/Driver.o
LFILE   = $(LINTDIR)/odisr.ln
BINARIES = $(MOD)
LOCALDEF = -DDL_STRLOG
PROBEFILE = srsup.c

FILES = srsup.o \
	srwrap.o 

CFILES = srsup.c \
	srwrap.c

LFILES = srsup.ln \
	srwrap.ln

SRCFILES = $(CFILES)

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
		"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

install: all
	cd odisr.cf; $(IDINSTALL) -R$(CONF) -M odisr

clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d odisr
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

headinstall: route.h
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) route.h

fnames:
	@for i in $(FILES);  \
	do                      \
		echo $$i;       \
	done

binaries: $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $@ $(FILES)

lintit:
 
include $(UTSDEPEND)

include $(MAKEFILE).dep
