#ident "@(#)stbg.mk	24.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
#	IBM Ethernet Driver Makefile
#

include $(UTSRULES)
KBASE = ../../../..
LOCALDEF = -DUNIXWARE
MAKEFILE = stbg.mk
SCP_SRC = stbgli.c

# DEBUG= -DDEBUG
# CFLAGS= -O -DAT386 -DENGLISH -D_KMEMUSER -I../.. -D_KERNEL $(DEBUG)

OBJ = stbgli.o stbgmac.o
LOBJS = stbgli.L stbgmac.L
DRV = stbg.cf/Driver.o

.SUFFIXES: .c .i .L

.c.i:
	$(CC) $(CFLAGS) -L $<

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

$(OBJ): stbg.h

$(DRV): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

install: all
	(cd stbg.cf; $(IDINSTALL) -R$(CONF) -M stbg)

lintit:	$(LOBJS)

clean:
	rm -f $(OBJ) tags *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	(cd stbg.cf; $(IDINSTALL) -R$(CONF) -d -e stbg)
