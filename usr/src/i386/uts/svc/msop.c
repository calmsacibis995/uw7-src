#ident	"@(#)kern-i386:svc/msop.c	1.1.9.3"
#ident	"$Header$"

#include <util/inline.h>
#include <mem/kmem.h>
#include <mem/hatstatic.h>
#include <mem/pmem.h>
#include <svc/psm.h>
#include <svc/bootinfo.h>
#include <svc/systm.h>
#include <svc/v86bios.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/cmn_err.h>
#include <util/mod/moddrv.h>
#include <proc/cg.h>
#include <proc/cguser.h>

extern ms_cpu_t		os_this_cpu;			/* a per-CPU variable */
extern char		kvpage0;	/*  pointer to page 0 */
extern boolean_t	enable_4gb_mem;	/* memory above 4GB enabled? */
extern unsigned int 	p6splitmem;

ms_intr_dist_t		intr_dist_nop;
ms_intr_dist_t		*os_intr_dist_nop=&intr_dist_nop;
ms_intr_dist_t		*os_intr_dist_stray; 		/* set in sysinit */

ms_intr_dist_t 		*(*uw_flhdlr_table[256])(ms_ivec_t vec);
void*			msops[MSOP_NUMBER_OF_MSOPS];	/* jump table for calling PSM routines. */

/*
 * Name of PSM module to load. If not NULL, only the PSM with the specified
 * name will be loaded. May be specified with a boot arg.
 */
char			*psmname=NULL;

/*
 * The following kernel variables are exported TO the PSM. The values of these
 * variables must be set prior to call the initpsm function.
 */
void			*os_page0;		/* pointer to physical page 0 */
ms_time_t		os_tick_period={(1000000000/HZ), 0}; /* 10 ms tick if HZ = 100 */

/*
 * The following are set by calls to os_set_msparam from PSM
 * when a PSM is loaded.
 */

char		*os_platform_name="(unknown)";	/* Name of platform. */
ms_cpu_t	os_ncpus=-1;		/* number of cpus physically present */
ms_time_t	os_time_res;		/* resolution of free running clock */
ms_time_t	os_tick_1_res;		/* resolution of tick 1 clock */
ms_time_t	os_tick_1_max;		/* maximun value of tick 1 clock */
ms_time_t	os_tick_2_res;		/* resolution of tick 2 clock */
ms_time_t	os_tick_2_max;		/* maximum value of tick 2 clock */
ms_islot_t	os_islot_max=0;		/* maximum islot supported by PSM */
ms_cgnum_t	os_cgnum_max=0;		/* maximum CG number supported by PSM */ 
unsigned int	os_intr_order_max=0;	/* maximum interrupt priority */
unsigned int	os_intr_taskpri_max=0;	/* maximum task priority */
ms_size_t	os_farcopy_min=0xffffffff;
			/* use ms_farcopy if block size exceeds this value */
unsigned int	os_shutdown_caps=0;	/* supported shutdown capabilities */
ms_topology_t	*os_topology_p=NULL;	/* system topology structure */
ms_topology_t	*os_default_topology = NULL;
char		*os_hw_serial = "no-serial";	/* machine serial number */


/*
 * The following are for the convenience of the base kernel.
 * They are set based on other msparam values.
 */
ms_cpu_t	os_cpu_max=0;		/* maximum cpu number, one
					 * less than number of cpus */

extern			msop_func_t msop_msops_default[];

/* For configuring memory above 4GBs */
extern unsigned int p6splitmem;

/*
 * ms_intr_dist_t*
 * msop_stray_interrupt
 *	Used for handling unclaimed IO vectors.
 *
 * Calling/Exit State:
 *	Returns ms_intr_dist_stray.
 */
/* ARGSUSED */
ms_intr_dist_t*
msop_stray_interrupt(ms_ivec_t vec)
{
	os_intr_dist_stray->msi_slot = 0xffff;		/* ZZZ unknown/null slot */
	return (os_intr_dist_stray);
}

STATIC ms_topology_t * build_default_topology();
/*
 * int
 * initpsm(msop_func_t *op)
 *	Called by kernel initialization routines to
 *	load a PSM module.
 *
 * Calling/Exit State:
 *	Returns 
 *		 0	if a PSM was loaded
 *		>0	if no PSM was loaded
 *	
 */
