#ident "@(#)vendors.mk	29.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1997
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(CMDRULES)

MAKEFILE = vendors.mk
SUBDIRS = cet

all clean lintit:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	 done

install:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		$(MAKE) -f $$d.mk install $(MAKEARGS)); \
		fi; \
	done

clobber: clean
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		$(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	done
