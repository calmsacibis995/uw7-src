#ident	"@(#)crash:i386/cmd/crash/vtop.c	1.1.4.1"

/*
 * This file contains code for the crash function vtop, and
 * the virtual to physical offset conversion routine cr_vtop.
 * But getvtop() command processing is now in misc.c,
 * leaving vtop.c as a low-level support module.
 */

#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/vnode.h>
#include <vm/vm_hat.h>
#include <vm/as.h>
#include <sys/proc.h>
#include <sys/vmparam.h>
#include <sys/mman.h>
#include <sys/kcore.h>
#include <sys/var.h>
#include <sys/ksym.h>
#include <sys/plocal.h>
#include <sys/psm.h>

#include "crash.h"

vaddr_t	kvbase = 0;			/* intitailly, will be set up later */
int pae;				/* Page Address Extension is enabled */
paddr_t KPD[32];			/* different physical for each engine */
pte_t *kpd_start;			/* same virtual for each engine */
#define KPD_PAGES 4			/* enough for Page Address Extension */
size_t sizeof_as = sizeof(struct as);	/* for any adjustments */
static	kcore_t header;

/*
 * For cr_ptof():
 * Table for finding dumpfile offset from physical address.
 */
#define PTOF_BLOCK 256	/* for easier debug; MZERO_GRANULARITY is 512 */
typedef struct {
	unsigned long pblock;
	unsigned long fblock;
} ptof_t;
static ptof_t *ptoftab;
static int ptoflen;

/*
 * For getmapped():
 * Page cache, mmap()ed in the active case: only used for
 * for page directories and page tables, to save repeatedly
 * reading ptes; but could be used elsewhere if appropriate.
 */
typedef struct {
	phaddr_t pbase;
	ulong	mapref;
	char    *map;
} mapix_t;
#define NMAPS	16
static	mapix_t mapix[NMAPS];
static	ulong	mapref;
static	int	segvnfd;	/* to work around current Gemini bug */

/*
 * Construct physical address to dumpfile offset conversion table
 * (or get bootmem ranges in the active case) for cr_ptof().
 */
void
ptofinit(void)
{
	mreg_chunk_t mchunk;
	phaddr_t paddr, faddr, alignmask;
	ulong_t type, prevtype;
	size_t size;
	register int i;
	register ptof_t *ptofp;
	int ptofcmp();

	static char readerr[] = "cannot read mreg header from dumpfile\n";
	static char finderr[] = "cannot find mreg header in dumpfile\n";

	if (active) {
		ms_topology_t	os_topology;
		ms_topology_t	*os_topology_p;
		ms_resource_t	*os_resources;
		vaddr_t		vaddr;

		vaddr = symfindval("os_topology_p");
		readmem64((phaddr_t)vaddr,1,-1,&os_topology_p,
			sizeof(os_topology_p), "os_topology_p");

		readmem64((phaddr_t)os_topology_p,1,-1,&os_topology,
			sizeof(os_topology), "os_topology");

		os_resources = (ms_resource_t *)
			cr_malloc(os_topology.mst_nresource *
				sizeof(ms_resource_t),"os_resources");

		readmem64((phaddr_t)os_topology.mst_resources,1,-1,
			os_resources,
			os_topology.mst_nresource * sizeof(ms_resource_t), 
			"os_resources");
		
		/*
		 * The active readmem() doesn't use cr_ptof(),
		 * but the physical search function does.
		 */

		
		ptoflen = 2;	/* initial range plus hole at the end */
		paddr = 0;

		for (i = 0; i < os_topology.mst_nresource; i++) {
			if (os_resources[i].msr_type != MSR_MEMORY)
				continue;
			if (paddr != os_resources[i].
				msri.msr_memory.msr_address) {
				ptoflen += 2;
				paddr = os_resources[i].
					msri.msr_memory.msr_address; 
			}
			paddr += os_resources[i].
				msri.msr_memory.msr_size; 
		}

		cr_unfree(ptoftab =
			cr_malloc((ptoflen + 1) * sizeof(ptof_t), "ptoftab"));

		paddr = 0;
		ptofp = ptoftab;
		ptofp->pblock = 0;
		ptofp->fblock = 0;
		ptofp++;

		for (i = 0; i < os_topology.mst_nresource; i++) {
			if (os_resources[i].msr_type != MSR_MEMORY)
				continue;
			if (paddr != os_resources[i].
				msri.msr_memory.msr_address) {
				ptofp->pblock = paddr / PTOF_BLOCK;
				ptofp->fblock = -1;
				ptofp++;
				paddr = os_resources[i].
					msri.msr_memory.msr_address; 
				ptofp->pblock =
				ptofp->fblock = paddr / PTOF_BLOCK;
				ptofp++;
			}
			paddr += os_resources[i].
				msri.msr_memory.msr_size; 
		}

		goto sortit;
	}

	/*
	 * get the header to verify we have the dumpfile.
	 */
	if (pread64(mem,&header,sizeof(header),(phaddr_t)0) != sizeof(header))
		fatal("header not found in dumpfile\n");
	if (header.k_magic != KCORMAG)
		fatal("wrong magic number in dumpfile\n");

	alignmask = header.k_align - 1;
	faddr = sizeof(header);
	prevtype = -1;
	ptoflen = 1;		/* for hole added at end */

	/*
	 * find out the number of memory chunks in the dumpfile.
	 */
	for (;;) {
		faddr = (faddr + alignmask) & ~alignmask;
		if (pread64(mem,&mchunk,sizeof(mchunk),faddr) != sizeof(mchunk))
			fatal(readerr);

		if (mchunk.mc_magic != MRMAGIC)
			fatal(finderr);
		/*
		 * We go this far even when having found size 0,
		 * in order to verify that the dump is complete.
		 */
		if (MREG_LENGTH(mchunk.mc_mreg[0]) == 0)
			break;

		faddr += sizeof(mchunk);
		faddr = (faddr + alignmask) & ~alignmask;

		/*
		 * Because our table only contains paddr and faddr,
		 * extent being deduced from the next entry, we have
		 * to note holes; but this is problematic when paddr
		 * wraps around zero - those holes mustn't be taken
		 * too seriously, they're just a way of getting to
		 * the next important paddr.  We must therefore take
		 * care to coalesce consecutive MREG_HOLE entries,
		 * and can do so for the other types too - but cannot
		 * coalesce an MREG_IMAGE at the start of one chunk
		 * with that at the end of the previous chunk, since
		 * faddr has advanced meanwhile.
		 */
		if (prevtype == MREG_IMAGE)
			prevtype = -1;

		for (i = 0; i < NMREG; i++) {
			if ((size = MREG_LENGTH(mchunk.mc_mreg[i])) == 0)
				break;
			type = MREG_TYPE(mchunk.mc_mreg[i]);
			if (type == MREG_IMAGE)
				faddr += size;
			else if (type == MREG_USER)	/* ignore distinction */
				type = MREG_ZERO;	/* to minimize table */
			if (type != prevtype) {
				prevtype = type;
				ptoflen++;
			}
		}
	}

	if (ptoflen == 1)
		fatal("no memory chunks found\n");
	cr_unfree(ptoftab =
		cr_malloc((ptoflen + 1) * sizeof(ptof_t), "ptoftab"));

	ptofp = ptoftab;
	size = sizeof(header);
	faddr = (phaddr_t)size;
	paddr = 0;
	prevtype = -1;

	/*
	 * read through again to fill our malloced array
	 * (there are likely to be many thousands of entries,
	 * so reading it twice is rather an overhead, but...)
	 */
	while (size) {
		faddr = (faddr + alignmask) & ~alignmask;
		if (pread64(mem,&mchunk,sizeof(mchunk),faddr) != sizeof(mchunk))
			fatal(readerr);

		if (mchunk.mc_magic != MRMAGIC)
			fatal(finderr);

		faddr += sizeof(mchunk);
		faddr = (faddr + alignmask) & ~alignmask;

		if (prevtype == MREG_IMAGE)
			prevtype = -1;		/* cannot coalesce */

		for (i = 0; i < NMREG; i++) {
			if ((size = MREG_LENGTH(mchunk.mc_mreg[i])) == 0)
				break;
			switch (type = MREG_TYPE(mchunk.mc_mreg[i])) {
			case MREG_IMAGE:
				ptofp->fblock = faddr / PTOF_BLOCK;
				faddr += size;
				break;
			case MREG_USER:			/* ignore distinction */
				type = MREG_ZERO;	/* to minimize table */
				/*FALLTHRU*/
			case MREG_ZERO:
				ptofp->fblock = 0;
				break;
			default:			/* MREG_HOLE */
				ptofp->fblock = -1;
				break;
			}
			ptofp->pblock = paddr / PTOF_BLOCK;
			paddr += size;
			if (type != prevtype) {
				prevtype = type;
				ptofp++;
			}
		}
	}

sortit:
	/*
	 * Yes, we had an Olivetti dump which wrapped around and
	 * needed this sorting.  First add a final hole extent.
	 * Code retained, but I don't think such a dump would
	 * fit in with Gemini's extended addressing.
	 */
	ptofp->pblock = paddr / PTOF_BLOCK;
	ptofp->fblock = -1;	/* we counted 1 more in ptoflen for this */
	qsort(ptoftab, ptoflen, sizeof(ptof_t), ptofcmp);

	/*
	 * Trim redundant entries from beginning and end
	 * (but there may be some embedded within which we leave)
	 */
	while (ptoftab[1].pblock == 0) {
		--ptoflen;
		++ptoftab;
	}
	while (ptofp[-1].fblock == -1) {
		--ptoflen;
		--ptofp;
	}

	(++ptofp)->pblock = -1;	/* cr_ptof() needs one entry beyond the end */
	ptofp->fblock = -1;	/* for which we allowed 1 more in the malloc */

	if (debugmode > 5) {
		for (i = 0, ptofp = ptoftab; i < ptoflen; i++, ptofp++) {
			fprintf(stderr, "mreg=%5u paddr=%07x00 faddr=%07x00\n",
				i, ptofp->pblock, ptofp->fblock);
		}
	}
}

/*
 * Set KPD[0] and kpd_start for cr_vtop(),
 * and prepare the page cache for getmapped().
 */
void
vtopinit(void)
{
	char *getmapped(phaddr_t, char *);
	pte_t *ptep;
	paddr_t paddr;
	int i, zfd;

	/*
	 * Allocate page cache for getmapped().
	 * Cannot use mmap() on dumpfile because of ptof holes,
	 * but mmap() here not malloc() so addresses are aligned.
	 */
	zfd = open("/dev/zero", 0);
	mapix[0].map = mmap(NULL, PAGESIZE*NMAPS,
		PROT_READ|PROT_WRITE, MAP_PRIVATE, zfd, (off32_t)0);
	close(zfd);

	if (mapix[0].map == (char *)(-1))
		fatal("cannot allocate page cache\n");
	for (i = 0; i < NMAPS; i++) {
		mapix[i].pbase = (phaddr_t)(-1);
		mapix[i].map = mapix[0].map + i*PAGESIZE;
	}
	if (active)	/* to work around current Gemini bug */
		segvnfd = open("/usr/lib/libc.so.1", 0);


	/*
	 * Set KPD[0] and kpd_start for cr_vtop():
	 * the kernel no longer sets up a convenient mapping for crash_kl1pt
	 * so we either get it from the dump header or ioctl(MIOC_READKSYM) 
	 * on /dev/kmem
	 */

	KPD[0] = 0;

	if(active) {
		int	kmemfd;
		struct mioc_rksym rks;
		ulong_t	type;
		paddr64_t	cr_kl1pt;

		rks.mirk_symname = "cr_kl1pt";
		rks.mirk_buf	= &cr_kl1pt;
		rks.mirk_buflen	= sizeof(cr_kl1pt);

		if((kmemfd = open("/dev/kmem",O_RDONLY,0)) < 0)
			error("Cannot open /dev/kmem");
		if(ioctl(kmemfd,MIOC_READKSYM,&rks) < 0) {
			if(debugmode > 1)
				fprintf(stderr,"cr_kl1pt not found\n");
		} else
			KPD[0] = (paddr_t) cr_kl1pt;
		close(kmemfd);

	} else {
		/*
		 * the dump header was read by ptofinit
		 * this should have contained the address of the kl1pt
		 */

		if(header.k_version == 2) {
			KPD[0] = (paddr_t) header.k_kl1pt;
			if (debugmode > 1)
				fprintf(stderr,"new style dump header KPD = %x\n", KPD[0]);
		}
		
	}

	/*
	 * if we have an old dump (or old active kernel)
	 * we get KPD by knowing that the symbol crash_kl1pt
	 * is mapped virtual == physical
	 */
	if(KPD[0] == 0) {
		if (debugmode > 1)
			fprintf(stderr,"old dump, crash_kl1pt mapped virt == phys\n");
		paddr = symfindval("crash_kl1pt");
		if (CR_KADDR(paddr))
			paddr -= kvbase;
		if (debugmode > 1)
			fprintf(stderr,"crash_kl1pt=%08x\n",paddr);

		/* This is the first call to readmem(physical) */
		/* paddr is 32-bit but use readmem64() to avoid a warning */
		readmem64((phaddr_t)paddr,0,-1,KPD,sizeof(KPD[0]),"crash_kl1pt");
	}


	ptep = (pte_t *)getmapped((phaddr_t)KPD[0], "page directory");
	ptep += (MMU_PAGESIZE/sizeof(pte_t)) - 1; /* point to last ulong */
	if ((pae = (ptep->pg_pte == 0))) {
		KPD[0] -= (KPD_PAGES-1)*MMU_PAGESIZE;
		--ptep;				  /* point to low ulong */
	}
	ptep = (pte_t *)getmapped((phaddr_t)
		(ptep->pg_pte & ~(MMU_PAGESIZE-1)), "top page table");
	ptep += (MMU_PAGESIZE/sizeof(pte_t)) - 1; /* point to last ulong */

	i = MMU_PAGESIZE;
	while ((ptep->pg_pte & ~(MMU_PAGESIZE-1)) != KPD[0]) {
		if (((vaddr_t)ptep & (MMU_PAGESIZE-1)) == 0) {
		    /* kpd_start is only required for process address space */
		    prerrmes("cannot find page directory in top page table");
		    break;
		}
		i += MMU_PAGESIZE;
		--ptep;
	}
	i >>= pae;
	kpd_start = (pte_t *)(-i);

	if (debugmode > 0)
		fprintf(stderr,"KPD[0]=%08x kpd_start=%08x pae=%d\n",
			KPD[0], kpd_start, pae);
#ifdef KVBASE_IS_VARIABLE
	/*
	 * kvbase is now a variable, we need to get it
	 * up to this point we have assumed it is MINUVEND
	 * infact it can be greater than this. 
	 */
	paddr = symfindval("kvbase");
	if (debugmode > 1)
		fprintf(stderr,"kvbase=%08x\n",paddr);
	readmem64((phaddr_t)paddr,1,-1,&kvbase,sizeof(kvbase),"kvbase");
#else
	kvbase = KVBASE;
#endif
}

/* used to sort entries if mreg chunks wrapped around */
static int
ptofcmp(register ptof_t *a, register ptof_t *b)
{
	if (a->pblock < b->pblock)
		return -1;
	if (a->pblock > b->pblock)
		return 1;
	/*
	 * MREG_HOLEs may overlap themselves or other types:
	 * set coincident MREG_HOLE slot to the other type
	 */
	if (a->fblock != b->fblock) {
		if (a->fblock == -1)
			a->fblock = b->fblock;
		else if (b->fblock == -1)
			b->fblock = a->fblock;
		else
			fatal("inconsistent mregs\n");
	}
	return 0;
}

/* convert physical address to dumpfile offset */
phaddr_t
cr_ptof(phaddr_t paddr, size_t *extentp)
{
	register int ll, mm, hh;
	register ptof_t *ptofp;
	unsigned long pblock;

	pblock = paddr / PTOF_BLOCK;

	ll = 0;
	hh = ptoflen;
	while ((mm = ((ll + hh) >> 1)) != ll) {		/* binary chop */
		if (pblock >= ptoftab[mm].pblock)
			ll = mm;
		else
			hh = mm;
	}

	ptofp = ptoftab + mm;
	while (ptofp->pblock == ptofp[1].pblock)	/* skip coincidents */
		++ptofp;
	if (extentp && ptofp[1].pblock != -1) {		/* truncate span */
		phaddr_t nextpaddr = (phaddr_t)ptofp[1].pblock * PTOF_BLOCK;
		if (paddr + *extentp > nextpaddr) {
			*extentp = (size_t)(nextpaddr - paddr);
			if (*extentp == 0)		/* make it non-zero */
				*extentp -= MMU_PAGESIZE;
		}
	}

	if (ptofp->fblock == 0)				/* MREG_ZERO/USER */
		return 0;
	if (ptofp->fblock == -1)			/* MREG_HOLE */
		return -1;
	paddr -= (phaddr_t)ptofp->pblock * PTOF_BLOCK;
	paddr += (phaddr_t)ptofp->fblock * PTOF_BLOCK;
	return paddr;					/* MREG_IMAGE */
}