ms_bool_t
initpsm()
{
	int (**funcp)(char*), i;
	extern int (*initpsm_funcs[])();
	extern char *modname_initpsm[];
	ms_resource_t *rp;

	os_default_topology = build_default_topology();

	os_page0 = (void*)&kvpage0;
	os_register_msops(msop_msops_default);
		
	/*
	 * Initialize the interrupt vector handler table.
	 */
	for (i = 0; i < 255; i++)
		uw_flhdlr_table[i] = msop_stray_interrupt;

	/*
	 * Scan thru initpsm_funcs table trying to load a PSM.
	 * Exit when successful.
	 */
	for (i = 0; initpsm_funcs[i] != NULL; i++) {
		if (psmname && strcmp(modname_initpsm[i], psmname) != 0)
			continue;
		if ((*initpsm_funcs[i])()) {
			psmname = modname_initpsm[i];
			goto found;
		}
	}

	return (MS_FALSE);

found:
	/*
	 * Scan the topology structure to find out the highest
	 * cpu and cg index.
	 */
       	for (i = 0; i < os_topology_p->mst_nresource; i++) {
		rp = &os_topology_p->mst_resources[i];
		if (rp->msr_type == MSR_CPU) {
			if (rp->msri.msr_cpu.msr_cpuid > os_cpu_max)
				os_cpu_max = rp->msri.msr_cpu.msr_cpuid;
		} else {
			if (rp->msr_type == MSR_CG) {
				if (rp->msr_cgnum > os_cgnum_max)
					os_cgnum_max = rp->msr_cgnum;
			}
		}
	}

#ifdef DEBUG
	cmn_err(CE_CONT, "Using %s PSM for %s platform\n",
		psmname, os_platform_name);
#endif

	return (MS_TRUE);	
}



/*
 * ms_boot_t
 * os_register_msops(msop_func_t *op)
 *	Called by psm_init routines to
 *	register a PSM function to be called for a specific MSOP.
 *
 * Calling/Exit State:
 *	Registers all valid MSOPs in the array.
 *	Returns 
 *		MS_TRUE  -  if all MSOP are valid. 
 *		MS_FALSE -  if an unsupported MSOP was found
 *
 * Note; if an invalid msop is found, no changes to the msop table will
 * 	have been made.  This is required to prevent side-effects when
 *	a PSM fails to load because of unsupported msops.
 *	
 */
ms_bool_t
os_register_msops(msop_func_t *op)
{
	msop_func_t		*opp;

	for(opp=op; opp->msop_op; opp++)
		if (opp->msop_op >= MSOP_NUMBER_OF_MSOPS)
			return (MS_FALSE);


	for(opp=op; opp->msop_op; opp++)
		msops[opp->msop_op] = opp->msop_func;

	return (MS_TRUE);
}


/*
 * OS interface for PSM routines. These routine are the ONLY os-callable
 * routines for the PSM.
 * 
 */


/*
 * void *
 * os_alloc(ms_size_t sz)
 *	Allocates at least <sz> bytes of memory (with no alignment
 *	guarantees or special properties).
 *
 * Calling/Exit State:
 *	Returns NULL on failure. This call will not wait for
 *	memory if it is not immediately available.
 *
 *	May be called from PSM init and unload code, or
 *	from the following MSOP entry points: 
 *		MSOP_xxx_INIT, MSOP_ONLINE, MSOP_INTR_ATTACH,
 *		MSOP_INTR_DETACH.
 *
 *	May not be called while holding a lock.
 */
void *os_alloc(ms_size_t sz)
{
	if (hat_static_callocup)
		return(calloc(sz));
	else
		return(kmem_alloc((size_t)sz, KM_NOSLEEP));
}


/*
 * void *
 * os_alloc_pages(ms_size_t sz)
 *	Allocates page-aligned contiguous memory for enough
 *	whole pages to contain <sz> bytes.  
 *
 * Calling/Exit State:
 *	Returns NULL on failure. This call will not wait for
 *	memory if it is not immediately available.
 *
 *	May be called from PSM initialization and unload code, or
 *	from the following MSOP entry points:
 *	 	MSOP_xxx_INIT, MSOP_ONLINE, MSOP_INTR_ATTACH,
 *		MSOP_INTR_DETACH.
 */
void *os_alloc_pages(ms_size_t sz)
{
	physreq_t 	*physreqp;
	void		*p;

	if (hat_static_callocup) {
		callocrnd(MMU_PAGESIZE);
		return(calloc_physio(sz));
	} else {
		if ((physreqp = physreq_alloc(KM_NOSLEEP)) == NULL)
			return (NULL);
		physreqp->phys_align = PAGESIZE;
		physreqp->phys_flags |= PREQ_PHYSCONTIG;
		if (physreq_prep(physreqp, KM_NOSLEEP) == B_FALSE) {
			physreq_free(physreqp);
			return NULL;
		}
		p =  kmem_alloc_physreq((size_t)sz, physreqp, KM_NOSLEEP);
		physreq_free(physreqp);
		return(p);
	}
}


