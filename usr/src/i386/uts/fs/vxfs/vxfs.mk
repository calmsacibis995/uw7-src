# @(#)src/uw/gemini/kernel/base/vxfs.mk	3.26.9.1 12/01/97 20:03:08 - 
#ident "@(#)vxfs:src/uw/gemini/kernel/base/vxfs.mk	3.26.9.1 (edited)"
#ident	"@(#)kern-i386:fs/vxfs/vxfs.mk	1.1.9.1"
#
# Copyright (c) 1997 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
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
#		RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#		VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
#

include $(UTSRULES)

KBASE	 = ../..
VXBASE	 = ../..
#TED	 = -DTED_
LOCALDEF = -D_FSKI=2 $(TED)
LOCALINC = -I $(VXBASE)

VXFS = vxfs.cf/Driver.o
MODULES = $(VXFS)

LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/vxfs.ln

PROBEFILE = vx_aioctl.c
HEADPROBEFILE = vx_aioctl.h
MAKEFILE = vxfs.mk
BINARIES = $(VXFS)

#TED_CFILE = vx_ted.c vx_tedlocal.c vx_tioctl.c
#TED_OBJ   = vx_ted.o vx_tedlocal.o vx_tioctl.o
#TED_LN	  = vx_ted.ln vx_tedlocal.ln vx_tioctl.ln

CFILES = \
	kdm_core.c  \
	kdm_machdep.c  \
	vx_acl.c  \
	vx_aioctl.c  \
	vx_alloc.c  \
	vx_attr.c  \
	vx_ausum.c  \
	vx_bio.c  \
	vx_bitmaps.c  \
	vx_bmap.c  \
	vx_bmapext4.c  \
	vx_bmaptops.c  \
	vx_bmaptyped.c  \
	vx_bsdqsubr.c  \
	vx_bsdquota.c  \
	vx_config.c  \
	vx_cut.c  \
	vx_dio.c  \
	vx_dira.c  \
	vx_dirl.c  \
	vx_dirop.c  \
	vx_dirsort.c  \
	vx_dmattr.c  \
	vx_doextop.c  \
	vx_freeze.c  \
	vx_fset.c  \
	vx_fsetsubr.c  \
	vx_full.c  \
	vx_gemini.c  \
	vx_getpage.c  \
	vx_hsm_vnops.c  \
	vx_ialloc.c  \
	vx_iclone.c  \
	vx_iflush.c  \
	vx_inode.c  \
	vx_itrunc.c  \
	vx_kdmi.c  \
	vx_kdmi_machdep.c  \
	vx_kernrdwri.c  \
	vx_lct.c  \
	vx_lite.c  \
	vx_log.c  \
	vx_lwrite.c  \
	vx_machdep.c  \
	vx_message.c  \
	vx_mount.c  \
	vx_namespace.c  \
	vx_oltmount.c  \
	vx_putpage.c  \
	vx_quota.c  \
	vx_read.c  \
	vx_reorg.c  \
	vx_resize.c  \
	vx_rwlib.c  \
	vx_sar.c  \
	vx_snap.c  \
	vx_strategy.c  \
	vx_swap.c  \
	vx_tran.c  \
	vx_tune.c  \
	vx_upgrade1.c  \
	vx_upgrade2.c  \
	vx_uioctl.c  \
	vx_vfsops.c  \
	vx_vnops.c  \
	vx_write.c  \
	$(TED_CFILE)

