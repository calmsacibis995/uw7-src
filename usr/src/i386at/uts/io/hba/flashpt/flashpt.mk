#ident	"@(#)kern-i386at:io/hba/flashpt/flashpt.mk	1.2.1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	flashpt.mk
KBASE = ../../..
LOCALDEF = -D_KERNEL -DFPDRV -DMM_ONLY
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/flashpt

FLASHPT = flashpt.cf/Driver.o
LFILE = $(LINTDIR)/flashpt.ln

UCBOFILES = \
	ucbmgr/src/automate.o \
	ucbmgr/src/busmstr.o \
	ucbmgr/src/diagnose.o \
	ucbmgr/src/phase.o \
	ucbmgr/src/scam.o \
	ucbmgr/src/sccb.o \
	ucbmgr/src/sccb_dat.o \
	ucbmgr/src/scsi.o \
	ucbmgr/src/utility.o

UCBCFILES = \
	ucbmgr/src/automate.c \
	ucbmgr/src/busmstr.c \
	ucbmgr/src/diagnose.c \
	ucbmgr/src/phase.c \
	ucbmgr/src/scam.c \
	ucbmgr/src/sccb.c \
	ucbmgr/src/sccb_dat.c \
	ucbmgr/src/scsi.c \
	ucbmgr/src/utility.c \
	ucbmgr/src/budi.c  

UCBHFILES = \
	ucbmgr/include/blx30.h \
	ucbmgr/include/eeprom.h \
	ucbmgr/include/globals.h \
	ucbmgr/include/harpoon.h \
	ucbmgr/include/osflags.h \
	ucbmgr/include/scsi2.h \
	ucbmgr/include/target.h \
	ucbmgr/include/sccbmgr.h
UCBINCLIST = -Iucbmgr/include

UCBDEFLIST = -DUNIX -D_KERNEL -DFPDRV -DMM_ONLY

FILES = \
	budidrv.o \
	$(UCBOFILES)

CFILES = \
	budidrv.c

SFILES =

LFILES = flashpt.ln

SRCFILES = $(CFILES) $(UCBCFILES) $(SFILES)

all:
	-@if [ -f budidrv.c ]; then \
		$(MAKE) -f $(MAKEFILE) $(FLASHPT) $(MAKEARGS) "KBASE=$(KBASE)" \
		"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(FLASHPT) ]; then \
			echo "ERROR: $(FLASHPT) is missing" 1>&2 ;\
			false ;\
		fi \
	fi

install:	all
		(cd flashpt.cf ; $(IDINSTALL) -R$(CONF) -M flashpt; \
		rm -f $(CONF)/pack.d/flashpt/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/flashpt	)

$(FLASHPT):	$(FILES)
		$(LD) -r -o $(FLASHPT) $(FILES)

$(UCBOFILES): \
	$(UCBCFILES) \
	$(UCBHFILES) 

FRC:

clean:
	-rm -f $(FILES) $(LFILES) *.L $(FLASHPT)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e flashpt

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(UCBCFILES:.c=.o):
	$(CC) $(CFLAGS) $(INCLIST) $(UCBINCLIST) $(DEFLIST) $(UCBDEFLIST) -c $<
	-mv `expr $@ : 'ucbmgr/src/\(.*\)'` $@

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
	flashpt.h

headinstall:$(sysHeaders) 
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
