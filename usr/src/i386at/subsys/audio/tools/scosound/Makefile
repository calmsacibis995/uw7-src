#
# @(#)Makefile	7.1	97/10/22
#
# 1997-May-07, shawnm, updated to use cross env for config, includes
# 1997-Jan-07, shawnm, created from motif makefile
#

MAKE_CMD = $(PFX)make -f xmakefile 
CONFIGDIR = ${TOOLS}/usr/X11R6.1/lib/X11/config
IMAKE_CMD = ${TOOLS}/usr/X11R6.1/bin/imake

.PRECIOUS: Makefile

xmakefile:
	$(IMAKE_CMD) -DUseInstalled -I$(CONFIGDIR) -DUSL -DUSEXENV -DBuildMotif -DTOPDIR=. -DCURDIR=. -s xmakefile

install: xmakefile
	$(MAKE_CMD) Makefiles
	$(MAKE_CMD) includes
	$(MAKE_CMD) depend
	$(MAKE_CMD) -k
	$(MAKE_CMD) $(MFLAGS) install DESTDIR=$(ROOT)/$(MACH)

all clean depend Everything includes Makefiles: xmakefile
	    $(MAKE_CMD) $@

clobber: clean
	    rm -f xmakefile

