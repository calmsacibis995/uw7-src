#ident	"@(#)debugger:debugsrc.mk	1.21"

#
#	Note: we use our make command even when using
#	other host tools.  We know our make will support
#	includes when used with macros, parallel make, etc.
#

include $(CMDRULES)
include util/common/defs.make


UI = motif
MACHDEFS = threads.defs
OSR5_DEFS = osr5.defs
UW_DEFS = uw.defs

LIBDIRS = libcmd libdbgen libedit libexecon libexp libint libmachine libsymbol libutil lib$(UI)
CLI_LIBDIRS = libcmd libdbgen libedit libexecon libexp libint libmachine libsymbol libutil 
GUI_LIBDIRS = lib$(UI)


CLI = debug

OSR5_CLI = debug.osr5

UW_CLI  = debug.uw

GUI = debug.$(UI).ui

OSR5_GUI = debug.$(UI).ui.osr5

UW_GUI = debug.$(UI).ui.uw

PRODUCTS = $(CLI) $(GUI) $(OSR5_CLI) $(OSR5_GUI) $(UW_CLI) $(UW_GUI)

DEBUGARGS= UI='$(UI)' MACHDEFS='$(MACHDEFS)'

OSR5ARGS= UI='$(UI)' MACHDEFS='$(OSR5_DEFS)'

DPATHS=debug_paths.o
UW_DPATHS=debug_paths.uw.o
OSR5_DPATHS=debug_paths.osr5.o

DIRS = $(LIBDIRS) debug.d gui.d tutorial.d
CLI_DIRS = $(CLI_LIBDIRS) debug.d
GUI_DIRS = $(GUI_LIBDIRS) gui.d

UTILS= lib

OSR5_UTILS = osr5_lib

CLLIBS = lib/libcmd.a lib/libdbgen.a lib/libedit.a lib/libexecon.a \
	lib/libexp.a lib/libint.a \
	lib/libmachine.a lib/libsymbol.a lib/libutil.a

GUILIBS = lib/libdbgen.a lib/libint.a lib/lib$(UI).a

OSR5_CLLIBS = osr5_lib/libcmd.a osr5_lib/libdbgen.a \
	osr5_lib/libedit.a osr5_lib/libexecon.a \
	osr5_lib/libexp.a osr5_lib/libint.a \
	osr5_lib/libmachine.a osr5_lib/libsymbol.a \
	osr5_lib/libutil.a

OSR5_GUILIBS = osr5_lib/libdbgen.a osr5_lib/libint.a \
	osr5_lib/lib$(UI).a

FORCE = force

.MUTEX:		$(UTILS) $(OSR5_UTILS) cltargets guitargets catalog.d/$(CPU) osr5_cltargets osr5_guitargets

.MUTEX:	$(CLI) $(GUI) $(OSR5_CLI) $(OSR5_GUI) $(UW_CLI) $(UW_GUI)

all:	$(PRODUCTS)

cli:	$(CLI)

gui:	$(GUI)

osr5_cli:	$(OSR5_CLI)

osr5_gui:	$(OSR5_GUI)

uw_cli:	$(UW_CLI)

uw_gui:	$(UW_GUI)

lib:
		mkdir lib

osr5_lib:
		mkdir osr5_lib

cltargets:	$(CLLIBS)

guitargets:	$(GUILIBS)

osr5_cltargets:	$(OSR5_CLLIBS)

osr5_guitargets:	$(OSR5_GUILIBS)

alias:	debug_alias

debug:	$(DPATHS) $(UTILS) catalog.d/$(CPU) cltargets $(FORCE)
		cd debug.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

debug.osr5:	$(OSR5_DPATHS) $(OSR5_UTILS) catalog.d/$(CPU) osr5_cltargets $(FORCE)
		cd debug.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

debug.uw:	$(UW_DPATHS) $(UTILS) catalog.d/$(CPU) cltargets $(FORCE)
		cd debug.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) uw

debug.$(UI).ui:	$(DPATHS) $(UTILS) catalog.d/$(CPU) guitargets $(FORCE)
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

debug.$(UI).ui.osr5:	$(OSR5_DPATHS) $(OSR5_UTILS) catalog.d/$(CPU) osr5_guitargets $(FORCE)
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

debug.$(UI).ui.uw:	$(UW_DPATHS) $(OSR5_UTILS) catalog.d/$(CPU) guitargets $(FORCE)
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) uw

