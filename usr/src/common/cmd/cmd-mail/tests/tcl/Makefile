#ident	"@(#)Makefile	11.1"

SHELL =		/bin/sh

all:		tcltrace gentcl

build:		all

install:

local-install:

local-remove:

tcltrace:	tcltrace.c
		cc -o tcltrace tcltrace.c

gentcl:		gentcl.c
		cc -o gentcl gentcl.c

clean:

clobber:	clean
		rm -f tcltrace gentcl