static char *
getmapped(phaddr_t paddr, char *name)
{
	register mapix_t *mapixp, *oldest;
	register phaddr_t pbase;

	pbase = paddr & ~(PAGESIZE-1);

	for (oldest = mapixp = mapix; mapixp < mapix + NMAPS; mapixp++) {
		if (mapixp->pbase == pbase)
			break;
		if (mapixp->mapref < oldest->mapref)
			oldest = mapixp;
	}

	if (mapixp >= mapix + NMAPS) {
		mapixp = oldest;
		mapixp->pbase = (phaddr_t)(-1);
		if (active) {
			/*
			 * Use mmap() on /dev/mem to track changes dynamically.
			 * There seems to be a Gemini bug, at least in BL6
			 * and BL7, such that a changed mapping of /dev/mem
			 * does not take effect immediately - do segdev and
			 * segdz miss a TLB flush which segvn remembers?
			 * So for now use a segvn mapping (from libc.so.1
			 * which is sure to be there) to wipe out the old
			 * mapping - alternatively, nap(1) also works.
			 */
			(void)mmap(mapixp->map, PAGESIZE, PROT_READ,
				MAP_FIXED|MAP_SHARED, segvnfd, (off32_t)0);
			if (mapixp->map != 
				(char *)mmap64(mapixp->map, PAGESIZE, PROT_READ,
				MAP_FIXED|MAP_SHARED, mem, (off64_t)pbase))
			error("getmapped: mmap error at offset %llx for %s\n",
				paddr, name);
		}
		else {
			/*
			 * Cannot use mmap() on dumpfile because of the
			 * MREG_ZERO part-pages; but base address was
			 * mmap()ed so caller can assume page alignment.
			 */
			readmem64(pbase, 0, -1, mapixp->map, PAGESIZE, name);
		}
		mapixp->pbase = pbase;
	}

	mapixp->mapref = ++mapref;
	return mapixp->map + (paddr & (PAGESIZE-1));
}

