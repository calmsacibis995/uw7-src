#ident	"@(#)kern-i386at:io/hba/ida/ida.mk	1.15.4.2"

IDA = ida.cf/Driver.o
MAKEFILE = ida.mk
PROBEFILE = idascsi.c 
BINARIES = $(IDA)

include $(UTSRULES)

.SUFFIXES: .ln
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-c -u $*.c >> $*.L

KBASE = ../../..

IDEBUG = # -DIDA_DEBUG0 -DIDA_DEBUG_ASSERT -DIDADBG -DIDA_DEBUG1 -DIDA_DEBUG=1 -DIDA_DEBUG2 # -DDEBUG

# ?? Is -DUNIWARE a typo of -DUNIXWARE ?
LOCALDEF = \
	-DUNIXWARE \
	$(IDEBUG) \
	-I../include \
	-DPDI_VERSION=4 \
	-DIDA_COUNT
# end of localdef options

DISABLED = \
	-DTRY_IT \
	-D_LTYPES \
	-D_SYSTEMENV=4 \
	-DPORTUW2 \
	-DSYSV \
	-DUNIWARE \
	-DSVR40 \
	-DHBA_PREFIX=ida
# End of disabled (or unused) localdef options

FS	= $(CONF)/pack.d/ida/Driver.o

GRP = bin
OWN = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

HEADERS =  \
	  ida.h idashare.h

DRVNAME=ida

OBJFILES = \
	pci_init.o \
	ida.o \
	hba.o \
	scsifake.o \
	idascsi.o \
	pcicall.o \
	int15call.o

CFILES =  \
	pci_init.c \
	ida.c \
	hba.c \
	scsifake.c \
	idascsi.c 

SFILES =  pcicall.s int15call.s

LINTFLAGS = -k -n -s
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/ida.ln
LFILES = \
	pci_init.ln \
	ida.ln \
	hba.ln \
	scsifake.ln \
	idascsi.ln

.s.o:
	$(AS) -m $<

DRVRFILES = ida.cf/Driver.o

all:
	@for fl in $(BINARIES); do \
		if [ ! -r $$fl ]; then \
			echo "ERROR: $$fl is missing" 1>&2 ;\
			echo "INFO: if you have the source files, use \"make -f $(MAKEFILE) force_all \"" 1>&2 ;\
			false ;\
			break ;\
		fi \
	done 

force_all:	
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) DRIVER $(MAKEARGS) "KBASE=$(KBASE)" \
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
	


DRIVER: $(DRVRFILES)

install: all
	(cd $(DRVNAME).cf; \
	$(IDINSTALL) -R$(CONF) -M $(DRVNAME); \
	$(INS) -f $(CONF)/pack.d/$(DRVNAME) $(INSPERM) disk.cfg )

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done


clean:
	@rm -f $(LFILES) *.L

force_clean:
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(OBJFILES) $(LFILES) *.L; \
	fi

clobber: clean
	$(IDINSTALL) -R$(CONF) -e -d $(DRVNAME)

force_clobber: force_clean
	$(IDINSTALL) -R$(CONF) -e -d $(DRVNAME)
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(DRVRFILES):	$(OBJFILES)
	$(LD) -r -o $@ $(OBJFILES)
	$(MCS) -c $@
	ls -l $@

$(OBJFILES): 	$(MAKEFILE)
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)  -c $<

headinstall:	
	@for i in $(HEADERS); \
	do \
		if [ -f $$i ]; then \
			$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
		fi ;\
	done

FRC: 

include $(UTSDEPEND)

include $(MAKEFILE).dep