lib/libcmd.a:	$(FORCE)
	cd libcmd/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libcmd.a:	$(FORCE)
	cd libcmd/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/libdbgen.a:	$(FORCE)
		cd libdbgen/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libdbgen.a:	$(FORCE)
		cd libdbgen/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/libedit.a:	$(FORCE)
	cd libedit/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libedit.a:	$(FORCE)
	cd libedit/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/libexecon.a:	$(FORCE)
		cd libexecon/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libexecon.a:	$(FORCE)
		cd libexecon/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/libexp.a:	$(FORCE)
		cd libexp/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libexp.a:	$(FORCE)
		cd libexp/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/libint.a:	$(FORCE)
		cd libint/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libint.a:	$(FORCE)
		cd libint/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/libmachine.a:	$(FORCE)
		cd libmachine/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libmachine.a:	$(FORCE)
		cd libmachine/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/libsymbol.a:	$(FORCE)
		cd libsymbol/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libsymbol.a:	$(FORCE)
		cd libsymbol/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/libutil.a:	$(FORCE)
		cd libutil/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/libutil.a:	$(FORCE)
		cd libutil/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

lib/lib$(UI).a:	$(FORCE)
	cd lib$(UI)/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

osr5_lib/lib$(UI).a:	$(FORCE)
	cd lib$(UI)/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) osr5

catalog.d/$(CPU):	$(FORCE)
	cd catalog.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

tutorial.d/$(CPU):	$(FORCE)
	cd tutorial.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

install_cli:	cli
	cd debug.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd catalog.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd tutorial.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cp debug_alias $(CCSLIB)/debug_alias

install_gui:	gui
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install
	cd config.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install

install_osr5_cli:	osr5_cli
	cd debug.d/$(CPU) ; $(MAKE) install_osr5 $(MAKEARGS) $(OSR5ARGS)

install_uw_cli:	uw_cli
	cd debug.d/$(CPU) ; $(MAKE) install_uw $(MAKEARGS) $(DEBUGARGS)

install_osr5_gui:	osr5_gui
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) install_osr5
	cd conf.osr5/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) install

install_uw_gui:	uwgui
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install_uw

install:	all
	cd debug.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install
	cd debug.d/$(CPU) ; $(MAKE) install_osr5 $(MAKEARGS) $(OSR5ARGS)
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) install_osr5
	cd catalog.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd debug.d/$(CPU) ; $(MAKE) install_uw $(MAKEARGS) $(DEBUGARGS)
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install_uw
	cd tutorial.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd tutorial.d/$(CPU) ; $(MAKE) install_uw $(MAKEARGS) $(DEBUGARGS)
	cd tutorial.d/$(CPU) ; $(MAKE) install_osr5 $(MAKEARGS) $(DEBUGARGS)
	cd config.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd config.d/$(CPU) ; $(MAKE) install_uw $(MAKEARGS) $(DEBUGARGS)
	cd conf.osr5/$(CPU) ; $(MAKE) $(MAKEARGS) $(OSR5ARGS) install
	cp debug_alias $(CCSLIB)/debug_alias
	cp debug_alias $(UW_CCSLIB)/debug_alias
	cp debug_alias $(OSR5_CCSLIB)/debug_alias

debug_paths.o:	debug_paths.C
	$(CPLUS) -c debug_paths.C

debug_paths.uw.o:	debug_paths.C
	$(CPLUS) -Wa,"-odebug_paths.uw.o" -DGEMINI_ON_UW \
	-DALT_PREFIX=\"$(ALT_PREFIX)\" -c debug_paths.C

debug_paths.osr5.o:	debug_paths.C
	$(CPLUS) -Wa,"-odebug_paths.osr5.o" -DGEMINI_ON_OSR5 \
	-DALT_PREFIX=\"$(ALT_PREFIX)\" -c debug_paths.C


depend:	lib osr5_lib catalog.d/$(CPU)
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) depend $(DEBUGARGS) ) ;\
	done

clean_cli:	
	@for i in $(CLI_DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) clean $(DEBUGARGS) ) ;\
	done

clean_gui:	
	@for i in $(GUI_DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) clean $(DEBUGARGS) ) ;\
	done

clean:	
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) clean $(DEBUGARGS) ) ;\
	done
	cd catalog.d/$(CPU) ; $(MAKE) clean $(DEBUGARGS)
	rm -f $(DPATHS) $(OSR5_DPATHS) $(UW_DPATHS)

clobber: 
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) clobber $(DEBUGARGS) ) ;\
	done
	cd catalog.d/$(CPU) ; $(MAKE) clobber $(DEBUGARGS)
	rm -f $(DPATHS) $(OSR5_DPATHS) $(UW_DPATHS)

lintit:
	@echo "can't lint C++"

rebuild:	clobber depend all

force:
	@:
