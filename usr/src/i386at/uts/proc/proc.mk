#ident	"@(#)kern-i386at:proc/proc.mk	1.31.9.2"
#ident	"$Header$"
include $(UTSRULES)

MAKEFILE=	proc.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = proc

PROC = proc.cf/Driver.o
PROC_CCNUMA = ../proc.cf/Driver_ccnuma.o

MODULES	= $(PROC)
CCNUMA_MODULES = $(PROC_CCNUMA)

LFILE = $(LINTDIR)/proc.ln

FILES = \
	acct.o \
	core.o \
	cred.o \
	cswtch.o \
	disp.o \
	exec.o \
	execseg.o \
	exit.o \
	iobitmap.o \
	pid.o \
	fork.o \
	procsubr.o \
	sig.o \
	sigcalls.o \
	sigmdep.o \
	scalls.o \
	pgrp.o \
	cg.o \
	cg_f.o \
	cgscalls.o \
	session.o \
	trapevt.o \
	rendez.o \
	resource.o \
	resource_f.o \
	uidquota.o \
	execargs.o \
	execmdep.o \
	dispmdep.o \
	priocntl.o \
	procset.o \
	bind.o \
	lwpsubr.o \
	grow.o \
	lwpscalls.o \
	procmdep.o \
	seize.o \
	usync.o

CFILES = \
	acct.c \
	core.c \
	cred.c \
	disp.c \
	exec.c \
	execseg.c \
	exit.c \
	iobitmap.c \
	pid.c \
	fork.c \
	procsubr.c \
	uidquota.c \
	sig.c \
	sigcalls.c \
	sigmdep.c \
	scalls.c \
	pgrp.c \
	cg.c \
	cg_f.c \
	cgscalls.c \
	session.c \
	trapevt.c \
	rendez.c \
	resource.c \
	resource_f.c \
	execargs.c \
	execmdep.c \
	dispmdep.c \
	priocntl.c \
	procset.c \
	bind.c \
	lwpsubr.c \
	grow.c \
	lwpscalls.c \
	procmdep.c \
	seize.c \
	usync.c

SFILES = \
	cswtch.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	acct.ln \
	core.ln \
	cred.ln \
	disp.ln \
	exec.ln \
	execseg.ln \
	exit.ln \
	iobitmap.ln \
	pid.ln \
	fork.ln \
	procsubr.ln \
	sig.ln \
	sigcalls.ln \
	sigmdep.ln \
	scalls.ln \
	pgrp.ln \
	cg.ln \
	cg_f.ln \
	cgscalls.ln \
	session.ln \
	trapevt.ln \
	rendez.ln \
	resource.ln \
	resource_f.ln \
	uidquota.ln \
	execargs.ln \
	execmdep.ln \
	dispmdep.ln \
	priocntl.ln \
	procset.ln \
	bind.ln \
	lwpsubr.ln \
	grow.ln \
	lwpscalls.ln \
	procmdep.ln \
	seize.ln \
	usync.ln

SUBDIRS = class obj ipc

all:	local FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

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
		$(MAKE) -f proc.mk $(CCNUMA_MAKEARGS) $(CCNUMA_MODULES); \
	fi

local:	$(PROC) ccnuma

$(PROC): $(FILES)
	$(LD) -r -o $(PROC) $(FILES)
	$(FUR) -W -o proc.order $(PROC)

$(PROC_CCNUMA): $(FILES)
	$(LD) -r -o $(PROC_CCNUMA) $(FILES)
#	$(FUR) -W -o proc_ccnuma.order $(PROC_CCNUMA)

cswtch.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

install: localinstall FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

localinstall: local FRC
	cd proc.cf; $(IDINSTALL) -R$(CONF) -M proc

clean: localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

localclean:
	-rm -f *.o $(LFILES) *.L $(MODULES) $(CCNUMA_MODULES)
	-rm -rf ccnuma.d

localclobber:	localclean
	-$(IDINSTALL) -R$(CONF) -d -e proc

clobber:	localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SUBDIRS);\
	do\
		( \
		cd  $$i;\
		$(MAKE) -f $$i.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$i/;" ; \
		) \
	done
	@for i in $(SRCFILES); do \
		echo $$i; \
	done


headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

sysHeaders = \
	acct.h \
	auxv.h \
	bind.h \
	cg.h \
	cguser.h \
	class.h \
	core.h \
	cred.h \
	disp.h \
	disp_p.h \
	exec.h \
	exec_f.h \
	iobitmap.h \
	lwp.h \
	lwp_f.h \
	mman.h \
	pid.h \
	priocntl.h \
	proc.h \
	proc_hier.h \
	procset.h \
	regset.h \
	resource.h \
	seg.h \
	session.h \
	siginfo.h \
	signal.h \
	times.h \
	tss.h \
	ucontext.h \
	uidquota.h \
	ulimit.h \
	unistd.h \
	user.h \
	usync.h \
	wait.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep

