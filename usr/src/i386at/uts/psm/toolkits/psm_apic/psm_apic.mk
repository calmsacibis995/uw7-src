#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/toolkits/psm_apic/psm_apic.mk	1.1.1.1"
#ident	"$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = psm_apic.mk
DIR = psm/toolkits/psm_apic
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/psm_apic.ln

LOCALINC = -I.
APIC = apic.cf/Driver.o

MODULES = $(APIC)
PROBEFILE = psm_apic.c

FILES = \
	psm_apic.o

CFILES = \
	psm_apic.c 

SFILES = 



SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	psm_apic.ln

all:	$(MODULES)

install: all
	cd apic.cf; $(IDINSTALL) -R$(CONF) -M psm_apic

$(APIC): $(FILES)
	$(LD) -r -o $(APIC) $(FILES) 
clean:
	-rm -f *.o $(LFILES) *.L $(APIC)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e psm_apic 

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
