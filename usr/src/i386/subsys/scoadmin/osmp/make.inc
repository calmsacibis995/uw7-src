#--------------------------------------------------------------------------
#	@(#)make.inc	1.2
#
#       Copyright (C) The Santa Cruz Operation, 1996.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#--------------------------------------------------------------------------

include $(CMDRULES)

MSGDEFAULT=en
INSDIR=$(ROOT)/$(MACH)

MODULEIDS=../../../ModuleIds

# Building message catalogs script path
DONLS=INSDIR=$(INSDIR) MODULEIDS="$(MODULEIDS)" $(DOSMROOT)/bin/donls
