# @(#)src/cmd/vxvm/unixware/fsgen/fs/s5/s5.mk	1.1 10/16/96 02:17:19 - 
#ident	"@(#)cmd.vxvm:unixware/fsgen/fs/s5/s5.mk	1.2"

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
VXS5 = $(VXFSFS)/s5

S5_TARGETS =

S5_LINK = $(ROOT)/$(MACH)/sbin/sync
S5_LTARG = vxsync

all:

install:
	[ -d $(VXDIR) ] || mkdir -p $(VXDIR)
	[ -d $(VXTYPE) ] || mkdir -p $(VXTYPE)
	[ -d $(VXFSGEN) ] || mkdir -p $(VXFSGEN)
	[ -d $(VXFSFS) ] || mkdir -p $(VXFSFS)
	[ -d $(VXS5) ] || mkdir -p $(VXS5)
	rm -f $(VXS5)/$(S5_LTARG)
	ln -s $(S5_LINK) $(VXS5)/$(S5_LTARG)

headinstall:

lint:
	@echo "Nothing to lint in cmd/vxvm/unixware/fsgen/fs/s5"

lintclean:
	@echo "Nothing to lintclean in cmd/vxvm/unixware/fsgen/fs/s5"

clean:
	@echo "Making clean in cmd/vxvm/unixware/fsgen/fs/s5"

clobber:
	@echo "Making clobber in cmd/vxvm/unixware/fsgen/fs/s5"

