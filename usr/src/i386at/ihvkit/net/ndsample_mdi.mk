#ident "@(#)ndsample_mdi.mk	22.1"
#
#	Copyright (C) The Santa Cruz Operation, 1997
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)

NETDIR = /usr/src/hdk/net
PKGDIR = $(NETDIR)/pkg
PKGFILE = $(PKGDIR)/prototype
DDPKGFILE = nd_mdi
NDDIR = /etc/inst/nd/mdi

all:
	@for d in * ; do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	done

install: all

clean:
	@for d in * ; do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	done

clobber: clean
	@rm -f $(PKGDIR)/prototype
	@for d in $(PKGDIR)/* ; do \
		if [ -d $$d ]; then \
			rm -rf $$d ; \
		fi; \
	done

pkg: all
	echo "# packaging files" > $(PKGFILE)
	echo "i depend" >> $(PKGFILE)
	echo "i pkginfo" >> $(PKGFILE)
	echo "#\n# directories" >> $(PKGFILE)
	echo "!default 0555 bin bin" >> $(PKGFILE)
	echo "d none /etc	?	?	?" >> $(PKGFILE)
	echo "d none /etc/inst	?	?	?" >> $(PKGFILE)
	echo "d none /etc/inst/nd	?	?	?" >> $(PKGFILE)
	echo "d none $(NDDIR)	?	?	?" >> $(PKGFILE)
	echo "!default 0644 root sys" >> $(PKGFILE)
	echo "#\n# drivers" >> $(PKGFILE)
	for d in * ; do \
		if [ $$d = "e3B" -o $$d = "e3D" -o $$d = "tok" ]; then \
			continue; \
		fi; \
		if [ -d $$d/$$d.cf ]; then \
			echo "#" >> $(PKGFILE); \
			echo "d none $(NDDIR)/$$d	0755	root	sys" >> $(PKGFILE); \
			(cd $$d/$$d.cf; for f in * ; do \
			 echo "f none $(NDDIR)/$$d/$$f=$(NETDIR)/mdi/$$d/$$d.cf/$$f"; \
			 done; \
			 cd ..; \
			 if [ -f $(DDPKGFILE) ]; then \
				echo "# Driver dependent nd_mdi packaging file follows:"; \
				ksh $(NETDIR)/mdi/$$d/$(DDPKGFILE) $(NDDIR) `pwd` ; \
				echo "# end of Driver dependent nd_mdi packaging file"; \
			 fi; \
			) >> $(PKGFILE); \
		fi; \
	done
	(cd $(PKGDIR); pkgmk -o -B 512 -d $(PKGDIR))
	@echo "\n\007Use \"pkgadd -d $(PKGDIR)\" to install the package, or"
	@echo "\"pkgtrans -s $(PKGDIR) diskette1\" to create an installable disk"
