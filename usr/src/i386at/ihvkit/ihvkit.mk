#ident	"@(#)ihvkit:ihvkit.mk	1.4.3.4"
#ident	"$Header$"

#	@(#)ihvkit:ihvkit.mk	1.4.3.2	1/20/97	19:15:32
#
#	Mon Jan 20 19:09:47 EST 1997	-	hiramc@sco.COM
#	- remove reference to display on the SUBDIRS list to avoid
#	- building the display portion of the ihvkit.
#	- a pget -Rgemini_bl6 will recover this file to the state
#	- before this change
#
include $(UTSRULES)

MAKEFILE = ihvkit.mk

INSDIR=$(ROOT)/$(MACH)/usr/src/ihvkit
NET = net
SUBDIRS = pdi
IHVSRC = ihvsrc
HBA = hba
HBADIR = ../uts/io/hba
PDIPKGDIR=../pkg/ihvhba

DDICHK = ddicheck
DDICHKDIR = ../ktool/ddicheck
DDICHKLIST =  \
	$(DDICHKDIR)/README \
	$(DDICHKDIR)/ddicheck \
	$(DDICHKDIR)/ddilint.c \
	$(DDICHKDIR)/ddicheck.mk \
	$(DDICHKDIR)/ddilint.data \
	$(DDICHKDIR)/flint.c
DDICHKFILE =  \
	README \
	ddicheck \
	ddilint.c \
	ddicheck.mk \
	ddilint.data \
	flint.c
DDICHKINSDIR = $(INSDIR)/pdi/dditools

HBAH = $(HBADIR)/hba.h
DPT = dpt
DPTCFG = dpt.cfg
DPTINSDIR = $(INSDIR)/pdi/$(DPT)
DPTCFGINSDIR = $(INSDIR)/pdi/$(DPT)/dpt.cf
DPTDIR = $(HBADIR)/$(DPT)
DPTCFGDIR = $(DPTDIR)/dpt.cf
DPTINSTSRC=$(PDIPKGDIR)/dpt
DPTINSTDIR=$(DPTINSDIR)/dpt.hbafloppy/dpt
DPTLIST = \
	$(DPTDIR)/dpt.c \
	$(DPTDIR)/dpt.h \
	$(HBAH)

DPTCFGLIST = \
	$(DPTCFGDIR)/Master \
	$(DPTCFGDIR)/Space.c \
	$(DPTCFGDIR)/Drvmap \
	$(DPTCFGDIR)/disk.cfg \
	$(DPTCFGDIR)/System

DPTFILE = \
	dpt.c \
	dpt.h \
	hba.h

DPTCFGFILE = \
	Master \
	Space.c \
	Drvmap \
	disk.cfg \
	System

DPTINSTFILE = \
	copyright \
	pkginfo \
	postinstall \
	preremove \
	prototype \
	request

ICTHA = ictha
ICTHACFG = ictha.cfg
ICTHAINSDIR = $(INSDIR)/pdi/$(ICTHA)
ICTHACFGINSDIR = $(INSDIR)/pdi/$(ICTHA)/ictha.cf
ICTHADIR = $(HBADIR)/$(ICTHA)
ICTHACFGDIR = $(ICTHADIR)/ictha.cf
ICTHALIST =  \
	$(ICTHADIR)/ictha.c \
	$(ICTHADIR)/ictha.h \
	$(HBAH)

ICTHACFGLIST = \
	$(ICTHACFGDIR)/Master \
	$(ICTHACFGDIR)/Space.c \
	$(ICTHACFGDIR)/Drvmap \
	$(ICTHACFGDIR)/disk.cfg \
	$(ICTHACFGDIR)/System

ICTHAFILE =  \
	ictha.c \
	ictha.h \
	hba.h

ICTHACFGFILE = \
	Master \
	Space.c \
	Drvmap \
	disk.cfg \
	System

MITSUMI = mitsumi
MITSUMICFG = mitsumi.cfg
MITSUMIINSDIR = $(INSDIR)/pdi/$(MITSUMI)
MITSUMICFGINSDIR = $(INSDIR)/pdi/$(MITSUMI)/mitsumi.cf
MITSUMIDIR = $(HBADIR)/$(MITSUMI)
MITSUMICFGDIR = $(MITSUMIDIR)/mitsumi.cf
MITSUMIINSTSRC =$(PDIPKGDIR)/mitsumi
MITSUMIINSTDIR =$(MITSUMIINSDIR)/mitsumi.hbafloppy/mitsumi
MITSUMILIST =  \
	$(MITSUMIDIR)/mitsumi.c  \
	$(MITSUMIDIR)/mitsumiscsi.c  \
	$(MITSUMIDIR)/hba.c  \
	$(MITSUMIDIR)/scsifake.c  \
	$(MITSUMIDIR)/mitsumi.h \
	$(HBAH)

MITSUMICFGLIST = \
	$(MITSUMICFGDIR)/Master \
	$(MITSUMICFGDIR)/Space.c \
	$(MITSUMICFGDIR)/Drvmap \
	$(MITSUMICFGDIR)/disk.cfg \
	$(MITSUMICFGDIR)/System

MITSUMIFILE =  \
	mitsumi.c  \
	mitsumiscsi.c  \
	hba.c  \
	scsifake.c  \
	mitsumi.h \
	hba.h

MITSUMICFGFILE = \
	Master \
	Space.c \
	Drvmap \
	disk.cfg \
	System

MITSUMIINSTFILE = \
	copyright \
	pkginfo \
	postinstall \
	preremove \
	prototype

all: $(NET)_mdi $(IHVSRC) $(HBA)

$(NET)_mdi:
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR);
	if [ -d $(NET) ]; then \
	(cd $(NET); echo "=== $(MAKE) -f $(NET).mk all"; \
	 $(MAKE) -f $(NET).mk all $(MAKEARGS)); \
	fi; 

$(IHVSRC):
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR); \
		for  i in $(SUBDIRS); do \
			find $$i -type f -follow -print | cpio -pdumL $(INSDIR); \
		done

$(HBA): $(DPT) $(ICTHA) $(MITSUMI) $(DPTCFG) $(ICTHACFG) $(MITSUMICFG) $(DDICHK)

install: all

$(DDICHK):
	for i in $(DDICHKFILE); do \
		rm -f $(DDICHKINSDIR)/$$i; \
	done
	for i in $(DDICHKLIST); do \
		cp $$i $(DDICHKINSDIR); \
	done

$(DPT):
	for i in $(DPTFILE); do \
		rm -f $(DPTINSDIR)/$$i; \
	done
	[ -d $(DPTINSDIR) ] || mkdir -p $(DPTINSDIR); \
	for i in $(DPTLIST); do \
		cp $$i $(DPTINSDIR); \
	done

$(DPTCFG):
	for i in $(DPTCFGFILE); do \
		rm -f $(DPTCFGINSDIR)/$$i; \
	done
	[ -d $(DPTCFGINSDIR) ] || mkdir -p $(DPTCFGINSDIR);
	for i in $(DPTCFGLIST); do \
		cp $$i $(DPTCFGINSDIR); \
	done
	for i in $(DPTINSTFILE); do \
		rm -f $(DPTINSTDIR)/$$i; \
	done
	[ -d $(DPTINSTDIR) ] || mkdir -p $(DPTINSTDIR); \
	for i in $(DPTINSTFILE); do \
		cp $(DPTINSTSRC)/$$i $(DPTINSTDIR); \
	done
 
$(ICTHA):
	for i in $(ICTHAFILE); do \
		rm -f $(ICTHAINSDIR)/$$i; \
	done
	[ -d $(ICTHAINSDIR) ] || mkdir -p $(ICTHAINSDIR); \
	for i in $(ICTHALIST); do \
		cp $$i $(ICTHAINSDIR) ; \
	done

$(ICTHACFG):
	for i in $(ICTHACFGFILE); do \
		rm -f $(ICTHACFGINSDIR)/$$i; \
	done
	[ -d $(ICTHACFGINSDIR) ] || mkdir -p $(ICTHACFGINSDIR);
	for i in $(ICTHACFGLIST); do \
		cp $$i $(ICTHACFGINSDIR); \
	done

$(MITSUMI):
	for i in $(MITSUMIFILE); do \
		rm -f $(MITSUMIINSDIR)/$$i; \
	done
	[ -d $(MITSUMIINSDIR) ] || mkdir -p $(MITSUMIINSDIR); \
	for i in $(MITSUMILIST); do \
		cp $$i $(MITSUMIINSDIR) ; \
	done

$(MITSUMICFG):
	for i in $(MITSUMICFGFILE); do \
		rm -f $(MITSUMICFGINSDIR)/$$i; \
	done
	[ -d $(MITSUMICFGINSDIR) ] || mkdir -p $(MITSUMICFGINSDIR);
	for i in $(MITSUMICFGLIST); do \
		cp $$i $(MITSUMICFGINSDIR); \
	done
	for i in $(MITSUMIINSTFILE); do \
		rm -f $(MITSUMIINSTDIR)/$$i; \
	done
	[ -d $(MITSUMIINSTDIR) ] || mkdir -p $(MITSUMIINSTDIR); \
	for i in $(MITSUMIINSTFILE); do \
		cp $(MITSUMIINSTSRC)/$$i $(MITSUMIINSTDIR); \
	done

clean:
	rm -rf $(INSDIR)

clobber: clean