/*
 * void
 * os_free(void *addr, ms_size_t sz)
 *	Free memory (if possible) previously allocated by (*os_alloc) or
 *	(*os_alloc_pages).
 *
 * Calling/Exit State:
 *	None.
 */
void os_free(void *addr, ms_size_t sz)
{
	if (!hat_static_callocup)
		kmem_free(addr, (size_t)sz);
}


/*
 * char *
 * os_get_option(const char *)
 *	Looks for a option parameter with the specified name and returns 
 *	its value as a string.
 *
 * Calling/Exit State:
 *	Returns NULL if the option is not found. 
 *
 * Description:
 *	Implemented by bs_getval() and remapped in the psm.2 interface file.
 */


/*
 * void
 * os_printf(const char *, ...)
 *	Print an informative message on the operator console.
 *
 * Calling/Exit State:
 *	None
 *
 * Description:
 *	Implemented by printf() and remapped in the psm.2 interface file.
 */


/*
 * void *
 * os_physmap(ms_paddr_t, ms_size_t)
 *	Allocates a virtual address for mapping a given range
 *	of physical addresses. On systems with caches, the mapping will
 *	bypass the cache.
 *
 * Calling/Exit State:
 *	Returns NULL on failure.
 */
void *os_physmap(ms_paddr_t physaddr, ms_size_t sz)
{
	return (physmap(physaddr, sz, KM_NOSLEEP));
}


/*
 * void
 * os_physmap_free(void*, ms_size_t)
 *	Releases a mapping allocated by a previous call to os_physmap.
 *
 * Calling/Exit State:
 *	None
 */
void os_physmap_free(void *vaddr, ms_size_t sz)
{
	physmap_free (vaddr, sz, KM_NOSLEEP);
}


/*
 * void
 * os_claim_vectors(ms_ivec_t basevec, ms_ivec_t nvec,
 *			ms_intr_dist_t *(*func)(ms_ivec_t vec))
 *	Claim a range of CPU interrupt vectors to be handled by the PSM.
 *	These would be the interrupt vectors numbers used by the interrupt
 *	controller circuitry.
 *
 * Calling/Exit State:
 *	None.
 *	May be called multiple times and for already-claimed vectors
 *	to change the mapping assigned to the vectors.
 *	Only affects the cpu on which the call is made.
 */
void
os_claim_vectors(ms_ivec_t basevec, ms_ivec_t nvec, 
    ms_intr_dist_t *(*func)(ms_ivec_t vec))
{
	int i;
	for (i = basevec; i < (basevec + nvec); i++)
		uw_flhdlr_table[i] = func;
}


/*
 * void
 * os_unclaim_vectors(ms_ivec_t basevec, ms_ivec_t nvec)
 *	Release a range of CPU interrupt vectors privously claimed
 *	with os_claim_vectors.
 *	Affects ONLY the cpu on which it is called.
 *
 * Calling/Exit State:
 *	None.
 */
void
os_unclaim_vectors(ms_ivec_t basevec, ms_ivec_t nvec)
{
	int i;

	for (i = basevec; (i < basevec + nvec); i++)
		uw_flhdlr_table[i] = msop_stray_interrupt;
}


/*
 * void
 * os_post_events(ms_event_t events)
 *	Notify the OS of one or more special events which have occurred
 *	since the last time os_post_events was called. The indicated
 *	bitmask of events will be logically ORed into a bitmask of events
 *	the OS has yet to process for the current CPU.
 *
 * Calling/Exit State:
 *	Called with IE flag off and it remains off throughout the call.
 *
 *	The events must not include MS_EVENT_PSM_xxx.
 *
 * 	It is only called from the PSM interrupt delivery functions. This
 *	allows the OS to limit the places it checks for posted events.
 *	Only delivery functions that return os_intr_dist_nop or an
 *	m_intr_dist_t with MSI_EVENTS flag set.
 */
void
os_post_events(ms_event_t events)
{
	l.msevents |= events;
}

/*
 * uint_t
 * himem_resources_physmem()
 *
 * 	Return the total amount of memory rounded to the next MB present on the  *      system by scanning the os_default_topology structure.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC memsize_t
himem_resources_physmem()
{
	memsize_t physmem = 0;
	ms_topology_t	*topo = os_default_topology;
	uint_t	n;
	
        for(n = 0; n < topo->mst_nresource; n++ ) {
		ms_resource_t *rp = &topo->mst_resources[n];
		
                if(rp->msr_type == MSR_MEMORY)
			physmem += rp->msri.msr_memory.msr_size;
        }

	return ((physmem + ONE_MB - 1) / ONE_MB) * ONE_MB;
}

/*
 * void
 * himem_resources_relocate(memsize_t half, ms_resource_t *dst)
 *
 * 	Relocate all memory resources (base, size) such that base > half
 * 	to (base - half + 4GB, size).
 *
 * 	If there is a resource that crosses this boundary, it will be split. 
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
himem_resources_relocate(memsize_t half, ms_topology_t *psm_tp, 
			 ms_resource_t *dst)
{
	uint_t	n;

	/* Fill in default value for the extra resource */
	dst->msr_cgnum = 0;
	dst->msr_private = 0;
	dst->msr_private_cpu = MS_CPU_ANY;
	dst->msri.msr_memory.msr_size = 0;
	dst->msri.msr_memory.msr_address = 0;
	dst->msri.msr_memory.msr_flags = 0;
	dst->msr_type = MSR_MEMORY;

        for(n = 0; n < psm_tp->mst_nresource; n++ ) {
		ms_resource_t *rp = &psm_tp->mst_resources[n];
		
                if(rp->msr_type != MSR_MEMORY)
			continue;

		if (rp->msri.msr_memory.msr_address > half) {
			if (rp->msri.msr_memory.msr_address < FOUR_GB) {
				rp->msri.msr_memory.msr_address =
					rp->msri.msr_memory.msr_address - half
					+ FOUR_GB;
			}
		} else if ((rp->msri.msr_memory.msr_address +
			     rp->msri.msr_memory.msr_size) > half) {
			/* needs a split */

			/* Fill in the new memory resource */
			dst->msri.msr_memory.msr_size =
				rp->msri.msr_memory.msr_address +
				rp->msri.msr_memory.msr_size - half;
			dst->msri.msr_memory.msr_address = FOUR_GB;

			rp->msri.msr_memory.msr_size = half -
				rp->msri.msr_memory.msr_address;

		}
        }
}

/*
 * void
 * os_set_msparam(msparam_t msparam, void *valuep)
 *	Used to set values of OS parameters. Must be called during
 *	the PSM load phase to set any OS parameters that differ
 *	from default values. Values cannot be subsequently changed.
 *
 * Calling/Exit State:
 *	None.
 */
void
os_set_msparam(msparam_t msparam, void *valuep)
{
	unsigned int	i;
	memsize_t	physmem;
	ms_resource_t	*rp;

	
	switch(msparam) {
	case MSPARAM_PLATFORM_NAME:
		os_platform_name = (char *)valuep;
		break;
	case MSPARAM_SW_SYSDUMP:
		os_soft_sysdump = *(ms_bool_t *)valuep;
		break;
	case MSPARAM_TIME_RES:
		os_time_res = *(ms_time_t*)valuep;
		break;
	case MSPARAM_TICK_1_RES:
		os_tick_1_res = *(ms_time_t*)valuep;
		break;
	case MSPARAM_TICK_1_MAX:
		os_tick_1_max = *(ms_time_t*)valuep;
		break;
	case MSPARAM_TICK_2_RES:
		os_tick_2_res = *(ms_time_t*)valuep;
		break;
	case MSPARAM_TICK_2_MAX:
		os_tick_2_max = *(ms_time_t*)valuep;
		break;
	case MSPARAM_ISLOT_MAX:
		os_islot_max = *(ms_islot_t *)valuep;
		break;
	case MSPARAM_INTR_ORDER_MAX:
		os_intr_order_max = *(unsigned *)valuep;
		break;
	case MSPARAM_INTR_TASKPRI_MAX:
		os_intr_taskpri_max = *(unsigned *)valuep;
		break;
	case MSPARAM_FARCOPY_MIN:
		os_farcopy_min = *(ms_size_t *)valuep;
		break;
	case MSPARAM_SHUTDOWN_CAPS:
		os_shutdown_caps = *(unsigned *) valuep;
		break;
	case MSPARAM_TOPOLOGY:
		os_topology_p = (ms_topology_t *) valuep;
		if (p6splitmem) {
	
			/*
			 * Compute physmem as the amount of memory
			 * present on the system.
			 */
			physmem = himem_resources_physmem();
#ifdef DEBUG
			cmn_err(CE_NOTE, "os_set_msparam: found "
				" %Lx memory. Splitting at %Lx\n",
				physmem, physmem/2);
#endif			

                	for (i = 0; i < os_topology_p->mst_nresource; i++) {
				if (os_topology_p->mst_resources[i].msr_type 
				    == MSR_NONE) {
					rp = &os_topology_p->mst_resources[i];
					break;
				}
			}

			if (i == os_topology_p->mst_nresource) {
				/* Didn't find an empty slot */
				cmn_err(CE_PANIC, "Unable to find an empty "
					"slot for P6_SPLIT_MEM resource\n");
			}		 	

			/*
			 * Relocate all memory above physmem/2 to
			 * above 4G in os_default_topology.
			 */
			himem_resources_relocate(physmem/2, os_topology_p, rp);
		}

                os_ncpus = 0;
                for (i = 0; i < os_topology_p->mst_nresource; i++) {
			if (os_topology_p->mst_resources[i].msr_type == MSR_CPU)
				os_ncpus++;
		}
		break;
	case MSPARAM_HW_SERIAL:
		os_hw_serial = (char *)valuep;
		break;
	default:
		break;
	}
}


