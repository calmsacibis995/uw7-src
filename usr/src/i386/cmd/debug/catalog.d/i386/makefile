#ident	"@(#)debugger:catalog.d/i386/makefile	1.18"

include $(CMDRULES)

AWK = awk
LIBINT = ../../libint
LIBCMD = ../../libcmd
DEBUGINC = ../../inc
GUI = ../../gui.d

include ../../util/common/std.make

include DEPEND
TARGET = $(OBJECTS)

all:	$(OBJECTS)

install:	$(OBJECTS)
	-if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; then \
		mkdir -p $(USRLIB)/locale/C/MSGFILES; \
	fi
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.str
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.newlab.str
	cp Mcatalog $(USRLIB)/locale/C/MSGFILES/debug.str
	cp GMcatalog $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	cp GLcatalog $(USRLIB)/locale/C/MSGFILES/debug.newlab.str
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.str
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.newlab.str
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.help
	rm -f $(USRLIB)/locale/C/MSGFILES/dbg.help.thr
	rm -f $(USRLIB)/locale/C/MSGFILES/dbg.help.pt
	cp $(COMMON)/cli.help $(USRLIB)/locale/C/MSGFILES/debug.help
	cp $(COMMON)/cli.help.thr $(USRLIB)/locale/C/MSGFILES/dbg.help.thr
	cp $(COMMON)/cli.help.pt $(USRLIB)/locale/C/MSGFILES/dbg.help.pt
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.help
	chmod 444 $(USRLIB)/locale/C/MSGFILES/dbg.help.thr
	chmod 444 $(USRLIB)/locale/C/MSGFILES/dbg.help.pt

install_uw11:	install $(HELPFILES)
	-if [ ! -d $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug ]; then \
		mkdir -p $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug; \
	fi
	rm -f $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/debug.hlp
	rm -f $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/debug.defs_CC
	cp $(COMMON)/debug.hlp $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug
	cp $(COMMON)/debug.defs $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/debug.defs_CC
	chmod 444 $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/debug.*
