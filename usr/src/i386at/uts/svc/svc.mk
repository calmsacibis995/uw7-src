#ident	"@(#)kern-i386at:svc/svc.mk	1.73.12.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = svc.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = svc

SVC = svc.cf/Driver.o
SVC_CCNUMA = ../svc.cf/Driver_ccnuma.o
NAME = name.cf/Driver.o
NAME_CCNUMA = ../name.cf/Driver_ccnuma.o
#PIT = pit.cf/Driver.o
FPE = fpe.cf/Driver.o
FPE_CCNUMA = ../fpe.cf/Driver_ccnuma.o
PSTART = pstart.cf/Driver.o
PSTART_CCNUMA = ../pstart.cf/Driver_ccnuma.o
SYSDUMP = sysdump.cf/Driver.o
SYSDUMP_CCNUMA = ../sysdump.cf/Driver_ccnuma.o

LFILE = $(LINTDIR)/svc.ln
# un-comment the following for gdb symbols
#
# CFLAGS = -Xa -Kno_lu -W2,-_s -Kno_host -W2,-A -Ki486 -W0,-d1 -g

MODULES = \
	$(SVC) \
	$(NAME) \
	$(PIT) \
	$(FPE) \
	$(PSTART) \
	$(SYSDUMP) \
	syms.o

CCNUMA_MODULES = \
	$(SVC_CCNUMA) \
	$(NAME_CCNUMA) \
	$(FPE_CCNUMA) \
	$(PSTART_CCNUMA) \
	$(SYSDUMP_CCNUMA) 

FILES = \
	autotune.o \
	bki.o \
	bs.o \
	clock.o \
	clock_p.o \
	cxenix.o \
	umem.o \
	sco.o \
	detect.o \
	hrtimers.o \
	intr.o \
	io_intr.o \
	limitctl.o \
	machdep.o \
	main.o \
	memory.o \
	mhi_p.o \
	mki.o \
	msintr.o \
	msop.o \
	msop_p.o \
	norm_tm.o \
	oemsup.o \
	panic.o \
	secsys.o \
	start.o \
	sysdat.o \
	sysent.o \
	sysi86.o \
	sysi86_p.o \
	sysinit.o \
	systrap.o \
	timers.o \
	trap.o \
	uadmin.o \
	utssys.o \
	v86bios.o \
	xcall.o \
	keyctl.o \
	xsys.o \
	eisa.o \
	nmi.o \
	spl.o

CFILES = \
	autotune.c \
	bs.c \
	clock.c \
	clock_p.c \
	cxenix.c \
	umem.c \
	sco.c \
	fpe.c \
	hrtimers.c \
	limitctl.c \
	machdep.c \
	main.c \
	memory.c \
	mki.c \
	mmu.c \
	msintr.c \
	msop.c \
	name.c \
	norm_tm.c \
	panic.c \
	secsys.c \
	sysent.c \
	sysi86.c \
	sysi86_p.c \
	sysinit.c \
	systrap.c \
	timers.c \
	trap.c \
	uadmin.c \
	utssys.c \
	v86bios.c \
	xcall.c \
	keyctl.c \
	xsys.c \
	eisa.c \
	nmi.c

SFILES = \
	bki.s \
	detect.s \
	intr.s \
	io_intr.s \
	mhi_p.s \
	msop_p.s \
	oemsup.s \
	pstart.s \
	start.s \
	syms.s \
	syms_p.s \
	sysdat.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	autotune.ln \
	bs.ln \
	clock.ln \
	clock_p.ln \
	cxenix.ln \
	umem.ln \
	sco.ln \
	fpe.ln \
	limitctl.ln \
	machdep.ln \
	main.ln \
	memory.ln \
	mki.ln \
	mmu.ln \
	msintr.ln \
	msop.ln \
	panic.ln \
	sysdump.ln \
	secsys.ln \
	sysent.ln \
	sysi86.ln \
	sysi86_p.ln \
	sysinit.ln \
	systrap.ln \
	timers.ln \
	trap.ln \
	uadmin.ln \
	utssys.ln \
	xcall.ln \
	keyctl.ln \
	xsys.ln \
	eisa.ln \
	nmi.ln

all:	local FRC

local:	$(MODULES) ccnuma

ccnuma: 
	if [ "$$DUALBUILD" = 1 ]; then \
		if [ ! -d ccnuma.d ]; then \
			mkdir ccnuma.d; \
			cd ccnuma.d; \
			for file in "../*.[csh] ../*.mk*"; do \
				ln -s $$file . ; \
			done; \
		else \
			cd ccnuma.d; \
		fi; \
		$(MAKE) -f svc.mk $(CCNUMA_MAKEARGS) $(CCNUMA_MODULES); \
	fi

install: localinstall FRC

localinstall:	local FRC
	cd svc.cf; $(IDINSTALL) -R$(CONF) -M svc
	cd name.cf; $(IDINSTALL) -R$(CONF) -M name
	cd pstart.cf; $(IDINSTALL) -R$(CONF) -M pstart
#cd pit.cf; $(IDINSTALL) -R$(CONF) -M pit
	cd fpe.cf; $(IDINSTALL) -R$(CONF) -M fpe
	cd sysdump.cf; $(IDINSTALL) -R$(CONF) -M sysdump

