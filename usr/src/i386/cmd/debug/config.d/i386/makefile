#ident	"@(#)debugger:config.d/i386/makefile	1.3"

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
	if [ ! -d $(CCSLIB)/debug_config ]; then \
		mkdir $(CCSLIB)/debug_config; \
	fi
	rm -rf $(CCSLIB)/debug_config/*
	cp $(CONFFILES) $(CCSLIB)/debug_config
	chmod 444 $(CCSLIB)/debug_config/*

install_uw:	$(CONFFILES)
	if [ ! -d $(UW_CCSLIB)/debug_config ]; then \
		mkdir $(UW_CCSLIB)/debug_config; \
	fi
	rm -rf $(UW_CCSLIB)/debug_config/*
	cp $(CONFFILES) $(UW_CCSLIB)/debug_config
	chmod 444 $(UW_CCSLIB)/debug_config/*
