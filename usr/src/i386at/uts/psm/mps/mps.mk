#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/mps/mps.mk	1.1.3.1"
#ident	"$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = mps.mk
DIR = psm/mps
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/mps.ln

LOCALINC = -I.

MPS = mps.cf/Driver.o
MPS_CCNUMA = ../mps.cf/Driver_ccnuma.o

MODULES = $(MPS)
CCNUMA_MODULES = $(MPS_CCNUMA)

PROBEFILE = mps.c

FILES = \
	mps.o

CFILES = \
	mps.c

SFILES = 



SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	mps.ln

all:	$(MODULES) ccnuma

ccnuma: 
	@if [ "$$DUALBUILD" = 1 ]; then \
		if [ ! -d ccnuma.d ]; then \
			mkdir ccnuma.d; \
			cd ccnuma.d; \
			for file in "../*.[csh] ../*.mk*"; do \
				ln -s $$file . ; \
			done; \
		else \
			cd ccnuma.d; \
		fi; \
		$(MAKE) -f mps.mk $(CCNUMA_MAKEARGS) $(CCNUMA_MODULES); \
	fi

install: all
	cd mps.cf; $(IDINSTALL) -R$(CONF) -M mps

$(MPS): $(FILES) 
	$(LD) -r -o $(MPS) $(FILES) 

$(MPS_CCNUMA): $(FILES) 
	$(LD) -r -o $(MPS_CCNUMA) $(FILES) 


clean:
	-rm -f *.o $(LFILES) *.L $(MPS)
	-rm -rf ccnuma.d

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e mps 

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
