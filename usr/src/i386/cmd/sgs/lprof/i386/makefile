#ident	"@(#)lprof:i386/makefile	1.5"

include $(CMDRULES)

LPROFBASE	= `pwd | sed 's/lprof.*/lprof/'`

SGSBASE		= $(LPROFBASE)/..
CMDBASE		= $(LPROFBASE)/../..

include ../lprofinc.mk

all install lintit clean clobber:
	cd ..; $(MAKE) -f lprof.mk $@
