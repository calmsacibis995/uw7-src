#ident	"@(#)kern-i386at:svc/memory.c	1.11.5.1"
#ident	"$Header$"

/*
 * Architecture dependent memory handling routines
 * to deal with configration, initialization, and
 * error polling.
 */

#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/pmem.h>
#include <mem/tuneable.h>
#include <proc/cg.h>
#include <proc/cguser.h>
#include <svc/bootinfo.h>
#include <svc/cpu.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/mod_obj.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define equal(a,b)	(strcmp(a, b) == 0)

/*
 * Storage for the pmem_extent lists. UNUSED_MAX must be large
 * enough to accomodate all demands.
 */
#ifdef CCNUMA
#define UNUSED_MAX	(225 + (MAXNUMCG - 1) * 32)
#else /* CCNUMA */
#define UNUSED_MAX 	225
#endif
int pmem_init_count = UNUSED_MAX;
pmem_extent_t pmem_init_array[UNUSED_MAX];

/*
 * Any memnotused chunk with size less than or equal to SMALL_EXTENT
 * is considered a small chunk for allocation purposes (we try to use small
 * chunks before big ones).
 */
#define SMALL_EXTENT		(64 * 1024)

/*
 * We start out assuming that all of physical memory above 4GB is
 * dedicated memory (no matter what DEDICATED_MEMORY is set to).
 */
#define DEFAULT_IDF_BASE	FOUR_GB

/*
 * Limit of physical memory available for calloc()s. Set to 2**32 so
 * that we don't foul non-PAE mode.
 */
#define CALLOC_PMEM_LIMIT	KERNEL_PHYS_LIMIT

/*
 * PHysical address for end of bios occupancy.
 */
#define BIOS_END		(1024 * (1024 + 64))

/*
 * How many CGs does our kernel actually support?
 */
#ifdef CCNUMA
#define CONFIGED_MAX_NCG	MAXNUMCG
#else /* CCNUMA */
#define CONFIGED_MAX_NCG	1
#endif /* CCNUMA */

/*
 * Exported Data.
 *
 */
size_t		totalmem;		/* total real memory (compatibility) */
paddr_t		bios_limit = BIOS_END;	/* end of bios data */
paddr_t		bios_offset;		/* Offset of the bios from 0 */
					/* for machines with displaced */
					/* memory */
/*
 * Only the CCNUMA kernel enables memory above 4GB by default.
 */
#ifdef CCNUMA
boolean_t	enable_4gb_mem = B_TRUE; /* memory above 4gb enabled? */
#else
boolean_t	enable_4gb_mem = B_FALSE; /* memory above 4gb enabled? */
#endif

/*
 * Imported data.
 */
extern void (*reset_code)(void);
extern paddr_t pmaplimit;
extern int bootstring_size;
extern vaddr_t ksym_base;
extern uint_t mem_dedicated;

/*
 * paddr64_t
 * getnextpaddr(uint_t size, uint_t flag)
 *
 *	Return (and consume) size bytes of pages aligned physical memory.
 *	Intended for use by palloc().
 *
 * Calling/Exit State:
 *
 *	Returns the physical address of size bytes of page aligned
 *	physically contiguous memory.
 *
 *	If flag is PMEM_ANY then non-DMAable memory is preferentially
 *	allocated, unless the DMAable list has a small extent (which
 *	we try to use up first).
 *
 *	If flag is PMEM_PHYSIO then only DMAable memory is allocated.
 *
 *	Thus, preference is given to small extents, and to non-DMAable
 *	memory (when permitted).
 */
/* ARGSUSED */
paddr64_t
getnextpaddr(uint_t size, uint_t flag)
{
	paddr64_t ret_paddr = PALLOC_FAIL;
	extern pmem_extent_t *memNOTused[PMEM_TYPES][MAXNUMCG];

	ASSERT(flag == PMEM_ANY || flag == PMEM_PHYSIO);

#ifndef NO_RDMA

	if (flag == PMEM_PHYSIO) {
		cgnum_t cgnum;

		/*
		 * First try to allocate the memory from mycg
		 */
		ret_paddr = phys_palloc4(size,
				&memNOTused[PMEM_DMA][mycg], MMU_PAGESIZE,
				CALLOC_PMEM_LIMIT);

		/*
		 * Now try the other CG.
		 */
		for (cgnum = BOOTCG;
		     ret_paddr == PALLOC_FAIL && cgnum < Ncg;
		     ++cgnum) {
			ret_paddr = phys_palloc4(size,
				&memNOTused[PMEM_DMA][cgnum], MMU_PAGESIZE,
				CALLOC_PMEM_LIMIT);
		}

		if (ret_paddr == PALLOC_FAIL) {
			/*
			 *+ During kernel initialization (boot), there
			 *+ was insufficient physical memory to allocate
			 *+ kernel data structures.  (Note that some
			 *+ physical memory may have been left unused in order
			 *+ to satisfy alignment and contiguity requirements.)
			 *+ Corrective action:  Add more physical memory.
			 */
			phys_panic("palloc(PMEM_PHYSIO): ran out "
					  "of physical memory");
		}

		return ret_paddr;
	}

	if (memNOTused[PMEM_DMA][mycg] != NULL &&
	     memNOTused[PMEM_DMA][mycg]->pm_extent < SMALL_EXTENT)
		ret_paddr = phys_palloc4(size,
				&memNOTused[PMEM_DMA][mycg], MMU_PAGESIZE,
				CALLOC_PMEM_LIMIT);

	if (ret_paddr == PALLOC_FAIL)
		ret_paddr = phys_palloc4(size,
				&memNOTused[PMEM_NODMA][mycg], MMU_PAGESIZE,
				CALLOC_PMEM_LIMIT);

#endif /* NO_RDMA */

	if (ret_paddr == PALLOC_FAIL)
		ret_paddr = phys_palloc4(size,
				&memNOTused[PMEM_DMA][mycg], MMU_PAGESIZE,
				CALLOC_PMEM_LIMIT);

	if (ret_paddr == PALLOC_FAIL) {
		/*
		 *+ During kernel initialization (boot), there
		 *+ was insufficient physical memory to allocate
		 *+ kernel data structures.  (Note that some
		 *+ physical memory may have been left unused in order
		 *+ to satisfy alignment and contiguity requirements.)
		 *+ Corrective action:  Add more physical memory.
		 */
		phys_panic("palloc: ran out of physical memory");
	}

	return ret_paddr;
}

/*
 * STATIC int
 * mem_scan1(const char *s, const char *p, void *dummy)
 *	Acquire ENABLE_4GB_MEM and PMAPLIMIT.
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 *
 * Remarks:
 *	The ENABLE_4GB_MEM should become "#ifdef UNIPROC"
 *	if it is determined that UNIPROC systems need not
 *	support PAE mode.
 */

/* ARGSUSED */
STATIC int
mem_scan1(const char *s, const char *p, void *dummy)
{
	long limit;

	if (equal(s, "ENABLE_4GB_MEM")) {
		if (p[0] == 'Y' || p[0] == 'y')
			enable_4gb_mem = B_TRUE;
		else
			enable_4gb_mem = B_FALSE;
		PHYS_PRINT("bs_pstart: enable_4gb_mem = %x\n", enable_4gb_mem);
	} else if (equal(s, "PMAPLIMIT")) {
		if (bs_lexnum(p, &limit)) {
			pmaplimit = limit;
			PHYS_PRINT("mem_scan1: pmaplimit = %x\n", pmaplimit);
		}
	} else if (equal(s, "BIOS_OFFSET")) {
		if (bs_lexnum(p, &limit)) {
			bios_offset = limit;
			PHYS_PRINT("mem_scan1: bios_offset = %x\n",
				   bios_offset);
			bios_limit += bios_offset;
		}
	}

	return 0;
}

STATIC memsize_t
mem_sizer(const char **pp)
{
	ulong_t size;
	memsize_t llsize;

	size = strtoul(*pp, (char **) pp, 10);
	llsize = size;
	switch (**pp) {
	case 'K':
		llsize *= ONE_KB;
		(*pp)++;
		break;
	case 'M':
		llsize *= ONE_MB;
		(*pp)++;
		break;
	case 'G':
		llsize *= ONE_GB;
		(*pp)++;
		break;
	default:
		break;
	}

	return llsize;
}

/*
 * STATIC boolean_t
 * mem_scan_equal(const char *bstrp, const char *wordp)
 *	Check if the token given by wordp is in the string given by
 *	bstrp. The token can either occur at the end of the string,
 *	or may be followed by a comma.
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 */

STATIC boolean_t
mem_scan_equal(const char *bstrp, const char *wordp)
{
	const char *p;
	int result;

	p = strchr(bstrp, ',');
	if (p == NULL)
		result = strcmp(bstrp, wordp);
	else
		result = strncmp(bstrp, wordp, p - bstrp);

	return result == 0;
}

/*
 * STATIC int
 * mem_scan2(const char *s, const char *p, void *dummy)
 *	Scan the MEMORY parameter of the boot arguments.
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 */

/* ARGSUSED */
STATIC int
mem_scan2(const char *s, const char *p, void *dummy)
{
	extern struct modobj *mod_obj_kern;

	if (equal(s, "MEMORY")) {
		const char *use;
		pmem_use_t mem_use;
		int k;
		paddr64_t byte_base, byte_end, base, end;

#ifdef PHYS_DEBUG
	PHYS_PRINT("mem_scan2: MEMORY=%s\n", p);
#endif
		while (*p) {
			byte_base = mem_sizer(&p);
			if (*p++ != '-')
				break;
			byte_end = mem_sizer(&p);

			/*
			 * We can't page to partial pages. So mark
			 * them as reserved. Note that the boot
			 * loader will never put anything in a partial
			 * page.
			 */
			base = ptob64(btopr64(byte_base));
			end = ptob64(btop64(byte_end));
			if (byte_base < base) {
				pmem_list_union_range(&memUsed, byte_base,
					base, BOOTCG, PMEM_RESERVED);
			}
			if (byte_end > end) {
				pmem_list_union_range(&memUsed, end,
					byte_end, BOOTCG, PMEM_RESERVED);
			}
			if (base == end)
				goto next;
			if (*p == ':') {
				use = ++p;
				if (mem_scan_equal(use, "boot")) {
					mem_use = PMEM_BOOT;
				} else if (mem_scan_equal(use, "bki")) {
					mem_use = PMEM_BKI;
					bootstring_size = end - base;
				} else if (mem_scan_equal(use, "ktext")) {
					mem_use = PMEM_KTEXT;
				} else if (mem_scan_equal(use, "kdata") ||
					 mem_scan_equal(use, "kdata2"))
					mem_use = PMEM_KDATA;
				else if (mem_scan_equal(use, "resmgr")) {
					mem_use = PMEM_RESMGR;
				} else if (mem_scan_equal(use, "memfs.meta")) {
					mem_use = PMEM_MEMFSROOT_META;
				} else if (mem_scan_equal(use, "memfs.fs")) {
					mem_use = PMEM_MEMFSROOT_FS;
				} else if (mem_scan_equal(use, "kdb.rc")) {
					extern char *kdbcommands;

					kdbcommands = (char *) base;
					mem_use = PMEM_KDEBUG;
				} else if (mem_scan_equal(use, "license")) {
					mem_use = PMEM_LICENSE;
				} else if (mem_scan_equal(use, "ksym")) {
					mem_use = PMEM_KSYM;
					ksym_base = base;
					mod_obj_kern->md_symspace += ksym_base;
					mod_obj_kern->md_strings += ksym_base;
				} else if (mem_scan_equal(use, "reserved")) {
					mem_use = PMEM_RESERVED;
				} else {
					mem_use = PMEM_RESERVED;
					PHYS_PRINT("mem_scan2: use unknown "
						   "<0x%Lx, 0x%Lx> use %d\n",
						   base, end, mem_use);
				}
				pmem_list_union_range(&memUsed, base, end,
						      BOOTCG, mem_use);
			} else {
				declare_mem_free(base, end, BOOTCG);
			}
next:
			if ((p = strchr(p, ',')) == NULL)
				break;
			++p;
		}
	}
	return 0;
}

