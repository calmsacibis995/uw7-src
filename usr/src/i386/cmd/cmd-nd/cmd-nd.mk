#ident "@(#)cmd-nd.mk	27.3"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1997
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(CMDRULES)

MAKEFILE = cmd-nd.mk
DRVDIR = $(ROOT)/usr/src/$(CPU)/uts/io/nd
NCFG_DIR = $(USRLIB)/netcfg
HOOK_DIR = $(USRLIB)/scohelp/hooks
NCFGBIN = $(NCFG_DIR)/bin
DEVINC1 = -I$(DRVDIR)
NDROOT = $(ETC)/inst/nd
NDROOTBIN = $(NDROOT)/bin
SUBDIRS = NLS intl dlpid ndstat ndcfg vendors

# OSR5 used S35dlpi but that is too late for UW.
# In UW we must start before stacks, namely S25nw for IPX and S69inet for TCP
STARTND = $(ETC)/rc2.d/S15nd
STOPND1 = $(ETC)/rc1.d/K85nd
STOPND0 = $(ETC)/rc0.d/K85nd
ETCND = $(ETC)/nd
INITD = $(ETC)/init.d

DIRS = $(ETC)/rc2.d $(ETC)/rc1.d $(ETC)/rc0.d $(INITD)

all clean lintit:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	 done

$(DIRS):
	[ -d $@ ] || mkdir -p $@

install: $(DIRS) all
	cd vendors; $(MAKE) -f vendors.mk install $(MAKEARGS)
	$(INS) -f $(INITD) -m 0555 -u $(OWN) -g $(GRP) dlpid/nd
	rm -f $(STARTND) $(STOPND1) $(STOPND0) $(ETCND)
	-ln $(INITD)/nd $(STARTND)
	-ln $(INITD)/nd $(STOPND1)
	-ln $(INITD)/nd $(STOPND0)
	-ln $(INITD)/nd $(ETCND)
	[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
	[ -d $(NCFG_DIR)/lib ] || mkdir -p $(NCFG_DIR)/lib
	[ -d $(NCFGBIN)/icons ] || mkdir -p $(NCFGBIN)/icons
	[ -d $(NDROOTBIN) ] || mkdir -p $(NDROOTBIN)
	[ -d $(NCFG_DIR)/netcfg.obj ] || mkdir -p $(NCFG_DIR)/netcfg.obj
	[ -d $(NCFG_DIR)/netcfg.obj/C ] || mkdir -p $(NCFG_DIR)/netcfg.obj/C
	[ -d $(NCFG_DIR)/netcfg.obj/en_US ] || mkdir -p $(NCFG_DIR)/netcfg.obj/en_US
	[ -d $(HOOK_DIR) ] || mkdir -p $(HOOK_DIR)

	$(INS) -f $(USRBIN)  -m 0555 -u $(OWN) -g $(GRP) ndstat/ndstat
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) dlpid/dlpid
# we put nd in /etc a few lines up since /etc is traditionally only in 
# root's path and it will conflict with the nd SCCS front-end.
# users won't be running the nd script anyway.
# the origin is really /etc/init.d.
##	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) dlpid/nd
#
	$(INS) -f $(NDROOTBIN) -m 0644 -u $(OWN) -g $(GRP) NLS/en/dlpid.msg.tcl
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) netcfg/netcfg
	$(INS) -f $(NCFGBIN) -m 0644 -u $(OWN) -g $(GRP) NLS/en/netcfg.msg.tcl
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) netcfg/ncfgprompter
	$(INS) -f $(NCFGBIN) -m 0644 -u $(OWN) -g $(GRP) NLS/en/ncfgprompter.msg.tcl
	$(INS) -f $(NCFG_DIR) -m 0644 -u $(OWN) -g $(GRP) netcfg/chains
	$(INS) -f $(NCFG_DIR)/lib -m 0555 -u $(OWN) -g $(GRP) netcfg/libSCO.tcl
	$(INS) -f $(NCFG_DIR)/lib -m 0644 -u $(OWN) -g $(GRP) NLS/en/libSCO.msg.tcl
	$(INS) -f $(NCFGBIN) -m 0555 -u $(OWN) -g $(GRP) netcfg/ncfgBE
	$(INS) -f $(NCFGBIN) -m 0644 -u $(OWN) -g $(GRP) NLS/en/ncfgBE.msg.tcl
	$(INS) -f $(NCFGBIN) -m 0555 -u $(OWN) -g $(GRP) netcfg/ncfgUI
	$(INS) -f $(NCFGBIN) -m 0644 -u $(OWN) -g $(GRP) NLS/en/ncfgUI.msg.tcl
	$(INS) -f $(NCFGBIN) -m 0555 -u $(OWN) -g $(GRP) ndcfg/ndcfg
	$(INS) -f $(NCFGBIN) -m 0555 -u $(OWN) -g $(GRP) ndcfg/ndcleanup.sh
	$(INS) -f $(NCFGBIN) -m 0555 -u $(OWN) -g $(GRP) netcfg/addNETCFGrole

	$(INS) -f $(NCFG_DIR)/netcfg.obj/C -m 0644 -u $(OWN) -g $(GRP) NLS/en/title
	$(INS) -f $(NCFG_DIR)/netcfg.obj/en_US -m 0644 -u $(OWN) -g $(GRP) NLS/en/title
	$(INS) -f $(NCFG_DIR)/netcfg.obj -m 0555 -u $(OWN) -g $(GRP) netcfg/activate.scoadmin
	$(INS) -f $(HOOK_DIR) -m 0644 -u $(OWN) -g $(GRP) netcfg/netcfgGUI.hk

	@for f in netcfg/icons/*; do \
		($(INS) -f $(NCFGBIN)/icons -m 0444 -u $(OWN) -g $(GRP) $$f) \
	done

clobber: clean
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	 done
