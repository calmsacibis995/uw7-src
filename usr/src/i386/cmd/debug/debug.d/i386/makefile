#ident	"@(#)debugger:debug.d/i386/makefile	1.11"

include $(CMDRULES)

BASENAME = debug

OSR5_BASENAME = debug.osr5

UW_BASENAME = debug.uw

TARGET = ../../$(BASENAME)

OSR5_TARGET = ../../$(OSR5_BASENAME)

UW_TARGET = ../../$(UW_BASENAME)

include ../../util/common/std.make
include ../../util/common/defs.make
include ../../util/i386/$(MACHDEFS)

CCSOURCES = \
	$(COMMON)/docommands.C \
	$(COMMON)/do_assoc.C \
	$(COMMON)/main.C \
	$(COMMON)/handlers.C

CSOURCES = \
	$(COMMON)/sig_handle.c

$(TARGET) $(UW_TARGET):	$(PRODLIB)/libcmd.a    $(PRODLIB)/libint.a
$(TARGET) $(UW_TARGET):	$(PRODLIB)/libutil.a   $(PRODLIB)/libexecon.a
$(TARGET) $(UW_TARGET):	$(PRODLIB)/libexp.a    $(PRODLIB)/libmachine.a
$(TARGET) $(UW_TARGET):	$(PRODLIB)/libsymbol.a
$(TARGET) $(UW_TARGET):	$(PRODLIB)/libdbgen.a 
$(TARGET) $(UW_TARGET):	$(PRODLIB)/libedit.a
$(TARGET):	../../debug_paths.o
$(UW_TARGET):	../../debug_paths.uw.o

$(OSR5_TARGET):	$(OSR5_LIB)/libcmd.a   
$(OSR5_TARGET):	$(OSR5_LIB)/libint.a   
$(OSR5_TARGET):	$(OSR5_LIB)/libutil.a
$(OSR5_TARGET):	$(OSR5_LIB)/libexecon.a
$(OSR5_TARGET):	$(OSR5_LIB)/libexp.a
$(OSR5_TARGET):	$(OSR5_LIB)/libmachine.a
$(OSR5_TARGET):	$(OSR5_LIB)/libsymbol.a
$(OSR5_TARGET):	$(OSR5_LIB)/libdbgen.a 
$(OSR5_TARGET):	$(OSR5_LIB)/libedit.a
$(OSR5_TARGET):	../../debug_paths.osr5.o

LIBRARIES = \
	$(PRODLIB)/libcmd.a \
	$(PRODLIB)/libexecon.a \
	$(PRODLIB)/libedit.a \
	$(PRODLIB)/libexp.a \
	$(PRODLIB)/libutil.a \
	$(PRODLIB)/libmachine.a \
	$(PRODLIB)/libsymbol.a \
	$(PRODLIB)/libint.a \
	$(PRODLIB)/libdbgen.a \
	$(SGSBASE)/cplusfe/i386/fe.so \
	$(SGSBASE)/libdwarf2/i386/libdwarf2.a \
	-lelf \
	-lgen

OSR5_LIBRARIES = \
	$(OSR5_LIB)/libcmd.a \
	$(OSR5_LIB)/libexecon.a \
	$(OSR5_LIB)/libedit.a \
	$(OSR5_LIB)/libexp.a \
	$(OSR5_LIB)/libutil.a \
	$(OSR5_LIB)/libmachine.a \
	$(OSR5_LIB)/libsymbol.a \
	$(OSR5_LIB)/libint.a \
	$(OSR5_LIB)/libdbgen.a \
	$(SGSBASE)/cplusfe/i386/fe.so \
	$(SGSBASE)/libdwarf2/i386/libdwarf2.a \
	-lelf \
	-lgen

include DEPEND

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	LD_RUN_PATH=/usr/ccs/lib $(CPLUS) -o $(TARGET) $(LINK_MODE) ../../debug_paths.o $(OBJECTS) $(LIBRARIES) $(THREADS_LIBS) $(LDLIBS)

$(OSR5_TARGET):	$(OSR5_OBJECTS)
	rm -f $(OSR5_TARGET)
	LD_RUN_PATH=$(ALT_PREFIX)/usr/ccs/lib $(CPLUS) -o $(OSR5_TARGET) $(LINK_MODE) ../../debug_paths.osr5.o $(OSR5_OBJECTS) $(OSR5_LIBRARIES) $(THREADS_LIBS) $(LDLIBS)

$(UW_TARGET):	$(OBJECTS)
	rm -f $(UW_TARGET)
	LD_RUN_PATH=$(ALT_PREFIX)/usr/ccs/lib $(CPLUS) -o $(UW_TARGET) $(LINK_MODE) ../../debug_paths.uw.o $(OBJECTS) $(LIBRARIES) $(THREADS_LIBS) $(LDLIBS)

osr5:	$(OSR5_TARGET)

uw:	$(UW_TARGET)

install:	$(CCSBIN)/$(BASENAME)

install_osr5:	$(OSR5_CCSBIN)/$(OSR5_BASENAME)

install_uw:	$(UW_CCSBIN)/$(UW_BASENAME)

$(CCSBIN)/$(BASENAME):	$(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) $(CCSBIN)/$(BASENAME)

$(OSR5_CCSBIN)/$(OSR5_BASENAME):	$(OSR5_TARGET)
	$(STRIP) $(OSR5_TARGET)
	cp $(OSR5_TARGET) $(OSR5_CCSBIN)/$(BASENAME)

$(UW_CCSBIN)/$(UW_BASENAME):	$(UW_TARGET)
	$(STRIP) $(UW_TARGET)
	cp $(UW_TARGET) $(UW_CCSBIN)/$(BASENAME)