/*
 * ms_topology_t *
 * build_default_topology(void)
 *	Build the default topology from the old bootinfo structure.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC ms_topology_t * build_default_topology() 
{
	static ms_topology_t default_topology;
	unsigned i, nresrc;
	ms_resource_t *rp;
	struct pmem_extent *memlistp;
	
	nresrc = 2;		/* One PCI, one CPU always present */
	nresrc +=  pmem_list_len(&totalMem, PMEM_NONE);
	if( bootinfo.machflags & MC_BUS )
		nresrc++;
	if( bootinfo.machflags & AT_BUS )
		nresrc++;
	if( bootinfo.machflags & EISA_IO_BUS )
		nresrc++;

	default_topology.mst_nresource = nresrc;
	default_topology.mst_resources = calloc(nresrc*sizeof(ms_resource_t));

	for( i = 0; i < nresrc; i++ ) {
		rp = &default_topology.mst_resources[i];
		rp->msr_cgnum = 0;
		rp->msr_private = MS_FALSE;
		rp->msr_private_cpu = MS_CPU_ANY;
		
	}
	rp = &default_topology.mst_resources[0];
	for (memlistp = totalMem; memlistp != NULL; memlistp = memlistp->pm_next) {
		rp->msr_type = MSR_MEMORY;
		rp->msri.msr_memory.msr_size = memlistp->pm_extent;
		rp->msri.msr_memory.msr_address = memlistp->pm_base;
		rp->msri.msr_memory.msr_flags = 0;
		if (memlistp->pm_use == PMEM_RESERVED)
			rp->msri.msr_memory.msr_flags |= MSR_RESERVED;
		rp++;
	}
	if( bootinfo.machflags & MC_BUS ) {
		rp->msr_type = MSR_BUS;
		rp->msri.msr_bus.msr_bus_type = MSR_BUS_MCA;
		rp->msri.msr_bus.msr_bus_number = 0;
		rp->msri.msr_bus.msr_n_routing = 0;
		rp++;
	}
	if( bootinfo.machflags & AT_BUS ) {
		rp->msr_type = MSR_BUS;
		rp->msri.msr_bus.msr_bus_type = MSR_BUS_ISA;
		rp->msri.msr_bus.msr_bus_number = 0;
		rp->msri.msr_bus.msr_n_routing = 0;
		rp++;
	}
	if( bootinfo.machflags & EISA_IO_BUS ) {
		rp->msr_type = MSR_BUS;
		rp->msri.msr_bus.msr_bus_type = MSR_BUS_EISA;
		rp->msri.msr_bus.msr_bus_number = 0;
		rp->msri.msr_bus.msr_n_routing = 0;
		rp++;
	}
	rp->msr_type = MSR_BUS;
	rp->msri.msr_bus.msr_bus_type = MSR_BUS_PCI;
	rp->msri.msr_bus.msr_bus_number = 0;
	rp->msri.msr_bus.msr_n_routing = 0;
	rp++;
	
	rp->msr_type = MSR_CPU;
	rp->msri.msr_cpu.msr_cpuid = BOOTENG;
	rp++;

	return &default_topology;
}

/*
 * An interface to allow a PSM to invoke the BIOS e820
 * call to discover memory above 4 GB.  This isn't a toolkit
 * because it needs OS support to make BIOS calls, and it isn't
 * a general purpose BIOS call interface because that's a very
 * wide diffuse interface; the interface.d mechanism will keep this
 * interface more under control.
 */

#define SMAP_SIG	0x534D4150	/* 'SMAP' */
#define SMAP_BASE	0
#define SMAP_BASE_HI	4
#define SMAP_SIZE	8
#define SMAP_SIZE_HI	12
#define SMAP_TYPE	16
#define SMAP_MAINSTORE	1

typedef enum {
	HIMEM_COUNT,
	HIMEM_FILL,
} himem_op_t;

