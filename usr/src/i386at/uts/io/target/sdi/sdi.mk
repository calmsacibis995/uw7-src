#ident	"@(#)kern-i386at:io/target/sdi/sdi.mk	1.13.8.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	sdi.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/sdi

LOCALDEF = -D_DDI_C

SDI = sdi.cf/Driver.o
LFILE = $(LINTDIR)/sdi.ln

FILES = sdi.o conf.o dynstructs.o sdi_bind.o sdi_autoconf.o sdi_layer.o
CFILES = sdi.c conf.c dynstructs.c sdi_bind.c sdi_autoconf.c sdi_layer.c
LFILES = sdi.ln conf.ln dynstructs.ln sdi_bind.ln sdi_autoconf.ln sdi_layer.ln

SRCFILES = $(CFILES)

all:	$(SDI)

install:	all
		(cd sdi.cf; \
		$(IDINSTALL) -R$(CONF) -M sdi )

$(SDI):	$(FILES)
		$(LD) -r -o $(SDI) $(FILES)
		$(FUR) -W -o sdi.order $(SDI)

clean:
	-rm -f *.o $(LFILES) *.L $(SDI)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e sdi

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
	dynstructs.h \
	sdi.h \
	sdi_comm.h \
	sdi_edt.h \
	sdi_hier.h \
	sdi_defs.h \
	sdi_layer.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
