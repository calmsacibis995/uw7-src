#ident	"@(#)lsl.mk	26.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= lsl.mk
KBASE   = ../../..
LINTDIR = $(KBASE)/lintdir
DIR     = io/odi/lsl
MOD     = lsl.cf/Driver.o
LFILE   = $(LINTDIR)/lsl.ln
BINARIES = $(MOD)

LOCALDEF = -DDL_STRLOG

PROBEFILE = lslodi.c

FILES	= lsl31mem.o \
	  lslcmdln.o \
	  lslmemory.o \
	  lslodi.o \
	  lslstr.o \
	  lslwrap.o \
	  lslcm.o \
	  lslintr.o \
	  lslprint.o \
	  lslsubr.o \
	  lslrealmode.o \
	  lslxmorg.o

LFILES	= lsl31mem.ln \
	  lslcmdln.ln \
	  lslmemory.ln \
	  lslodi.ln \
	  lslstr.ln \
	  lslwrap.ln \
	  lslcm.ln \
	  lslintr.ln \
	  lslprint.ln \
	  lslsubr.ln \
	  lslrealmode.ln \
	  lslxmorg.ln

CFILES	= lsl31mem.c \
	  lslcmdln.c \
	  lslmemory.c \
	  lslodi.c \
	  lslstr.c \
	  lslwrap.c \
	  lslcm.c \
	  lslintr.c \
	  lslprint.c \
	  lslsubr.c \
	  lslrealmode.c \
	  lslxmorg.c

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
	(cd lsl.cf; $(IDINSTALL) -R$(CONF) -M lsl)


clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d lsl
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

lslHeaders = \
	lslxmog.h \
        lsl.h

headinstall:$(lslHeaders)
	@for f in $(lslHeaders);     \
	do                              \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)    \
			-g $(GRP) $$f;  \
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