LFILES = \
	kdm_core.ln  \
	kdm_machdep.ln  \
	vx_acl.ln  \
	vx_aioctl.ln  \
	vx_alloc.ln  \
	vx_attr.ln  \
	vx_ausum.ln  \
	vx_bio.ln  \
	vx_bitmaps.ln  \
	vx_bmap.ln  \
	vx_bmapext4.ln  \
	vx_bmaptops.ln  \
	vx_bmaptyped.ln  \
	vx_bsdqsubr.ln  \
	vx_bsdquota.ln  \
	vx_config.ln  \
	vx_cut.ln  \
	vx_dio.ln  \
	vx_dira.ln  \
	vx_dirl.ln  \
	vx_dirop.ln  \
	vx_dirsort.ln  \
	vx_dmattr.ln  \
	vx_doextop.ln  \
	vx_freeze.ln  \
	vx_fset.ln  \
	vx_fsetsubr.ln  \
	vx_full.ln  \
	vx_gemini.ln  \
	vx_getpage.ln  \
	vx_hsm_vnops.ln  \
	vx_ialloc.ln  \
	vx_iclone.ln  \
	vx_iflush.ln  \
	vx_inode.ln  \
	vx_itrunc.ln  \
	vx_kdmi.ln  \
	vx_kdmi_machdep.ln  \
	vx_kernrdwri.ln  \
	vx_lct.ln  \
	vx_lite.ln  \
	vx_log.ln  \
	vx_lwrite.ln  \
	vx_machdep.ln  \
	vx_message.ln  \
	vx_mount.ln  \
	vx_namespace.ln  \
	vx_oltmount.ln  \
	vx_putpage.ln  \
	vx_quota.ln  \
	vx_read.ln  \
	vx_reorg.ln  \
	vx_resize.ln  \
	vx_rwlib.ln  \
	vx_sar.ln  \
	vx_snap.ln  \
	vx_strategy.ln  \
	vx_swap.ln  \
	vx_tran.ln  \
	vx_tune.ln  \
	vx_upgrade1.ln  \
	vx_upgrade2.ln  \
	vx_uioctl.ln  \
	vx_vfsops.ln  \
	vx_vnops.ln  \
	vx_write.ln  \
	$(TED_LN)

VXFS_OBJS =  \
	kdm_core.o  \
	kdm_machdep.o  \
	vx_acl.o  \
	vx_aioctl.o  \
	vx_alloc.o  \
	vx_attr.o  \
	vx_ausum.o  \
	vx_bio.o  \
	vx_bitmaps.o  \
	vx_bmap.o  \
	vx_bmapext4.o  \
	vx_bmaptops.o  \
	vx_bmaptyped.o  \
	vx_bsdqsubr.o  \
	vx_bsdquota.o  \
	vx_config.o  \
	vx_cut.o  \
	vx_dio.o  \
	vx_dira.o  \
	vx_dirl.o  \
	vx_dirop.o  \
	vx_dirsort.o  \
	vx_dmattr.o  \
	vx_doextop.o  \
	vx_freeze.o  \
	vx_fset.o  \
	vx_fsetsubr.o  \
	vx_full.o  \
	vx_gemini.o  \
	vx_getpage.o  \
	vx_hsm_vnops.o  \
	vx_ialloc.o  \
	vx_iclone.o  \
	vx_iflush.o  \
	vx_inode.o  \
	vx_itrunc.o  \
	vx_kdmi.o  \
	vx_kdmi_machdep.o  \
	vx_kernrdwri.o  \
	vx_lct.o  \
	vx_lite.o  \
	vx_log.o  \
	vx_lwrite.o  \
	vx_machdep.o  \
	vx_message.o  \
	vx_mount.o  \
	vx_namespace.o  \
	vx_oltmount.o  \
	vx_putpage.o  \
	vx_quota.o  \
	vx_read.o  \
	vx_reorg.o  \
	vx_resize.o  \
	vx_rwlib.o  \
	vx_sar.o  \
	vx_snap.o  \
	vx_strategy.o  \
	vx_swap.o  \
	vx_tran.o  \
	vx_tune.o  \
	vx_upgrade1.o  \
	vx_upgrade2.o  \
	vx_uioctl.o  \
	vx_vfsops.o  \
	vx_vnops.o  \
	vx_write.o  \
	$(TED_OBJ)

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

clean:
	-rm -f $(VXFS_OBJS) $(LFILES) *.L

clobber: clean
	-$(IDINSTALL) -e -R$(CONF) -d vxfs
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(LINTDIR):
	[ -d $@ ] || mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES);	\
	do \
		echo $$i; \
	done

#
# Configuration Section
#

install: all
	(cd vxfs.cf; $(IDINSTALL) -R$(CONF) -M vxfs)

