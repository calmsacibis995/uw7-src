#ident "@(#)eeE.mk	29.1"
#
#       Copyright (C) The Santa Cruz Operation, 1997
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential. 
#
# ********************************************************************** 
# Dearest Intel staffers,
#
# Please also make changes to the following 2 lines to reduce 
# confusion.  This makefile is for a Gemini target.
#
# Thanks, David
# ********************************************************************** 
# eeE.mk - Makefile for EtherExpress(TM) PRO/100B LAN Adapter driver for 
#	   UnixWare 1.1 & 2.x
# (C) 1994, 1997 Intel
#

include $(UTSRULES)
MAKEFILE = eeE.mk
KBASE = ../../../..
DRIVER_UNIXWARE11=1
DRIVER_UNIXWARE20=2
DRIVER_VERSION=$(DRIVER_UNIXWARE20)
SCP_SRC = dlpi_ether.c

#uncomment this line when building on unixware ver >= 2.0 multi processor 
MULTI_THREADING= -DMP_VERSION

#uncomment this line when building on unixware ver 2.0 or higher 
UW_VER=-DUW20

TEST_FLAGS= #
DEBUG_FLAGS= #-DTCB_DEBUG #-DINIT_DEBUG -DPHY_DEBUG

LOCALDEF = -DDL_STRLOG $(UW_VER) -DALLOW_SET_EADDR -DFLEX_TX -DEEE \
-DDRIVER_VERSION=$(DRIVER_VERSION) $(MULTI_THREADING) -DESMP

EEE =	  eeE.cf/Driver.o
EEEFILES =     eeE_config.o eeE_dlpi.o eeE_eprom.o dlpi_ether.o \
	  eeE_util.o eeE_phy.o  \
	  eeE_hw.o eeE_sw.o eeE_io.o eeE_debug.o

CFILES =  eeE_config.c eeE_dlpi.c eeE_eprom.c dlpi_ether.c \
	  eeE_util.c eeE_phy.c \
	  eeE_hw.c eeE_sw.c eeE_io.c eeE_debug.c

LOBJS =     eeE_config.L eeE_dlpi.L eeE_eprom.L dlpi_ether.L \
	  eeE_util.L eeE_phy.L \
	  eeE_hw.L eeE_sw.L eeE_io.L eeE_debug.L

HFILES =      82557.h eeE_sw.h equates.h eeE_hw.h  \
	  eeE_dlpi.h eeE_externs.h

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
	        $(MAKE) -f $(MAKEFILE) $(EEE) $(MAKEARGS); \
	fi

lintit: $(LOBJS)

$(EEE):     $(EEEFILES)
	$(LD) -r -o $@ $(EEEFILES)

clean:
	-rm -f *.o

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
	        rm -f $(EEE); \
	fi
	(cd eeE.cf; $(IDINSTALL) -R$(CONF) -d -e eeE)

#
# Install the driver.
#
install: all
	(cd eeE.cf; $(IDINSTALL) -R$(CONF) -M eeE)
