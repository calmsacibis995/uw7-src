#
# Makefile for hw
#
# @(#) hw.mk 65.5 97/07/23 
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
CC =		$(TOOLS)/usr/ccs/bin/i386cc

include make.inc

MAKEFILE =	hw.mk
YARC =		libhw.a
YLIBS =		-lgen -lsocket -lld

DIST =		$(ROOT)
DIST_BIN =	/etc
DIST_LIB =	/usr/lib/hw

BIN =		hw

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

#CFLAGS =	-g -w3 -wx $(DFLAGS) $(IFLAGS) -b coff $(AFLAGS)
#LDFLAGS =	-g -b coff

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

hw:		$(YARC) \
		$(MAKEFILE)
	@$(MAKE) -f $(MAKEFILE) linkdate.o
	$(CC) $(LDFLAGS) $(YARC) $(YLIBS) linkdate.o -o $@
	chmod 711 $@
	@$(RM) linkdate.o

#
# The objects

$(YARC):	$(YARC)(hw.o) \
		$(YARC)(hw_isa.o) \
		$(YARC)(hw_eisa.o) \
		$(YARC)(hw_util.o) \
		$(YARC)(eisa_data.o) 
	$(RANLIB) $@

$(YARC)(hw.o):		hw.c \
			hw_util.h \
			$(MAKEFILE)
	$(RM) hw.o
	$(CC) $(CFLAGS) -c hw.c
	$(AR) $(YARC) hw.o
	@$(RM) hw.o

#
# Report generators

$(YARC)(hw_buf.o):	hw_buf.c \
			hw_buf.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_buf.o
	$(CC) $(CFLAGS) -c hw_buf.c
	$(AR) $(YARC) hw_buf.o
	@$(RM) hw_buf.o

$(YARC)(hw_bus.o):	hw_bus.c \
			hw_bus.h \
			hw_isa.h \
			hw_eisa.h \
			hw_mca.h \
			hw_pci.h \
			hw_pcmcia.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_bus.o
	$(CC) $(CFLAGS) -c hw_bus.c
	$(AR) $(YARC) hw_bus.o
	@$(RM) hw_bus.o

$(YARC)(hw_cache.o):	hw_cache.c \
			hw_cache.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_cache.o
	$(CC) $(CFLAGS) -c hw_cache.c
	$(AR) $(YARC) hw_cache.o
	@$(RM) hw_cache.o

$(YARC)(hw_con.o):	hw_con.c \
			hw_con.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_con.o
	$(CC) $(CFLAGS) -c hw_con.c
	$(AR) $(YARC) hw_con.o
	@$(RM) hw_con.o

$(YARC)(hw_cpu.o):	hw_cpu.c \
			hw_cpu.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_cpu.o
	$(CC) $(CFLAGS) -c hw_cpu.c
	$(AR) $(YARC) hw_cpu.o
	@$(RM) hw_cpu.o

$(YARC)(hw_eisa.o):	hw_eisa.c \
			hw_eisa.h \
			hw_util.h \
			eisa_data.h \
			$(MAKEFILE)
	@$(RM) hw_eisa.o
	$(CC) $(CFLAGS) -c hw_eisa.c
	$(AR) $(YARC) hw_eisa.o
	@$(RM) hw_eisa.o

$(YARC)(hw_isa.o):	hw_isa.c \
			hw_isa.h \
			hw_util.h \
			hw_eisa.h \
			eisa_data.h \
			$(MAKEFILE)
	@$(RM) hw_isa.o
	$(CC) $(CFLAGS) -c hw_isa.c
	$(AR) $(YARC) hw_isa.o
	@$(RM) hw_isa.o

$(YARC)(hw_mp.o):	hw_mp.c \
			hw_mp.h \
			hw_cpu.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_mp.o
	$(CC) $(CFLAGS) -c hw_mp.c
	$(AR) $(YARC) hw_mp.o
	@$(RM) hw_mp.o

