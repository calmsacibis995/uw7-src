#ident	"@(#)cmd-nm.mk	1.3"
#ident	"$Header$"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

include $(CMDRULES)

TOP = .

SUBDIRS = usr.sbin etc


INCMODE = 444
INCNETMGT = $(ROOT)/$(MACH)/usr/include/netmgt
BINNETMGT = $(ROOT)/$(MACH)/usr/sbin
all clean clobber: 
	@for i in $(SUBDIRS) ; \
	do \
	cd $$i ; \
	$(MAKE) -f $$i.mk $@ $(MAKEARGS) ; cd .. ; \
	done

pkgHeaders = \
        objects.h \
        snmp-mib.h \
        snmp.h \
        snmpio.h \
        snmpio.svr4.h \
        snmpuser.h

localhead:
	@-[ -d $(INCNETMGT) ] || mkdir -p $(INCNETMGT)
	@for f in $(pkgHeaders); \
	do \
	(cd ../../head/netmgt ; \
	$(INS) -f $(INCNETMGT) -m $(INCMODE) -u $(OWN) -g $(GRP) $$f) \
	 done

install : localhead
	@-[ -d $(BINNETMGT) ] || mkdir -p $(BINNETMGT)
	@for i in $(SUBDIRS) ; \
	do \
	cd $$i ; \
	$(MAKE) -f $$i.mk $@ $(MAKEARGS) ; cd .. ; \
	done

lintit: 
	-@for i in $(SUBDIRS) ; \
	do \
	cd $$i ; \
	$(MAKE) -f $$i.mk $@ ; cd .. ; \
	done
