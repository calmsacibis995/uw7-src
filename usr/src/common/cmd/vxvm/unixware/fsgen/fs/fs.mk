# @(#)src/cmd/vxvm/unixware/fsgen/fs/fs.mk	1.1 10/16/96 02:17:18 - 
#ident	"@(#)cmd.vxvm:unixware/fsgen/fs/fs.mk	1.3"

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

SUBDIRS = s5 \
	ufs \
	vxfs

all:
	@echo "Making all in cmd/vxvm/unixware/fsgen/fs"
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		$(MAKE) -f $$d.mk LIBVXVM="$(LIBVXVM)" \
			LOCAL="$(LOCAL)" $@ ); \
	done

lint:
	@echo "Making lint in cmd/vxvm/unixware/fsgen/fs"
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		echo "\n\t\tMaking lint in $$d\n" ; \
		$(MAKE) -f $$d.mk LIBVXVM="$(LIBVXVM)" \
			LOCAL="$(LOCAL)" $@ ); \
	done

lintclean:
	@echo "Making lintclean in cmd/vxvm/unixware/fsgen/fs"
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		echo "\n\t\tMaking linclean in $$d\n" ; \
		$(MAKE) -f $$d.mk $@ ); \
	done

headinstall:
	@echo "Making headinstall in cmd/vxvm/unixware/fsgen/fs"
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		$(MAKE) -f $$d.mk $@ ); \
	done

install:
	@echo "Making install in cmd/vxvm/unixware/fsgen/fs"
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		$(MAKE) -f $$d.mk LIBVXVM="$(LIBVXVM)" \
			LOCAL="$(LOCAL)" $@ ); \
	done

clean:
	@echo "Making clean in cmd/vxvm/unixware/fsgen/fs"
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		$(MAKE) -f $$d.mk $@ ); \
	done

clobber:
	@echo "Making clobber in cmd/vxvm/unixware/fsgen/fs"
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		$(MAKE) -f $$d.mk $@ ); \
	done
