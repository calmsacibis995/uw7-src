#ident	"@(#)cnet.mk	29.2"
#ident	"$Header$"
#
#       Copyright (C) The Santa Cruz Operation, 1997
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#
OS_MODEL=UW500
MODELS=-D_NF3
EFS_VER=Dangerous
include $(UTSRULES)

MAKEFILE	= cnet.mk
PWD=`pwd`
KBASE		= ../../..
DRV		= cnet
PKGDIR		= $(PWD)/pkg
PKGOUT		= $(PKGDIR)
DIR		= io/dlpi_cpq/$(DRV)
TLANDIR		= ./nss/tlan
NSSINCDIR	= ./nss/include
SCRIPTSDIR	= ./scripts
MISCDIR		= ./misc
#-D_DDI=8 -DDDI8_ENABLE \
LOCALDEF	= -D$(OS_MODEL) -D_SVR4 $(MODELS) -DEFS_VER=\"$(EFS_VER)\" \
				-U_KERNEL_HEADERS
LOCALINC	= -I ./include -I ./nss/include -I ../include -I ./usr/src/uts
CMDOWN		= root
CMDGRP		= sys

TLAN		= Tlan.a
CFDIR		= $(DRV).cf
CNET		= $(CFDIR)/Driver.o

INSDIR		= $(ETC)/inst/nd/mdi/$(DRV)

SCP_SRC		= $(DRV)_uw5.c
BINARIES	= $(CNET)

SCODIR		= ../$(DRV)
GEM		= uw5
NSS		= tlan

CNETFILES = \
	$(DRV)_uw5mdi.o \
	rom_call.o \
	$(DRV)_uw5.o \
	$(DRV)_lib.o \
	$(DRV)_mac.o \
	$(DRV)_tlan.o \
	$(TLAN)

#CFILES for depend.rules
CFILES = \
	$(DRV)_uw5.c \
	$(DRV)_lib.c \
	$(DRV)_uw5mdi.c \
	$(DRV)_mac.c \
	$(DRV)_tlan.c

LOCLINKCFILES = \
	$(DRV)_uw5.c \
	$(DRV)_lib.c \
	$(DRV)_uw5mdi.c \
	$(DRV)_mac.c \
	$(DRV)_tlan.c

LFILES = \
	$(DRV)_uw5.ln \
	$(DRV)_lib.ln \
	$(DRV)_uw5mdi.ln \
	$(DRV)_mac.ln \
	$(DRV)_tlan.ln

SRCFILES = \
	$(DRV)_uw5.c \
	rom_call.s \
	$(DRV)_lib.c \
	$(DRV)_uw5mdi.c \
	$(DRV)_mac.c \
	$(DRV)_tlan.c 

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:	
	[ -d $(CFDIR) ] || mkdir $(CFDIR)

	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) DRIVER $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
		mcs -d $(CNET) ;\
		mcs -a "@(#)Compaq cnet network driver (ver $(EFS_VER))" $(CNET) ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi
	

DRIVER: $(CNET)

install: all
	-[ -d $(INSDIR) ] || mkdir $(INSDIR)
	cd $(CFDIR); $(IDINSTALL) -R$(CONF) -M $(DRV)

$(TLAN):
	cd $(TLANDIR); make -f tlan.mk

$(CNET): $(CNETFILES)
	$(LD) -r -o $(CNET) $(CNETFILES)

clean:
	-rm -f *.o $(CNET)
	cd $(TLANDIR); make -f tlan.mk clean
	
clobber: clean
	-$(IDINSTALL) -R$(CONF) -e -d $(DRV)
	@if [ -f $(SCP_SRC) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi
