#ident "@(#)mdi.mk	28.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1997
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
# note we still must copy .bcfg files to the assorted LOCALE_ROOT directories
# for inclusion on the assorted locale-dependent boot floppies

include $(UTSRULES)

MAKEFILE = mdi.mk
KBASE = ../../..
LOCALDEF = -DUNIXWARE
PKGFILE = $(ROOT)/usr/src/$(WORK)/pkg/nics/nd_mdi
# driver dependent packaging script, should exist in driver.cf directory
DDPKGFILE = nd_mdi
NDDIR = /etc/inst/nd/mdi
MDI_ROOT=$(ROOT)/$(MACH)$(NDDIR)
CF = =/etc/conf

# It's a long story, but the bcfg files used for netinstall are on the
# _boot_ floppies in a file called config.z, part of the memfs image
# that goes in .extra.d.  The ODI/DLPI/MDI driver itself is on the
# netinstall diskettes.  A shell script called cmd/conframdfs.sh takes
# all of the bcfg files in NICLOC and copies them to a staging area, where
# NICLOC="${ROOT}/${MACH}/etc/inst/locale/${LANG}/menus/nics"
# The niccfg.mk file for ODI drivers for make install does the following code
# to get the bcfg files from our source tree tree to locale/C/menus/nics:
# LOCALE_DIR = $(ROOT)/$(MACH)/etc/inst/locale/C/menus/nics
# CONFIG_DIR = $(LOCALE_DIR)/config
# ( cd $(ROOT)/usr/src/$(WORK)/cmd/niccfg/config; \
#   for i in * ; \
#   do \
#      $(INS) -f $(CONFIG_DIR) -m 0644 -u $(OWN) -g $(GRP) $$i; \
#   done ; \
# )
# The other localized .bcfg files go in en, ja, it, etc. instead of 'C' dir.
#
# In short, in order for MDI bcfg files to be recognized by the 
# netinstall process we must copy them into NICLOC.  That is, _each_
# locale directory specified by LANG.  We know that the conframdfs.sh
# is run multiple times with different LANG settings, so we must copy
# our non-internationalized bcfg files into each possible directory
# so that each time conframdfs.sh is run in each locale the expected bcfg
# files will already be there.  This won't affect packaging files.
# Rob/Barry/Nathan agree that this scheme is ok since this only affects
#   netinstall (netcfg already i18n/l10n so non-issue post-netinstall) AND
#   MDI driver (ODI/DLPI already taken care of by niccfg.mk) AND
#   bcfg wants to set a CUSTOM[] parameter
# then the short help file won't exist and the prompt itself won't be 
# localized.  However, we must copy bcfg files into proper place to
# appease the scripts which build the boot floppies.
#
# Worse, all of the netinstall scripts assume that NAME= in the bcfg
# file is one word -- no spaces.  So we must also sed through the bcfg file
# and replace the spaces with underscores.  Too much work to alter the
# offending scripts (ii_do_netinst and ii_hw_select) given short time.
# Sigh.

LOCALE_ROOT=$(ROOT)/$(MACH)/etc/inst/locale

# the "known" locales that we must put each bcfgs in; others discovered below
C_LOCALE=$(LOCALE_ROOT)/C/menus/nics/config
DE_LOCALE=$(LOCALE_ROOT)/de/menus/nics/config
ES_LOCALE=$(LOCALE_ROOT)/es/menus/nics/config
FR_LOCALE=$(LOCALE_ROOT)/fr/menus/nics/config
IT_LOCALE=$(LOCALE_ROOT)/it/menus/nics/config
JA_LOCALE=$(LOCALE_ROOT)/ja/menus/nics/config

all clean lintit:
	@for d in * ; do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	done

install: all
	[ -d $(CONF)/cf.d ] || mkdir -p $(CONF)/cf.d
	[ -f $(CONF)/cf.d/res_major ] || touch $(CONF)/cf.d/res_major
	[ -d $(CONF)/pack.d ] || mkdir -p $(CONF)/pack.d
	[ -d $(CONF)/mdevice.d ] || mkdir -p $(CONF)/mdevice.d
	[ -d $(CONF)/node.d ] || mkdir -p $(CONF)/node.d
# if the locale directories don't exist yet, create them for copy below.
# If more localized directories already exist, we'll discover them below
# If more localized directories get created later, then that's a problem.
# these are the minimum directories we need to populate with our MDI bcfg files.
	[ -d $(C_LOCALE) ] || mkdir -p $(C_LOCALE)
	[ -d $(DE_LOCALE) ] || mkdir -p $(DE_LOCALE)
	[ -d $(ES_LOCALE) ] || mkdir -p $(ES_LOCALE)
	[ -d $(FR_LOCALE) ] || mkdir -p $(FR_LOCALE)
	[ -d $(IT_LOCALE) ] || mkdir -p $(IT_LOCALE)
	[ -d $(JA_LOCALE) ] || mkdir -p $(JA_LOCALE)
# if there are any other directories in LOCALE_ROOT we missed, ensure they have
# a menus/nics/config subdirectory for our copy below.  This
# assumes everything in LOCALE_ROOT is a further subdirectory and not a file
# for the mkdir.
	(cd $(LOCALE_ROOT); for d in * ; do \
		[ -d $$d/menus/nics/config ] || mkdir -p $$d/menus/nics/config ; \
	done; ) > /dev/null 2>&1
	for d in * ; do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	done
	[ ! -f $(PKGFILE).base ] || { \
		cp -f $(PKGFILE).base $(PKGFILE); \
		chmod u+w $(PKGFILE); \
		for d in * ; { \
			if [ -d $$d/$$d.cf ]; then \
			echo "#" >> $(PKGFILE); \
			echo "d none $(NDDIR)/$$d	0755	root	sys" >> $(PKGFILE); \
			(cd $$d/$$d.cf; for f in * ; do \
			case $$f in \
				Driver.o) TGT="$(CF)/pack.d/$$d/Driver.o" ;; \
				Drvmap) TGT="$(CF)/drvmap.d/$$d" ;; \
				Master) TGT="$(CF)/mdevice.d/$$d" ;; \
				Node) TGT="$(CF)/node.d/$$d" ;; \
				Space.c) TGT="$(CF)/pack.d/$$d/space.c" ;; \
				System) TGT="$(CF)/sdevice.d/$$d" ;; \
				*.bcfg)	TGT="$(CF)/bcfg.d/$$d/$$f" ; \
						rm -f /tmp/$$f >/dev/null 2>&1 ; \
						sed -e '/^[ 	]*NAME=/s/ /_/gp' $$f > /tmp/$$f 2>/dev/null; \
						for q in $(LOCALE_ROOT)/*/menus/nics/config ; \
						do \
							$(INS) -f $$q -m 0644 -u $(OWN) -g $(GRP) /tmp/$$f > /dev/null 2>&1 ; \
						done ; \
						rm -f /tmp/$$f >/dev/null 2>&1 ; \
				;; \
				*)	[ -d $(MDI_ROOT)/$$d ] || mkdir -p $(MDI_ROOT)/$$d ; \
					$(INS) -f $(MDI_ROOT)/$$d -m 0644 -u $(OWN) -g $(GRP) $$f > /dev/null 2>&1 ; \

					TGT="" ; \
				;; \
			esac; \
			echo "f none $(NDDIR)/$$d/$$f$$TGT"; \
			done; \
			cd ..; \
			if [ -f $(DDPKGFILE) ]; then \
				echo "# Driver dependent nd_mdi packaging file follows:"; \
				ksh $(ROOT)/usr/src/$(WORK)/uts/io/nd/mdi/$$d/$(DDPKGFILE) $(NDDIR) `pwd` ; \
				echo "# end of Driver dependent nd_mdi packaging file"; \
			fi; \
			) >> $(PKGFILE); \
			fi; \
		} ; \
	}

clobber: clean
	@for d in * ; do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	done
	rm -f $(PKGFILE)
