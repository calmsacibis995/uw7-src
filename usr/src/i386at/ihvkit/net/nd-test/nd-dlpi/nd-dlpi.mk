#ident "@(#)nd-dlpi.mk  4.2"
#ident "$Header$"
#
# 	Copyright (C) The Santa Cruz Operation, 1996
# 	This module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as
#	Confidential
#
include $(CMDRULES)


all :
	cd lib; $(MAKE) -f lib.mk $@
	cd automate; $(MAKE) -f automate.mk $@
	cd src; $(MAKE) -f src.mk $@
		
CLOBBER:
	rm -f bin/*	
	cd lib; $(MAKE) -f lib.mk $@
	
