#ident	"@(#)toktsm.mk	26.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= toktsm.mk
KBASE	= ../../../..
LINTDIR = $(KBASE)/lintdir
DIR	= io/odi/tsm/toktsm
MOD	= toktsm.cf/Driver.o
LFILE	= $(LINTDIR)/toktsm.ln
BINARIES = $(MOD)
PROBEFILE = tokentsm.c

LOCALDEF = $(DEBUGDEF)

FILES	= \
	tokwrap.o \
	tstrings.o \
	tokentsm.o \
	tokenglu.o

LFILES 	= \
	tokwrap.ln \
	tstrings.ln \
	tokentsm.ln \
	tokenglu.ln

CFILES 	= \
	tstrings.c \
	tokwrap.c \
	tstrings.c 

SRCFILES= $(CFILES)

SFILES 	= \
	tokenglu.s

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
	(cd toktsm.cf; $(IDINSTALL) -R$(CONF) -M toktsm)

clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d toktsm
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

toktsmHeaders = \
	tokdef.h

headinstall:$(toktsmHeaders)
	@for f in $(toktsmHeaders);	\
	do				\
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)	\
		-g $(GRP) $$f;	\
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
