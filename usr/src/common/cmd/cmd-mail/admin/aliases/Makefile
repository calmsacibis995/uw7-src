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
# Makefile for aliases client
#
#
# Modification History
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
MKCATDEFS =     $(TOOLS)/usr/bin/mkcatdefs
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
# makefile for mail admin aliases client and supporting utilities
#

ALL =		aliases

# aliases.tcl goes last
TCLSRC =	$(CONFIGFILE) \
		$(TABLE) \
		aliasFile.tcl \
		aliases.tcl

.SUFFIXES:	.tcl .tclt

# change individual entries to .tclt suffix to have them instrumented
TSTSRC =	$(CONFIGFILE) \
		$(TABLE) \
		aliasFile.tclt \
		aliases.tclt

build:		$(ALL) nls

# the aliases editor client
aliases:	alias.msg.tcl $(TCLSRC) Makefile
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat alias.msg.tcl >> $@
		cat $(TCLSRC) >> $@
		$(P) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

# our branch coverage instrumented target
aliasest:	alias.msg.tcl $(TSTSRC) Makefile
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat alias.msg.tcl >> $@
		cat $(TSTSRC) >> $@
		$(P) -D test $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

alias.msg.tcl:	NLS/en/alias.msg modfile
		$(MKCATDECL) -i modfile NLS/en/alias.msg

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
		rm -f $(ALL) aliasest alias.gen alias.cat alias.msg.tcl
		-rm -f *.tclt alias.msg.en.vs.*
		$(DONLS) -d NLS clean
		rm -f NLS/en/alias.gen
