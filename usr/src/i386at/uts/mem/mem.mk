#ident	"@(#)kern-i386at:mem/mem.mk	1.49.7.5"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	mem.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = mem

MEM = mem.cf/Driver.o
MEM_CCNUMA = ../mem.cf/Driver_ccnuma.o
KMA = kma.cf/Driver.o
KMA_CCNUMA = ../kma.cf/Driver_ccnuma.o
SEGDEV = segdev.cf/Driver.o
SEGDEV_CCNUMA = ../segdev.cf/Driver_ccnuma.o
PSE = pse.cf/Driver.o
PSE_CCNUMA = ../pse.cf/Driver_ccnuma.o
SEGSHM = segshm.cf/Driver.o
SEGSHM_CCNUMA = ../segshm.cf/Driver_ccnuma.o
VSWAPISL = vswapisl.cf/Driver.o

LFILE = $(LINTDIR)/mem.ln


# un-comment the following for gdb symbols
#
# CFLAGS = -Xa -Kno_lu -W2,-_s -Kno_host -W2,-A -Ki486 -W0,-d1 -g

#
# The symbol _MEM_INTERNAL_ should be defined for VM modules only
# Other modules which define this symbol will not remain compatible
# with update releases.
#
LOCALDEF = -D_MEM_INTERNAL_

MODULES = \
	$(MEM) \
	$(KMA) \
	$(SEGDEV) \
	$(SEGSHM) \
	$(PSE) \
	$(VSWAPISL)

CCNUMA_MODULES = \
	$(MEM_CCNUMA) \
	$(KMA_CCNUMA) \
	$(SEGDEV_CCNUMA) \
	$(SEGSHM_CCNUMA) \
	$(PSE_CCNUMA)

FILES = \
	amp.o \
	copy.o \
	memresv.o \
	move.o \
	pageflt.o \
	pageidhash.o \
	pooldaemon.o \
	rdma.o \
	rdma_p.o \
	rff.o \
	seg_dz.o \
	seg_kmem.o \
	seg_kvn.o \
	seg_map.o \
	seg_vn.o \
	ublock.o \
	ucopy.o \
	vcopy.o \
	vm_anon.o \
	vm_as.o \
	vm_hat.o \
	vm_hat32.o \
	vm_hat64.o \
	vm_hatstatic.o \
	vm_lock.o \
	vm_mapfile.o \
	vm_misc_f.o \
	vm_page.o \
	vm_page_f.o \
	vm_pageout.o \
	vm_pmem.o \
	vm_pvn.o \
	vm_scalls.o \
	vm_sched.o \
	vm_seg.o \
	vm_swap.o \
	vm_sysinit.o \
	vm_sysinit_p.o \
	vm_vpage.o \
	vm_zbm.o \
	vm_rzbm.o

CFILES = \
	amp.c \
	fgashm.c \
	kma.c \
	drv_mmap.c \
	memresv.c \
	move.c \
	pageflt.c \
	pageidhash.c \
	pooldaemon.c \
	pse_drv.c \
	pse_hat.c \
	rdma.c \
	rdma_p.c \
	rff.c \
	seg_dev.c \
	seg_dz.c \
	seg_fga.c \
	seg_kmem.c \
	seg_kpse.c \
	seg_kvn.c \
	seg_map.c \
	seg_pse.c \
	seg_vn.c \
	seg_umap.c \
	ublock.c \
	ucopy.c \
	vm_anon.c \
	vm_as.c \
	vm_hat.c \
	vm_hat32.c \
	vm_hat64.c \
	vm_hatshm.c \
	vm_hatstatic.c \
	vm_lock.c \
	vm_mapfile.c \
	vm_misc_f.c \
	vm_page.c \
	vm_page_f.c \
	vm_pageout.c \
	vm_pmem.c \
	vm_pvn.c \
	vm_scalls.c \
	vm_sched.c \
	vm_seg.c \
	vm_swap.c \
	vm_sysinit.c \
	vm_sysinit_p.c \
	vm_vpage.c \
	vm_zbm.c \
	vm_rzbm.c

SFILES = \
	copy.s \
	vcopy.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	amp.ln \
	fgashm.ln \
	kma.ln \
	drv_mmap.ln \
	memresv.ln \
	move.ln \
	pageflt.ln \
	pageidhash.ln \
	pooldaemon.ln \
	pse_drv.ln \
	pse_hat.ln \
	rdma.ln \
	rdma_p.ln \
	rff.ln \
	seg_dev.ln \
	seg_dz.ln \
	seg_fga.ln \
	seg_kmem.ln \
	seg_kpse.ln \
	seg_kvn.ln \
	seg_map.ln \
	seg_pse.ln \
	seg_vn.ln \
	seg_umap.ln \
	ublock.ln \
	ucopy.ln \
	vswap_isl.ln \
	vm_anon.ln \
	vm_as.ln \
	vm_hat.ln \
	vm_hat32.ln \
	vm_hat64.ln \
	vm_hatshm.ln \
	vm_hatstatic.ln \
	vm_lock.ln \
	vm_mapfile.ln \
	vm_misc_f.ln \
	vm_page.ln \
	vm_page_f.ln \
	vm_pageout.ln \
	vm_pmem.ln \
	vm_pvn.ln \
	vm_scalls.ln \
	vm_sched.ln \
	vm_seg.ln \
	vm_swap.ln \
	vm_sysinit.ln \
	vm_sysinit_p.ln \
	vm_vpage.ln \
	vm_zbm.ln \
	vm_rzbm.ln

