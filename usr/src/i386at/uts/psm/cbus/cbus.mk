#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/cbus/cbus.mk	1.4.2.2"
#ident	"$Header$"

LOCALDEF=-D_PSM=2

include $(UTSRULES)

MAKEFILE = cbus.mk
DIR = psm/cbus
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/cbus.ln

LOCALINC = -I.
CBUS = cbus.cf/Driver.o

MODULES = $(CBUS)
PROBEFILE = cbus.c

FILES = \
	cbus_msop.o \
	cbus.o \
	cbus_II.o \
	cbus_globals.o

CFILES = \
	cbus_msop.c \
	cbus.c \
	cbus_II.c \
	cbus_globals.c

SFILES = 



SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	cbus.ln

all:	$(MODULES)

install: all
	cd cbus.cf; $(IDINSTALL) -R$(CONF) -M cbus

$(CBUS): $(FILES)
	$(LD) -r -o $(CBUS) $(FILES) 

clean:
	-rm -f *.o $(LFILES) *.L $(CBUS)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e cbus 

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