/*
 * unsigned int
 * himem_resources_common (himem_op_t op, void *dst)
 * 	Return the number of the ms_resource_t entries describing ranges
 *	of the high memery (memory above 4GB).
 *
 *	Common routine used by os_count_himem_resources() and
 *	os_fill_himem_resouces() for having the same exit condition.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC unsigned int
himem_resources_common (himem_op_t op, void *dst)
{
	intregs_t	regs;
	caddr_t		v86bios_vbase;
	unsigned int	count = 0;
	ms_resource_t 	*rp = (ms_resource_t *) dst;
	uchar_t		*bp;
	uint_t		base, baseHI, size, sizeHI, type;

	/*
	 * Don't bother with memory resources above 4GB resources unless
	 * the facility to address it (PAE mode) is enabled.
	 */
	if (!enable_4gb_mem)
		return 0;
	
	if (p6splitmem) {
		/*
		 *  There can be at most one resource that crosses the
		 *  boundary. 
		 */
		count++;

		if (op == HIMEM_FILL) {
			/* Reserve a slot for later filling */

			/* Assumes type 0 is unused. */
			rp->msr_type = MSR_NONE;
			rp++;
		}
	}

	v86bios_vbase = physmap(V86BIOS_PBASE, MMU_PAGESIZE, KM_NOSLEEP);
	bp = (uchar_t *) (v86bios_vbase + V86BIOS_PDATA_BASE);

	regs.type = V86BIOS_INT32;
	regs.entry.intval = 0x15;
	regs.edi.edi = V86BIOS_PDATA_BASE;
	regs.es = 0;
	regs.ebx.ebx = 0;
	
	do {
		regs.eax.eax = 0xe820;
		regs.ecx.ecx = 20;
		regs.edx.edx = SMAP_SIG;

		v86bios( &regs );
	
		if (regs.error != 0 || regs.retval != 0 ||
		    regs.eax.eax != SMAP_SIG || regs.ecx.ecx != 20)
			break;

		type = *(ulong_t *) &bp[SMAP_TYPE];
		
		if (type != SMAP_MAINSTORE)
			continue;

#ifdef DEBUG
		base = *(ulong_t *) &bp[SMAP_BASE];
		baseHI = *(ulong_t *) &bp[SMAP_BASE_HI];
		size = *(ulong_t *) &bp[SMAP_SIZE];
		sizeHI = *(ulong_t *) &bp[SMAP_SIZE_HI];

		cmn_err(CE_CONT, "himem_resources_common: base = %x%x, "
			"length = %x%x\n", baseHI, base, sizeHI, size);
#endif		

		if (is_himem(bp)) {
			count++;
			if (op == HIMEM_FILL) {
				rp->msr_cgnum = 0;
				rp->msr_private = 0;
				rp->msr_private_cpu = MS_CPU_ANY;
				rp->msri.msr_memory.msr_size =
					*(ullong_t *) &bp[SMAP_SIZE];
				rp->msri.msr_memory.msr_address =
					*(ullong_t *) &bp[SMAP_BASE];
				rp->msri.msr_memory.msr_flags = 0;
				rp->msr_type = MSR_MEMORY;
				rp++;
			}
		}
	} while (regs.ebx.ebx != 0);

	physmap_free(v86bios_vbase, MMU_PAGESIZE, 0);

#ifdef DEBUG
	cmn_err(CE_CONT, "himem_resources_common: himem_count = %d\n", count);
#endif

	return count;
}

/*
 * Return 1 if the high memory (> 4GB) is included in the SMAP entry, adjusting
 * the base and lenght. Otherwise, return 0.
 * 
 */
STATIC int
is_himem (uchar_t *desc)
{
	ullong_t	delta, base, size;


	base = *(ullong_t *) &desc[SMAP_BASE];
	size = *(ullong_t *) &desc[SMAP_SIZE];
	
	if( base + size <= 0xffffffff )
		return 0;

	if( base <= 0xffffffff ) {
		delta = 0x100000000ll - base;
		*(ullong_t *) &desc[SMAP_SIZE] -= delta;
		*(ullong_t *) &desc[SMAP_BASE] += delta;
	}

	return 1;
}


/*
 * unsigned int
 * os_count_himem_resources (void)
 * 	Return the number of the ms_resource_t entries describing ranges
 *	of the high memery (memory above 4GB).
 *
 * Calling/Exit State:
 * 	Called from pfxinitpsm()
 */
unsigned int
os_count_himem_resources(void)
{
	return (himem_resources_common(HIMEM_COUNT, NULL));
}

