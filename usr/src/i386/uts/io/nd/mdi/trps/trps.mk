#ident "@(#)trps.mk	10.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1997
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
#	IBM Token Ring Driver Makefile
#

include $(UTSRULES)
KBASE = ../../../..
#LOCALDEF = -DUNIXWARE
MAKEFILE = trps.mk
SCP_SRC = scolli.c

# DEBUG= -DDEBUG
# CFLAGS= -O -I../.. -D_KERNEL -DUNIXWARE $(DEBUG)

#
# trps
#

#DEBUG_2=-DDEBUG
#DEBUG_2=-DSCO_DEBUG
DEBUG_2=

FLAGS_2 = $(DEBUG_2) -DNOSYSPERF -DNOTRACE -DMCA_MASK -D_USHORT -D_ULONG -D_UINT

LOCALDEF = -DUNIXWARE $(FLAGS_2)


O_SYS = scio.o sfindadp.o shutdown.o squeues.o sscoinit.o sscointr.o sscomem.o  sscotmrs.o strace.o sysmsg.o scolli.o
O_NET = nconfig.o ngenreq.o ninit.o nmessage.o nopen.o nrecv.o nsend.o nstatus.o
O_ADP = acmd.o aconfig.o ainit.o ainithw.o aintr.o arecv.o areset.o asend.o asend2.o astatus.o atrcmode.o
O_UTL = ucanon.o uebc2asc.o uhex2str.o ulocparm.o unumeric.o uprocprm.o uscpylim.o ustrchr.o ustrcmpi.o ustrlen.o uucase.o uyesno.o

L_SYS = scio.L sfindadp.L shutdown.L squeues.L sscoinit.L sscointr.L sscomem.L  sscotmrs.L strace.L sysmsg.L scolli.L
L_NET = nconfig.L ngenreq.L ninit.L nmessage.L nopen.L nrecv.L nsend.L nstatus.L
L_ADP = acmd.L aconfig.L ainit.L ainithw.L aintr.L arecv.L areset.L asend.L asend2.L astatus.L atrcmode.L
L_UTL = ucanon.L uebc2asc.L uhex2str.L ulocparm.L unumeric.L uprocprm.L uscpylim.L ustrchr.L ustrcmpi.L ustrlen.L uucase.L uyesno.L

OBJ = $(O_SYS) $(O_NET) $(O_ADP) $(O_UTL) $(O_PCI)
LOBJS = $(L_SYS) $(L_NET) $(L_ADP) $(L_UTL) $(L_PCI)

#
# 

#OBJ = tokli.o tokmac.o
#LOBJS = tokli.L tokmac.L
DRV = trps.cf/Driver.o

.SUFFIXES: .c .i .L

.c.i:
	$(CC) $(CFLAGS) -L $<

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): sdi.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd trps.cf; $(IDINSTALL) -R$(CONF) -M trps)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd trps.cf; $(IDINSTALL) -R$(CONF) -d -e trps)
