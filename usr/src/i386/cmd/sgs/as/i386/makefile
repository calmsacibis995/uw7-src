#ident	"@(#)nas:i386/makefile	1.10"
#
# i386 assembler makefile
#
include $(CMDRULES)

AWK	= nawk
DEBUG	=#-DDEBUG
FORMAT	= -DFORMAT=ELF
NUMSIZE	= -DNUMSIZE=64
FLTEXPR	= -DFLOATEXPRS
FP_OBJS = $(FPEOBJS)	# FP emulation is selected
FP_INC	= $(FPEINC)
DEFLIST	= $(DEBUG) $(FORMAT) $(NUMSIZE) $(FLTEXPR)
OBJFLIB = $(LIBELF)
INS=$(SGSASE)/sgs.install

MACHOBJS= dirs386.$o inst386.$o main386.$o relo386.$o stmt386.$o

include ../common/mkdeps
gram.$o: chkgen.h stmt386.h $C/stmt.h

$(MACHOBJS): $C/as.h
dirs386.$o: amode.h chkgen.h dirs386.h stmt386.h \
		$C/dirs.h $C/eval.h $C/expr.h $C/sect.h $C/stmt.h $C/syms.h
inst386.$o: chkgen.h inst386.h stmt386.h $C/stmt.h
main386.$o: chkgen.h stmt386.h $(CPUINC)/sgs.h $C/stmt.h
relo386.$o: chkgen.h relo386.h stmt386.h \
		$C/expr.h $C/relo.h $C/sect.h $C/stmt.h $C/syms.h
stmt386.$o: amode.h chkgen.h dirs386.h relo386.h stmt386.h \
		$C/expr.h $C/sect.h $C/stmt.h $C/syms.h

chkgen.h:  chkgen.awk chkgen.in chkgen.0.h
	$(AWK) -f chkgen.awk chkgen.in chkgen.h chkgen.c

install::
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/cm4defs cm4defs

clobber::
	rm -f chkgen.c chkgen.h
