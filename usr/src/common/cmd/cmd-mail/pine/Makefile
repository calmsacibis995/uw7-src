#ident "@(#)Makefile	11.1"

SHELL=/bin/sh

include		$(CMDRULES)

SUBDIRS		= c-client pico pine

#
# Default targets.
#
all:		build

#
# Run make in all our subdirectories.
#
build clean clobber:
	@for dir in $(SUBDIRS); \
	do \
	    if [ ! -d $$dir ]; \
	    then \
		continue; \
	    fi; \
	    ( \
		echo "------- '$(MAKE) -$(MAKEFLAGS) $@' in $${dir}"; \
		cd $$dir; \
		$(MAKE) -$(MAKEFLAGS) $@ \
	    ); \
	done

install:	build
		if [ ! -d $(BIN) ] ; then mkdir -p $(BIN) ; fi
		$(INS) -f $(BIN) pine/pine
		if [ ! -d $(ETC) ] ; then mkdir -p $(ETC) ; fi
		$(INS) -f $(ETC) mime.types

local-install:
		cp pine/pine /usr/bin
		ln /usr/bin/pine /usr/bin/pinef
		cp mime.types /etc

local-remove:
		rm -f /usr/bin/pine /usr/bin/pinef /etc/mime.types
