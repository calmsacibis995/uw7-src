#ident	"@(#)OSRcmds:make.inc	1.2"
CMDINC=$(ROOT)/usr/src/$(WORK)/cmd/include
OSRDIR=$(ROOT)/$(MACH)/OpenServer/bin
__DSROOT=
INC=$(ROOT)/$(MACH)/usr/include
SYSINC=$(INC)/sys
MAKE=$(TOOLS)/usr/ccs/bin/$(PFX)make
MCS=$(TOOLS)/usr/ccs/bin/$(PFX)mcs
SIZE=$(TOOLS)/usr/ccs/bin/$(PFX)size
AS=$(TOOLS)/usr/ccs/bin/$(PFX)as
AR=$(TOOLS)/usr/ccs/bin/$(PFX)ar
INS=$(TOOLS)/bin/install
STRIP=$(TOOLS)/usr/ccs/bin/$(PFX)strip
INTL=-DINTL
CC=ROOT=$(TOOLS)/usr/ccs/bin __DSROOT= INTL="$(INTL)" $(TOOLS)/usr/ccs/bin/$(PFX)cc -DINTL
LINT=$(TOOLS)/usr/bin/$(PFX)lint
INSPECT=:
CPP=$(TOOLS)/usr/ccs/bin/$(PFX)CC
LD=$(TOOLS)/usr/ccs/bin/$(PFX)ld
CFLAGS=-O -s
LDFLAGS=-s
#CHOWN=/bin/true
#CHGRP=/bin/true
SHELL=/bin/sh
LIBPROT=-lprot -lx -lm -lcurses
LIBMOUNTOPS=-lmnt
NETLIB=-lsocket
XPGUTIL=-a xpg4plus
GENCAT=$(TOOLS)/usr/bin/mkcatdecl
#GENCAT=/usr/bin/gencat
MKCATDEFS=$(TOOLS)/usr/bin/mkcatdefs -a Unix
NLSLIB=$(OSRDIR)/usr/lib/nls/msg/en/Unix
#NLSLIB=/x/builds/Netos/bld/usr/lib/nls/msg/en/Unix
NLSSRC=$(OSRDIR)/usr/lib/nls.src/msg/en/Unix
DOCATS=__DSROOT= $(TOOLS)/usr/bin/docats -a Unix
.SUFFIXES: .cat .gen .msg _msg.h
.gen.cat:
	$(MKCATDEFS) $* $*.gen | $(GENCAT) $@ -
.gen_msg.h:
	$(MKCATDEFS) $* $*.gen > /dev/null
.msg.cat:
	$(MKCATDEFS) $* $*.msg | $(GENCAT) $@ -
.msg_msg.h:
	$(MKCATDEFS) $* $*.msg > /dev/null
