#ident	"@(#)cet.mk	29.1"
#ident "$Header$"
#
#       Copyright (C) The Santa Cruz Operation, 1997
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#
OS_MODEL=UW500
MODELS=-D_NF2
EFS_VER=Dangerous

MAKEFILE	= cet.mk

include $(UTSRULES)

KBASE		= ../../..
PWD=`pwd`
DRV		= cet
PKGDIR		= $(PWD)/pkg
PKGOUT		= $(PKGDIR)
NSS		= nflx
NFLXDIR		= ./nss/$(NSS)
NSSINCDIR	= ./nss/include
SCP_SRC		= $(DRV)_uw5.c
LOCALDEF	= -D$(OS_MODEL) -D_SVR4 $(MODELS) -DEFS_VER=\"$(EFS_VER)\" \
		-U_KERNEL_HEADERS
LOCALINC	= -I ./include -I ./nss/include -I ../include -I ./usr/src/uts
CMDOWN		= root
CMDGRP		= sys

NFLX		= NetFlex.a
CFDIR		= $(DRV).cf
CET		= $(CFDIR)/Driver.o

INSDIR		= $(ETC)/inst/nd/mdi/$(DRV)

PROBEFILE	= $(DRV)_uw5.c
BINARIES	= $(CET)
SCODIR		= ../$(DRV)
GEM		= uw5

INFOFILE	= misc/info
INFOFILE2	= misc/.info
PRESCRIPT	= misc/pre_script
POSTSCRIPT	= misc/post_script

CETFILES = \
	$(DRV)_uw5mdi.o \
	$(DRV)_uw5.o \
	$(DRV)_lib.o \
	$(DRV)_mac.o \
	$(DRV)_$(NSS).o \
	$(NFLX)

#CFILES for depend.rules
CFILES = \
	$(DRV)_uw5.c \
	$(DRV)_lib.c \
	$(DRV)_uw5mdi.c \
	$(DRV)_mac.c \
	$(DRV)_$(NSS).c

LOCLINKCFILES = \
	$(DRV)_uw5.c \
	$(DRV)_lib.c \
	$(DRV)_uw5mdi.c \
	$(DRV)_mac.c \
	$(DRV)_$(NSS).c

SRCFILES = \
	$(DRV)_uw5.c \
	$(DRV)_lib.c \
	$(DRV)_uw5mdi.c \
	$(DRV)_mac.c \
	$(DRV)_$(NSS).c 

STRIPFILES = \
	$(NFLXDIR)/NFLXEVNT.c \
	$(NFLXDIR)/NFLXINIT.c \
	$(NFLXDIR)/NFLXREQ.c \
	$(NFLXDIR)/NFLXRX.c \
	$(NFLXDIR)/NFLXTX.c \
	$(NSSINCDIR)/CPQTYPES.H \
	$(NSSINCDIR)/NSS.H \
	$(NSSINCDIR)/NFLX.H \
	$(NSSINCDIR)/NFLXPROT.H \
	$(NSSINCDIR)/T_OS.H \
	$(NSSINCDIR)/T_OS_STD.H

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

# checking for PROBEFILE is the same as checking for SCP_SRC
all:	
	[ -d $(CFDIR) ] || mkdir $(DRV).cf
	if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) DRIVER $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
		mcs -d $(CET) ;\
		mcs -a "@(#)Compaq cet network driver (ver $(EFS_VER))" $(CET) ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi
	
DRIVER: $(CET)

strip:
	@echo "Preparing non-ansi files"
	@for i in $(STRIPFILES); do \
		chmod +w $$i; \
	   sed "s/\/\/.*//" $$i >$$i.1; \
	   cp $$i.1 $$i; \
	   rm $$i.1; \
		chmod -w $$i; \
	done

mkcfg:
	rm -rf $(DRV).cf
	mkdir $(DRV).cf

install: all
	-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
# cet_start and cet_stop commands built in usr/src/work/cmd/cmd-nd/vendors/cet
# at a different point in time so they may not exist so we cannot $INS them
# packaging will pick them up from the nd_mdi file
# must copy info to ROOT/MACH as .info for cut.netflop.sh to get EXTRA_FILES
	-rm -f $(INFOFILE2) > /dev/null 2>&1
	cp $(INFOFILE) $(INFOFILE2)
	$(INS) -f $(INSDIR) -m 0644 -u $(CMDOWN) -g $(CMDGRP) $(INFOFILE2)
	$(INS) -f $(INSDIR) -m 0755 -u $(CMDOWN) -g $(CMDGRP) $(PRESCRIPT)
	$(INS) -f $(INSDIR) -m 0755 -u $(CMDOWN) -g $(CMDGRP) $(POSTSCRIPT)
	$(INS) -f $(INSDIR) -m 0755 -u $(CMDOWN) -g $(CMDGRP) misc/S02cet
	$(INS) -f $(INSDIR) -m 0444 -u $(CMDOWN) -g $(CMDGRP) misc/unieth.bin
	$(INS) -f $(INSDIR) -m 0444 -u $(CMDOWN) -g $(CMDGRP) misc/uniethf.bin
	$(INS) -f $(INSDIR) -m 0444 -u $(CMDOWN) -g $(CMDGRP) misc/unitok.bin
	$(INS) -f $(INSDIR) -m 0444 -u $(CMDOWN) -g $(CMDGRP) misc/unitokf.bin
	cd $(CFDIR); $(IDINSTALL) -R$(CONF) -M $(DRV)

$(NFLX):
	cd $(NFLXDIR); make -f $(NSS).mk

$(CET): $(CETFILES)
	$(LD) -r -o $(CET) $(CETFILES)

# $(APPS):
# 	cd $(APPSDIR); make -f apps.mk LOCALDEF="$(LOCALDEF)"
# 	cp $(APPS) $(DRV).cf

clean:
	-rm -f *.o $(CET)
	cd $(NFLXDIR); make -f $(NSS).mk clean
#	cd $(APPSDIR); make -f apps.mk clean
	
# clobber used to handle APPS stuff (cet_start/cet_stop). look in SCCS for code
# checking for PROBEFILE is the same as checking for SCP_SRC
clobber: clean
	-$(IDINSTALL) -R$(CONF) -e -d $(DRV)
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi
	-rm -f $(INSDIR)/$(INFOFILE2) 2>/dev/null
	-rm -f $(INSDIR)/$(PRESCRIPT) 2>/dev/null
	-rm -f $(INSDIR)/$(POSTSCRIPT) 2>/dev/null
	-rm -f $(INSDIR)S02cet 2>/dev/null
	-rm -f $(INSDIR)/unieth.bin 2>/dev/null
	-rm -f $(INSDIR)/uniethf.bin 2>/dev/null
	-rm -f $(INSDIR)/unitok.bin 2>/dev/null
	-rm -f $(INSDIR)/unitokf.bin 2>/dev/null

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done

#
# Header Install Section
#

#sysHeaders = \
#	../include/$(DRV).h
#
#HEADERS = ./include/net.h
#
#headinstall: $(sysHeaders)
#	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
#	@for f in $(sysHeaders); \
#	 do \
#	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
#	 done
#	@if [ -f $(PROBEFILE) ]; then \
#		for f in $(HEADERS); \
#	 	do \
#	    	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
#	 	done; \
#	fi

