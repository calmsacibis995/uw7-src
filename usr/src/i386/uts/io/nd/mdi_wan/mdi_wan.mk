#ident "@(#)mdi_wan.mk	13.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(UTSRULES)

MAKEFILE = mdi_wan.mk
KBASE = ../../..
LOCALDEF = -DUNIXWARE
PKGFILE = $(ROOT)/usr/src/$(WORK)/pkg/nics/nd_mdi
NDDIR = /etc/inst/nd/mdi
MDI_ROOT=$(ROOT)/$(MACH)$(NDDIR)
CF = =/etc/conf

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

	for d in * ; do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	done
	if [ -f $(PKGFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) appendpkg $(MAKEARGS); \
	else \
		if [ -f $(PKGFILE).base ]; then \
			cp -f $(PKGFILE).base $(PKGFILE); \
			chmod u+w $(PKGFILE); \
			$(MAKE) -f $(MAKEFILE) appendpkg $(MAKEARGS); \
		fi; \
	fi

appendpkg:
	for d in * ; { \
		if [ -d $$d ]; then \
		    for dsp in $$d/*.cf ; { \
			if [ -d $$dsp ]; then \
	    		    drv="`basename $$dsp .cf`"; \
		 	    echo "#" >> $(PKGFILE); \
			    echo "d none $(NDDIR)/$$drv	0755	root	sys" >> $(PKGFILE); \
			    (cd $$dsp; for f in * ; do \
			        case $$f in \
				    Driver.o) TGT="$(CF)/pack.d/$$drv/Driver.o" ;; \
				    Drvmap) TGT="$(CF)/drvmap.d/$$drv" ;; \
				    Master) TGT="$(CF)/mdevice.d/$$drv" ;; \
				    Node) TGT="$(CF)/node.d/$$drv" ;; \
				    Space.c) TGT="$(CF)/pack.d/$$drv/space.c" ;; \
				    System) TGT="$(CF)/sdevice.d/$$drv" ;; \
				    *.bcfg)	TGT="$(CF)/bcfg.d/$$drv/$$f" ;; \
				    *)	[ -d $(MDI_ROOT)/$$drv ] || mkdir -p $(MDI_ROOT)/$$drv ; \
				    	$(INS) -f $(MDI_ROOT)/$$drv -m 0644 -u $(OWN) -g $(GRP) $$f > /dev/null 2>&1 ; \
				    	TGT="" ; \
				    ;; \
				esac; \

				echo "f none $(NDDIR)/$$drv/$$f$$TGT"; \
			    done; ) >> $(PKGFILE); \
			fi; \
		    } ; \
		fi; \
	}

clobber: clean
	@for d in * ; do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		 $(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi; \
	done
	rm -f $(PKGFILE)
