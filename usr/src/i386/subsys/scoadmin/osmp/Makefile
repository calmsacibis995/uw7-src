#--------------------------------------------------------------------------
#	"@(#)Makefile	1.2"
#
#       Copyright (C) The Santa Cruz Operation, 1996.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#--------------------------------------------------------------------------

SHELL=/bin/sh

OSMPLIST=*

MODULES=ModuleIds

all install clean clobber:	modules
	for i in $(OSMPLIST); \
	do \
	   if [ -f $$i/$$i.mk ]; \
	   then \
		echo "=== make $$i $@ `date`"; \
		cd $$i; $(MAKE) -f $$i.mk $@; cd ..; \
	    fi \
	done

modules:
	-/bin/rm -f $(MODULES)
	for i in $(OSMPLIST); \
	do \
	   if [ -f $$i/NLS/$$i.mod ]; \
	   then \
		echo "--- module $$i.mod"; \
		cat $$i/NLS/$$i.mod >> $(MODULES); \
	   elif [ $$i = "osa" ]; \
	   then \
		echo "--- module $$i/NLS/*.mod"; \
		cat $$i/NLS/*.mod >> $(MODULES); \
	   fi \
	done

lintit:

