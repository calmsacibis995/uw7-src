#ident	"@(#)msm.mk	26.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= msm.mk
KBASE 	= ../../..
LINTDIR	= $(KBASE)/lintdir
DIR	= io/odi/msm
MOD 	= msm.cf/Driver.o
LFILE	= $(LINTDIR)/msm.ln
BINARIES = $(MOD)
PROBEFILE = msm.c

LOCALDEF= $(DEBUGDEF) -DUNIXWARE

FILES	= \
	msm.o \
	msmintr.o \
	msmnbi.o \
	msmprint.o \
	msmstring.o \
	msmcbus.o \
	msmioctl.o \
	msmos.o \
	msmsched.o \
	msmtime.o \
	msmcnbi.o \
	msmmem.o \
	msmparse.o \
	msmsndrcv.o \
	msmwrap.o \
	msmfile.o \
	msmmsg.o \
	msmparseold.o \
	msmstr.o \
	msmglu.o \
	msmtest.o \
	msmasmtest.o

LFILES	= \
	msm.ln \
	msmintr.ln \
	msmnbi.ln \
	msmprint.ln \
	msmstring.ln \
	msmcbus.ln \
	msmioctl.ln \
	msmos.ln \
	msmsched.ln \
	msmtime.ln \
	msmcnbi.ln \
	msmmem.ln \
	msmparse.ln \
	msmsndrcv.ln \
	msmwrap.ln \
	msmfile.ln \
	msmmsg.ln \
	msmparseold.ln \
	msmstr.ln \
	msmtest.ln \
	msmasmtest.ln

CFILES	= \
	msm.c \
	msmintr.c \
	msmnbi.c \
	msmprint.c \
	msmstring.c \
	msmcbus.c \
	msmioctl.c \
	msmos.c \
	msmsched.c \
	msmtime.c \
	msmcnbi.c \
	msmmem.c \
	msmparse.c \
	msmsndrcv.c \
	msmwrap.c \
	msmfile.c \
	msmmsg.c \
	msmparseold.c \
	msmstr.c \
	msmtest.c

SRCFILES= $(CFILES)

SFILES 	= \
	msmglu.s \
	msmasmtest.s

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
	(cd msm.cf; $(IDINSTALL) -R$(CONF) -M msm)

clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d msm
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

msmHeaders = \
	msm.h \
	msmstruc.h \
	msmnbi.h

headinstall:$(msmHeaders)
	@for f in $(msmHeaders);\
	do			\
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)	\
						-g $(GRP) $$f;	\
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
