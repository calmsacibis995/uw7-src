#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# Makefile
#------------------------------------------------------------------------------
# @(#)Makefile	4.1	97/05/16
#------------------------------------------------------------------------------
# Makefile for audinfo/Generic
#------------------------------------------------------------------------------
# Revision History
# 1997-Jan-21, shawnm, created
#==============================================================================

AUDINFODIR = $(ROOT)/$(MACH)/usr/lib/audio/audioconfig/audinfo/Generic
INS = $(TOOLS)/etc/install

install:
	@if [ ! -d $(AUDINFODIR) ]; then \
		mkdir -p $(AUDINFODIR); \
	fi
	$(INS) -f $(AUDINFODIR) SoundBlasterCompatible
	$(INS) -f $(AUDINFODIR) ES688
	$(INS) -f $(AUDINFODIR) ES1688
	$(INS) -f $(AUDINFODIR) Jazz16

all clean clobber:
	echo Nothing to do here.