/*
 * void
 * os_fill_himem_resources(ms_resource_t *)
 *	Fill all fields of ms_resource_t entries as appropriate for MSR_MEMORY.
 *	The caller (PSM) must allocate sufficient memory to be filled in
 *	advance. 
 * 
 * Calling/Exit State:
 * 	Called from pfxinitpsm()
 */

void
os_fill_himem_resources(ms_resource_t * res)
{
	himem_resources_common(HIMEM_FILL, res);
}


/*
 * Structure for a simple lock. This is a structure because we may want
 * to pad it to cache line boundaries & it's easier to add pad to a structure.
 * (Nothing except the following few routines know/care about the structure of a lock.
 */
typedef struct {
	char	lock;
} ms_lock_t;

/*
 * ms_lockp_t
 * os_mutex_alloc(void)
 *	Allocates and initializes a simple mutex.
 *
 * Calling/Exit State:
 *	None.
 */
ms_lockp_t
os_mutex_alloc(void)
{
	ms_lock_t	*m;
	m = os_alloc(sizeof(ms_lock_t));
	m->lock = 0;
	return(m);
}



/*
 * void
 * os_mutex_free(void)
 *	Fress a simple mutex previously allocated by os_mutex_alloc.
 *
 * Calling/Exit State:
 *	None.
 */
void
os_mutex_free(ms_lockp_t m)
{
	os_free (m, sizeof(ms_lock_t));
}



/*
 * ms_lockstate_t
 * os_mutex_lock(ms_lockp_t)
 *	Allocates and initializes a simple mutex.
 *
 * Calling/Exit State:
 *	None.
 */
asm ms_lockstate_t
os_mutex_lock_i(ms_lockp_t m)
{
%mem m; lab again, spin, done;
	movl	m,%edx
again:
	pushfl
	cli
	movb	$1,%al
	xchgb	%al,(%edx)
	cmpb	$0,%al
	je	done
	/ Could not get lock. Restore flags and spin
	popfl
spin:	cmpb	$0,(%edx)
	je	again
	jmp	spin
done:
	popl	%eax	
}
/*#pragma asm partial_optimization os_mutex_lock_i */

ms_lockstate_t
os_mutex_lock(ms_lockp_t m)
{
	ms_lockstate_t	state;
	state = os_mutex_lock_i(m);
	return(state);
}


/*
 * ms_lockp_t
 * os_mutex_unlock(ms_lockp_t, ms_lockstate_t)
 *	Allocates and initializes a simple mutex.
 *
 * Calling/Exit State:
 *	None.
 */
void
os_mutex_unlock(ms_lockp_t m, ms_lockstate_t state)
{
	((ms_lock_t*) m)->lock = 0;
	intr_restore(state);
}


/*
 * Default msop routines.......
 */

void
msop_panic()
{
	/*
	 *+ The base kernel called a PSM routine that was not supported
	 *+ on this platform. This is usually caused by running the wrong
	 *+ PSM or by a defective PSM.
	 */
	cmn_err(CE_PANIC, "Called non-supported PSM/MSOP routine.\n");
}

void
msop_noop()
{
}

void
msop_halt()
{
	for (;;)
		asm("hlt");
	/* NOTREACHED */
}



extern unsigned char	msop_io_read_8(ms_port_t);
extern unsigned short	msop_io_read_16(ms_port_t);
extern unsigned int	msop_io_read_32(ms_port_t);
extern void		msop_io_write_8(ms_port_t, unsigned char);
extern void		msop_io_write_16(ms_port_t, unsigned short);
extern void		msop_io_write_32(ms_port_t, unsigned int);

extern void		msop_io_rep_read_8(ms_port_t, unsigned char*, int);
extern void		msop_io_rep_read_16(ms_port_t, unsigned short*, int);
extern void		msop_io_rep_read_32(ms_port_t, unsigned int*, int);
extern void		msop_io_rep_write_8(ms_port_t, unsigned char*, int);
extern void		msop_io_rep_write_16(ms_port_t, unsigned short*, int);
extern void		msop_io_rep_write_32(ms_port_t, unsigned int*, int);

extern void		msop_time_add(ms_rawtime_t*, ms_rawtime_t*);
extern void		msop_time_sub(ms_rawtime_t*, ms_rawtime_t*);

/*
 * This table is used at boot time to initialize the default
 * MSOP array of PSM routines. When a PSM is loaded, it may selectively
 * replace entries in this table.
 */
