#ident	"@(#)acomp:i386/makefile	1.18.2.14"

#	i386 machine-dependent makefile for
#	ANSI C compiler.

include $(CMDRULES)

o=o
INS=	$(SGSBASE)/sgs.install
INSDIR=	$(CCSLIB)

I386INC = $(SGSBASE)/inc/$(CPU)

# lint requires SGSBASE-relative path
ACOMP=	acomp
COMDIR=	$(SGSBASE)/$(ACOMP)/common
MDPDIR=	$(SGSBASE)/$(ACOMP)/$(CPU)

AMIGO=	$(SGSBASE)/amigo
AMIGO_MDP = $(AMIGO)/$(CPU)
AMIGO_MIP = $(AMIGO)/common
AMIGO_O=	$(AMIGO_MDP)/amigo.o

CG=	$(SGSBASE)/cg
TARGET=	$(CPU)
CG_MDP=	$(CG)/$(TARGET)
CG_COMMON= $(CG)/common
AC_MDP= $(SGSBASE)/acomp/$(TARGET)
CG_O=	$(CG_MDP)/cg.o
CG_INCS=	-I$(CG_MDP) -I$(CG)/common -I../../fpemu/$(CPU)
ACC_MDP=	../$(CPU)
COMPVERS=	'"01.01"'
CV=	COMPVERS=-DCOMPVERS=$(COMPVERS)
ACOMPMAKEFILE=	$(COMDIR)/acomp.mk

# Enable handling of enhanced asm's
INLINE=	-DIN_LINE
#  Use floating point emulation
FP_EMULATE=	-DFP_EMULATE

# MERGED_CPP enables a merged preprocessor/compiler
# Several things must be changed to enable/disable a
# merged preprocessor.
# To enable:
# MERGED_CPP=	-DMERGED_CPP=0		turns support on (selects acomp, too)
# ACLEX_O=	lex.$o			selects lexical interface
# CPP_O=	<location of acpp.o>	partially linked .o for CPP
# CPP_INC=	-I$(CPP_COMMON)		enables search of directory
# CPP_INTERFACE= $(CPP_COMMON)/interface.h for dependency
#
# Corresponding values to enable a standalone scanner are:
# MERGED_CPP=				turn support off
# ACLEX_O=	aclex.$o		select standalone scanner
# CPP_O=				no CPP object file
# CPP_INC=				no search of other directory
# CPP_INTERFACE=			no extra dependency
#
CPP=		$(SGSBASE)/acpp
CPP_COMMON=	$(CPP)/common
CPP_MDP=	$(CPP)/$(CPU)
#
MERGED_CPP=	-DMERGED_CPP=0
ACLEX_O=	lex.$o
CPP_O=		$(CPP_MDP)/acpp.o
CPP_INC=	-I$(CPP_COMMON)
CPP_INTERFACE=	$(CPP_COMMON)/interface.h

# OPTIM_SUPPORT enables support for HALO optimizer
# OPTIM_SUPPORT=			turns support off
# OPTIM_SUPPORT= -DOPTIM_SUPPORT	turns support on
OPTIM_SUPPORT=	-DOPTIM_SUPPORT

# RODATA enables support of a separate read-only data
# section for data and strings.
# RODATA=				turns support off
# RODATA= -DRODATA			turns support on
RODATA=		-DRODATA

# VOL_SUPPORT enables support for volatile objects in CG.
# VOL_SUPPORT=				turns support off
# VOL_SUPPORT= -DVOL_SUPPORT		turns support on
VOL_SUPPORT=	-DVOL_SUPPORT

# YYDEBUG enables yacc debugging in the ANSI C grammar
# YYDEBUG=			support off if NODBG off
# YYDEBUG=	-DYYDEBUG	support on
YYDEBUG=

# include files when building lint (passed in):  LINT_H for
# dependencies, LINT_INC for -I options to cc.
# CXREF is used for making acpp.o
LINT_H=
LINT_INC=
LINT_MDP=
CXREF=

NODBG=	-DNODBG
DEFLIST=

# PASS... definitions get passed along to the next level of
# makefile.  Each ANSI C compiler builds the machine-dependent
# cg.o and the semi-machine-independent compiler from the
# common source.  A preprocessor portion may also be built if
# the preprocessor and compiler are a single process.

PASSCOMMON= \
	-$(MAKEFLAGS) $(MAKEARGS) NODBG="$(NODBG)" DFLTINC="$(DFLTINC)"
PASSDEFINE= \
	INLINE="$(INLINE)" FP_EMULATE="$(FP_EMULATE)" \
	VOL_SUPPORT="$(VOL_SUPPORT)" OPTIM_SUPPORT="$(OPTIM_SUPPORT)" \
	RODATA="$(RODATA)"
PASSAMIGO=$(PASSCOMMON) $(PASSDEFINE) CG_INCS="$(CG_INCS)" \
	COMDIR=$(AMIGO_MIP) AC_MDP=$(AC_MDP) CG_MDP=$(CG_MDP) \
	MDPINC="$(I386INC)"
PASSCG=	$(PASSCOMMON) $(PASSDEFINE) COMMON=$(CG_COMMON)
PASSACOMP= \
	$(PASSCOMMON) $(PASSDEFINE) \
	$(CV) TARGET="$(TARGET)" COMDIR="$(COMDIR)" ACC_MDP="$(ACC_MDP)" \
	CG_O="$(CG_O)" CG_INCS="$(CG_INCS)" \
	AMIGO_O=$(AMIGO_O) \
	MDPINC="$(I386INC)" COMINC="$(COMINC)" \
	YYDEBUG="$(YYDEBUG)" \
	MERGED_CPP="$(MERGED_CPP)" ACLEX_O='$(ACLEX_O:$o=$o)' CPP_O="$(CPP_O)" \
	CPP_INC="$(CPP_INC)" CPP_INTERFACE="$(CPP_INTERFACE)"
PASSCPP= $(PASSCOMMON) CXREF="$(CXREF)"
PASSLINT=	LINT_H="$(LINT_H)" DEFLIST=-DLINT \
		LINT_INC="$(LINT_INC)" $(PASSACOMP)

build all:	cg acpp amigo fpemu intemu
	$(MAKE) $(PASSACOMP) -f $(ACOMPMAKEFILE) all

cg $(CG_O):
	cd $(CG_MDP); $(MAKE) -f cg.mk $(PASSCG) cg.o

acpp $(CPP_O):
	if [ "$(CPP_O)" ]; then \
	    cd $(CPP_MDP); $(MAKE) -f acpp.mk $(PASSCPP) acpp.o; \
	fi

acppforlint $(CPP_O):
	if [ "$(CPP_O)" ]; then \
	    cd $(CPP_MDP); $(MAKE) -f acpp.mk $(PASSCPP) acppforlint; \
	fi

amigo $(AMIGO_O):
	cd $(AMIGO_MDP); $(MAKE) -f $(AMIGO_MIP)/amigo.mk $(PASSAMIGO) amigo.o

include ../../fpemu/common/fp.mkdeps
FPE=		$(SGSBASE)/fpemu
FPEFATAL=	cerror

fpemu:	fpemu.$o

include ../../intemu/common/mkdeps

intemu:	intemu.$o

# target to build an acomp.o for lint
forlint:	acppforlint fpemu intemu
	$(MAKE) $(PASSLINT) -f $(ACOMPMAKEFILE) forlint

clean:
	cd $(CG_MDP); $(MAKE) -f cg.mk $(PASSCG) clean
	cd $(AMIGO_MDP); $(MAKE) -f $(AMIGO_MIP)/amigo.mk $(PASSAMIGO) clean
	$(MAKE) -f $(ACOMPMAKEFILE) $(PASSACOMP) clean
	if [ "$(CPP_O)" ]; then \
	    cd $(CPP_MDP); $(MAKE) -f acpp.mk $(PASSCPP) clean; \
	fi

