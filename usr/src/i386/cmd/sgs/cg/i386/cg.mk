#ident	"@(#)cg:i386/cg.mk	1.3.2.14"
#
#cg.mk: local makefile for the i386 instance of cg
#
include $(CMDRULES)

OBJECTS=	allo.$o arena.$o cgen.$o comm2.$o cost.$o match.$o \
		reader.$o xdefs.$o local.$o local2.$o inline.$o \
		table.$o nail.$o stasg.$o picode.$o 
o=	o

# Special set of objects for the acomp needed to support C++ on
# UnixWare 1.1 
CC_OBJECTS=	allo.$o arena.$o cgen.$o comm2.$o cost.$o match.$o \
		reader.$o xdefs.$o local.$o local2.$o inline_CC.$o \
		table.$o nail.$o stasg.$o picode.$o 


# Enhanced asm's disabled by default
# Enable with INLINE=-DIN_LINE
INLINE=
# NODBG= -DNODBG	to suppress debug code
# NODBG=		to include debug code
NODBG=
# FP emulator for Amdahl disabled by default.
# Enable with FP_EMULATE=-DFP_EMULATE
FP_EMULATE=
# For floating point emulation package
FPE=            $(SGSBASE)/fpemu
FPE_COMMON=     $(FPE)/common
FPE_MDP=        $(FPE)/$(CPU)
FPEFATAL=       cerror
FPE_O=          fpemu.o
FPE_INC=        -I$(FPE_MDP)

INCPATH= -I. -I$(LOCAL) -I$(COMMON) $(FPE_INC) -I$(SGSBASE)/inc/i386

# For read-only data section
# Enable with RODATA= -DRODATA
RODATA=
DEFLIST= -D$(CPU) -DSTINCC -DCG -DFLEXNAMES $(OPTIM_SUPPORT)\
		$(INLINE) $(FP_EMULATE) $(NODBG) $(RODATA) 
COMMON=../common
LOCAL=.
COMSRC= $(COMMON)/allo.c $(COMMON)/arena.c $(COMMON)/cgen.c \
	$(COMMON)/comm2.c $(COMMON)/cost.c \
	$(COMMON)/inline.c $(COMMON)/match.c $(COMMON)/nail.c   \
	$(COMMON)/reader.c $(COMMON)/xdefs.c 
LOCSRC= $(LOCAL)/local.c $(LOCAL)/local2.c $(LOCAL)/stasg.c $(LOCAL)/picode.c
SRC=$(COMSRC) $(LOCSRC)

INCLUDES= $(COMMON)/arena.h $(COMMON)/mfile1.h $(COMMON)/mfile2.h \
		$(COMMON)/manifest.h ./macdefs.h \
		$(FPE_COMMON)/fpemu.h $(FPE_MDP)/fpemu_md.h
CPRS=cprs
CC_CMD=	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCPATH)

cg.o:	$(OBJECTS)
	$(LD) -r -o cg.o $(OBJECTS)

# Special target to build the code generator portion of an acomp to
# support C++ on UnixWare 1.1

cg_CC.o:	$(CC_OBJECTS)
	$(LD) -r -o cg.o $(CC_OBJECTS)

allo.o:	$(COMMON)/allo.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/allo.c

arena.o:	$(COMMON)/arena.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/arena.c

cgen.o:	$(COMMON)/cgen.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/cgen.c

comm2.o:	$(COMMON)/comm2.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/comm2.c

cost.o:	$(COMMON)/cost.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/cost.c

match.o:	$(COMMON)/match.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/match.c

reader.o:	$(COMMON)/reader.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/reader.c

nail.o:		$(COMMON)/nail.c $(INCLUDES) $(COMMON)/dope.h
	$(CC_CMD) $(COMMON)/nail.c

xdefs.o:	$(COMMON)/xdefs.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/xdefs.c

local.o:	$(LOCAL)/local.c $(INCLUDES)
	$(CC_CMD) $(LOCAL)/local.c

local2.o:	$(LOCAL)/local2.c $(INCLUDES)
	$(CC_CMD) $(LOCAL)/local2.c

stasg.o:	$(LOCAL)/stasg.c $(INCLUDES)
	$(CC_CMD) $(LOCAL)/stasg.c

picode.o:	$(LOCAL)/picode.c $(INCLUDES)
	$(CC_CMD) $(LOCAL)/picode.c

table.o:	table.c $(INCLUDES)
	$(CC_CMD) table.c

table.c:	stin sty
		./sty -S stin >table.c

sty:	$(COMMON)/sty.y $(INCLUDES) $(COMMON)/dope.h
	$(YACC) $(YACCFLAGS) $(COMMON)/sty.y
	if [ -f /usr/include/pfmt.h ]; \
        then \
                $(HCC) $(DEFLIST) $(INCPATH) -o sty y.tab.c -ly; \
        else \
                $(HCC) $(DEFLIST) $(INCPATH) -I$(CPUINC) -o sty y.tab.c ../../libsgs/libsgs.a -ly; \
        fi

inline.o:	$(COMMON)/inline.c $(INCLUDES)
	$(CC_CMD) $(COMMON)/inline.c


# Special target to allow use of a special intrinsics file with the acomp
# built to support C++ on UnixWare 1.1

inline_CC.o:	$(COMMON)/inline.c $(INCLUDES)
	$(CC_CMD) -DUX_1_1_CPLUSPLUS_SUPPORT=1  $(COMMON)/inline.c
	/bin/mv inline.o inline_CC.o

clean:
	/bin/rm -f $(OBJECTS) $(CC_OBJECTS) table.c sty y.tab.c
	/bin/rm -f *.ln

clobber:	clean
	/bin/rm -f core make.out cg.o

lintit: $(SRC) table.c
	$(LINT) $(INCPATH) $(DEFLIST) -DNODBG \
	$(SRC) table.c >lint.out  2>&1

cg.ln:	$(SRC) table.c
	rm -f $(OBJECTS:$o=ln)
	$(LINT) -c $(INCPATH) $(DEFLIST) $(SRC) table.c
	cat $(OBJECTS:$o=ln) >cg.ln