/* return process address space pointer */
struct as *
readas(int proc, struct as **kaspp, boolean_t multiple)
{
	static int prev_proc = -2;
	static struct as asbuf, *kasp;
	proc_t procbuf, *kprocp;

	if (proc < -1 || proc >= vbuf.v_proc) {
		prerrmes("process %d is out of range 0-%d\n",
			proc, vbuf.v_proc - 1);
		return NULL;
	}
	if (!active && proc == prev_proc) {
		*kaspp = kasp;
		return &asbuf;
	}
	if (proc == -1) {
		/*
		 * proc -1 here may mean that the user is doing a command on
		 * idle context, or that an internal readmem demands kernel
		 * address space: choose a message wording suitable to both
		 */
		prwarning(multiple,"kernel context has no user address space\n");
		return NULL;
	}
	if ((kprocp = slot_to_proc(proc)) == NULL) {
		if (!multiple)
			prerrmes("process %d does not exist\n",proc);
		return NULL;
	}
	readmem((vaddr_t)kprocp,1,-1,&procbuf,sizeof(procbuf),"proc structure");
	if (procbuf.p_nlwp == 0) {
		prwarning(multiple,"process %d is a zombie\n",proc);
		return NULL;
	}
	if (procbuf.p_as == NULL) {
		prwarning(multiple,"process %d has no user address space\n",proc);
		return NULL;
	}
	if (!(procbuf.p_flag & P_LOAD)) {
		prwarning(multiple,"process %d is swapped out\n",proc);
		return NULL;
	}
	if (procbuf.p_as < (struct as *)kvbase) {
		prwarning(multiple,"process %d has invalid as pointer %x\n",
			proc, procbuf.p_as);
		return NULL;
	}
	prev_proc = -2;
	readmem((vaddr_t)procbuf.p_as,1,-1,&asbuf,sizeof_as,"as structure");
	if (asbuf.a_hat.hat_pts < (struct hatpt *)kvbase) {
		if (asbuf.a_hat.hat_pts == NULL)
			prwarning(multiple,
				"process %d has null hatpt pointer\n",proc);
		else
			prwarning(multiple,
				"process %d has invalid hatpt pointer %x\n",
					proc, asbuf.a_hat.hat_pts);
		return NULL;
	}
	prev_proc = proc;
	*kaspp = kasp = procbuf.p_as;
	return &asbuf;
}

