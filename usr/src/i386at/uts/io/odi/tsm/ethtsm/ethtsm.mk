#ident	"@(#)ethtsm.mk	26.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= ethtsm.mk
KBASE	= ../../../..
LINTDIR = $(KBASE)/lintdir
DIR	= io/odi/tsm/ethtsm
MOD	= ethtsm.cf/Driver.o
LFILE	= $(LINTDIR)/ethtsm.ln
BINARIES = $(MOD)
PROBEFILE = ethertsm.c
LOCALDEF = $(DEBUGDEF)

FILES	= \
	ethwrap.o \
	ethertsm.o \
	etherglu.o

LFILES 	= \
	ethwrap.ln \
	ethertsm.ln \
	etherglu.ln

CFILES 	= \
	ethwrap.c \
	ethertsm.c

SRCFILES= $(CFILES)

SFILES 	= \
	etherglu.s

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
	(cd ethtsm.cf; $(IDINSTALL) -R$(CONF) -M ethtsm)

clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d ethtsm
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
		cat `basename $$i .ln`.L >> `expr $(LFILE) :	\
				'\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do			\
		echo $$i;	\
	done

binaries: $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $@ $(FILES)

ethtsmHeaders = \
	ethdef.h

headinstall: $(ethtsmHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(ethtsmHeaders);	\
	do				\
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