all:	$(MODULES) ccnuma

install: all
	cd mem.cf; $(IDINSTALL) -R$(CONF) -M mem
	cd kma.cf; $(IDINSTALL) -R$(CONF) -M kma
	cd segdev.cf; $(IDINSTALL) -R$(CONF) -M segdev
	cd pse.cf; $(IDINSTALL) -R$(CONF) -M pse
	cd segshm.cf; $(IDINSTALL) -R$(CONF) -M segshm
	cd vswapisl.cf; $(IDINSTALL) -R$(CONF) -M vswapisl

ccnuma: 
	if [ "$$DUALBUILD" = 1 ]; then \
		if [ ! -d ccnuma.d ]; then \
			mkdir ccnuma.d; \
			cd ccnuma.d; \
			for file in "../*.[csh] ../*.mk*"; do \
				ln -s $$file . ; \
			done; \
		else \
			cd ccnuma.d; \
		fi; \
		$(MAKE) -f mem.mk $(CCNUMA_MAKEARGS) $(CCNUMA_MODULES); \
	fi

$(MEM): $(FILES)
	$(LD) -r -o $(MEM) $(FILES)
	$(FUR) -W -o mem.order $(MEM)

$(MEM_CCNUMA): $(FILES)
	$(LD) -r -o $(MEM_CCNUMA) $(FILES); \
#	$(FUR) -W -o mem_ccnuma.order $(MEM_CCNUMA); 

$(KMA): kma.o
	$(LD) -r -o $(KMA) kma.o
	$(FUR) -W -o kma.order $(KMA)

$(KMA_CCNUMA): kma.o
	$(LD) -r -o $(KMA_CCNUMA) kma.o
#	$(FUR) -W -o kma_ccnuma.order $(KMA_CCNUMA)

$(SEGDEV): drv_mmap.o seg_dev.o
	$(LD) -r -o $(SEGDEV) drv_mmap.o seg_dev.o

$(SEGDEV_CCNUMA): drv_mmap.o seg_dev.o
	$(LD) -r -o $(SEGDEV_CCNUMA) drv_mmap.o seg_dev.o

$(PSE): seg_pse.o seg_kpse.o pse_hat.o pse_drv.o
	$(LD) -r -o $(PSE) seg_pse.o seg_kpse.o pse_hat.o pse_drv.o

$(PSE_CCNUMA): seg_pse.o seg_kpse.o pse_hat.o pse_drv.o
	$(LD) -r -o $(PSE_CCNUMA) seg_pse.o seg_kpse.o \
		pse_hat.o pse_drv.o

$(VSWAPISL): vswap_isl.o
	$(LD) -r -o $(VSWAPISL) vswap_isl.o


# un-comment the following for gdb symbols
#
#	temporary rule while rff fails to compile with -g
#
# rff.o:	rff.c
# 	$(CC) $(MINUSG) -Xa -Kno_lu -W2,-_s -Kno_host -W2,-A -Ki486 \
# 		$(INCLIST) $(DEFLIST) -c $<

$(SEGSHM): seg_umap.o  seg_fga.o fgashm.o vm_hatshm.o
	$(LD) -r -o $(SEGSHM) seg_umap.o seg_fga.o fgashm.o vm_hatshm.o

$(SEGSHM_CCNUMA): seg_umap.o seg_fga.o fgashm.o vm_hatshm.o
	$(LD) -r -o $(SEGSHM_CCNUMA) seg_umap.o seg_fga.o fgashm.o vm_hatshm.o


copy.o: $(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

vcopy.o: $(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L $(MODULES) $(CCNUMA_MODULES)
	-rm -rf ccnuma.d

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e mem
	-$(IDINSTALL) -R$(CONF) -d -e kma
	-$(IDINSTALL) -R$(CONF) -d -e segdev
	-$(IDINSTALL) -R$(CONF) -d -e pse
	-$(IDINSTALL) -R$(CONF) -d -e segshm

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	immu.h \
	kmem.h \
	lock.h \
	swap.h \
	tuneable.h \
	vmmeter.h \
	vmparam.h
vmHeaders = \
	amp.h \
	anon.h \
	as.h \
	faultcatch.h \
	faultcode.h \
	hat.h \
	immu64.h \
	kma.h \
	kma_p.h \
	kmalist.h \
	mem_hier.h \
	memresv.h \
	page.h \
	pmem.h \
	pmem_f.h \
	pmem_p.h \
	pvn.h \
	rff.h \
	rzbm.h \
	seg.h \
	seg_dev.h \
	seg_dz.h \
	seg_kmem.h \
	seg_kmem_f.h \
	seg_kvn.h \
	seg_map.h \
	seg_map_u.h \
	seg_vn.h \
	seg_umap.h \
	seg_vn_f.h \
	ublock.h \
	vm_hat.h \
	vpage.h \
	zbm.h

headinstall: $(sysHeaders) $(vmHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@-[ -d $(INC)/vm ] || mkdir -p $(INC)/vm
	@for f in $(vmHeaders); \
	 do \
	    $(INS) -f $(INC)/vm -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
