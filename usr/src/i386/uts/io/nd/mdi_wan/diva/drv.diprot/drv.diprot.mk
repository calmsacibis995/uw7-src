#ident "@(#)drv.diprot.mk	29.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(UTSRULES)

MAKEFILE = drv.diprot.mk
SCP_SRC = protux.c
KBASE = ../../../../..
#DEBUG = -DDEBUG
LOCALDEF = -DUNIX -DUNIXWARE -D_KERNEL -D_INKERNEL -DNEW_DRIVER -DDIVAPP  $(DEBUG)
OBJ =  protux.o isac_drv.o dsp_drv.o util.o hdlca.o serial.o utilux.o \
		 l2_execn.o cm_util.o\
	   lapd.o lapd_tem.o tr.o x75.o nl_exec.o tp.o x25.o pc_nl.o di_exec.o \
 	   arco_drv.o isa_pnp.o divapp.o protux.o revise.o
LOBJS =  protux.L isac_drv.L dsp_drv.L util.L hdlca.L serial.L \
		 utilux.L l2_execn.L \
	   lapd.L lapd_tem.L tr.L x75.L nl_exec.L tp.L x25.L pc_nl.L di_exec.L \
 	   arco_drv.L isa_pnp.L divapp.L revise.L
DRV = Driver.o
DIVOBJ = divad.o
EXE = ../EtdD.cf/divad

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@



all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(EXE) $(MAKEARGS); \
	fi

$(EXE): $(DIVOBJ)
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -o $@ $(DIVOBJ)

$(DRV): $(OBJ) 
	$(LD) -r -o $@ $(OBJ)

install:

lintit:	$(LOBJS)

clean: 
	rm -f $(DRV) $(OBJ) $(LOBJS) $(DIVOBJ) tags

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(EXE); \
	fi
