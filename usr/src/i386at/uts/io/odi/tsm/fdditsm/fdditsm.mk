#ident	"@(#)fdditsm.mk	26.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= fdditsm.mk
KBASE	= ../../../..
LINTDIR = $(KBASE)/lintdir
DIR	= io/tsm/fdditsm
MOD	= fdditsm.cf/Driver.o
LFILE	= $(LINTDIR)/fdditsm.ln
BINARIES = $(MOD)
PROBEFILE = fdditsm.c

#DEBUGDEF = -DDEBUG_TRACE -DNVLT_ModMask=NVLTM_odi
#CC	= epicc -W0,-2N -W0,"-M 0x00020100"	# ODI mask
LOCALDEF = -DODI_3_0 $(DEBUGDEF)
CFLAGS	= -O -I$(ODIINC) $(LOCALDEF)

FILES	= \
	fddiwrap.o \
	fdditsm.o \
	fddiglu.o

LFILES 	= \
	fddiwrap.ln \
	fdditsm.ln \
	fddiglu.ln

CFILES 	= \
	fdditsm.c \
	fddiwrap.c 

SRCFILES= $(CFILES)

SFILES 	= \
	fddiglu.s

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
	(cd fdditsm.cf; $(IDINSTALL) -R$(CONF) -M fdditsm)

clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d fdditsm
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

fdditsmHeaders = \
	fddidef.h

headinstall:$(fdditsmHeaders)
	@for f in $(fdditsmHeaders);	\
	do				\
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)	\
		-g $(GRP) $$f;	\
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
