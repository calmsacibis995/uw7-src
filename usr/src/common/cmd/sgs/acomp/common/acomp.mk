#ident	"@(#)acomp:common/acomp.mk	55.2.11.2"

#	Generic makefile for ANSI C Compiler

include $(CMDRULES)

o=o

MDPINC=		$(SGSBASE)/inc/$(CPU)

# YYDEBUG=		turns off yacc debug code
# YYDEBUG=-DYYDEBUG	turns on yacc debug code
YYDEBUG=	-DYYDEBUG
# NODBG=		enables internal debugging code
# NODBG=-DNODBG		suppresses internal debugging code
NODBG=

#NO_LDOUBLE_SUPPORT= enable long double support
# NO_LDOUBLE_SUPPORT= -DNO_LDOUBLE_SUPPORT    disable long double support
NO_LDOUBLE_SUPPORT=

# FP_EMULATE affects manifest.h, gets passed in.
FP_EMULATE=	-DFP_EMULATE
# OPTIM_SUPPORT enables support for HALO optimizer
# OPTIM_SUPPORT=			turns support off
# OPTIM_SUPPORT= -DOPTIM_SUPPORT	turns support on
OPTIM_SUPPORT=	-DOPTIM_SUPPORT

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
#
MERGED_CPP=
ACLEX_O=	aclex.$o
CPP_O=
CPP_INC=
CPP_INTERFACE=

# include files when building lint (passed in):  LINT_H for
# dependencies, LINT_INC for -I options to cc.
LINT_H=
LINT_INC=

# Must derive y.tab.h from acgram.y
YACC_CMD=	$(YACC) $(YFLAGS) -d
INLINE=		-DIN_LINE
OPTIONS=	$(INLINE) $(OPTIM_SUPPORT) $(FP_EMULATE) $(MERGED_CPP) \
		$(NO_LDOUBLE_SUPPORT) $(NODBG)
LIBS=	-ll
TARGET=
CG=		$(SGSBASE)/cg
CG_MDP=		$(CG)/$(TARGET)
CG_O=		$(CG_MDP)/cg.o
FPE_COMMON=	../../fpemu/common
FPE_MDP=	../../fpemu/$(CPU)
FPE_O=		../$(CPU)/fpemu.o
INT_O=		../$(CPU)/intemu.o

AMIGO=		$(SGSBASE)/amigo
AMIGO_O=	$(AMIGO)/$(TARGET)/amigo.o
AMIGO_INC=	$(AMIGO)/common/bitvector.h $(CG_COMMON)/arena.h

# for debugging on Amdahl
#CG_O=	$(CG_MDP)/allo.o $(CG_MDP)/cgen.o $(CG_MDP)/comm2.o $(CG_MDP)/cost.o \
#	$(CG_MDP)/match.o $(CG_MDP)/reader.o $(CG_MDP)/xdefs.o $(CG_MDP)/local.o \
#	$(CG_MDP)/local2.o $(CG_MDP)/inline.o \
#	$(CG_MDP)/nail.o $(CG_MDP)/stasg.o $(CG_MDP)/table.o
CG_COMMON=	$(CG)/common
CG_INCS=	-I$(CG_MDP) -I$(SGSBASE)/cg/m32com -I$(CG_COMMON)
MFILE2=		$(CG_COMMON)/mfile2.h
# Define shorthand for directory of source.
A=	$(COMDIR)
# ACC machine dependent files
ACC_MDP=	.


ACC_INC=        -I$(ACC_MDP) -I$A $(CG_INCS) -I$(MDPINC) -I$(AMIGO)/common
INCLIST=      $(ACC_INC) $(LINT_INC)

CC_CMD=	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) '$(COMPVERS)' $(OPTIONS)

# Splitting up the object files is a hack to work around bugs in
# the Amdahl loader, dealing with ld -r files.
LNFILES=	$(OF1) $(OF2) $(OF3) $(ACLEX_O) $(MDLN)
OF1=	acgram.$o cgstuff.$o decl.$o
OF2=	elfdebug.$o err.$o file.$o init.$o lexsup.$o main.$o optim.$o
OF3=	p1allo.$o sharp.$o stmt.$o sym.$o trees.$o types.$o
NO_AMIGO_ACOMP_O=	$(OF2) $(CG_O) $(OF1) $(OF3) $(ACLEX_O) \
			$(MDOBJ) $(FPE_O) $(INT_O)
