#ident	"@(#)kern-i386at:io/cpqw/cpqw.mk	1.5.3.1"
#ident	"$Header$"

#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.

include $(UTSRULES)

MAKEFILE = cpqw.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/cpqw

LOCALDEF = -DUNIXWARE

CPQW = cpqw.cf/Driver.o
BINARIES = $(CPQW)
LFILE = $(LINTDIR)/cpqw.ln
PROBEFILE = cpqw.c

LOCALDEF = -c -O -D_KERNEL -DUNIXWARE -DTROPICAL_STORM

FILES= \
	cpqw.o \
	ecc_nmi.o \
	asr.o \
	csm.o \
	cpqw_lib.o \
	hw_readSIT.o \
	pciutil.o \
	mca.o \
	pentlib.o 

CFILES = \
	cpqw.c \
	ecc_nmi.c \
	asr.c \
	csm.c \
	cpqw_lib.c \
	hw_readSIT.c \
	pciutil.c \
	mca.c \
	pentlib.s 

SRCFILES = $(CFILES)

LFILES = \
	cpqw.ln \
	ecc_nmi.ln \
	asr.ln \
	csm.ln \
	cpqw_lib.ln \
	hw_readSIT.ln \
	pciutil.ln \
	mca.ln \
	pentlib.ln 

SUBDIRS =

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

install:	all
		(cd cpqw.cf; $(IDINSTALL) -R$(CONF) -M cpqw)

binaries:	$(BINARIES)

$(BINARIES):	$(FILES)
		$(LD) -r -o $(CPQW) $(FILES)

touch:
		touch *.c

clean:
		-rm -f *.o $(LFILES) *.L
clobber:	clean
		-$(IDINSTALL) -R$(CONF) -d -e cpqw
		@if [ -f $(PROBEFILE) ]; then \
			echo "rm -f $(BINARIES)" ;\
			rm -f $(BINARIES) ;\
		fi


$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
		-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
		for i in $(LFILES); do \
				cat $$i >> $(LFILE); \
				cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln' `.L; \
		done

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
		cpqw.h

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

cpqw.o:		cpqw.c 


ecc_nmi.o:	ecc_nmi.c 

asr.o:		asr.c 

csm.o:		csm.c

cpqw_lib.o:		cpqw_lib.c

hw_readSIT.o:	hw_readSIT.c

pciutil.o:	pciutil.c

mca.o:		mca.c

pentlib.o:	pentlib.s
		$(AS) pentlib.s
