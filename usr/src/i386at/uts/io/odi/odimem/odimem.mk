#ident	"@(#)odimem.mk	26.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= odimem.mk
KBASE   = ../../..
LINTDIR = $(KBASE)/lintdir
DIR     = io/odi/odimem
MOD     = odimem.cf/Driver.o
LFILE   = $(LINTDIR)/odimem.ln
BINARIES = $(MOD)

LOCALDEF = -DDL_STRLOG

PROBEFILE = odimem.c

FILES	= odimem.o

LFILES	= odimem.ln

CFILES	= odimem.c

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

install:all
	(cd odimem.cf; $(IDINSTALL) -R$(CONF) -M odimem)


clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d odimem
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(LINTDIR):
	-mkaux -p $@

lintit: $(LFILE)

$(LFILE):$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) :    \
			'\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
	do                      \
		echo $$i;       \
	done

binaries: $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $@ $(FILES)

odimemHeaders = \
        odimem.h

headinstall:$(odimemHeaders)
	@for f in $(odimemHeaders);     \
	do                              \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)    \
			-g $(GRP) $$f;  \
	done

include $(UTSDEPEND)
