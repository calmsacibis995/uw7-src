#ident	"@(#)debugger:gui.d/i386/makefile	1.28"

include $(CMDRULES)

BASENAME = debug.$(UI).ui

TARGET = ../../$(BASENAME)

OSR5_BASENAME = debug.$(UI).ui.osr5

OSR5_TARGET = ../../$(OSR5_BASENAME)

UW_BASENAME = debug.$(UI).ui.uw

UW_TARGET = ../../$(UW_BASENAME)

include ../../util/common/std.make

include ../../lib$(UI)/$(CPU)/ui.defs

DFLAGS = $(XDEFS) -I../../lib$(UI)/common -I$(XINC)

# following does not use UIBASE because it's used by LD_RUN_PATH, 
# and as such needs to be the path in a native environment
XLIBPATH = /usr/X11R6.1/lib
#XLIBPATH = /usr/X/lib

CCSOURCES = \
	$(COMMON)/Base_win.C \
	$(COMMON)/Btn_dlg.C \
	$(COMMON)/Buttons.C \
	$(COMMON)/Command.C \
	$(COMMON)/Ctrl_dlg.C \
	$(COMMON)/Dialogs.C \
	$(COMMON)/Dis.C \
	$(COMMON)/Dispatcher.C \
	$(COMMON)/Event_dlg.C \
	$(COMMON)/Eventlist.C \
	$(COMMON)/Events.C \
	$(COMMON)/FileInfo.C \
	$(COMMON)/Help.C \
	$(COMMON)/Label.C \
	$(COMMON)/Mem_dlg.C \
	$(COMMON)/Notifier.C \
	$(COMMON)/Panes.C \
	$(COMMON)/Proclist.C \
	$(COMMON)/Ps_pane.C \
	$(COMMON)/Queries.C \
	$(COMMON)/Regs_pane.C \
	$(COMMON)/Sch_dlg.C \
	$(COMMON)/Show_value.C \
	$(COMMON)/Source.C \
	$(COMMON)/Stack_pane.C \
	$(COMMON)/Status.C \
	$(COMMON)/Syms_pane.C \
	$(COMMON)/Transcript.C \
	$(COMMON)/Windows.C \
	$(COMMON)/config.C \
	$(COMMON)/main.C \
	$(COMMON)/util.C \
	CoreInfo.C \
	ps.C \
	GLabel.C

CSOURCES = GMtable.c

include DEPEND

$(TARGET) $(UW_TARGET):	$(PRODLIB)/libint.a
$(TARGET) $(UW_TARGET):	$(PRODLIB)/libdbgen.a
$(TARGET) $(UW_TARGET):	$(PRODLIB)/lib$(UI).a
$(TARGET):	../../debug_paths.o
$(UW_TARGET):	../../debug_paths.uw.o

$(OSR5_TARGET):	$(OSR5_LIB)/libint.a
$(OSR5_TARGET):	$(OSR5_LIB)/libdbgen.a
$(OSR5_TARGET):	$(OSR5_LIB)/lib$(UI).a
$(OSR5_TARGET):	../../debug_paths.osr5.o

include ../../lib$(UI)/$(CPU)/ui.libs

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	LD_RUN_PATH=$(XLIBPATH) $(CPLUS) -o $(TARGET) $(LINK_MODE) ../../debug_paths.o $(OBJECTS) $(LIBRARIES) $(LDLIBS)

$(OSR5_TARGET):	$(OSR5_OBJECTS)
	rm -f $(OSR5_TARGET)
	LD_RUN_PATH=$(XLIBPATH) $(CPLUS) -o $(OSR5_TARGET) $(LINK_MODE) ../../debug_paths.osr5.o $(OSR5_OBJECTS) $(OSR5_LIBRARIES) $(LDLIBS)

$(UW_TARGET):	$(OBJECTS)
	rm -f $(UW_TARGET)
	LD_RUN_PATH=$(XLIBPATH) $(CPLUS) -o $(UW_TARGET) $(LINK_MODE) ../../debug_paths.uw.o $(OBJECTS) $(LIBRARIES) $(LDLIBS)

osr5:	$(OSR5_TARGET)

uw:	$(UW_TARGET)

install_osr5:	$(OSR5_CCSLIB)/$(OSR5_BASENAME) $(UI)install_osr5

install:	$(CCSLIB)/$(BASENAME) $(UI)install

install_uw:	$(UW_CCSLIB)/$(UW_BASENAME) $(UI)install_uw

$(CCSLIB)/$(BASENAME):	$(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) $(CCSLIB)/$(BASENAME)

$(OSR5_CCSLIB)/$(OSR5_BASENAME):	$(OSR5_TARGET)
	$(STRIP) $(OSR5_TARGET)
	cp $(OSR5_TARGET) $(OSR5_CCSLIB)/$(BASENAME)

$(UW_CCSLIB)/$(UW_BASENAME):	$(UW_TARGET)
	$(STRIP) $(UW_TARGET)
	cp $(UW_TARGET) $(UW_CCSLIB)/$(BASENAME)

local_depend:
	cp BASEDEPEND /tmp
	sed -e 's/libol/lib$$(UI)/g' </tmp/BASEDEPEND | \
	sh ../../util/common/substdir $(XINC) '$$(XINC)' | \
	sh ../../util/common/substdir /usr/include '$$(INC)' >BASEDEPEND
	rm  -f /tmp/BASEDEPEND
