#ident "@(#)nd-test.mk  4.2"
#
# 	Copyright (C) The Santa Cruz Operation, 1996
# 	This module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as
#	Confidential
#

include $(CMDRULES)

MAKEFILE=nd-test.mk

SUITE_ROOT=$(ROOT)/usr/src/$(WORK)/ihvkit/net/nd-test
TET_ROOT=$(SUITE_ROOT)/etet

TEST=test
TESTSUBINS=test_sub_ins
BASIX=basix
BASIXINS=basix_ins
BASIXDINS=basix_dir_ins
BASIXSRCINS=basix_src_ins
TLIINS=tli_ins
DLPIINS=dlpi_ins
DLPIDINS=dlpi_dir_ins
SRCINS=src_ins
DIRINS=dir_ins

# target directories to place built files into
INSDIR=$(ROOT)/$(MACH)/usr/src/ihvkit/nd-test
#directories to be built
SUBDIRS= etet nd-basix nd-dlpi nd-tli nd-util

# directories where files needs to be copied from

BASIX= nd-basix/cmd.list nd-basix/common.profile nd-basix/nd-basix.cfg nd-basix/ndcert.xpm nd-basix/dtfclass

BASIX_DIRS= nd-basix/bin nd-basix/lib nd-basix/inc

BASIX_SRC_DIRS= nd-basix/src/dlpiut nd-basix/src/guic nd-basix/src/ndsu 

BASIX_SRC_D= nd-basix/src/dlpiut 

DLPI= nd-dlpi/assertions nd-dlpi/build_scen nd-dlpi/clean_scen nd-dlpi/dlpi.mkf nd-dlpi/dlpi_profile nd-dlpi/exec_scen nd-dlpi/param.list nd-dlpi/tetexec.cfg nd-dlpi/tet_scen

TLI= nd-tli/tli_profile nd-tli/tetexec.spx nd-tli/param.list nd-tli/tetexec.tcp nd-tli/tet_scen

DLPI_DIRS= nd-dlpi/automate nd-dlpi/bin nd-dlpi/inc

DLPI_H= nd-dlpi/lib nd-dlpi/src/listen nd-dlpi/src/tc_addr nd-dlpi/src/tc_frame

SRC_DIR= nd-tli/src nd-dlpi/src/tc_attach nd-dlpi/src/tc_detach nd-dlpi/src/tc_info nd-dlpi/src/tc_bind nd-dlpi/src/tc_close nd-dlpi/src/tc_ioc nd-dlpi/src/tc_subsbind nd-dlpi/src/tc_open nd-dlpi/src/tc_promisc nd-dlpi/src/tc_snd nd-dlpi/src/tc_stress nd-dlpi/src/tc_subsunbind nd-dlpi/src/tc_test nd-dlpi/src/tc_unbind  nd-dlpi/src/tc_multi nd-dlpi/src/tc_xid  

TEST_SUBDIRS= doc nd-dlpmdi nd-drvr nd-isdn nd-mdi nd-ncard nd-nfs nd-util

all: $(TEST) $(DIRINS) 

$(TEST):
	@echo "Processing all packages ....  "

	for d in $(SUBDIRS); do \
		(cd $$d; $(MAKE) SUITE_ROOT=$(SUITE_ROOT) TET_ROOT=$(TET_ROOT) -f $$d.mk ); \
	done 

$(DIRINS):
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)

$(TESTSUBINS):
	for d in $(TEST_SUBDIRS); do \
		$(CP) -rf $$d $(INSDIR); \
	done

$(BASIXDINS):
	for d in $(BASIX_DIRS); do \
		[ -d $(INSDIR)/$$d ] || mkdir -p $(INSDIR)/$$d; \
		$(CP) -rf $$d $(INSDIR)/nd-basix; \
	done

$(BASIXSRCINS):
	for d in $(BASIX_SRC_DIRS); do \
		[ -d $(INSDIR)/$$d ] || mkdir -p $(INSDIR)/$$d; \
		$(CP) -f $$d/*.c $(INSDIR)/$$d; \
	done
	for d in $(BASIX_SRC_D); do \
		[ -d $(INSDIR)/$$d ] || mkdir -p $(INSDIR)/$$d; \
		$(CP) -f $$d/*.h $(INSDIR)/$$d; \
	done

$(BASIXINS):
	for f in $(BASIX); do \
		$(CP) -f $$f $(INSDIR)/$$f; \
	done

$(SRCINS):
	for d in $(SRC_DIR); do \
		[ -d $(INSDIR)/$$d ] || mkdir -p $(INSDIR)/$$d; \
		($(CP) -f $$d/*.c $(INSDIR)/$$d; \
		$(CP) -f $$d/*.h $(INSDIR)/$$d); \
	done
	for d in $(DLPI_H); do \
		[ -d $(INSDIR)/$$d ] || mkdir -p $(INSDIR)/$$d; \
		$(CP) -f $$d/*.c $(INSDIR)/$$d; \
	done

$(TLIINS):
	for f in $(TLI); do \
		[ -d $(INSDIR)/nd-tli ] || mkdir -p $(INSDIR)/nd-tli; \
		$(CP) -f $$f $(INSDIR)/$$f; \
	done

	[ -d $(INSDIR)/nd-tli/bin ] || mkdir -p $(INSDIR)/nd-tli/bin; \
	$(CP) -rf nd-tli/bin $(INSDIR)/nd-tli; 

$(DLPIINS):
	for f in $(DLPI); do \
		[ -d $(INSDIR)/nd-dlpi ] || mkdir -p $(INSDIR)/nd-dlpi; \
		$(CP) -f $$f $(INSDIR)/$$f; \
	done

$(DLPIDINS):
	for d in $(DLPI_DIRS); do \
		$(CP) -rf $$d $(INSDIR)/$$d; \
	done

install: all $(TESTSUBINS) $(BASIXDINS) $(BASIXSRCINS) $(BASIXINS) $(SRCINS) $(TLIINS) $(DLPIINS) $(DLPIDINS) 

clean:
	rm -rf $(INSDIR)

clobber: clean
