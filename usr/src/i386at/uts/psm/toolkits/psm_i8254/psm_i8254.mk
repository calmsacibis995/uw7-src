#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/toolkits/psm_i8254/psm_i8254.mk	1.1.1.1"
#ident  "$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = psm_i8254.mk
DIR = psm/toolkits/psm_i8254
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/psm_i8254.ln

LOCALINC = -I.
PSM_i8254 = psm_i8254.cf/Driver.o

MODULES = $(PSM_i8254)
PROBEFILE = psm_i8254.c

FILES = \
	psm_i8254.o

CFILES = \
	psm_i8254.c 

SFILES = 



SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	psm_i8254.ln

all:	$(MODULES)

install: all
	cd psm_i8254.cf; $(IDINSTALL) -R$(CONF) -M psm_i8254

$(PSM_i8254): $(FILES)
	$(LD) -r -o $(PSM_i8254) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L $(PSM_i8254)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e psm_i8254 

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