/*
 * STATIC int
 * mem_scan3(const char *s, const char *p, void *dummy)
 *	Acquire the address of the pstart memory.
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 *
 * Remarks:
 *	Most of this function should become "#ifdef UNIPROC" (i.e.
 *	all but the PMAPLIMIT piece) if it is determined that UNIPROC
 *	systems need not support PAE mode.
 */

/* ARGSUSED */
STATIC int
mem_scan3(const char *s, const char *p, void *dummy)
{
	long pstart;

	if (equal(s, "PSTART")) {
		if (bs_lexnum(p, &pstart)) {
			reset_code = (void(*)(void)) pstart;
			pmem_list_subtract_range(&memUsed, pstart,
				pstart + PAGESIZE);
			pmem_list_union_range(&memUsed, pstart,
				pstart + PAGESIZE, BOOTCG, PMEM_PSTART);
		}
		PHYS_PRINT("mem_scan3: pstart = %x\n", pstart);
	}
	return 0;
}

/*
 * void
 * find_unused_mem(void)
 *	Parse the MEMORY string passed to us by the boot loader.
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 */
void
find_unused_mem(void)
{
	extern boolean_t pae_supported(void);

	/*
	 * Initialize the pmem extent allocator.
	 */
	PHYS_PRINT("find_unused_mem: enter\n");
	pmem_extent_init();
	PHYS_PRINT("find_unused_mem: pmem_extent_init returned\n");

	/*
	 * Use PAE mode only if enabled by the boot loader flag
	 * ENABLE_4GB_MEM and our engine supports PAE mode.
	 */
	PHYS_PRINT("find_unused_mem: enter\n");
	bs_scan(mem_scan1, NULL);
	PHYS_PRINT("find_unused_mem: mem_scan1 returned\n");
	PHYS_ASSERT(pmaplimit != 0);
#ifdef CCNUMA
	if (!pae_supported()) {
		phys_panic("The CCNUMA kernel requires processor support "
			   "for PAE mode.");
	}
#elif defined(PAE_MODE)
	if (enable_4gb_mem) {
		if (!pae_supported()) {
			printf("WARNING: not using memory above 4GB.\n"
			       "         no PAE support in this processor.\n");
		} else {
			using_pae = B_TRUE;
		}
	}
#else /* !PAE_MODE */
	if (enable_4gb_mem) {
		printf("WARNING: not using memory above 4GB.\n"
		       "         no PAE support in this kernel.\n");
	}
#endif /* PAE_MODE */
	PHYS_PRINT("find_unused_mem: using_pae = %d\n", using_pae);
	bs_scan(mem_scan2, NULL);
	PHYS_PRINT("find_unused_mem: mem_scan2 returned\n");
	bs_scan(mem_scan3, NULL);
	PHYS_PRINT("find_unused_mem: mem_scan3 returned\n");
	PHYS_ASSERT(reset_code != NULL);
	pmem_total();
	PHYS_PRINT("find_unused_mem: pmem_total returned\n");
#ifdef PHYS_DEBUG
	pmem_physprint_lists();
#endif /* PHYS_DEBUG */
}

/*
 * void
 * declare_mem_free(paddr64_t base, paddr64_t end, cgnum_t cgnum)
 *	Add an unused fragment to the appropriate mem_list.
 *	The caller does not know if the fragment is DMAable,
 *	non-DMAable, or a little of each. This functions
 *	determines that.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single-threaded, so no
 *	locks are needed.
 *
 * Description:
 *	If the fragment contains both DMAable and non-DMAable pages,
 *	then segregate into two fragments at the tune.t_dmalimit
 *	boundary. Also segregate all idf memory.
 */
