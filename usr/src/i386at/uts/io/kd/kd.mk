#ident	"@(#)kd.mk	1.17"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	kd.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/kd

KD = kd.cf/Driver.o
LFILE = $(LINTDIR)/kd.ln

FILES = \
	kdv.o \
	kdkb.o \
	kdstr.o \
	kdvt.o \
	kdvmstr.o \
	kdconsio.o \
	vtables.o \
	vdc.o \
	evga.o \
	evc.o \
	i8042.o \
	kbmode.o \
	kd_cgi.o

CFILES = \
	kdv.c \
	kdkb.c \
	kdstr.c \
	kdconsio.c \
	kdvmstr.c \
	kdvt.c \
	vtables.c \
	vdc.c \
	evga.c \
	evc.c \
	i8042.c \
	kbmode.c \
	kd_cgi.c


LFILES = \
	kdv.ln \
	kdkb.ln \
	kdstr.ln \
	kdconsio.ln \
	kdvmstr.ln \
	kdvt.ln \
	vtables.ln \
	vdc.ln \
	evga.ln \
	evc.ln \
	i8042.ln \
	kbmode.ln \
	kd_cgi.ln


all: $(KD)

install: all
	(cd kd.cf; $(IDINSTALL) -R$(CONF) -M kd)

$(KD): $(FILES)
	$(LD) -r -o $(KD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(KD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e kd 

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
	kb.h \
	kd.h \
	kd_cgi.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
