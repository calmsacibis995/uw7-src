#===============================================================================
#
#	ident @(#) Makefile 11.1 97/10/30 
#
#******************************************************************************
#
#	Copyright (C) 1993-1997 The Santa Cruz Operation, Inc.
#		All Rights Reserved.
#
#	The information in this file is provided for the exclusive use of
#	the licensees of The Santa Cruz Operation, Inc.  Such users have the
#	right to use, modify, and incorporate this code into other products
#	for purposes authorized by the license agreement provided they include
#	this notice and the associated copyright notice with any such product.
#	The information in this file is provided "AS IS" without warranty.
#
#******************************************************************************
#
# Makefile for domain client
#
#
# Modification History
#
# M001, 27-Mar-97, andrean
#	- Added branch coverage instrumented test targets.
#
# M000, 20-Feb-97, andrean
#       - Created.
#
#===============================================================================

SHELL=/bin/sh

include		$(CMDRULES)

TCL =		$(TOOLS)/usr/bin/tcl
KSH =		/bin/ksh
MKCATDECL =	$(TCL) $(TOOLS)/usr/bin/mkcatdecl
MKCATDEFS =	$(TOOLS)/usr/bin/mkcatdefs
LIB =		../lib
CONFIGFILE =	$(LIB)/configFile.tcl
TABLE =		$(LIB)/table.tcl
I =		../../tests/tcl/tcltrace
P =		../../tests/tcl/gentcl
DONLS =		TCL=$(TCL) \
		INS=$(INS) \
		MKCATDECL="$(MKCATDECL)" \
		MKCATDEFS=$(MKCATDEFS) \
		USRLIB=$(USRLIB) ETC=$(ETC) \
		$(KSH) ../../bin/donls

#
# makefile for mail admin domain client and supporting utilities
#

ALL =		domain

# Ordering of the source is important:
# intl.tcl first, main.tcl last
TCLSRC =	intl.tcl \
		$(CONFIGFILE) \
		$(TABLE) \
		mapFile.tcl \
		ui.tcl \
		uimenu.tcl \
		uitoolbar.tcl \
		domain.tcl \
		main.tcl

.SUFFIXES:      .tcl .tclt

# change individual entries to .tclt suffix to have them instrumented
TSTSRC =        intl.tclt \
		$(CONFIGFILE) \
		$(TABLE) \
		mapFile.tclt \
		ui.tclt \
		uimenu.tclt \
		uitoolbar.tclt \
		domain.tclt \
		main.tclt

build:		$(ALL) nls

# the domain table editor client
domain:		domain.msg.tcl $(TCLSRC) Makefile
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat domain.msg.tcl >> $@
		cat $(TCLSRC) >> $@
		$(P) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

# our branch coverage instrumented target
domaint:	domain.msg.tcl $(TSTSRC) Makefile
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat domain.msg.tcl >> $@
		cat $(TSTSRC) >> $@
		$(P) -D test $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

domain.msg.tcl:	NLS/en/domain.msg modfile
		$(MKCATDECL) -i modfile NLS/en/domain.msg

nls:		modfile
		$(DONLS) -d NLS build

msgchk:		modfile
		$(DONLS) -d NLS msgchk

.tcl.tclt:
		( \
		$(P) -D test $? > $@; \
		$(I) -i $@ > /tmp/$$$$; \
		mv /tmp/$$$$ $@; \
		rm -f /tmp/$$$$ \
		)

clean clobber:
		rm -f $(ALL) domaint domain.gen domain.cat domain.msg.tcl
		-rm -f *.tclt domain.msg.en.vs.*
		$(DONLS) -d NLS clean
		rm -f NLS/en/domain.gen
