#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# Makefile
#------------------------------------------------------------------------------
# @(#)Makefile	4.1	97/05/16
#------------------------------------------------------------------------------
# Makefile for audinfo/AdLib
#------------------------------------------------------------------------------
# Revision History
# 1997-Jan-21, shawnm, created
#==============================================================================

AUDINFODIR = $(ROOT)/$(MACH)/usr/lib/audio/audioconfig/audinfo/AdLib
INS = $(TOOLS)/etc/install

install:
	@if [ ! -d $(AUDINFODIR) ]; then \
		mkdir -p $(AUDINFODIR); \
	fi
	$(INS) -f $(AUDINFODIR) FMSynth

all clean clobber:
	echo Nothing to do here.

