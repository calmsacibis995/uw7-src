#ident "@(#)nd.mk	19.2"
#ident "$Header$"

#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)

MAKEFILE = nd.mk
KBASE = ../..
SUBDIRS = tools dlpi mdi mdi_wan

all clean lintit:
	@for d in $(SUBDIRS) ; do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
	 done

install: all
	@for d in $(SUBDIRS) ; do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
	 done

sysHeaders = \
	sys/dlpimod.h \
	sys/scodlpi.h \
	sys/mdi.h \
	sys/sr.h \
	sys/scoisdn.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	do \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	done

clobber: clean
	@for d in $(SUBDIRS) ; do \
		if [ -d $$d -a -f $$d/$$d.mk ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	 done
