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
# Makefile for user admin vacation client
#
#
# Modification History
#
# M000, 14-Apr-97, andrean
#       - Created.
#
#===============================================================================

SHELL=/bin/sh

include		$(CMDRULES)

TCL =		$(TOOLS)/usr/bin/tcl
KSH =		/bin/ksh
MKCATDECL =	$(TCL) $(TOOLS)/usr/bin/mkcatdecl
MKCATDEFS =	$(TOOLS)/usr/bin/mkcatdefs
DONLS =		TCL=$(TCL) \
		INS=$(INS) \
		MKCATDECL="$(MKCATDECL)" \
		MKCATDEFS=$(MKCATDEFS) \
		USRLIB=$(USRLIB) ETC=$(ETC) \
		$(KSH) ../../bin/donls
I =		../../tests/tcl/tcltrace
P =		../../tests/tcl/gentcl

#
# makefile for mail admin channel table client and supporting utilities
#

ALL =		vacation

# Ordering of the source is important:
# intl.tcl first, main.tcl last
TCLSRC =	vacation.tcl

.SUFFIXES:      .tcl .tclt

# change individual entries to .tclt suffix to have them instrumented
TSTSRC =        vacation.tclt

build:		$(ALL) nls

# the channel table editor client
vacation:	vacation.msg.tcl $(TCLSRC) Makefile
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat vacation.msg.tcl >> $@
		cat $(TCLSRC) >> $@
		$(P) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

# our branch coverage instrumented target
vacationt:	vacation.msg.tcl $(TSTSRC) Makefile
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat vacation.msg.tcl >> $@
		cat $(TSTSRC) >> $@
		$(P) -D test $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

vacation.msg.tcl:	NLS/en/vacation.msg modfile
			$(MKCATDECL) -i modfile NLS/en/vacation.msg

nls:			modfile
			$(DONLS) -d NLS build

msgchk:			modfile
			$(DONLS) -d NLS msgchk

.tcl.tclt:
		( \
		$(P) -D test $? > $@; \
		$(I) -i $@ > /tmp/$$$$; \
		mv /tmp/$$$$ $@; \
		rm -f /tmp/$$$$ \
		)

clean clobber:
		rm -f $(ALL) vacationt vacation.gen vacation.cat \
			vacation.msg.tcl
		-rm -f *.tclt vacation.msg.en.vs.*
		$(DONLS) -d NLS clean
		rm -f NLS/en/vacation.gen
