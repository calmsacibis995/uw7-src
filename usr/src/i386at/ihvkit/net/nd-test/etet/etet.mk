#ident "@(#)etet.mk  4.2"
#
# 	Copyright (C) The Santa Cruz Operation, 1996
# 	This module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as
#	Confidential
#
#include $(CMDRULES)

MAKEFILE=etet.mk

SUBDIRS= src 

all:
	@for d in $(SUBDIRS); do \
	 (cd $$d; $(MAKE) -f $$d.mk all); \
	done 
