#ident	"@(#)kern-i386at:io/hba/c7xx/c7xx.mk	1.1.2.1"

include $(UTSRULES)

C7XX = c7xx.cf/Driver.o
MAKEFILE = c7xx.mk
PROBEFILE = c7xx.c
BINARIES = $(C7XX)
KBASE = ../../..

.SUFFIXES: .ln
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(LOCALDEF) $(INCLIST) -c -u $*.c >> $*.L

INCLIST = -I. -I$(INC) -I$(INC)/sys

# C7XX_DEBUG 	- enable the c7xx debugging stuff (off).
# IS700 	- build a 53c700 driver (off).
# IS710 	- build a 53c710 driver (on).
# IS720 	- build a 53c720 driver (off).
LOCALDEF = -DHBA_PREFIX=c7xx -DC7XX -D_KERNEL -DSTATIC="" -DIS710 

LINT = /usr/bin/lint
LINTFLAGS = -k -n -s
LINTDIR = ./lintdir
LFILE = $(LINTDIR)/c7xx.ln
LFILES = c7xx.ln 	\
	 sim.ln 	\
	 siminit.ln 	\
	 simmain.ln 	\
	 xpt.ln 	\
	 pwr_mod.ln 	\
	 core.ln

CFLAGS = -O 

GRP = bin
OWN = bin

INS = install
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

FRC =

HEADERS = boards.h \
	  c7xx.h \
	  c7xxhdrs.h \
	  ccb.h \
	  pwr_mgmt.h \
	  pwr_mod.h \
	  rdwrreg.h \
	  scripts.h \
	  script70.h \
	  script71.h \
	  script72.h \
	  scsicore.h \
	  sim.h \
	  simport.h \
	  siop.h \
	  tune.h \
	  typedefs.h \
	  dbug.h

OBJFILES= c7xx.o sim.o siminit.o xpt.o simmain.o pwr_mod.o \
	  core.o dbug.o

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(C7XX) $(MAKEARGS) "KBASE=$(KBASE)" \
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
		(cd c7xx.cf ; $(IDINSTALL) -R$(CONF) -M c7xx; \
		rm -f $(CONF)/pack.d/c7xx/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/c7xx	)

clean:
	rm -f *.o

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e c7xx
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(LINTDIR):
	-mkdir -p $@

c7xx.ln: 	c7xx.c 
sim.ln: 	sim.c
sim.ln: 	sim0.c
sim.ln: 	sim1.c
sim.ln: 	sim2.c
siminit.ln: 	siminit.c
simmain.ln: 	simmain.c
xpt.ln: 	xpt.c
pwr_mod.ln: 	pwr_mod.c
core.ln: 	core.c

lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(C7XX):	$(OBJFILES)
	$(LD) -r -o $@ $(OBJFILES)

headinstall:	

c7xx.o: c7xx.c \
	c7xx.h \
	$(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c c7xx.c

sim.o	: sim0.c sim1.c sim2.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c sim.c

siminit.o: siminit.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c siminit.c

simmain.o: simmain.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c simmain.c

xpt.o:	xpt.c $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c xpt.c

pwr_mod.o:	pwr_mod.c  $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c pwr_mod.c

core.o:	core.c  $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c core.c

dbug.o:	dbug.c  $(HEADERS)
	$(CC) $(CFLAGS) $(LOCALDEF)  $(INCLIST) -c dbug.c