$(YARC)(hw_mca.o):	hw_mca.c \
			hw_mca.h \
			hw_util.h \
			mca_data.h \
			$(MAKEFILE)
	@$(RM) hw_mca.o
	$(CC) $(CFLAGS) -c hw_mca.c
	$(AR) $(YARC) hw_mca.o
	@$(RM) hw_mca.o

$(YARC)(hw_net.o):	hw_net.c \
			hw_net.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_net.o
	$(CC) $(CFLAGS) -c hw_net.c
	$(AR) $(YARC) hw_net.o
	@$(RM) hw_net.o

$(YARC)(hw_pci.o):	hw_pci.c \
			hw_pci.h \
			hw_util.h \
			pci_data.h \
			$(MAKEFILE)
	@$(RM) hw_pci.o
	$(CC) $(CFLAGS) -c hw_pci.c
	$(AR) $(YARC) hw_pci.o
	@$(RM) hw_pci.o

$(YARC)(hw_pcmcia.o):	hw_pcmcia.c \
			hw_pcmcia.h \
			hw_util.h \
			pcmcia_data.h \
			$(MAKEFILE)
	@$(RM) hw_pcmcia.o
	$(CC) $(CFLAGS) -c hw_pcmcia.c
	$(AR) $(YARC) hw_pcmcia.o
	@$(RM) hw_pcmcia.o

$(YARC)(hw_ram.o):	hw_ram.c \
			hw_ram.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_ram.o
	$(CC) $(CFLAGS) -c hw_ram.c
	$(AR) $(YARC) hw_ram.o
	@$(RM) hw_ram.o

$(YARC)(hw_scsi.o):	hw_scsi.c \
			hw_scsi.h \
			hw_util.h \
			scsi_data.h \
			$(MAKEFILE)
	@$(RM) hw_scsi.o
	$(CC) $(CFLAGS) -c hw_scsi.c
	$(AR) $(YARC) hw_scsi.o
	@$(RM) hw_scsi.o

$(YARC)(hw_util.o):	hw_util.c \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) hw_util.o
	$(CC) $(CFLAGS) -c hw_util.c
	$(AR) $(YARC) hw_util.o
	@$(RM) hw_util.o

#
# Data file parsers

$(YARC)(eisa_data.o):	eisa_data.c \
			eisa_data.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) eisa_data.o
	$(CC) $(CFLAGS) -c eisa_data.c
	$(AR) $(YARC) eisa_data.o
	@$(RM) eisa_data.o

$(YARC)(mca_data.o):	mca_data.c \
			mca_data.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) mca_data.o
	$(CC) $(CFLAGS) -c mca_data.c
	$(AR) $(YARC) mca_data.o
	@$(RM) mca_data.o

$(YARC)(pci_data.o):	pci_data.c \
			pci_data.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) pci_data.o
	$(CC) $(CFLAGS) -c pci_data.c
	$(AR) $(YARC) pci_data.o
	@$(RM) pci_data.o

$(YARC)(pcmcia_data.o):	pcmcia_data.c \
			pcmcia_data.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) pcmcia_data.o
	$(CC) $(CFLAGS) -c pcmcia_data.c
	$(AR) $(YARC) pcmcia_data.o
	@$(RM) pcmcia_data.o

$(YARC)(scsi_data.o):	scsi_data.c \
			scsi_data.h \
			hw_util.h \
			$(MAKEFILE)
	@$(RM) scsi_data.o
	$(CC) $(CFLAGS) -c scsi_data.c
	$(AR) $(YARC) scsi_data.o
	@$(RM) scsi_data.o

#
# Special

linkdate.o:
	echo "char LinkDate[] = \042 \042 __DATE__ \042 \042 __TIME__ ;" \
		> linkdate.c
	$(CC) $(CFLAGS) $(DFLAGS) -c linkdate.c
	@$(RM) linkdate.c