msop_func_t msop_msops_default[] = {
	{ MSOP_INIT_CPU,	(void *) msop_panic },
	{ MSOP_TICK_2,		(void *) msop_panic },

	{ MSOP_INTR_ATTACH,	(void *) msop_panic },
	{ MSOP_INTR_DETACH,	(void *) msop_panic },
	{ MSOP_INTR_MASK,	(void *) msop_panic },
	{ MSOP_INTR_UNMASK,	(void *) msop_panic },
	{ MSOP_INTR_COMPLETE,	(void *) msop_panic },

	{ MSOP_INTR_TASKPRI,	(void *) msop_noop },

	{ MSOP_XPOST,		(void *) msop_panic },

	{ MSOP_TICK_2,		(void *) msop_panic },
	{ MSOP_TIME_GET,	(void *) msop_panic },
	{ MSOP_TIME_ADD,	(void *) msop_panic },
	{ MSOP_TIME_SUB,	(void *) msop_panic },
	{ MSOP_TIME_CVT,	(void *) msop_panic },
	{ MSOP_TIME_SPIN,	(void *) msop_noop },
	{ MSOP_RTODC,		(void *) msop_panic },
	{ MSOP_WTODC,		(void *) msop_panic },

	{ MSOP_IDLE_SELF,	(void *) msop_noop },
	{ MSOP_IDLE_EXIT,	(void *) msop_noop },
	{ MSOP_SHUTDOWN,	(void *) msop_halt },
	{ MSOP_OFFLINE_PREP,	(void *) msop_noop },
	{ MSOP_OFFLINE_SELF,	(void *) msop_halt },
	{ MSOP_START_CPU,	(void *) msop_panic },
	{ MSOP_SHOW_STATE,	(void *) msop_noop },

	{ MSOP_IO_READ_8,	(void *) msop_io_read_8 },
	{ MSOP_IO_READ_16,	(void *) msop_io_read_16 },
	{ MSOP_IO_READ_32,	(void *) msop_io_read_32 },
	{ MSOP_IO_WRITE_8,	(void *) msop_io_write_8 },
	{ MSOP_IO_WRITE_16,	(void *) msop_io_write_16 },
	{ MSOP_IO_WRITE_32,	(void *) msop_io_write_32 },

	{ MSOP_IO_REP_READ_8,	(void *) msop_io_rep_read_8 },
	{ MSOP_IO_REP_READ_16,	(void *) msop_io_rep_read_16 },
	{ MSOP_IO_REP_READ_32,	(void *) msop_io_rep_read_32 },
	{ MSOP_IO_REP_WRITE_8,	(void *) msop_io_rep_write_8 },
	{ MSOP_IO_REP_WRITE_16,	(void *) msop_io_rep_write_16 },
	{ MSOP_IO_REP_WRITE_32,	(void *) msop_io_rep_write_32 },

	{ MSOP_FARCOPY,		(void *) bcopy },

	{ 0,			NULL}};

#if defined(DEBUG) || defined(DEBUG_TOOLS)
 
static char *
busname(int n)
{
	static char *names[] = {"Non-standard", "PCI", "EISA", "MCA", "ISA"};
	if( 0 <= n && n <=4 )
		return names[n];
	else
		return "Unknown";
}

/*
 * dump_topology(ms_topology_t * tp, uint_t type)
 *
 * 	Dump the topology structure. If type == 0, all resource types will
 *	be dumped. Otherwise, only the resources of the give type will
 *	be dumped.
 */
dump_topology(ms_topology_t * tp, uint_t type)
{
	unsigned i;
	ms_resource_t * rp;

	for( i = 0; i < tp->mst_nresource; i++ ) {
		rp = &tp->mst_resources[i];

		if (type)
			if (rp->msr_type != type)
				continue;
		
		debug_printf("%d:%s ", rp->msr_cgnum,
			(rp->msr_private?"PRV":"GLB") );
		switch( rp->msr_type ) {
		case MSR_CPU:
			debug_printf("CPU %d\n", rp->msri.msr_cpu.msr_cpuid );
			break;
 
		case MSR_MEMORY:
			debug_printf("MEM base %Lx, size %Lx\n",
			       rp->msri.msr_memory.msr_address,
			       rp->msri.msr_memory.msr_size);
			break;
		case MSR_BUS:
			debug_printf("BUS %s\n",
			       busname(rp->msri.msr_bus.msr_bus_type));
			break;

		case MSR_CG:
			debug_printf("CG: id = %Lx\n",
				     rp->msri.msr_cg.msr_cgid);
			break;
 
		case MSR_DCACHE:
		case MSR_ICACHE:
		case MSR_UCACHE:
			debug_printf( "CACHE\n" );
			break;
 
		default:
			debug_printf( "Other\n" );
			break;
		}
	}
}
 
#endif /* DEBUG || DEBUG_TOOLS */