ACOMP_O=	$(NO_AMIGO_ACOMP_O) $(AMIGO_O) expand.$o file.$o

ACCHDR=	$A/p1.h $A/aclex.h $A/ansisup.h $A/cgstuff.h \
	$A/debug.h $A/decl.h $A/file.h $A/err.h $A/host.h $A/init.h \
	$A/inline.h $A/lexsup.h $A/node.h $A/optim.h $A/p1allo.h \
	$A/stmt.h $A/sym.h $A/target.h $A/tblmgr.h \
	$A/trees.h $A/types.h $(FPE_COMMON)/fpemu.h $(FPE_MDP)/fpemu_md.h \
	$(ACC_MDP)/mddefs.h
HFILES=	$(ACCHDR) $(COMINC)/syms.h $(COMINC)/storclass.h $(COMINC)/dwarf.h \
	$(MDPINC)/sgs.h
P1_H=	$(ACCHDR) $(CG_MDP)/macdefs.h $(CG_COMMON)/manifest.h $(LINT_H)

SOURCES= $A/acgram.y $A/aclex.l $A/cgstuff.c \
	$A/decl.c $A/elfdebug.c $A/err.c $A/expand.c $A/init.c $A/lex.c $A/lexsup.c \
	$A/main.c $A/optim.c $A/p1allo.c $A/sharp.c $A/stmt.c $A/sym.c \
	$A/trees.c $A/types.c $(MDSRC)

PRODUCTS= acomp




all build:		acomp

acomp.o:		$(ACOMP_O)
			$(LD) -r -o acomp.o $(ACOMP_O)

forlint:		acompcpp.$o

acompcpp.o:		acgram.c acompcpp
			$(LD) -r -o acompcpp.o $(NO_AMIGO_ACOMP_O) $(CPP_O)

.MUTEX:			acgram.c acompcpp

acompcpp:		$(NO_AMIGO_ACOMP_O) $(CPP_O)

acomp:			acgram.c everything_else
			$(CC) -o acomp $(CFLAGS) $(LINK_MODE) \
				$(ACOMP_O) $(CPP_O) $(LIBSGS) $(LIBS)

.MUTEX:			acgram.c everything_else

everything_else:	$(ACOMP_O) $(CPP_O)
			>everything_else

# Special targets (all_uw11 and acomp_CC) for building an acomp to
# support C++ 2.0 on UnixWare 1.1

all_uw11:		acomp_CC

acomp_CC:		acgram.c everything_else
			$(CC) -o acomp_CC $(CFLAGS) $(LINK_MODE) \
				$(ACOMP_O) $(CPP_O) $(LIBSGS) $(LIBS)



PASSLINT=		CC="$(LINT)" CFLAGS="$(CFLAGS) $(LINTFLAGS)" o=ln \
			DEFLIST="$(DEFLIST)" INCLIST="$(INCLIST)" \
			NODBG="$(NODBG)" YYDEBUG="$(YYDEBUG)" \
			OPTIM_SUPPORT="$(OPTIM_SUPPORT)" \
			COMDIR="$(COMDIR)" COMINC="$(COMINC)" LIBS="$(LIBS)" \
			CG="$(CG)" TARGET="$(TARGET)" CG_MDP="$(CG_MDP)" \
			CPP_INC="$(CPP_INC)" CPP_INTERFACE="$(CPP_INTERFACE)" \
			ACLEX_O='$(ACLEX_O:$o=$o)' MERGED_CPP="$(MERGED_CPP)" \
			ACC_MDP="$(ACC_MDP)" \
			COMPVERS="$(COMPVERS)" \
			NO_LDOUBLE_SUPPORT="$(NO_LDOUBLE_SUPPORT)"

lintit:
			$(MAKE) -$(MAKEFLAGS) -f $(COMDIR)/acomp.mk \
				$(PASSLINT) lintp2

acomp.ln:		$(SOURCES) $(ACCHDR)
			$(MAKE) -$(MAKEFLAGS) -f $(COMDIR)/acomp.mk \
				$(PASSLINT) acomp_ln

