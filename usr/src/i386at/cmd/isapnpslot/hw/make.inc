INC=$(TOOLS)/usr/include
PNPINC=../../../uts/io/autoconf/ca/isapnp
SYSINC=$(TOOLS)/usr/include
MAKE=make
AS=/bin/as
LINT=/usr/bin/lint
INSPECT=:
CPP=/lib/cpp
LD=/bin/ld
CHOWN=/bin/true
CHGRP=/bin/true
SHELL=/bin/sh
COMPFMT=
INTL=-N
LIBPROT=-lprot -lx -lm -lcurses
LIBMOUNTOPS=-lmnt
NETLIB=-lsocket
XPGUTIL=-a xpg4plus
GENCAT=/usr/bin/gencat
MKCATDEFS=/bin/mkcatdefs -a Unix
.SUFFIXES: .cat .gen .msg _msg.h
.gen.cat:
	$(MKCATDEFS) $* $*.gen | $(GENCAT) $@ -
.gen_msg.h:
	$(MKCATDEFS) $* $*.gen > /dev/null
.msg.cat:
	$(MKCATDEFS) $* $*.msg | $(GENCAT) $@ -
.msg_msg.h:
	$(MKCATDEFS) $* $*.msg > /dev/null

