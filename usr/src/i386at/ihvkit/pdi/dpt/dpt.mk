#ident	"@(#)ihvkit:pdi/dpt/dpt.mk	1.4.2.1"
# include $(UTSRULES)

.SUFFIXES: .ln
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(LOCALDEF) $(INCLIST) -c -u $*.c >> $*.L

MAKEFILE = dpt.mk


INC = /usr/include
INCLIST = -I$(INC) -I$(INC)/sys
INS = install
OWN = bin
GRP = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

LOCALDEF = -D_DDI=8 -DDPT_TARGET_MODE -DDPT_TIMEOUT_RESET_SUPPORT -D_KERNEL -DSTATIC=static

ETC = /etc
CONF = $(ETC)/conf
CONFBIN = $(CONF)/bin
IDINSTALL = $(CONFBIN)/idinstall
IDBUILD = $(CONFBIN)/idbuild
DPT = dpt.cf/Driver.o

DRIVER = dpt
IHVKIT_BASE = /usr/src/hdk/sdi/hba
DRIVER_BASE = $(IHVKIT_BASE)/$(DRIVER)
DRIVER_CFG_BASE = $(DRIVER_BASE)/$(DRIVER).cf
HBA_FLP_BASE = $(DRIVER_BASE)/$(DRIVER).hbafloppy
HBA_FLP_BASE_TMP = $(HBA_FLP_BASE)/$(DRIVER)/tmp
HBA_FLP_OBJECT_LOC = $(HBA_FLP_BASE)/$(DRIVER)/tmp/$(DRIVER)

OBJECTS = Space.c System Master Drvmap disk.cfg loadmods dpt dpt.h

LINT = /usr/bin/lint
LINTFLAGS = -k -n -s
LINTDIR = ./lintdir
LFILE = $(LINTDIR)/dpt.ln
LFILES = dpt.ln

FRC =

HEADERS = dpt.h 

OBJFILES= dpt.o


all: $(DPT)

install: headinstall all
	( cd dpt.cf; \
	$(IDINSTALL) -d $(DRIVER) > /dev/null 2>&1; \
	$(IDINSTALL) -a -k $(DRIVER); \
	$(IDBUILD) -M $(DRIVER); \
	cp $(CONF)/mod.d/$(DRIVER) .; \
	grep $(DRIVER) $(ETC)/mod_register | sort | uniq >loadmods; \
	cp disk.cfg $(CONF)/pack.d/$(DRIVER) )

hbafloppy: 
	( cd $(DRIVER_BASE); \
	rm -rf $(HBA_FLP_OBJECT_LOC); \
	mkdir -p $(HBA_FLP_OBJECT_LOC); \
	cd $(DRIVER_CFG_BASE); \
	for i in ${OBJECTS}; do \
		cp $$i $(HBA_FLP_OBJECT_LOC); \
	done ; \
	cd $(HBA_FLP_BASE); \
	./bldscript; \
	)

clean:
	rm -f *.o
	rm -rf $(HBA_FLP_BASE_TMP)

clobber: clean
	$(IDINSTALL)  -e -d $(DRIVER)

$(LINTDIR):
	-mkdir -p $@

dpt.ln: dpt.c

lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(DPT): $(OBJFILES)
	$(LD) -r -o $@ $(OBJFILES)

headinstall:	
	cp dpt.h dpt.cf
	@for i in $(HEADERS); \
	do \
		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
	done

dpt.o: dpt.c \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/cred.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/debug.h \
	$(INC)/sys/scsi.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/sdi.h \
	$(INC)/sys/dynstructs.h \
	dpt.h \
	$(INC)/sys/moddefs.h \
	$(INC)/sys/dma.h  \
	$(INC)/sys/ddi.h  \
	$(INC)/sys/ddi_i386at.h
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c dpt.c