clobber:
	cd $(CG_MDP); $(MAKE) -f cg.mk $(PASSCG) clobber
	cd $(AMIGO_MDP); $(MAKE) -f $(AMIGO_MIP)/amigo.mk $(PASSAMIGO) clobber
	$(MAKE) -f $(ACOMPMAKEFILE) $(PASSACOMP) clobber
	if [ "$(CPP_O)" ]; then \
	    cd $(CPP_MDP); $(MAKE) -f acpp.mk $(PASSCPP) clobber; \
	fi

shrink:		clobber


install:	$(INSDIR)/$(SGS)acomp

$(INSDIR)/$(SGS)acomp:	acomp
	cp acomp acomp.bak
	$(STRIP) acomp
	sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)acomp acomp
	mv acomp.bak acomp
	sh $(INS) 444 $(OWN) $(GRP) $(INSDIR)/intrinsics $(CG_MDP)/intrinsics


# Special targets (all_uw11, cg_CC and install_all_uw11) for building
# an acomp to support C++ 2.0 on UnixWare 1.1

all_uw11:	cg_CC acpp amigo fpemu
	$(MAKE) $(PASSACOMP) -f $(ACOMPMAKEFILE) all_uw11

cg_CC $(CG_O):
	cd $(CG_MDP); rm -f $(CG_O) inline_CC.o
	cd $(CG_MDP); $(MAKE) -f cg.mk $(PASSCG) cg_CC.o

install_all_uw11:	$(INSDIR)/$(SGS)acomp_CC

$(INSDIR)/$(SGS)acomp_CC:	acomp_CC
	cp acomp_CC acomp_CC.bak
	$(STRIP) acomp_CC
	sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)acomp_CC acomp_CC
	mv acomp_CC.bak acomp_CC
	sh $(INS) 444 $(OWN) $(GRP) $(INSDIR)/intrinsics_CC $(CG_MDP)/intrinsics

lintit:
	exec >lint.out 2>&1;					\
	$(MAKE) $(PASSACOMP) -f $(ACOMPMAKEFILE) acomp.ln;	\
	cd $(CG_MDP); $(MAKE) -f cg.mk $(PASSCG) cg.ln;		\
	cd $(AMIGO_MDP); $(MAKE) -f $(AMIGO_MIP)/amigo.mk 	\
	    $(PASSAMIGO) amigo.ln; 				\
	if [ "$(CPP_O)" ]; then					\
	    cd $(CPP_MDP);					\
	    $(MAKE) -f acpp.mk $(PASSCPP) acpp.ln;		\
	fi;							\
	cd $(MDPDIR);						\
	if [ "$(CPP_O)" ]; then					\
	    $(LINT) $(LINTFLAGS) acomp.ln $(CG_MDP)/cg.ln	\
	    $(CPP_MDP)/acpp.ln  $(AMIGO_MDP)/amigo.ln; 		\
	else							\
	    $(LINT) $(LINTFLAGS) acomp.ln $(AMIGO_MDP)/amigo.ln	\
	    $(CG_MDP)/cg.ln;					\
	fi

llintit:
	$(MAKE) $(PASSLINT) -f $(ACOMPMAKEFILE) acomp.ln;	\
	if [ "$(CPP_O)" ]; then					\
	    cd $(CPP_MDP);					\
	    $(MAKE) -f acpp.mk $(PASSCPP) acpp.ln;		\
	fi;							\
	cd $(LINT_MDP);						\
	if [ "$(CPP_O)" ]; then 				\
	    cat acomp.ln cg.ln $(CPP_MDP)/acpp.ln > others.ln; 	\
	else 							\
	    cat acomp.ln cg.ln > others.ln;			\
	fi
