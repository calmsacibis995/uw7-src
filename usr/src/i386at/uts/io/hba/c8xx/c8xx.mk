#ident	"@(#)kern-pdi:io/hba/c8xx/c8xx.mk	1.1.2.2"

include $(UTSRULES)

C8XX = c8xx.cf/Driver.o
MAKEFILE = c8xx.mk
PROBEFILE = c8xx.c
BINARIES = $(C8XX)
KBASE = ../../..

.SUFFIXES: .ln
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(LOCALDEF) $(INCLIST) -c -u $*.c >> $*.L

#INCLIST = -I$(INC) -I$(INC)/sys -I.
LOCALDEF = -D_KERNEL -DSTATIC="" -D_32bit -DMPX -DMEMORY_MAPPED -DUWARE
#LOCALDEF = -D_KERNEL -DSTATIC="" -D_32bit -DMPX -DMEMORY_MAPPED -DUWARE -DDEBUG -D_LOCKTEST
LINT = /usr/bin/lint
LINTFLAGS = -k -n -s
LINTDIR = ./lintdir
LFILE = $(LINTDIR)/c8xx.ln
LFILES = c8xx.ln 	\
	 sim.ln 	\
	 siminit.ln 	\
	 simmain.ln 	\
	 xpt.ln 	\
	 xptmain.ln 	\
	 pwr_mod.ln 	\
	 pci.ln 	\
	 core.ln

#CFLAGS = -E 
CFLAGS = -O 

GRP = bin
OWN = bin

INS = install
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

FRC =

HEADERS = c8xx.h \
	  c8xxhdrs.h \
	  camcore.h \
	  ccb.h \
	  fromcam.h \
	  pwr_mgmt.h \
	  pwr_mod.h \
	  rdwrreg.h \
	  scsicore.h \
	  sim.h \
	  simport.h \
	  siop.h \
	  tune.h \
	  typedefs.h \
	  utils.h

OBJFILES= c8xx.o os_intfc.o  core.o coreinit.o pci.o simmain.o xptmain.o xpt.o \
	  util.o pwr_mod.o nvram.o nvs.o timing.o siminit.o simact.o sim.o

# all:	headinstall $(C8XX) ID
all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(C8XX) $(MAKEARGS) "KBASE=$(KBASE)" \
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

install:	all
		(cd c8xx.cf ; $(IDINSTALL) -R$(CONF) -M c8xx; \
		rm -f $(CONF)/pack.d/c8xx/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/c8xx	)

IDUPDATE:	
	(cp c8xx.h $(INC)/sys; \
	cd c8xx.cf; \
	cp Driver.o /etc/conf/pack.d/c8xx; \
	cp Space.c /etc/conf/pack.d/c8xx/space.c; \
	/etc/conf/bin/idbuild -M c8xx; \
	cp /etc/conf/mod.d/c8xx .; \
	grep c8xx /etc/mod_register | sort | uniq >loadmods; \
	cp disk.cfg /etc/conf/pack.d/c8xx )

ID:	
	(cp c8xx.h $(INC)/sys; \
	cd c8xx.cf; \
	/etc/conf/bin/idinstall -M c8xx; \
	/etc/conf/bin/idbuild -M c8xx; \
	cp /etc/conf/mod.d/c8xx .; \
	grep c8xx /etc/mod_register | sort | uniq >loadmods; \
	cp disk.cfg /etc/conf/pack.d/c8xx )
clean:
	rm -f *.o

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e c8xx
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(LINTDIR):
	-mkdir -p $@

c8xx.ln: 	c8xx.c 
sim.ln: 	sim.c
sim.ln: 	sim0.c
sim.ln: 	sim1.c
sim.ln: 	sim2.c
siminit.ln: 	siminit.c
simmain.ln: 	simmain.c
xpt.ln: 	xpt.c
xptmain.ln: 	xptmain.c
pwr_mod.ln: 	pwr_mod.c
pci.ln: 	pci.c
core.ln: 	core.c

lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(C8XX):	$(OBJFILES)
	$(LD) -r -o $@ $(OBJFILES)

headinstall:	
#	@for i in $(HEADERS); \
#	do \
#		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
#	done


c8xx.o: c8xx.c \
	c8xx.h \
	$(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c c8xx.c

os_intfc.o: os_intfc.c
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c os_intfc.c

coreinit.o: coreinit.c
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c coreinit.c

util.o: util.c
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c util.c

timing.o: timing.c
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c timing.c

simact.o: simact.c
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c simact.c

nvram.o: nvram.c
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c nvram.c

sim.o	: sim0.c sim1.c sim2.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c sim.c

siminit.o: siminit.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c siminit.c

simmain.o: simmain.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c simmain.c

xpt.o:	xpt.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c xpt.c

xptmain.o:	xptmain.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c xptmain.c

pwr_mod.o:	pwr_mod.c  $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c pwr_mod.c

pci.o:	pci.c  $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c pci.c

core.o:	core.c  $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c core.c

pcicall.o:	pcicall.s  $(HEADERS)
	$(AS) pcicall.s

pcidir.o:	pcidir.s  $(HEADERS)
	$(AS) pcidir.s

nvs.o: nvs.c 
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c nvs.c
