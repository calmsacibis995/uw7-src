#ident	"@(#)amigo:common/amigo.mk	1.30"

include $(CMDRULES)
#following must be set externally
CG_INCS=
CG_MDP=
AC_MDP=
MDPINC=

MDDIR=.
CG=$(SGSBASE)/cg
CG_COM=$(CG)/common
AC_COM=$(SGSBASE)/acomp/common
AC_INCS=-I$(AC_COM) -I$(AC_MDP) -I$(MDPINC)

ACCHDR=	$(AC_COM)/p1.h $(AC_COM)/aclex.h $(AC_COM)/ansisup.h $(AC_COM)/cgstuff.h \
	$(AC_COM)/debug.h $(AC_COM)/decl.h $(AC_COM)/err.h $(AC_COM)/host.h \
	$(AC_COM)/inline.h \
	$(AC_COM)/lexsup.h $(AC_COM)/node.h $(AC_COM)/optim.h $(AC_COM)/p1allo.h \
	$(AC_COM)/stmt.h $(AC_COM)/sym.h $(AC_COM)/target.h $(AC_COM)/tblmgr.h \
	$(AC_COM)/init.h $(AC_COM)/trees.h $(AC_COM)/types.h \
	$(AC_MDP)/mddefs.h

AMIGOHDR=$(COMDIR)/amigo.h $(COMDIR)/bitvector.h $(COMDIR)/debug.h

INCLUDES= $(AMIGOHDR) $(ACCHDR) $(CG_MDP)/macdefs.h \
	$(CG_COM)/manifest.h $(CG_COM)/mfile2.h

INCDIR=-I$(MDDIR) -I$(COMDIR) $(CG_INCS) $(AC_INCS)

NODBG=

PASSLINT = O=ln AC_MDP=$(AC_MDP) CG_INCS="$(CG_INCS)" COMDIR=$(COMDIR) \
	NODBG=$(NODBG) OPTIM_SUPPORT=$(OPTIM_SUPPORT) CC=$(LINT) \
	FP_EMULATE=$(FP_EMULATE) CG_MDP=$(CG_MDP) MDPINC=$(MDPINC)

O=o

OPTIONS=	$(NODBG) $(OPTIM_SUPPORT) $(FP_EMULATE)
CC_CMD=$(CC) $(CFLAGS) $(OPTIONS) $(INCDIR) -c
PR=		pr -n

OBJECTS= bitvector.o blocks.o code_mo.o const.o copy_prop.o costing.o \
	cse.o data_flow.o dead_store.o expr.o interface.o local_info.o \
	loops.o l_simplify.o l_unroll.o mdopt.o reg_alloc.o rewrite.o \
	scopes.o str_red.o util.o

SRC= $(COMDIR)/bitvector.c \
	$(COMDIR)/blocks.c $(COMDIR)/code_mo.c $(COMDIR)/const.c \
	$(COMDIR)/copy_prop.c $(COMDIR)/data_flow.c $(COMDIR)/cse.c \
	$(COMDIR)/dead_store.c $(COMDIR)/expr.c $(COMDIR)/interface.c \
	$(COMDIR)/local_info.c $(COMDIR)/loops.c \
	$(COMDIR)/l_simplify.c $(COMDIR)/l_unroll.c \
	$(COMDIR)/reg_alloc.c  $(COMDIR)/rewrite.c \
	$(COMDIR)/scopes.c  $(COMDIR)/str_red.c $(COMDIR)/util.c

$(OBJECTS):	$(INCLUDES)


amigo.o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o amigo.o


bitvector.$O:	$(COMDIR)/bitvector.c
		$(CC_CMD) $(COMDIR)/bitvector.c

blocks.$O:	$(COMDIR)/blocks.c $(COMDIR)/l_unroll.h
		$(CC_CMD) $(COMDIR)/blocks.c

code_mo.$O:	$(COMDIR)/code_mo.c  $(MDDIR)/costing.h
		$(CC_CMD) $(COMDIR)/code_mo.c

copy_prop.$O:	$(COMDIR)/copy_prop.c  $(MDDIR)/costing.h
		$(CC_CMD) $(COMDIR)/copy_prop.c

const.$O:	$(COMDIR)/const.c  $(MDDIR)/costing.h
		$(CC_CMD) $(COMDIR)/const.c

costing.$O:	costing.c $(MDDIR)/costing.h
		$(CC_CMD) costing.c

mdopt.$O:	mdopt.c 
		$(CC_CMD) mdopt.c
cse.$O:	$(COMDIR)/cse.c  $(MDDIR)/costing.h
		$(CC_CMD) $(COMDIR)/cse.c

data_flow.$O:	$(COMDIR)/data_flow.c
		$(CC_CMD) $(COMDIR)/data_flow.c

dead_store.$O:	$(COMDIR)/dead_store.c
		$(CC_CMD) $(COMDIR)/dead_store.c

expr.$O:	$(COMDIR)/expr.c $(MDDIR)/costing.h
		$(CC_CMD) $(COMDIR)/expr.c

interface.$O:	$(COMDIR)/interface.c $(MDDIR)/costing.h
		$(CC_CMD) $(COMDIR)/interface.c

local_info.$O:	$(COMDIR)/local_info.c
		$(CC_CMD) $(COMDIR)/local_info.c

loops.$O:	$(COMDIR)/loops.c $(COMDIR)/l_unroll.h
		$(CC_CMD) $(COMDIR)/loops.c

l_simplify.$O:	$(COMDIR)/l_simplify.c
		$(CC_CMD) $(COMDIR)/l_simplify.c

l_unroll.$O:	$(COMDIR)/l_unroll.c $(COMDIR)/l_unroll.h $(COMDIR)/str_red.h
		$(CC_CMD) $(COMDIR)/l_unroll.c

reg_alloc.$O:	$(COMDIR)/reg_alloc.c $(MDDIR)/costing.h $(COMDIR)/scopes.h
		$(CC_CMD) $(COMDIR)/reg_alloc.c

rewrite.$O:	$(COMDIR)/rewrite.c 
		$(CC_CMD) $(COMDIR)/rewrite.c

scopes.$O:	$(COMDIR)/scopes.c $(COMDIR)/scopes.h
		$(CC_CMD) $(COMDIR)/scopes.c

str_red.$O:	$(COMDIR)/str_red.c $(COMDIR)/l_unroll.h \
$(COMDIR)/str_red.h $(MDDIR)/costing.h
		$(CC_CMD) $(COMDIR)/str_red.c

util.$O:	$(COMDIR)/util.c
		$(CC_CMD) $(COMDIR)/util.c

clobber:	clean
	rm -f amigo.o

clean:
	rm -f $(OBJECTS) 
	rm -f $(OBJECTS:.o=.ln) amigo.ln 

lintit:	$(SRC)
	$(LINT) $(OPTIONS) $(INCDIR) $(SRC) > lint.out 2>&1

amigo.ln:	$(SRC)
	$(MAKE) -$(MAKEFLAGS) -f $(COMDIR)/amigo.mk \
		$(PASSLINT) $(OBJECTS:.o=.ln)
	cat $(OBJECTS:.o=.ln) >amigo.ln

print:
	$(PR) $(COMDIR)/amigo.mk $(AMIGOHDR) $(SRC)
