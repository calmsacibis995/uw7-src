#-----------------------------------------------------------------------------
#
#       Copyright (C) The Santa Cruz Operation, 1996-1997.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#---------------------------------------------------------------------------
# @(#)Makefile	6.1	97/09/05
#
# Revision History:
# 1997-Jan-06, shawnm, created
#===========================================================================

CURDIR = audio/drivers
SUBDIRS=audioconfig oss371

all clean clobber install:
	@case '${MFLAGS}' in *[ik]*) set +e;; esac; \
	for i in $(SUBDIRS) ;\
	do \
	(cd $$i ; echo "making" $@ "in $(CURDIR)/$$i..."; \
	$(MAKE) -f Makefile $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' $@); \
	done

