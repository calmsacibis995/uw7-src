# @(#)src/cmd/vxvm/common/voladm/voladm.mk	1.1 10/16/96 02:23:05 - 
#ident	"@(#)cmd.vxvm:common/voladm/voladm.mk	1.1"

# Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043

COM_USRSBINTARGETS = $(COMMON_DIR)/vxdiskadm \
	vxdiskadd \
	vxinstall

COM_BINTARGETS = $(COMMON_DIR)/disk.ckinit \
	$(COMMON_DIR)/disk.deport \
	$(COMMON_DIR)/disk.doinit \
	$(COMMON_DIR)/disk.encap \
	$(COMMON_DIR)/disk.import \
	$(COMMON_DIR)/disk.init \
	$(COMMON_DIR)/disk.k-rm \
	$(COMMON_DIR)/disk.hot-on \
	$(COMMON_DIR)/disk.hot-off \
	$(COMMON_DIR)/disk.list \
	$(COMMON_DIR)/disk.menu \
	$(COMMON_DIR)/disk.mirror \
	$(COMMON_DIR)/disk.offline \
	$(COMMON_DIR)/disk.online \
	$(COMMON_DIR)/disk.repl \
	$(COMMON_DIR)/disk.rm \
	$(COMMON_DIR)/disk.vmove \
	$(COMMON_DIR)/inst.allcap \
	$(COMMON_DIR)/inst.custom \
	$(COMMON_DIR)/inst.top \
	$(COMMON_DIR)/inst.one \
	$(COMMON_DIR)/inst.quick \
	$(COMMON_DIR)/inst.allinit

COM_LIBTARGETS = $(COMMON_DIR)/vxadm_lib.sh \
	vxadm_syslib.sh

COM_HELPFILES = disk.deport.help \
	disk.encap.help \
	disk.import.help \
	disk.init.help \
	disk.k-rm.help \
	disk.hot-on.help \
	disk.hot-off.help \
	disk.hotadd.help \
	disk.nameadd.help \
	disk.list.help \
	disk.menu.help \
	disk.mirror.help \
	disk.offline.help \
	disk.online.help \
	disk.repl.help \
	disk.rm.help \
	disk.vmove.help \
	inst.boot.help \
	inst.inst.help \
	inst.pend.help \
	inst.shut.help \
	inst.top.help \
	inst.custom.help \
	inst.dmname.help \
	inst.one.help \
	inst.quick.help \
	yorn_batch_elem.help \
	yorn_batch_list.help \
	yorn_batch_single.help \
	vxadm.info \
	yorn.help

COM_OBJS =
