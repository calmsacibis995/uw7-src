#
# Makefile for hw
#
# @(#) test.mk 65.5 97/07/23 
# Copyright (C) The Santa Cruz Operation, 1993-1997
# This Module contains Proprietary Information of
# The Santa Cruz Operation, and should be treated as Confidential.
#
# It is suggested that this program be constructed with as few
# system dependence as possible:
#	static - no dlls
#	COFF - maybe it will work on ODT 3.0

AR =		ar -r
RM =		rm -f
CP =		cp
MKDIR =		mkdir
LN =		ln
RANLIB =	@:
CC =            $(TOOLS)/usr/ccs/bin/i386cc

include make.inc

MAKEFILE =	test.mk
YARC =		libhw.a
YLIBS =		-lgen -lsocket -lld

DIST =		$(ROOT)
DIST_BIN =	/etc
DIST_LIB =	/usr/lib/hw

BIN =		test

CFGLIBDIR =	cfglib
CFGLIB =	$(CFGLIBDIR)/eisa.cfg \
		$(CFGLIBDIR)/mca.cfg \
		$(CFGLIBDIR)/pci.cfg \
		$(CFGLIBDIR)/pcmcia.cfg
#not yet	$(CFGLIBDIR)/scsi_asc.cfg
#not yet	$(CFGLIBDIR)/scsi_op.cfg
#not yet	$(CFGLIBDIR)/scsi_vendor.cfg

DFLAGS =	-DHWLIBDIR=\"$(DIST_LIB)\" -DGEMINI
IFLAGS =	-I$(SYSINC) -I$(PNPINC)

CFLAGS =	-O -w3 -wx $(DFLAGS) $(IFLAGS) $(AFLAGS)
LDFLAGS =	-s 

all:	$(BIN)

clean:
	$(RM) *.o linkdate.c core $(YARC)

clobber:	clean
	$(RM) $(BIN)

install \
dist:		all \
		dist_dirs
	$(CP) $(BIN) $(DIST)$(DIST_BIN)
	$(CP) $(CFGLIB) $(DIST)$(DIST_LIB)

dist_dirs:
	test -d $(DIST) || $(MKDIR) -p $(DIST)
	test -d $(DIST)$(DIST_BIN) || $(MKDIR) -p $(DIST)$(DIST_BIN)
	test -d $(DIST)$(DIST_LIB) || $(MKDIR) -p $(DIST)$(DIST_LIB)

#
# The binaries

test:		test.o hw_util.o hw_eisa.o eisa_data.o \
		$(MAKEFILE)
	$(CC) test.o  hw_eisa.o eisa_data.o hw_util.o $(YLIBS) -o $@
	chmod 711 $@

