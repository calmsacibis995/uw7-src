#ident	"@(#)kern-i386at:io/hba/ide/ide.mk	1.3.1.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	ide.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/ide
#LOCALDEF = -DATAPI_DEBUG
#LOCALDEF = -DTIMEOUT_DEBUG
#LOCALDEF = -DATA_DEBUG_TRACE -DATAPI_DEBUG
#LOCALDEF = -U_LOCKTEST -DAHA_DEBUG2
#LOCALDEF = -U_LOCKTEST -DAHA_DEBUG_TRACE

IDE = ide.cf/Driver.o
LFILE = $(LINTDIR)/ide.ln

FILES = \
	ide.o \
	gendev.o \
	ata/atascsi.o \
	ata/ata.o \
	ata/atapi.o \
	mcesdi/mcesdi.o

CFILES = \
	ide.c \
	gendev.c

HFILES = \
	ata/atascsi.c \
	ata/ata.c \
	ata/atapi.c \
	mcesdi/mcesdi.c

SFILES =

LFILES = ide.ln

#SRCFILES = $(CFILES) $(HFILES) $(SFILES)
SRCFILES = $(CFILES) $(SFILES)

all:	$(IDE)

install:	all
		(cd ide.cf ; $(IDINSTALL) -R$(CONF) -M ide; \
		rm -f $(CONF)/pack.d/ide/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/ide	)

$(IDE):	$(FILES)
		$(LD) -r -o $(IDE) $(FILES)

clean:
	-rm -f *.o ata/*.o mcesdi/*.o $(LFILES) *.L $(IDE)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e ide

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(HFILES:.c=.o):
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	-mv `expr $@ : '[am][tc][ae].*/\(.*\)'` $@

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
	mcesdi/mc_esdi.h \
	ata/ata_ha.h \
	ide.h \
	ide_ha.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep

ata/him.o: \
	ata/him.c \
	../../../io/hba/ide/ata/him_equ.h \
	../../../io/hba/ide/ata/him_scb.h \
	../../../io/hba/ide/ata/seq_off.h \
	$(FRC)

ata/him_init.o: \
	ata/him_init.c \
	../../../io/hba/ide/ata/him_equ.h \
	../../../io/hba/ide/ata/him_rel.h \
	../../../io/hba/ide/ata/him_scb.h \
	../../../io/hba/ide/ata/seq_off.h \
	../../../io/hba/ide/ata/sequence.h \
	$(FRC)
