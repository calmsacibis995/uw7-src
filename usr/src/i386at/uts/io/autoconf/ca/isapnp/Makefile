#	@(#)Makefile	6.1	9/9/97	15:50:53
#
#    Portions Copyright 1988-1997 The Santa Cruz Operation, Inc
#		      All Rights Reserved

include make.inc
MAKEFILE =	Makefile

DBGFLGS =	-DPNP_DEBUG
CPPFLAGS =	-I$(TOOLS)/usr/include $(MORECPP) -DM_S_UNIX -D_IBCS2
OSCFLAGS =	-DUNIXWARE -DGEMINI
CFLAGS =	-w3 -wx -O$(KERNELOPT) $(CPPFLAGS) -DOLD_INT -DINTEL=32 $(OSCFLAGS)
INS=$(TOOLS)/etc/install

DOBJ =		$(XDIR)pnp.o \
		$(XDIR)svcs.o \
		$(XDIR)ioctl.o \
		$(XDIR)pnp_resmgr.o \
		$(XDIR)pnp_bios.o

all:	gemdsp/Driver.o

floppy:		$(XDIR)Driver.o \
		$(MAKEFILE)
	tar cv	Master \
		System \
		Node \
		Driver.o \
		Space.c \
		Stubs.c \
		space.h \
		sys/pnp.h

clean:
	rm -f $(XDIR)*.o

clobber:	clean
	rm -f gemdsp/Driver.o

$(XDIR)pnp.o:		pnp.c \
			sys/pnp.h \
			space.h \
			pnp_private.h \
			pnp_bios.h \
			$(MAKEFILE)
	$(CC) $(CFLAGS) $(DBGFLGS) -c pnp.c

$(XDIR)svcs.o:		svcs.c \
			sys/pnp.h \
			space.h \
			pnp_private.h \
			$(MAKEFILE)
	$(CC) $(CFLAGS) $(DBGFLGS) -c svcs.c

$(XDIR)ioctl.o:		ioctl.c \
			sys/pnp.h \
			space.h \
			pnp_private.h \
			$(MAKEFILE)
	$(CC) $(CFLAGS) $(DBGFLGS) -c ioctl.c

$(XDIR)pnp_resmgr.o:	pnp_resmgr.c \
			sys/pnp.h \
			space.h \
			pnp_private.h \
			$(MAKEFILE)
	$(CC) $(CFLAGS) $(DBGFLGS) -c pnp_resmgr.c

$(XDIR)pnp_bios.o:	pnp_bios.s \
			$(MAKEFILE)
	@rm -f $@
	$(AS) -o $@ pnp_bios.s


$(XDIR)Driver.o: \
			$(DOBJ) \
			$(MAKEFILE)
	@rm -f $@
	$(LD) -r -o $@ $(DOBJ)

gemdsp/Driver.o:	$(XDIR)Driver.o \
				$(MAKEFILE)
	@rm -f $@
	cp $(XDIR)Driver.o $@

install: gemdsp/Driver.o
	@if [ ! -d $(ROOT)/$(MACH)/usr/lib/pnp ] ; then \
		mkdir -p $(ROOT)/$(MACH)/usr/lib/pnp ; \
	fi
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/pnp gemdsp/Master
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/pnp gemdsp/System
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/pnp gemdsp/Node
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/pnp gemdsp/Driver.o
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/pnp gemdsp/Space.c
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/pnp gemdsp/Stubs.c 
	$(INS) -f $(ROOT)/$(MACH)/usr/include/sys sys/pnp.h
