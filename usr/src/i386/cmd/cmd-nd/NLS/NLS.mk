#ident "@(#)NLS.mk	5.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(CMDRULES)

MAKEFILE = NLS.mk
MSGDEFAULT = en
SUBDIRS = en

# ugly hack to get xenv mkcatdecl - remove when it's added to rulefiles!!!
TCL_LIBRARY=$(TOOLS)/usr/lib/tcl7.5/
TCLX_LIBRARY=$(TOOLS)/usr/lib/tclX/7.5.2
TCL=LD_LIBRARY_PATH=$(TOOLS)/usr/lib $(TOOLS)/usr/bin/tcl
RUNTCL=TCL_LIBRARY=$(TCL_LIBRARY) TCLX_LIBRARY=$(TCLX_LIBRARY) $(TCL)
MKCATDECL=$(RUNTCL) $(TOOLS)/usr/bin/mkcatdecl -i ../moduleIds

all: ncfg

ncfg:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; $(MKCATDECL) -m SCO_NETCONFIG_UI -m SCO_NCFGPROMPTER -m SCO_NCFGINSTALL -m SCO_NCFG -m SCO_NETCONFIG_BE -m SCO_LIBSCO -m SCO_DLPID $@.msg) \
		fi; \
	done

install: all
	# built default message catalogs installed by ../cmd-nd.mk

clobber clean:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; rm -f *.msg.tcl *.gen); \
		fi; \
	 done

lintit:
