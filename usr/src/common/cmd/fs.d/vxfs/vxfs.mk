# @(#)src/uw/gemini/cmd/vxfs/vxfs.mk	3.9 09/12/97 15:52:14 - 
#ident	"@(#)vxfs:src/uw/gemini/cmd/vxfs/vxfs.mk	3.9"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/vxfs.mk	1.1.2.1"
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
#	       RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#	       VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043

# Make all the subdirectories and install

include $(CMDRULES)

LARGEFILES = -D_FILE_OFFSET_BITS=64
CFLAGS = $(OPTCFLAG)

DIRS = libvxfs \
	ckroot \
	df \
	ff \
	fsadm \
	fscat \
	fsck \
	fsdb \
	fstyp \
	getext \
	labelit \
	mkfs \
	mount \
	ncheck \
	setext \
	volcopy \
	vxconvert \
	vxdiskusg \
	vxdump \
	vxedquota \
	vxquot \
	vxquota \
	vxquotaon \
	vxrepquota \
	vxrestore \
	vxtunefs \
	vxupgrade \
	vxzip

#
#  This is to build all the vxfs commands
#
.DEFAULT:
	for i in $(DIRS);\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f $$i.mk "LARGEFILES=$(LARGEFILES)" $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

