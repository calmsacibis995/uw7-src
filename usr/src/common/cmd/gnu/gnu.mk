#ident	"@(#)gnu.mk	1.3"
#ident "$Header$"

#	Makefile for GNU distribution

include $(CMDRULES)

GNUBIN = $(ROOT)/$(MACH)/usr/gnu/bin
GNULIB = $(ROOT)/$(MACH)/usr/gnu/lib

all clean clobber lintit localinstall:
	cd shellutils-1.6; $(MAKE) $(MAKEFLAGS) -f shellutils.mk $@

install: $(GNUBIN) $(GNULIB) $(GNULIBPERL)
	cd shellutils-1.6; $(MAKE) $(MAKEFLAGS) -f shellutils.mk $@

$(GNUBIN) $(GNULIB):
	mkdir -p $@