/* crash virtual to 32-bit physical offset address translation */
static phaddr_t
cr_vtop32(vaddr_t vaddr, int proc, size_t *extentp, boolean_t verbose)
{
	pte_t *kpd_entry, *ptep, pte;
	struct as *asp, *kasp;
	struct hatpt *ptap, *hatptp;
	struct hatpt ptapbuf;
	size_t extent, pgsize, offset;

	if (CR_KADDR(vaddr)) {
		if (debugmode > 1)
			fprintf(stderr,
			"cr_vtop:   kernel addr %x\n", vaddr);

		ptep = (pte_t *)getmapped((phaddr_t)KPD[Cur_eng] +
			ptnum(vaddr)*sizeof(pte_t), "page directory entry");
	}
	else {
		if (debugmode > 1)
			fprintf(stderr,
			"cr_vtop:   process %d addr %x must be transformed\n",
				proc, vaddr);

		if (extentp) {
			/* avoid further tests in the most common case */
			if ((vaddr&(MMU_PAGESIZE-1))+*extentp <= MMU_PAGESIZE)
				extentp = NULL;
			else if (*extentp > kvbase - vaddr)
				*extentp = kvbase - vaddr;
		}

		if (proc == -1 && !verbose)
			return -1;

		/* Read in process address space structure pointer */
		if ((asp = readas(proc, &kasp, B_FALSE)) == NULL)
			return -1;

		ptap = hatptp = asp->a_hat.hat_pts;
		kpd_entry = kpd_start + ptnum(vaddr);

		/* Scan each HAT structure */
		for (;;) {
			/*
			 * Strict checking as in prptbls(), because we have
			 * potential for an infinite loop if something is wrong
			 * e.g. memory freed and reused on an active system
			 */

			if ((vaddr_t)ptap < kvbase) {
				prerrmes("process %d has invalid hatpt pointer %x\n",
					proc, ptap);
				return -1;
			}

			/* Read in Page Table aligned HAT structure */
			readmem((vaddr_t)ptap, 1, -1,
				&ptapbuf, sizeof(ptapbuf), "hatpt structure");

			if (ptapbuf.hatpt_as != kasp) {
				prerrmes("process %d hatpt_as %x does not match p_as %x\n",
					proc, ptapbuf.hatpt_as, kasp);
				return -1;
			}

			if (ptapbuf.hatpt_pdtep == kpd_entry) {
				ptep = &ptapbuf.hatpt_pde;
				if (ptep->pgm.pg_v)
					break;
			}

			/* ASSERT: HAT pointer must be in kpd0 range */
			if (ptapbuf.hatpt_pdtep <  kpd_start
			||  ptapbuf.hatpt_pdtep >= kpd_start
				+ MMU_PAGESIZE/sizeof(pte_t)) {
				prerrmes("process %d has invalid hatpt page table pointer %x\n",
					proc, ptapbuf.hatpt_pdtep);
				return -1;
			}

			if (extentp && ptapbuf.hatpt_pdtep > kpd_entry) {
				extent = (ptapbuf.hatpt_pdtep - kpd_start)
					* (NPGPT*MMU_PAGESIZE) - vaddr;
				if (*extentp > extent)
					*extentp = extent;
			}

			if ((ptap = ptapbuf.hatpt_forw) == hatptp) {
				if (verbose)
					prerrmes("page table is not in hat\n");
				return -1;
			}
		}
	}

	pgsize = NPGPT*MMU_PAGESIZE;
	offset = vaddr & ((NPGPT*MMU_PAGESIZE)-1);
	pte = *ptep;		/* since *ptep may be volatile */

	if (pte.pgm.pg_v && !pte.pgm.pg_ps) {
		ptep = (pte_t *)getmapped(
			(phaddr_t)(pte.pg_pte & ~(MMU_PAGESIZE-1)) +
			pnum(vaddr)*sizeof(pte_t), "page table entry");

		pgsize  =  MMU_PAGESIZE;
		offset &= (MMU_PAGESIZE-1);
		pte = *ptep;	/* since *ptep may be volatile */
	}

	if (extentp) {
		extent = pgsize - offset;
		if (!pte.pgm.pg_v) {
			while (((unsigned)(++ptep) & (MMU_PAGESIZE-1))
			&& !ptep->pgm.pg_v)
				extent += pgsize;
		}
		if (*extentp > extent)
			*extentp = extent;
	}

	if (!pte.pgm.pg_v) {
		if (verbose)
			prerrmes("page%s is not in core\n",
				(pgsize == MMU_PAGESIZE)? "": " table");
		return -1;
	}

	pte.pg_pte &= ~(MMU_PAGESIZE-1);
	pte.pg_pte |= offset;
	if (debugmode > 5)
		fprintf(stderr, "cr_vtop:   %08x->%08x\n", vaddr, pte.pg_pte);
	return (phaddr_t)(pte.pg_pte);
}

