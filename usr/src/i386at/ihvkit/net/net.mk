#ident "@(#)net.mk	22.3"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)

MAKEFILE = net.mk

# target directories to place built stuff into
INSDIR = $(ROOT)/$(MACH)/usr/src/ihvkit/net
MDIINSDIR = $(INSDIR)/mdi

# driver source locations in build tree
DRVDIR = $(ROOT)/usr/src/$(WORK)/uts/io/nd
MDIDIR = $(DRVDIR)/mdi

# source for these drivers can be shipped in HDK
HDK_DLIST = e3B e3D tok

NDTEST=nd-test

all: ndtest drvsrc

drvsrc:
	[ -d $(INSDIR)/pkg ] || mkdir -p $(INSDIR)/pkg
	[ -d $(MDIINSDIR) ] || mkdir -p $(MDIINSDIR)
	cp -f README $(INSDIR)
	cp -f ndsample_mdi.mk $(MDIINSDIR)
	cp -f ndsample_depend $(INSDIR)/pkg
	cp -f ndsample_pkginfo $(INSDIR)/pkg
	for d in $(HDK_DLIST); do \
		if [ -d $(MDIDIR)/$$d ]; then \
		(cd $(MDIDIR)/$$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS); \
		 rm -f $$d.cf/Driver.o; \
		 cd $(MDIDIR); \
		 [ -d $(MDIINSDIR)/$$d ] || mkdir -p $(MDIINSDIR)/$$d; \
		 find ./$$d -type f -follow -print | cpio -pdumL $(MDIINSDIR)); \
		fi; \
	done
	echo "KTOOL = /etc/conf/bin\nSTATIC=static" > $(MDIINSDIR)/uts.rulefile
	sed -e "s/^PFX[ 	]*=/#PFX =/" \
	    -e "s/^DDEBUG[ 	]*=/#DDEBUG =/" \
	    -e "s/^MACH[ 	]*=/#MACH =/" \
	    -e "s/^DMPSTATS[ 	]*=/#DMPSTATS =/" \
	    -e "s/-D_KERNEL_HEADERS//" \
	    -e "s/-D_LOCKTEST//" \
	$(UTSRULES) >> $(MDIINSDIR)/uts.rulefile

ndtest:
	cd $(NDTEST); \
	$(MAKE) -f nd-test.mk install;

install: all

clean:
	rm -rf $(INSDIR)

clobber: clean

