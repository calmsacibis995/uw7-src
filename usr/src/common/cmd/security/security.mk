#******************************************************************************
#	Makefile
#------------------------------------------------------------------------------
# Comments:
# security profile utilities Makefile
#
#------------------------------------------------------------------------------
#       @(#)Makefile	7.1	97/08/30
# 
#       Copyright (C) The Santa Cruz Operation, 1996.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#------------------------------------------------------------------------------
#  Revision History:
#
#	Mon Dec 23 17:16:36 PST 1996	louisi
#		Created file.
#	Tue Sep  9 11:50:17 BST 1997	andrewma
#		Rewritten.
#
#================================================================================

include $(CMDRULES)

DIRS = utils/relax utils/secdefs

all clean :
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		cd $$i ;\
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ../.. ; \
	done

install :  all
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		cd $$i ;\
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ../.. ; \
	done