void
declare_mem_free(paddr64_t base, paddr64_t end, cgnum_t cgnum)
{
#ifndef NO_RDMA
	pmem_type_t type = PMEM_NODMA;
#else /* NO_RDMA */
	const pmem_type_t type = PMEM_DMA;
#endif /* NO_RDMA */

	/*
	 * If not using PAE mode, of if ENABLE_4GB_MEM is not set,
	 * then reserve all memory above 4GB.
	 */
	if (!using_pae) {
		if (end > FOUR_GB) {
			paddr64_t fourg_base = MAX(base, FOUR_GB);

			pmem_list_union_range(&memUsed, fourg_base,
					      end, cgnum, PMEM_RESERVED);

			/*
			 * Compute what remains.
			 */
			end = fourg_base;
			if (end == base)
				return;
		}
	}

	/*
	 * Lop off idf memory first. If memory is IDF, then
	 * we have no interest in its DMAability status.
	 */
	if (end > DEFAULT_IDF_BASE) {
		paddr64_t idf_base = MAX(base, DEFAULT_IDF_BASE);

		pmem_list_union_range(&memNOTused[PMEM_IDF][cgnum],
				      idf_base, end, cgnum, PMEM_FREE_IDF);

		/*
		 * Compute what remains.
		 */
		end = idf_base;
		if (end == base)
			return;
	}

#ifndef NO_RDMA
	/*
	 * Chop off DMAable memory.
	 */
	if (tune.t_dmalimit == 0) {
		type = PMEM_DMA;
	} else if (base < ptob64(tune.t_dmalimit)) {
		paddr64_t dma_end = MIN(ptob64(tune.t_dmalimit), end);

		pmem_list_union_range(&memNOTused[PMEM_DMA][cgnum],
				      base, dma_end, cgnum, PMEM_FREE);

		/*
		 * Compute what remains.
		 */
		base = dma_end;
		if (base == end)
			return;
	}
#endif /* NO_RDMA */

	/*
	 * Finally, the non-DMAable (or default type) last piece.
	 */
	pmem_list_union_range(&memNOTused[type][cgnum], base, end, cgnum,
			      PMEM_FREE);
}

