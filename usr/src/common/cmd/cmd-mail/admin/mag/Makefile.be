#ident "@(#)Makefile.be	11.1"
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
MKCATDECL =	$(TCL) $(TOOLS)/usr/bin/mkcatdecl
# uncomment this if you want debug code enabled, needed for the test suites.
DEBUG =		-D debug

#
# makefile for mail admin sysadm client and supporting utilities
#

ALL =		mailadmin

# main goes last
TCLSRC =	utils.tcl \
		object.tcl \
		objedit.tcl \
		flags.tcl \
		bad.tcl \
		chadd.tcl \
		host.tcl \
		cmd.tcl \
		ma_ms1.tcl \
		ma_cf.tcl \
		ma_sendmail.tcl \
		../lib/configFile.tcl \
		../lib/table.tcl \
		main.tcl

.SUFFIXES:	.tcl .tclt

I =		../../tests/tcl/tcltrace
P =		../../tests/tcl/gentcl

# change individual entries to .tclt suffix to have them instrumented
TSTSRC =	utils.tcl \
		object.tcl \
		objedit.tcl \
		flags.tcl \
		bad.tcl \
		chadd.tcl \
		host.tcl \
		cmd.tclt \
		ma_ms1.tclt \
		ma_cf.tclt \
		ma_sendmail.tclt \
		../lib/configFile.tcl \
		../lib/table.tcl \
		main.tcl

build:		$(ALL)

mailadmin:	mag.msg.tcl $(TCLSRC) Makefile px
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat mag.msg.tcl >> $@
		cat $(TCLSRC) >> $@
		$(P) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

# our branch coverage instrumented target
mailadmint:	mag.msg.tcl $(TSTSRC) Makefile px
		echo "#!/bin/osavtcl" > $@
		echo "loadlibindex /usr/lib/sysadm.tlib" >> $@
		cat mag.msg.tcl >> $@
		cat $(TSTSRC) >> $@
		$(P) -D test $(DEBUG) $@ > /tmp/$@
		cp /tmp/$@ $@
		rm -f /tmp/$@
		chmod 755 $@

# this makes it so you can run the program from this directory.
px:		../px
		ln -s ../px px

.tcl.tclt:
		( \
		$(P) -D test $(DEBUG) $? > $@; \
		$(I) -i $@ > /tmp/$$$$; \
		mv /tmp/$$$$ $@; \
		rm -f /tmp/$$$$ \
		)
		

mag.msg.tcl:	mag.msg modfile
		$(MKCATDECL) -i modfile mag.msg

clean:
		rm -f $(ALL) mailadmint mag.gen mag.cat mag.msg.tcl
		-rm -f *.tclt
		-rm -f ../lib/*.tclt

clobber:	clean
		rm -f px
