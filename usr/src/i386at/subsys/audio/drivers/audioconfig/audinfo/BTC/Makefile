#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# Makefile
#------------------------------------------------------------------------------
# @(#)Makefile	4.1	97/05/16
#------------------------------------------------------------------------------
# Makefile for audinfo/BTC
#------------------------------------------------------------------------------
# Revision History
# 1997-Jan-21, shawnm, created
#==============================================================================

AUDINFODIR = $(ROOT)/$(MACH)/usr/lib/audio/audioconfig/audinfo/BTC
INS = $(TOOLS)/etc/install

install:
	@if [ ! -d $(AUDINFODIR) ]; then \
		mkdir -p $(AUDINFODIR); \
	fi
	$(INS) -f $(AUDINFODIR) BTC1831

all clean clobber:
	echo Nothing to do here.

