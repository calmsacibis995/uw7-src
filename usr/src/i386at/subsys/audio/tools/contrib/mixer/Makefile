#-----------------------------------------------------------------------------
#
#       Copyright (C) The Santa Cruz Operation, 1996-1997.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#---------------------------------------------------------------------------
# @(#)Makefile	8.2	2/17/98	12:08:29
#
# Revision History:
# 1997-Sep-02, stevegi, changed references to "oss" to "vox30"
# 1997-Jan-06, shawnm, created
#===========================================================================

AUBINDIR = $(ROOT)/$(MACH)/usr/bin
COPYRIGHTDIR = $(ROOT)/$(MACH)/usr/lib/audio/copyright
CC = $(TOOLS)/usr/ccs/bin/i386cc
AUDRVDIR = $(ROOT)/usr/src/$(WORK)/subsys/audio/drivers/vox30
EXTRA_INCLUDES = -I$(AUDRVDIR)
CFLAGS = -O $(EXTRA_INCLUDES)
INS = $(TOOLS)/etc/install
RM = rm -f

all: mixer

mixer:
	$(CC) $(CFLAGS) mixer.c -o mixer
	-strip mixer

clean:
	$(RM) *.o core

clobber: clean
	$(RM) mixer

install: all
	@if [ ! -d $(AUBINDIR) ]; then \
		mkdir -p $(AUBINDIR); \
	fi
	$(INS) -f $(AUBINDIR) mixer
	@if [ ! -d $(COPYRIGHTDIR) ]; then \
		mkdir -p $(COPYRIGHTDIR); \
	fi
	cp -f COPYRIGHT $(COPYRIGHTDIR)/COPYRIGHT.mixer

