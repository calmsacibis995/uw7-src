#-----------------------------------------------------------------------------
#
#       Copyright (C) The Santa Cruz Operation, 1996-1997.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#---------------------------------------------------------------------------
# @(#)Makefile	7.1	97/10/22
#
# Revision History:
# 1997-Jan-06, shawnm, created
#===========================================================================

CURDIR = audio/drivers/audioconfig
SUBDIRS = audinfo src

all clean clobber install:
	@case '${MFLAGS}' in *[ik]*) set +e;; esac; \
	for i in $(SUBDIRS) ;\
	do \
	(cd $$i ; echo "making" $@ "in $(CURDIR)/$$i..."; \
	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' $@); \
	done

