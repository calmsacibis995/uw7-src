#ident "@(#)wwdu.mk	10.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
#	IBM Token Ring Driver Makefile
#

include $(UTSRULES)
KBASE = ../../../..
#LOCALDEF = -DUNIXWARE -DTOKENFDX -DOSYSSW  -DSCOSW -DDEBUG
LOCALDEF = -DUNIXWARE -DTOKENFDX -DOSYSSW  -DSCOSW
MAKEFILE = wwdu.mk
SCP_SRC = wwdulli.c

#DEBUG= -DDEBUG
#CFLAGS= -O -I../.. -D_KERNEL -DUNIXWARE $(DEBUG)

OBJ  =wwdulli.o wwduinit.o wwduintr.o wwdutxrx.o wwdudiag.o 
LOBJS=wwdulli.L wwduinit.L wwduintr.L wwdutxrx.L wwdudiag.L
DRV  =wwdu.cf/Driver.o

.SUFFIXES: .c .i .L

.c.i:
	[ -f $(SCP_SRC) ] && $(CC) $(CFLAGS) -L $<

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): wwdu.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd wwdu.cf; $(IDINSTALL) -R$(CONF) -M wwdu)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd wwdu.cf; $(IDINSTALL) -R$(CONF) -d -e wwdu)
