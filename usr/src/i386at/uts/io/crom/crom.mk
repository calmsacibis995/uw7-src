#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/crom/crom.mk	1.6.2.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = crom.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/crom 

LOCALDEF = -DUNIXWARE

CROM = crom.cf/Driver.o
BINARIES = $(CROM)
LFILE = $(LINTDIR)/crom.ln
PROBEFILE = crom.c

MODULES = \
	$(CROM)

FILES = \
	crom.o 

CFILES = \
	crom.c

SFILES = 

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	crom.ln

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
		"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi
             


install: all
	cd crom.cf; $(IDINSTALL) -R$(CONF) -M crom

binaries:	$(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $(CROM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L

clobber:	clean
		-$(IDINSTALL) -R$(CONF) -d -e crom 
		@if [ -f $(PROBEFILE) ]; then \
			echo "rm -f $(BINARIES)" ;\
			rm -f $(BINARIES) ;\
		fi         

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	crom.h

headinstall:
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
		if [ -f $$f ]; then \
		  $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
		fi ;\
	 done

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep

