#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/compaq/compaq.mk	1.7.3.2"
#ident	"$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = compaq.mk
DIR = psm/compaq
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/compaq.ln

LOCALINC = -I.

COMPAQ = compaq.cf/Driver.o

MODULES = $(COMPAQ)
PROBEFILE = compaq.c


FILES = \
	compaq.o \
	syspro.o \
	xl.o

CFILES = \
	compaq.c \
	syspro.c \
	xl.c

SFILES =

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	compaq.ln \
	syspro.ln \
	xl.ln

all:	$(MODULES)

install: all
	cd compaq.cf; $(IDINSTALL) -R$(CONF) -M compaq 

$(COMPAQ): $(FILES)
	$(LD) -r -o $(COMPAQ) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(COMPAQ)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e compaq 

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
