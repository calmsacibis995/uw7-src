#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:util/kdb/scodb/scodb.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	scodb.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = util/scodb
TMP=$(ROOT)/$(MACH)/tmp

SCODB_CFLAGS=-O -DUSE_SCOBD_ADDRCHECK -Dprintf=dbprintf -Dputchar=dbputc -DNOTSTATIC=
SCODB_KCFLAGS=-W0,-d1 -Dprintf=dbprintf -Dputchar=dbputc -DNOTSTATIC=

SCODB = scodb.cf/Driver.o
LFILE = $(LINTDIR)/scodb.ln

KSTRUCT = \
	kstruct.o.atup \
	kstruct.o.mp

.MUTEX: $(KSTRUCT)

OBJS = \
	alias.o \
	ansi.o \
	bkp.o \
	bkp_extra.o \
	calc.o \
	chdump.o \
	dcl.o \
	debug.o \
	dis.o \
	entry.o \
	help.o \
	io.o \
	posit.o \
	psv.o \
	quit.o \
	regs.o \
	sregs.o \
	stack.o \
	steps.o \
	struct.o \
	sym.o \
	val.o \
	bell.o \
	init.o \
	memory.o \
	qif.o \
	space.o \
	mod_ksym.o


CFILES = \
	alias.c \
	ansi.c \
	bkp.c \
	bkp_extra.c \
	calc.c \
	chdump.c \
	dcl.c \
	debug.c \
	dis.c \
	help.c \
	io.c \
	posit.c \
	psv.c \
	quit.c \
	regs.c \
	routines.c \
	stack.c \
	steps.c \
	struct.c \
	sym.c \
	val.c \
	bell.c \
	init.c \
	memory.c \
	qif.c \
	space.c

SRCFILES = $(CFILES)

LFILES = \
	scodb.ln


all:	$(SCODB)  $(KSTRUCT)

install: all
	-[ -d $(CONF)/pack.d/scodb ] || mkdir -p $(CONF)/pack.d/scodb
	(cd scodb.cf; \
	 $(IDINSTALL) -M -R$(CONF) scodb; )
	-[ -d $(CONF)/pack.d/scodb/info ] || mkdir -p $(CONF)/pack.d/scodb/info
	$(INS) -f $(CONF)/pack.d/scodb/info -m $(INCMODE) -u $(OWN) -g $(GRP) kstruct.o.atup
	$(INS) -f $(CONF)/pack.d/scodb/info -m $(INCMODE) -u $(OWN) -g $(GRP) kstruct.o.mp
	$(INS) -f $(CONF)/pack.d/scodb/info -m $(INCMODE) -u $(OWN) -g $(GRP) kstruct.c

headinstall: localhead
	
sysHeaders = \
	stunv.h \
	dbg.h \
	sent.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys/scodb ] || mkdir -p $(INC)/sys/scodb
	@for f in $(sysHeaders); \
	 do \
		$(INS) -f $(INC)/sys/scodb -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


kstruct.o.atup: kstruct.c
	$(CC) -g $(SCODB_KCFLAGS) $(INCLIST) $(DEFLIST) -DUNIPROC -c kstruct.c
	mv kstruct.o kstruct.o.atup

kstruct.o.mp: kstruct.c
	$(CC) -g $(SCODB_KCFLAGS) $(INCLIST) $(DEFLIST) -UUNIPROC -c kstruct.c
	mv kstruct.o kstruct.o.mp

$(SCODB):  $(OBJS)
	$(LD) -r -o $@ $(OBJS)


.SUFFIXES: .atup.o .mp.o

.c.o:
	$(CC) -c $(CFLAGS) $(SCODB_CFLAGS) $(INCLIST) $(DEFLIST)  $<


clean:
	-rm -f *.o $(LFILES) *.L $(SCODB)  $(KSTRUCT)

clobber:	clean
	$(IDINSTALL) -R$(CONF) -d -e scodb

lintit: $(LFILE)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

$(LFILE): $(LINTDIR) $(LFILES)

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done


headinstall:


include $(UTSDEPEND)

include $(MAKEFILE).dep
