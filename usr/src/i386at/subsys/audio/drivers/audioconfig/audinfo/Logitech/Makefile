#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# Makefile
#------------------------------------------------------------------------------
# @(#)Makefile	4.1	97/05/16
#------------------------------------------------------------------------------
# Makefile for audinfo/Logitech
#------------------------------------------------------------------------------
# Revision History
# 1997-Jan-21, shawnm, created
#==============================================================================

AUDINFODIR = $(ROOT)/$(MACH)/usr/lib/audio/audioconfig/audinfo/Logitech
INS = $(TOOLS)/etc/install

install:
	@if [ ! -d $(AUDINFODIR) ]; then \
		mkdir -p $(AUDINFODIR); \
	fi
	$(INS) -f $(AUDINFODIR) SoundManGames
	$(INS) -f $(AUDINFODIR) SoundManWave

all clean clobber:
	echo Nothing to do here.