$(SVC): $(FILES) gdb_maybe
	@GDB=0; \
	for def in $(DEFLIST) $(DEVDEF); do \
		case $$def in \
		-DUSE_GDB) GDB=1;; \
		esac; \
	done; \
	if [ $$GDB -eq 1 ]; then \
		echo $(LD) -r -o $(SVC) $(FILES) gdb.d/gdb.o; \
		$(LD) -r -o $(SVC) $(FILES) gdb.d/gdb.o; \
	else \
		echo $(LD) -r -o $(SVC) $(FILES); \
		$(LD) -r -o $(SVC) $(FILES); \
		$(FUR) -W -o svc.order $(SVC); \
	fi

$(SVC_CCNUMA): $(FILES) gdb_maybe
	@GDB=0; \
	for def in $(DEFLIST) $(DEVDEF); do \
		case $$def in \
		-DUSE_GDB) GDB=1;; \
		esac; \
	done; \
	if [ $$GDB -eq 1 ]; then \
		echo $(LD) -r -o $(SVC_CCNUMA) $(FILES) gdb.d/gdb.o; \
		$(LD) -r -o $(SVC_CCNUMA) $(FILES) gdb.d/gdb.o; \
	else \
		echo $(LD) -r -o $(SVC_CCNUMA) $(FILES); \
		$(LD) -r -o $(SVC_CCNUMA) $(FILES); \
	fi

gdb_maybe:
	@GDB=0; \
	for def in $(DEFLIST) $(DEVDEF); do \
		case $$def in \
		-DUSE_GDB) GDB=1;; \
		esac; \
	done; \
	if [ $$GDB -eq 1 ]; then \
		cd gdb.d ;\
		echo "=== $(MAKE) -f gdb.mk all" ;\
		pwd ;\
		$(MAKE) -f gdb.mk all $(MAKEARGS) ;\
	fi

$(NAME):	name.o
	$(LD) -r -o $(NAME) name.o
	@rm -f name.o
	# remove to force a rebuild every time, to pick up RELEASE, VERSION

$(NAME_CCNUMA):	name.o
	$(LD) -r -o $(NAME_CCNUMA) name.o
	@rm -f name.o
	# remove to force a rebuild every time, to pick up RELEASE, VERSION

name.o: name.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c name.c \
		-DRELEASE=`expr '"$(RELEASE)' : '\(..\{0,8\}\)'`\" \
		-DVERSION=`expr '"$(VERSION)' : '\(..\{0,8\}\)'`\"

name.ln: name.c
	echo "\n$(DIR)/name.c:" > name.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-DRELEASE=`expr '"$(RELEASE)' : '\(..\{0,8\}\)'`\" \
		-DVERSION=`expr '"$(VERSION)' : '\(..\{0,8\}\)'`\" \
		-c -u name.c >> name.L

$(PSTART): pstart.o mmu.o vbios.o
	$(LD) -r -o $(PSTART) pstart.o mmu.o vbios.o

$(PSTART_CCNUMA): pstart.o mmu.o vbios.o
	$(LD) -r -o $(PSTART_CCNUMA) pstart.o mmu.o vbios.o

$(FPE): fpe.o
	$(LD) -r -o $(FPE) fpe.o

$(FPE_CCNUMA): fpe.o
	$(LD) -r -o $(FPE_CCNUMA) fpe.o

$(SYSDUMP): sysdump.o
	$(LD) -r -o $(SYSDUMP)	sysdump.o

$(SYSDUMP_CCNUMA): sysdump.o
	$(LD) -r -o $(SYSDUMP_CCNUMA)	sysdump.o

pstart.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

vbios.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

start.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

detect.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

mhi_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

msop_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

oemsup.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

sysdat.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

syms.o:		syms.s syms_p.s $(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h
		$(M4) -DKBASE=$(KBASE) $(DEFLIST) syms.s syms_p.s | \
			$(AS) $(ASFLAGS) -o syms.o -

#	Enhanced Application Compatibility Support

sco.o: sco.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c sco.c \
		-DSCODATE=\"`date "+%y/%m/%d"`\"

#	End Enhanced Application Compatibility Support

clean:	localclean

localclean: FRC
	-rm -f *.o $(LFILES) name.ln *.L gdb.d/*.o $(MODULES) $(CCNUMA_MODULES)
	-rm -rf ccnuma.d

clobber: localclobber

localclobber: localclean FRC
	-$(IDINSTALL) -R$(CONF) -d -e svc
	-$(IDINSTALL) -R$(CONF) -d -e name
	-$(IDINSTALL) -R$(CONF) -d -e pstart
	-$(IDINSTALL) -R$(CONF) -d -e pit
	-$(IDINSTALL) -R$(CONF) -d -e fpe
	-$(IDINSTALL) -R$(CONF) -d -e sysdump

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES) name.ln
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES) name.ln; do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

FRC:

sysHeaders = \
	autotune.h \
	bootinfo.h \
	cbus.h \
	clock.h \
	clock_p.h \
	corollary.h \
	debugreg.h \
	errno.h \
	eisa.h \
	fault.h \
	fp.h \
	kcore.h \
	keyctl.h \
	locking.h \
	mki.h \
	p_sysi86.h \
	pic.h \
	pit.h \
	proctl.h \
	psm.h \
	reg.h \
	sco.h \
	sd.h \
	secsys.h \
	syscall.h \
	sysconfig.h \
	sysi86.h \
	systeminfo.h \
	systm.h \
	time.h \
	timeb.h \
	trap.h \
	uadmin.h \
	utime.h \
	utsname.h \
	utssys.h \
	v86bios.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