/*
 * void
 * mem_parse_topology(void)
 *	Parse the MEMORY informatiion given to us by the PSM.
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 */
void
mem_parse_topology(void)
{
	pmem_type_t type;
	ms_resource_t *rp;
	memsize_t idfcgmem;
	memsize_t dedicated_memory;
	paddr64_t big_page;
	cgnum_t cgno;
	int i, j, k, cpu_slot;
	processorid_t cpuid;
	processorid_t *p;
	extern int gToLeader[];
	extern cgnum_t ctog[];
	extern cgid_t cgids[];

	for (i = 0; i < MAXNUMCG; ++i) {
		for (j = 0; j < MAXNUMCPU; ++j)
			cg_array[i].cg_cpuid[j] = -1;
		gToLeader[i] = -1;
	}
	for (i = 0; i < MAXNUMCPU; ++i)
		ctog[i] = CG_NONE;

	/*
	 * First, empty out all exists lists except for the
	 * "memory used" (memUsed) list.
	 */
	pmem_list_free(&totalMem);
        for (i = 0; i < MAXNUMCG; ++i)
                for (type = PMEM_DMA; type <= PMEM_IDF; ++type)
			pmem_list_free(&memNOTused[type][i]);

	/*
	 * Now, start filling up the lists with information from
	 * the PSM.
	 *
	 * Also remember to keep some statistics for later on.
	 */
	rp = &os_topology_p->mst_resources[0];
	for (i = 0; i < os_topology_p->mst_nresource; ++i, ++rp) {
		if (rp->msr_type == MSR_MEMORY) {
			if ((rp->msri.msr_memory.msr_flags & MSR_RESERVED) ||
			    rp->msr_private ||
			    rp->msr_private_cpu != MS_CPU_ANY ||
			    rp->msr_cgnum >= CONFIGED_MAX_NCG) {
				pmem_list_union_range(&memUsed,
					rp->msri.msr_memory.msr_address,
					rp->msri.msr_memory.msr_address +
					rp->msri.msr_memory.msr_size,
					rp->msr_cgnum, PMEM_RESERVED);
			} else {
				declare_mem_free(
					rp->msri.msr_memory.msr_address,
					rp->msri.msr_memory.msr_address +
					rp->msri.msr_memory.msr_size,
					rp->msr_cgnum);
				global_memsize.tm_cg[rp->msr_cgnum].tm_useable
					+= rp->msri.msr_memory.msr_size;
				global_memsize.tm_global.tm_useable
					+= rp->msri.msr_memory.msr_size;
			}
			global_memsize.tm_cg[rp->msr_cgnum].tm_total
				+= rp->msri.msr_memory.msr_size;
			global_memsize.tm_global.tm_total
				+= rp->msri.msr_memory.msr_size;
			continue;
		}

		if (rp->msr_cgnum >= CONFIGED_MAX_NCG) {
			if (rp->msr_type == MSR_CG) {
				printf("WARNING: ignoring CG %d\n",
				       rp->msr_cgnum);
			}
			continue;
		}

		if (rp->msr_private ||
		    rp->msr_private_cpu != MS_CPU_ANY) {
			continue;
		}

		/*
		 * Count up the Cpu Groups and fill in information
		 * regarding the Cpu Groups.
		 */
		if (rp->msr_type == MSR_CG) {
			cgids[rp->msr_cgnum] = rp->msri.msr_cg.msr_cgid;
			cg_array[rp->msr_cgnum].cg_status = CG_CONFIGURED;
			++Ncg;
		} else if (rp->msr_type == MSR_CPU) {
			if (rp->msri.msr_cpu.msr_cpuid >= MAXNUMCPU) {
				printf("WARNING: ignoring CPU %d\n",
				       rp->msri.msr_cpu.msr_cpuid);
				continue;
			}
			cgno = rp->msr_cgnum;
			cpu_slot = cg_array[cgno].cg_cpunum++;
			cpuid = rp->msri.msr_cpu.msr_cpuid;
			PHYS_ASSERT(cpuid >= 0);

			/*
			 * Maintain the list of cpu for each CG in sorted
			 * order so that the leader always ends up at
			 * the lowest number CPU.
			 */
			p = &cg_array[cgno].cg_cpuid[0];
			for (j = 0; j < cpu_slot && cpuid > p[j]; ++j)
				;
			PHYS_ASSERT(cpuid != p[j]);
			for (k = cpu_slot; k > j; --k)
				p[k] = p[k - 1];
			p[j] = cpuid;

			/*
			 * Initialize the "cpu to group" mapping.
			 */
			ctog[cpuid] = cgno;
		}
	}

	/*
	 * We have to have at least one CPU group.
	 */
	if (Ncg == 0) {
		Ncg = 1;
		cgids[BOOTCG] = 0;
		cg_array[BOOTCG].cg_cpuid[0] = BOOTENG;
	}

	/*
	 * Apoint CG leaders.
	 */
	for (i = 0; i < CONFIGED_MAX_NCG; ++i)
		gToLeader[i] = cg_array[i].cg_cpuid[0];

#ifdef DEBUG
	/*
	 * Assert that:
	 *	1. The usable CGs represent a dense cgnum space.
	 *	2. That every CG has at least one engine.
	 *	3. That CG 0 contains engine 0.
	 *	5. That gToLeader really points at the leader.
	 *	4. That ctog really points to the Cpu Group.
	 */
	for (i = 0; i < Ncg; ++i) {
		PHYS_ASSERT(cgids[i] != CG_BAD);
		PHYS_ASSERT(cg_array[i].cg_cpuid[0] != -1);
		PHYS_ASSERT(gToLeader[i] == cg_array[i].cg_cpuid[0]);
		for (j = 0; j < MAXNUMCPU && cg_array[i].cg_cpuid[j] != -1; ++j)
			PHYS_ASSERT(ctog[cg_array[i].cg_cpuid[j]] == i);
	}
	PHYS_ASSERT(cg_array[0].cg_cpuid[0] == 0);
	for (i = Ncg; i < MAXNUMCG; ++i)
		ASSERT(cgids[i] == CG_BAD);
#endif

	/*
	 * Now, subtract the used list from each one of the free lists.
	 */
        for (i = BOOTCG; i < Ncg; ++i)
                for (type = PMEM_DMA; type <= PMEM_IDF; ++type)
			pmem_list_subtract(&memNOTused[type][i], &memUsed);

	/*
	 * Calculate how much IDF memory we now have.
	 */
	for (i = BOOTCG; i < Ncg; ++i) {
		idfcgmem = pmem_list_size(&memNOTused[PMEM_IDF][i], PMEM_NONE);
		global_memsize.tm_cg[i].tm_dedicated = idfcgmem;
		global_memsize.tm_global.tm_dedicated += idfcgmem;
	}

	/*
	 * Calculate the DEDICATED_MEMORY shortfall. Take from memory
	 * below 4GB to take up the shortfall.
	 */
	big_page = 0;
	dedicated_memory = ptob64(mem_dedicated);
	while (global_memsize.tm_global.tm_dedicated < dedicated_memory) {
		big_page = pse_palloc(&cgno, KERNEL_PHYS_LIMIT);
		if (big_page == PALLOC_FAIL) {
			phys_printf("WARNING: unable to allocate %d pages of"
				    " dedicated memory\n", mem_dedicated);
			break;
		}
		pmem_list_subtract_range(&memUsed,
			big_page, big_page + PSE_PAGESIZE);
		pmem_list_union_range(&memNOTused[PMEM_IDF][cgno],
			big_page, big_page + PSE_PAGESIZE, cgno, PMEM_FREE_IDF);
	}

	/*
	 * Now build the list containing everything.
	 */
	pmem_total();

#ifdef PHYS_DEBUG
	pmem_physprint_lists();
#endif /* PHYS_DEBUG */
}

