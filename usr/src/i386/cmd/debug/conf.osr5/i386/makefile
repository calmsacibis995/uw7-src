#ident	"@(#)debugger:conf.osr5/i386/makefile	1.2"

include $(CMDRULES)

include ../../util/common/std.make

CONFFILES= \
	../common/Command \
	../common/Disassembly \
	../common/Event \
	../common/Process \
	../common/Source \
	../common/Source-new \
	../common/Symbols

all:	$(CONFFILES)

install:	$(CONFFILES)
	if [ ! -d $(OSR5_CCSLIB)/debug_config ]; then \
		mkdir $(OSR5_CCSLIB)/debug_config; \
	fi
	rm -rf $(OSR5_CCSLIB)/debug_config/*
	cp $(CONFFILES) $(OSR5_CCSLIB)/debug_config
	chmod 444 $(OSR5_CCSLIB)/debug_config/*
