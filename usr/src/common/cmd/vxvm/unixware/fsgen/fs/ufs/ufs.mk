# @(#)src/cmd/vxvm/unixware/fsgen/fs/ufs/ufs.mk	1.1 10/16/96 02:17:20 - 
#ident	"@(#)cmd.vxvm:unixware/fsgen/fs/ufs/ufs.mk	1.2"

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

include $(CMDRULES)

LOCALINC = ../../../../common/libcmd
VXDIR = $(ROOT)/$(MACH)/usr/lib/vxvm
VXTYPE = $(VXDIR)/type
VXFSGEN = $(VXTYPE)/fsgen
VXFSFS = $(VXFSGEN)/fs.d
VXUFS = $(VXFSFS)/ufs

UFS_TARGETS = vxresize

UFS_LINK = $(ROOT)/$(MACH)/sbin/sync
UFS_LTARG = vxsync

.sh:
	cat $< > $@; chmod +x $@

all:	vxresize

install:
	[ -d $(VXDIR) ] || mkdir -p $(VXDIR)
	[ -d $(VXTYPE) ] || mkdir -p $(VXTYPE)
	[ -d $(VXFSGEN) ] || mkdir -p $(VXFSGEN)
	[ -d $(VXFSFS) ] || mkdir -p $(VXFSFS)
	[ -d $(VXUFS) ] || mkdir -p $(VXUFS)
	rm -f $(VXUFS)/$(UFS_LTARG)
	ln -s $(UFS_LINK) $(VXUFS)/$(UFS_LTARG)
	$(INS) -f $(VXUFS) -m 0755 -u root -g sys vxresize

lint:
	@echo "Nothing to lint in cmd/vxvm/unixware/fsgen/fs/ufs"

lintclean:
	@echo "Nothing to lintclean in cmd/vxvm/unixware/fsgen/fs/ufs"

clean:
	@echo "Making clean in cmd/vxvm/unixware/fsgen/fs/ufs"

clobber:
	@echo "Making clobber in cmd/vxvm/unixware/fsgen/fs/ufs"
	rm -f $(UFS_TARGETS)

headinstall:
	
vxresize:
	@if [ -f volresize.sh ] ; then \
		cat volresize.sh > $@; \
		chmod +x $@; \
	fi

