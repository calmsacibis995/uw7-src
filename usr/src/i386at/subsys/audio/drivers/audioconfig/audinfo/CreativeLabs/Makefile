#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# Makefile
#------------------------------------------------------------------------------
# @(#)Makefile	7.1	97/10/22
#------------------------------------------------------------------------------
# Makefile for audinfo/CreativeLabs
#------------------------------------------------------------------------------
# Revision History
# 1997-Jan-21, shawnm, created
#==============================================================================

AUDINFODIR = $(ROOT)/$(MACH)/usr/lib/audio/audioconfig/audinfo/CreativeLabs
INS = $(TOOLS)/etc/install

install:
	@if [ ! -d $(AUDINFODIR) ]; then \
		mkdir -p $(AUDINFODIR); \
	fi
	$(INS) -f $(AUDINFODIR) SoundBlaster
	$(INS) -f $(AUDINFODIR) SoundBlasterPro
	$(INS) -f $(AUDINFODIR) SoundBlaster16
	$(INS) -f $(AUDINFODIR) SoundBlaster16ASP
	$(INS) -f $(AUDINFODIR) SoundBlaster16PnP
	$(INS) -f $(AUDINFODIR) SoundBlaster16Vibra
	$(INS) -f $(AUDINFODIR) SoundBlaster32
	$(INS) -f $(AUDINFODIR) SoundBlaster32PnP
	$(INS) -f $(AUDINFODIR) SoundBlasterAWE32
	$(INS) -f $(AUDINFODIR) SoundBlasterAWE32PnP
	$(INS) -f $(AUDINFODIR) SoundBlasterAWE64
	$(INS) -f $(AUDINFODIR) SoundBlasterAWE64Gold
	$(INS) -f $(AUDINFODIR) ViBRA16CPnP

all clean clobber:
	echo Nothing to do here.