acomp_ln:		$(OF1:$o=ln) $(OF2:$o=ln) $(OF3:$o=ln) \
			$(ACLEX_O:$o=ln) $(MDLN)
			cat $(OF1:$o=ln) $(OF2:$o=ln) $(MDLN) \
				$(OF3:$o=ln) $(ACLEX_O:$o=ln) >acomp.ln

acgram.c acgram.h:	$A/acgram.y
			$(YACC_CMD) $A/acgram.y
			mv y.tab.c acgram.c
			mv y.tab.h acgram.h

# Keeping this off the acgram.c dependency line prevents
# unnecessary rebuilds.  However, to accomplish that we
# need a hack in case acgram.h is missing altogether (but
# acgram.c is present).  In that case, run yacc to get it.

acgram.$o:		acgram.c $(P1_H)
			$(CC_CMD) $(YYDEBUG) acgram.c

aclex.c:		$A/aclex.l
			$(LEX) $(LFLAGS) $A/aclex.l
			mv lex.yy.c aclex.c

aclex.$o:		aclex.c acgram.h $(P1_H)
			$(CC_CMD) aclex.c

cgstuff.$o:		$A/cgstuff.c $(P1_H) $(MFILE2)
			$(CC_CMD) $(CPP_INC) $A/cgstuff.c

decl.$o:		$A/decl.c $(P1_H)
			$(CC_CMD) $(CPP_INC) $A/decl.c

elfdebug.$o:		$A/elfdebug.c $(COMINC)/dwarf.h $(COMINC)/dwarf2.h \
			$(P1_H)
			$(CC_CMD) -I$(COMINC) $A/elfdebug.c

err.$o:			$A/err.c $(P1_H) $(CPP_INTERFACE)
			$(CC_CMD) $(CPP_INC) $A/err.c

file.$o:		$A/file.c $(P1_H) $(CPP_INTERFACE)
			$(CC_CMD) $(CPP_INC) $A/file.c

init.$o:		$A/init.c $(P1_H)
			$(CC_CMD) $A/init.c

expand.$o:		$A/expand.c $(P1_H) $(MFILE2) $(AMIGO_INC)
			$(CC_CMD) $A/expand.c

lex.$o:			$A/lex.c $(P1_H) acgram.h $(CPP_INTERFACE)
			$(CC_CMD) $(CPP_INC) $A/lex.c

lexsup.$o:		$A/lexsup.c acgram.h $(P1_H)
			$(CC_CMD) $(CPP_INC) $A/lexsup.c

main.$o:		$A/main.c $(P1_H) $(MDPINC)/sgs.h $(CPP_INTERFACE)
			$(CC_CMD) $(YYDEBUG) $(CPP_INC) $A/main.c

optim.$o:		$A/optim.c $(P1_H)
			$(CC_CMD) $A/optim.c

p1allo.$o:		$A/p1allo.c $(P1_H)
			$(CC_CMD) $A/p1allo.c

sharp.$o:		$A/sharp.c $(P1_H) acgram.h $(CPP_INTERFACE)
			$(CC_CMD) $(CPP_INC) $A/sharp.c

stmt.$o:		$A/stmt.c $(P1_H)
			$(CC_CMD) $A/stmt.c

sym.$o:			$A/sym.c $(P1_H)
			$(CC_CMD) $(CPP_INC) $A/sym.c

trees.$o:		$A/trees.c $(P1_H)
			$(CC_CMD) $A/trees.c

types.$o:		$A/types.c $(P1_H)
			$(CC_CMD) $A/types.c

cg_prepass.$o:		cg_prepass.c $(P1_H) $(MFILE2)
			$(CC_CMD) cg_prepass.c

# install done by machine-dependent makefile

clean:
		-rm -f $(OF1:$o=o) $(OF2:$o=o) $(OF3:$o=o) $(ACLEX_O:$o=o)
		-rm -f $(OF1:$o=ln) $(OF2:$o=ln) $(OF3:$o=ln) $(ACLEX_O:$o=ln)
		-rm -f $(FPE_O) $(INT_O) aclex.o lex.o
		-rm -f expand.o file.o $(MDOBJ) $(MDLN)
		-rm -f lint.out
		-rm -f acomp.ln


clobber:	clean
		-rm -f acomp acgram.[ch] aclex.c acomp.o acomp_CC

# lint pass2 stuff
lintp2:		$(LNFILES)
# CC is assumed to be "lint" here, because of recursive make
		$(CC) $(LNFILES) $(LIBS)
