#ident "@(#)Makefile	11.1"
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

# uncomment this if you want debug code enabled, needed for the test suites.
DEBUG =		-D debug

#
# makefile for mail admin multihome client and supporting utilities
#

ALL =		multihome vdomains

# main goes last
TCLSRC =	host.tcl \
		mh.tcl \
		cmd.tcl \
		edit.tcl \
		utils.tcl \
		main.tcl

.SUFFIXES:	.tcl .tclt

I =		../../tests/tcl/tcltrace
P =		../../tests/tcl/gentcl

# change individual entries to .tclt suffix to have them instrumented
TSTSRC =	host.tcl \
		mh.tcl \
		cmd.tcl \
		edit.tclt \
		utils.tclt \
		main.tclt

build:		$(ALL) nls

multihome:	mhome.msg.tcl $(TCLSRC) Makefile
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat mhome.msg.tcl >> $@
		cat $(TCLSRC) >> $@
		$(P) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

# our branch coverage instrumented target
multihomet:	mhome.msg.tcl $(TSTSRC) Makefile
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat mhome.msg.tcl >> $@
		cat $(TSTSRC) >> $@
		$(P) -D test $(DEBUG) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

vdomains:	vdomains.tcl
		echo "#!/bin/osavtcl" > $@
		cat $? >> $@
		$(P) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

vdomainst:	vdomains.tclt
		echo "#!/bin/osavtcl" > $@
		cat $? >> $@
		$(P) -D test $(DEBUG) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

.tcl.tclt:
		( \
		$(P) -D test $(DEBUG) $? > $@; \
		$(I) -i $@ > /tmp/$$$$; \
		mv /tmp/$$$$ $@; \
		rm -f /tmp/$$$$ \
		)
		

mhome.msg.tcl:	NLS/en/mhome.msg modfile
		$(MKCATDECL) -i modfile NLS/en/mhome.msg

nls:		modfile
		$(DONLS) -d NLS build

msgchk:		modfile
		$(DONLS) -d NLS msgchk

clean:
		rm -f $(ALL) multihomet mhome.gen mhome.cat mhome.msg.tcl
		rm -f vdomainst
		-rm -f *.tclt
		-rm -f mhome.msg.en.vs.*
		$(DONLS) -d NLS clean
		rm -f NLS/en/mhome.gen

clobber:	clean
