#ident "@(#)nd-test.mk  4.2"
#
# 	Copyright (C) The Santa Cruz Operation, 1996
# 	This module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as
#	Confidential
#
include $(CMDRULES)

SUBDIRS= listen tc_addr tc_attach tc_bind tc_close tc_detach tc_frame tc_info tc_info tc_ioc tc_multi tc_open tc_promisc tc_snd tc_stress tc_subsbind tc_subsunbind tc_test tc_unbind tc_xid


all :
	for d in $(SUBDIRS); do \
	 (cd $$d; $(MAKE) -f $$d.mk $@); \
	 done
	