VXFS_HEADBIN = \
	$(VXBASE)/fs/vxfs/dmapi.h  \
	$(VXBASE)/fs/vxfs/dmapi_size.h  \
	$(VXBASE)/fs/vxfs/kdm_dmi.h  \
	$(VXBASE)/fs/vxfs/kdm_machdep.h  \
	$(VXBASE)/fs/vxfs/kdm_vnode.h  \
	$(VXBASE)/fs/vxfs/vx_aioctl.h  \
	$(VXBASE)/fs/vxfs/vx_const.h  \
	$(VXBASE)/fs/vxfs/vx_disklog.h  \
	$(VXBASE)/fs/vxfs/vx_ext.h  \
	$(VXBASE)/fs/vxfs/vx_gemini.h  \
	$(VXBASE)/fs/vxfs/vx_ioctl.h  \
	$(VXBASE)/fs/vxfs/vx_inode.h  \
	$(VXBASE)/fs/vxfs/vx_layout.h  \
	$(VXBASE)/fs/vxfs/vx_license.h  \
	$(VXBASE)/fs/vxfs/vx_machdep.h  \
	$(VXBASE)/fs/vxfs/vx_mlink.h  \
	$(VXBASE)/fs/vxfs/vx_param.h  \
	$(VXBASE)/fs/vxfs/vx_quota.h  \
	$(VXBASE)/fs/vxfs/vx_sysmacros.h  \
	$(VXBASE)/fs/vxfs/vxio.h  \
	$(VXBASE)/fs/vxfs/vxld.h

VXFS_HEADSRC = \
	$(VXBASE)/fs/vxfs/dmapi.h  \
	$(VXBASE)/fs/vxfs/dmapi_size.h  \
	$(VXBASE)/fs/vxfs/kdm_vnode.h \
	$(VXBASE)/fs/vxfs/vx_const.h  \
	$(VXBASE)/fs/vxfs/vx_disklog.h  \
	$(VXBASE)/fs/vxfs/vx_ext.h  \
	$(VXBASE)/fs/vxfs/vx_gemini.h  \
	$(VXBASE)/fs/vxfs/vx_ioctl.h  \
	$(VXBASE)/fs/vxfs/vx_inode.h  \
	$(VXBASE)/fs/vxfs/vx_layout.h  \
	$(VXBASE)/fs/vxfs/vx_machdep.h  \
	$(VXBASE)/fs/vxfs/vx_mlink.h  \
	$(VXBASE)/fs/vxfs/vx_param.h  \
	$(VXBASE)/fs/vxfs/vx_quota.h  \
	$(VXBASE)/fs/vxfs/vx_sysmacros.h  \
	$(VXBASE)/fs/vxfs/vxio.h  \
	$(VXBASE)/fs/vxfs/vxld.h

headinstall:
	@if [ -f $(HEADPROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) binheadinstall $(MAKEARGS) \
			"KBASE=$(KBASE)" "VXBASE=$(VXBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		$(MAKE) -f $(MAKEFILE) srcheadinstall $(MAKEARGS) \
			"KBASE=$(KBASE)" "VXBASE=$(VXBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	fi

binheadinstall: $(VXFS_HEADBIN)
	[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	[ -d t ] || mkdir t
	for fl in $(VXFS_HEADBIN) ; do \
		bfl=`basename $$fl` ;\
		rm -f t/$$bfl ;\
		grep -v TED_ $$fl > t/$$bfl ;\
		$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) t/$$bfl ;\
	done
	rm -fr t

srcheadinstall: $(VXFS_HEADSRC)
	[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	[ -d t ] || mkdir t
	for fl in $(VXFS_HEADSRC) ; do \
		bfl=`basename $$fl` ;\
		rm -f t/$$bfl ;\
		grep -v TED_ $$fl > t/$$bfl ;\
		$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) t/$$bfl ;\
	done
	rm -fr t

binaries: $(BINARIES)

$(BINARIES): $(VXFS_OBJS)
	$(LD) -r -o $@ $(VXFS_OBJS)

include $(UTSDEPEND)

include $(MAKEFILE).dep
