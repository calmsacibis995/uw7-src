#ident	"@(#)shellutils.mk	1.2"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "$Header$"

#	Makefile for GNU shell utilities

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr

all: clean
	[ -f config.status ] || CC="$(CC)" LD="$(LD)" INCLUDEDIR="$(INC)" INSTALL="cp" INSTALLDATA="cp" PREFIX="$(USR)/gnu" sh configure
	$(MAKE) $(MAKEFLAGS) $@

install:
	[ -f config.status ] || CC="$(CC)" LD="$(LD)" INCLUDEDIR="$(INC)" INSTALL="cp" INSTALLDATA="cp" PREFIX="$(USR)/gnu" sh configure
	$(MAKE) $(MAKEFLAGS) $@

clean:
	[ -f config.status ] || CC="$(CC)" LD="$(LD)" INCLUDEDIR="$(INC)" INSTALL="cp" INSTALLDATA="cp" PREFIX="$(USR)/gnu" sh configure
	$(MAKE) $(MAKEFLAGS) mostlyclean

clobber:
	[ -f config.status ] || CC="$(CC)" LD="$(LD)" INCLUDEDIR="$(INC)" INSTALL="cp" INSTALLDATA="cp" PREFIX="$(USR)/gnu" sh configure
	$(MAKE) $(MAKEFLAGS) clean

lintit: mailalias.lint

mailalias.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
