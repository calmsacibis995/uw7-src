#ident	"@(#)alint:i386/makefile	1.17"
#	i386 machine-dependent makefile for
#	lint.

include $(CMDRULES)
#
# LPASS should be set to where lint1 and lint2 are (.../usr/lib) 
#

# Required Macros
INS=		$(SGSBASE)/sgs.install

# Macros for cg, acomp
FP_EMULATE=	-DFP_EMULATE
FPE=            $(SGSBASE)/fpemu
FPE_COMMON=     $(FPE)/common
FPE_MDP=        $(FPE)/$(CPU)
FPEFATAL=       cerror
FPE_O=          fpemu.o
FPEINC=         -I$(FPE_MDP)

IN_LINE=	-DIN_LINE
OPTIM_SUPPORT=
NODBG=		-DNODBG
o=o

# Required macros passed to sub-makefiles
ENVPARMS=ROOT="$(ROOT)" SGS="$(SGS)" OWN="$(OWN)" GRP="$(GRP)" \
	CC="$(CC)" CFLAGS="$(CFLAGS)" LDLIBS="$(LDLIBS)" \
	LINT="$(LINT)" LINTFLAGS="$(LINTFLAGS)" INS="$(INS)" \
	INSDIR="$(INSDIR)" STRIP="$(STRIP)" LIBSGS="$(LIBSGS)"


# macros to pass to common lint makefile, and acomp makefiles
CG_OBJ= 	$(LINT_MDP)/comm2.o $(LINT_MDP)/arena.o
TARGET=		$(CPU)
CG=		$(SGSBASE)/cg
CG_COMMON= 	$(CG)/common
CG_MDP=		$(CG)/$(TARGET)
CG_O=		$(SGSBASE)/alint/$(TARGET)/cg.o
ACCCOMMON=	$(SGSBASE)/acomp/common
ACC_MDP=	$(SGSBASE)/alint/$(TARGET)
LINTCOMMON=	$(SGSBASE)/alint/common
LINT_INC=	-I$(LINTCOMMON) 
LINT_MDP=	$(SGSBASE)/alint/$(TARGET)
CXREF=		-DCXREF

CG_INC= $(CG_COMMON)/mfile1.h $(CG_COMMON)/mfile2.h \
        $(CG_COMMON)/manifest.h \
        $(CG_MDP)/macdefs.h $(CG)/$(CPU)/macdefs.h 
CG_DEF = -DNOSIMPSTR -DSTINCC -DCG -DFLEXNAMES \
	 $(INLINE) $(FP_EMULATE) $(NODBG) $(VOL_SUPPORT) $(RODATA) \
	 $(OPTIM_SUPPORT)
CG_INCP= -I$(CG_MDP) -I$(CG)/$(CPU) -I$(CG_COMMON) $(FPEINC)
CC_CMD= $(CC) -c $(CFLAGS) $(CG_DEF) $(CG_INCP)

CG_INCS=-I$(CG_MDP) -I$(CG)/$(CPU) -I$(CG)/common -I$(LINTCOMMON) $(FPEINC)

DEFLIST= -DLINT 
DEF_OPT= FP_EMULATE=$(FP_EMULATE) IN_LINE=$(IN_LINE) \
	OPTIM_SUPPORT=$(OPTIM_SUPPORT) NODBG=$(NODBG)

OBJECTS=	$(CG_OBJ)
PRODUCTS=	lint1 lint2
LLIBS=llib-lc.ln llib-lm.ln llib-ll.ln llib-ly.ln llib-lmalloc.ln \
	llib-lcrypt.ln llib-lld.ln llib-ldl.ln llib-lelf.ln

# PASS... definitions get passed along to the next level of
# makefile.  Each lint builds the machine-dependent
# cg.o and the semi-machine-independent compiler from the
# common source.  A preprocessor portion may also be built if
# the preprocessor and compiler are a single process.

PASSCOMMON= \
	-$(MAKEFLAGS) $(ENVPARMS)

PASSLINT= \
	$(PASSCOMMON) \
	ACCCOMMON=$(ACCCOMMON) \
	ACC_MDP=$(ACC_MDP) \
	CG_MDP=$(CG_MDP) \
	CG_COMMON=$(CG_COMMON) \
	DEFLIST="$(DEFLIST)" \
	$(DEF_OPT) \
	ACC_INC="-I$(ACC_MDP) -I$(ACCCOMMON) $(CG_INCS) -I$(CPUINC)" \
	ACOMP_O=acompcpp.o

PASSACOMP= \
	$(PASSCOMMON) \
	ACC_MDP=$(ACC_MDP) \
	DFLTINC=$(DFLTINC) \
	CG_O=$(CG_O) \
	$(ENVPARMS) \
	$(DEF_OPT) \
	LINT_INC="$(LINT_INC)" \
	CXREF="$(CXREF)" \
	LINT_H="$(SGSBASE)/alint/common/lint.h $(SGSBASE)/alint/common/msgbuf.h" \
	FPEINC="$(FPEINC)"