#if defined(PHYS_DEBUG) || defined(lint)
/*
 * void
 * pmem_physprint_list(struct pmem_extent *mnup)
 * 	Print out a memory list.
 *
 * Calling/Exit State:
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 */
void
pmem_physprint_list(struct pmem_extent *mnup)
{
	while (mnup != NULL) {
		PHYS_PRINT("<0x%Lx, 0x%Lx> cg %d, use %d\n", mnup->pm_base,
			   mnup->pm_base + mnup->pm_extent, mnup->pm_cg,
			   mnup->pm_use);
		mnup = mnup->pm_next;
	}
}

/*
 * void
 * pmem_physprint_lists(struct pmem_extent *mnup)
 * 	Print out all physical memory lists.
 *
 * Calling/Exit State:
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 */
void
pmem_physprint_lists(void)
{
	cgnum_t cgnum;
	pmem_type_t type;

	PHYS_PRINT("memUsed:\n");
	pmem_physprint_list(memUsed);
	PHYS_PRINT("totalMem:\n");
	pmem_physprint_list(totalMem);
	for (cgnum = BOOTCG; cgnum < MAXNUMCG; ++cgnum) {
		for (type = PMEM_DMA; type <= PMEM_IDF; ++type) {
			if (memNOTused[type][cgnum] != NULL) {
				PHYS_PRINT("unused memory for "
					   "type(%x) cg(%x)\n", type, cgnum);
				pmem_physprint_list(memNOTused[type][cgnum]);
			}
		}
	}
}
#endif /* PHYS_DEBUG || lint */

#ifdef DEBUG
/*
 * void
 * pmem_physprint_list(struct pmem_extent *mnup)
 * 	Print out a memory list.
 *
 * Calling/Exit State:
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 */
void
print_pmem_list(struct pmem_extent *mnup)
{
	while (mnup != NULL) {
		debug_printf("<0x%Lx, 0x%Lx> cg %d, use %d\n", mnup->pm_base,
			     mnup->pm_base + mnup->pm_extent, mnup->pm_cg,
			     mnup->pm_use);
		mnup = mnup->pm_next;
	}
}
#endif