/* crash virtual to 36-bit physical offset address translation */
static phaddr_t
cr_vtop64(vaddr_t vaddr, int proc, size_t *extentp, boolean_t verbose)
{
#define cr_pae_ptnum(X) ((vaddr_t)(X) >> PAE_PTNUMSHFT) /* no PAE_PTNUMMASK */
#define kpd_start64	((pte64_t *)kpd_start)
	pte64_t *kpd_entry, *ptep, pte;
	struct as *asp, *kasp;
	struct hatpt *ptap, *hatptp;
	struct hatpt ptapbuf;
	size_t extent, pgsize, offset;

	if (CR_KADDR(vaddr)) {
		if (debugmode > 1)
			fprintf(stderr,
			"cr_vtop64: kernel addr %x\n", vaddr);

		ptep = (pte64_t *)getmapped((phaddr_t)KPD[Cur_eng] +
			cr_pae_ptnum(vaddr)*sizeof(pte64_t),
			"page directory entry");
	}
	else {
		if (debugmode > 1)
			fprintf(stderr,
			"cr_vtop64: process %d addr %x must be transformed\n",
				proc, vaddr);

		if (extentp) {
			/* avoid further tests in the most common case */
			if ((vaddr&(MMU_PAGESIZE-1))+*extentp <= MMU_PAGESIZE)
				extentp = NULL;
			else if (*extentp > kvbase - vaddr)
				*extentp = kvbase - vaddr;
		}

		if (proc == -1 && !verbose)
			return -1;

		/* Read in process address space structure pointer */
		if ((asp = readas(proc, &kasp, B_FALSE)) == NULL)
			return -1;

		ptap = hatptp = asp->a_hat.hat_pts;
		kpd_entry = kpd_start64 + cr_pae_ptnum(vaddr);

		/* Scan each HAT structure */
		for (;;) {
			/*
			 * Strict checking as in prptbls(), because we have
			 * potential for an infinite loop if something is wrong
			 * e.g. memory freed and reused on an active system
			 */

			if ((vaddr_t)ptap < kvbase) {
				prerrmes("process %d has invalid hatpt pointer %x\n",
					proc, ptap);
				return -1;
			}

			/* Read in Page Table aligned HAT structure */
			readmem((vaddr_t)ptap, 1, -1,
				&ptapbuf, sizeof(ptapbuf), "hatpt structure");

			if (ptapbuf.hatpt_as != kasp) {
				prerrmes("process %d hatpt_as %x does not match p_as %x\n",
					proc, ptapbuf.hatpt_as, kasp);
				return -1;
			}

			if (ptapbuf.hatpt_pdtep64 == kpd_entry) {
				ptep = &ptapbuf.hatpt_pde64;
				if (ptep->pgm.pg_v)
					break;
			}

			/* ASSERT: HAT pointer must be in kpd0 range */
			if (ptapbuf.hatpt_pdtep64 <  kpd_start64
			||  ptapbuf.hatpt_pdtep64 >= kpd_start64
				+ (KPD_PAGES*MMU_PAGESIZE)/sizeof(pte64_t)) {
				prerrmes("process %d has invalid hatpt page table pointer %x\n",
					proc, ptapbuf.hatpt_pdtep64);
				return -1;
			}

			if (extentp && ptapbuf.hatpt_pdtep64 > kpd_entry) {
				extent = (ptapbuf.hatpt_pdtep64 - kpd_start64)
					* (PAE_NPGPT*MMU_PAGESIZE) - vaddr;
				if (*extentp > extent)
					*extentp = extent;
			}

			if ((ptap = ptapbuf.hatpt_forw) == hatptp) {
				if (verbose)
					prerrmes("page table is not in hat\n");
				return -1;
			}
		}
	}

	pgsize = PAE_NPGPT*MMU_PAGESIZE;
	offset = vaddr & ((PAE_NPGPT*MMU_PAGESIZE)-1);
	pte = *ptep;		/* since *ptep may be volatile */

	if (pte.pgm.pg_v && !pte.pgm.pg_ps) {
		ptep = (pte64_t *)getmapped(
			((phaddr_t)pte.pg_pte & ~(MMU_PAGESIZE-1)) +
			pae_pnum(vaddr)*sizeof(pte64_t), "page table entry");

		pgsize  =  MMU_PAGESIZE;
		offset &= (MMU_PAGESIZE-1);
		pte = *ptep;	/* since *ptep may be volatile */
	}

	if (extentp) {
		extent = pgsize - offset;
		if (!pte.pgm.pg_v) {
			while (((unsigned)(++ptep) & (MMU_PAGESIZE-1))
			&& !ptep->pgm.pg_v)
				extent += pgsize;
		}
		if (*extentp > extent)
			*extentp = extent;
	}

	if (!pte.pgm.pg_v) {
		if (verbose)
			prerrmes("page%s is not in core\n",
				(pgsize == MMU_PAGESIZE)? "": " table");
		return -1;
	}

	pte.pg_pte &= ~(MMU_PAGESIZE-1);
	pte.pg_pte |= offset;
	if (debugmode > 5)
		fprintf(stderr, "cr_vtop64: %08x->%09llx\n", vaddr, pte.pg_pte);
	return (phaddr_t)(pte.pg_pte);
}

/* crash virtual to physical offset address translation */
phaddr_t
cr_vtop(vaddr_t vaddr, int proc, size_t *extentp, boolean_t verbose)
{
	return pae?	cr_vtop64(vaddr, proc, extentp, verbose):
			cr_vtop32(vaddr, proc, extentp, verbose);
}