build all:	cg.$o 
	$(MAKE) $(PASSACOMP) -f $(SGSBASE)/acomp/$(TARGET)/makefile forlint 
	$(MAKE) $(PASSLINT) -f $(LINTCOMMON)/alint.mk all

cg.o:	comm2.$o arena.$o
	$(LD) -r -o cg.o $(CG_OBJ)

cg.ln:	comm2.$o arena.$o
	cat comm2.ln arena.ln > cg.ln

comm2.$o:	$(CG_COMMON)/comm2.c $(CG_INC)
		$(CC_CMD) $(FPE_INC) $(CG_COMMON)/comm2.c

arena.$o:	$(CG_COMMON)/arena.c $(CG_INC)
		$(CC_CMD) $(FPE_INC) $(CG_COMMON)/arena.c

lintit:
	$(MAKE) o=ln CC=$(LINT) $(PASSLINT) -f ./makefile cg.ln
	$(MAKE) $(PASSACOMP) LINT_MDP=$(LINT_MDP) -f $(SGSBASE)/acomp/$(TARGET)/makefile llintit
	$(MAKE) o=ln CC=$(LINT) $(PASSLINT) -f $(LINTCOMMON)/alint.mk lintit

#
# install - make sure lint1 and lint2 are installed before building
# the libraries
#

# We must ensure that the ins is done before starting the libs target

.MUTEX:		ins libs

install:	ins libs
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-lc.ln llib-lc.ln
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-lm.ln llib-lm.ln
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-ll.ln llib-ll.ln
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-ly.ln llib-ly.ln
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-lmalloc.ln llib-lmalloc.ln
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(USRLIB)/llib-lcrypt.ln llib-lcrypt.ln
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-lld.ln llib-lld.ln
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
                        $(CCSLIB)/llib-ldl.ln llib-ldl.ln
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
                        $(CCSLIB)/llib-lelf.ln llib-lelf.ln
		if [ -f $(TINC)/libgen.h ]; \
		then \
			/bin/sh $(INS) 755 $(OWN) $(GRP) \
				$(CCSLIB)/llib-lgen.ln llib-lgen.ln; \
		fi
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-lc ../lib/llib-lc
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-lm ../lib/llib-lm
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-ll ../lib/llib-ll
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-ly ../lib/llib-ly
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-lmalloc ../lib/llib-lmalloc
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(USRLIB)/llib-lcrypt ../lib/llib-lcrypt
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
			$(CCSLIB)/llib-lld ../lib/llib-lld
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
                        $(CCSLIB)/llib-ldl ../lib/llib-ldl
		/bin/sh $(INS) 755 $(OWN) $(GRP) \
                        $(CCSLIB)/llib-lelf ../lib/llib-lelf
		if [ -f $(TINC)/libgen.h ]; \
		then \
			/bin/sh $(INS) 755 $(OWN) $(GRP) \
				$(CCSLIB)/llib-lgen ../lib/llib-lgen; \
		fi

#
# install lint1, lint2 and lint (the shellscript)
#
ins:
		cp lint1 lint1.bak
		$(STRIP) lint1
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/lint1 lint1
		mv lint1.bak lint1
		cp lint2 lint2.bak
		$(STRIP) lint2
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/lint2 lint2
		mv lint2.bak lint2
		rm -rf lint
		cp $(LINTCOMMON)/lint lint
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSBIN)/$(SGS)lint lint
		rm -f lint

#
# make the lint libraries - if installing, then lint1 and lint2 should
# already be in CCSLIB; and LPASS should be passed to this makefile
# as CCSLIB.
#
libs:
		rm -f ./lint
		cp $(LINTCOMMON)/lint lint
		chmod ugo+x lint
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -oc -n ../lib/llib-lc
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -om -n ../lib/llib-lm
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -ol -n ../lib/llib-ll
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -oy -n ../lib/llib-ly
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -omalloc -n ../lib/llib-lmalloc
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -ocrypt -n ../lib/llib-lcrypt
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -old -n ../lib/llib-lld
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -odl -n ../lib/llib-ldl
		_CCSLIB=$(LPASS) ./lint -Y$(TINC) -oelf -n ../lib/llib-lelf
		if [ -f $(TINC)/libgen.h ]; \
		then \
			_CCSLIB=$(LPASS) ./lint -Y$(TINC) -ogen -n ../lib/llib-lgen; \
		fi

clean:
	rm -f $(OBJECTS)
	$(MAKE) $(PASSACOMP) -f $(SGSBASE)/acomp/$(TARGET)/makefile clean
	$(MAKE) $(PASSLINT) -f $(LINTCOMMON)/alint.mk clean

clobber:
	rm -f $(OBJECTS)
	rm -f $(PRODUCTS)
	rm -f $(LLIBS)
	rm -f llib-lgen.ln
	rm -f lint
	rm -f cg.o
	rm -f acompcpp.o
	$(MAKE) $(PASSACOMP) -f $(SGSBASE)/acomp/$(TARGET)/makefile clobber
	$(MAKE) $(PASSLINT) -f $(LINTCOMMON)/alint.mk clean
