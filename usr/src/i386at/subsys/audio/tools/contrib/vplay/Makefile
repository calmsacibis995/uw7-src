#-----------------------------------------------------------------------------
#
#       Copyright (C) The Santa Cruz Operation, 1996-1997.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#---------------------------------------------------------------------------
# @(#)Makefile	8.2	2/17/98	12:11:20
#
# Revision History:
# 1997-Sep-02, stevegi, made AUDRVDIR point to vox30 instead of oss
# 1997-Jan-06, shawnm, modified from original
#===========================================================================

AUBINDIR = $(ROOT)/$(MACH)/usr/bin
COPYRIGHTDIR = $(ROOT)/$(MACH)/usr/lib/audio/copyright
CC = $(TOOLS)/usr/ccs/bin/i386cc
AUDRVDIR = $(ROOT)/usr/src/$(WORK)/subsys/audio/drivers/vox30
EXTRA_INCLUDES = -I$(AUDRVDIR)
CFLAGS = -O $(EXTRA_INCLUDES)
INS = $(TOOLS)/etc/install
RM = rm -f

all: vplay vrec

vplay: vplay.c fmtheaders.h
	$(CC) $(CFLAGS) vplay.c -o vplay
	-strip vplay

vrec: vplay
	rm -f vrec
	-ln -s vplay vrec

install: all
	@if [ ! -d $(AUBINDIR) ]; then \
		mkdir -p $(AUBINDIR); \
	fi
	$(INS) -f $(AUBINDIR) vplay
	cd $(AUBINDIR); rm -f vrec; ln -s vplay vrec
	@if [ ! -d $(COPYRIGHTDIR) ]; then \
		mkdir -p $(COPYRIGHTDIR); \
	fi
	cp -f COPYRIGHT $(COPYRIGHTDIR)/COPYRIGHT.vplay

clean:
	$(RM) *.o core

clobber: clean
	$(RM) vplay vrec

