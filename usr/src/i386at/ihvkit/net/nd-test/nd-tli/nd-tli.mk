#ident "@(#)nd-tli.mk  4.2"
#
# 	Copyright (C) The Santa Cruz Operation, 1996
# 	This module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as
#	Confidential
#

include $(CMDRULES)

all:
	cd  src; $(MAKE) -f src.mk $@

