#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/toolkits/psm_mc146818/psm_mc146818.mk	1.1.1.1"
#ident	"$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = psm_mc146818.mk
DIR = psm/toolkits/psm_mc146818
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/psm_mc146818.ln

LOCALINC = -I.
PSM_MC146818 = psm_mc146818.cf/Driver.o

MODULES = $(PSM_MC146818)
PROBEFILE = psm_mc146818.c

FILES = \
	psm_mc146818.o

CFILES = \
	psm_mc146818.c 

SFILES = 



SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	psm_mc146818.ln

all:	$(MODULES)

install: all
	cd psm_mc146818.cf; $(IDINSTALL) -R$(CONF) -M psm_mc146818

$(PSM_MC146818): $(FILES)
	$(LD) -r -o $(PSM_MC146818) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L $(PSM_MC146818)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e psm_mc146818 

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
