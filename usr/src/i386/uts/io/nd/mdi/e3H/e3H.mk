#ident "@(#)e3H.mk	10.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE -DSCO
MAKEFILE = e3H.mk
SCP_SRC = upmgem.c

UNIXWARE = -DUNIXWARE -D_KERNEL -D_KMEMUSER
# DEBUG = -DDEBUG
# CFLAGS = -O -I../.. $(UNIXWARE) $(DEBUG)

OBJ = upmgem.o lpminit.o pssutil.o upmutil.o lpmrecv.o lpmsend.o lpmtick.o \
	lpmutil.o lpmisr.o upmsend.o upminit.o upmmsg.o upmioc.o upmprm.o

LOBJS = upmgem.L lpminit.L pssutil.L upmutil.L lpmrecv.L lpmsend.L lpmtick.L \
	lpmutil.L lpmisr.L upmsend.L upminit.L upmmsg.L upmioc.L upmprm.L
DRV = e3H.cf/Driver.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd e3H.cf; $(IDINSTALL) -R$(CONF) -M e3H)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd e3H.cf; $(IDINSTALL) -R$(CONF) -d -e e3H)
