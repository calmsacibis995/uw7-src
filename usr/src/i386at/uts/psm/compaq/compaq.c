/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/compaq.c	1.17.8.2"
#ident	"$Header$"

/*
 * Compaq PSM hardware-specific routines and data.
 *
 * This file contains a set of subroutines and a set of data, all subject
 * to the PSMv2 Interface Specification. All global symbols are using
 * the prefix "cpq_".
 *
 * This MP PSM/mHAL file supports both the Systempro, Systempro/XL
 * dual processor and Proliant system.
 *
 * TODO & NICE TO HAVE:
 * . some way to reserve PSM private resources (ports, interrupt)
 * . moving all per cpu data on cache boundary arrays
 * . prevent offlining of CPUs with interrupts bound at PSM level
 * 
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>
#include <psm/toolkits/psm_i8254/psm_i8254.h>
#include <psm/toolkits/psm_mc146818/psm_mc146818.h>
#include <psm/toolkits/psm_time/psm_time.h>
#include <psm/toolkits/psm_string.h>
#include <psm/toolkits/psm_eisa.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_assert.h>
#include <psm/compaq/compaq.h>
#include <psm/compaq/syspro.h>
#include <psm/compaq/xl.h>


/*
 * The major differences between CPQ_ASYMINTR/CPQ_SYMINTR are:
 * . no interrupt controller on CPQ_ASYMINTR and os_this_cpu != cpq_booteng
 * . MS_EVENT_PSM_1 is timer propagation on CPQ_ASYMINTR
 * . MS_EVENT_PSM_1 is idle wake up on CPQ_SYMINTR
 *
 * MS_EVENT_PSM_2 is in both cases interrupt cross-masking for interrupt
 * attach. This is needed for CPQ_ASYMINTR too because a cpq_intr_attach()
 * can run on the secondary CPU while the cpq_booteng is clearly the CPU
 * to attach the interrupt to.
 *
 */
#define	ISASYMINTR	(cpq_sp_iomode == CPQ_ASYMINTR)
#define	ISSYMINTR	(cpq_sp_iomode == CPQ_SYMINTR)
#define	ISCONFIGURED	(ISASYMINTR || ISSYMINTR)

#define cpq_vec2slot(v)	(v-I8259_VBASE)		      /* vectors start at 32 */



/*
 * Configurable objects, defined in Space.c
 */
extern int cpq_spxl_iomodemask;			        /* mask the I/O mode */
extern int cpq_intrdistmask[];



msop_func_t cpq_msops[];
ms_intr_dist_t * cpq_service_int(ms_ivec_t vec);
ms_intr_dist_t * cpq_service_high_int(ms_ivec_t vec);
ms_intr_dist_t * cpq_service_timer(ms_ivec_t vec);
ms_intr_dist_t * cpq_service_xint(ms_ivec_t vec);
void cpq_intr_mask(ms_intr_dist_t *idtp);
void cpq_intr_unmask(ms_intr_dist_t *idtp);
void cpq_xpost(ms_cpu_t cpu, ms_event_t eventmask);



/*
 * Global variables exported to other modules
 */
unsigned char		cpq_mptype;
unsigned char		cpq_sp_iomode;
ms_cpu_t		cpq_booteng=MS_CPU_ANY;
ms_cpu_t 		cpq_howmany_online = 0;
i8254_params_t		cpq_i8254_params;		     /* timer params */
unsigned int		cpq_rawtime=0;			     /* time counter */
unsigned int		cpq_rawtime_lock;		  /* asymmetric lock */

/* Per interrupt slot array */
struct cpq_slot_info	cpq_intr_slot[CPQ_MAX_SLOT+1];

/* Per cpu arrays */
unsigned int	cpq_lintmask[CPQ_MAXNUMCPU] =  /* Logical interrupt mask */
		 {CPQ_INIT_MASK, CPQ_INIT_MASK, CPQ_INIT_MASK, CPQ_INIT_MASK};
unsigned int	cpq_pintmask[CPQ_MAXNUMCPU] = /* Physical interrupt mask */
		 {CPQ_INIT_MASK, CPQ_INIT_MASK, CPQ_INIT_MASK, CPQ_INIT_MASK};
ms_bool_t 	cpq_engine_online[CPQ_MAXNUMCPU] =       /* Online state */
		 {MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE};


/*
 * Global variables not exported to other modules
 */
STATIC ms_lockp_t		cpq_attach_lock;
STATIC ms_lockp_t		cpq_mask_lock;

STATIC struct cpq_psmops 	*cpq_mpfuncs;
STATIC ms_event_t       	cpq_clock_event = MS_EVENT_TICK_1;

/* Per cpu arrays */
STATIC volatile unsigned int	cpq_xpost_eventflags[CPQ_MAXNUMCPU];
STATIC ms_bool_t	cpq_idle_engine[CPQ_MAXNUMCPU] =   /* Idle state */
			 {MS_FALSE, MS_FALSE, MS_FALSE, MS_FALSE};

/*
 * Values to pass to os_set_msparam
 */
STATIC char		cpq_platform_sym[] = "Compaq Proliant/SProXL";
STATIC char		cpq_platform_asym[] = "Compaq SystemPro";
STATIC char		*cpq_platform_name;
STATIC ms_bool_t	cpq_sw_sysdump = MS_TRUE;
STATIC ms_time_t	cpq_time_res = {CPQ_NSECS_IN_USEC,0};  /* 1 microsec */
STATIC ms_time_t	cpq_tick_1_res = {100000,0};	    /* min. 100 usec */
STATIC ms_time_t	cpq_tick_1_max = {27000000,0};	     /* max. 27 msec */
STATIC ms_islot_t	cpq_islot_max=CPQ_MAX_SLOT;
STATIC ms_cpu_t		cpq_cpu_max;
STATIC ms_topology_t 	*cpq_topology;
STATIC ms_cpu_t		cpq_shutdown_caps =
			      (MS_SD_HALT | MS_SD_AUTOBOOT | MS_SD_BOOTPROMPT);





/*
 * void
 * cpq_noop(void)
 *
 * Calling/Exit State:
 *      None.
 */
void
cpq_noop(void)
{
}


/*
 * STATIC ms_bool_t
 * cpq_ckidstr(char *, char *, int)
 *
 * Calling/Exit State:
 *      Return MS_TRUE, if the string <strp> is found within 
 *	(addrp) to ((addrp) + (cnt)) range, otherwise return MS_FALSE.
 */
STATIC ms_bool_t
cpq_ckidstr(char *addrp, char *strp, int cnt)
{
        int tcnt;

	for (tcnt = 0; tcnt < cnt; tcnt++) {
		if (*addrp != strp[0]) {
			addrp++;
			continue;
		}

		if (strncmp(addrp, strp, strlen(strp)) == 0) {
			return (MS_TRUE);
		}
		addrp++;
        }

        return (MS_FALSE);
}

/*
 * STATIC ms_bool_t
 * cpq_find_sys(void)
 *
 * Search for "COMPAQ" signature inside Bios and find out the model.
 * Return MS_TRUE or MS_FALSE as this effort succeeded or failed.
 *
 */

STATIC ms_bool_t
cpq_find_sys(void)
{
	extern struct cpq_psmops cpq_spmpfuncs;
	extern struct cpq_psmops cpq_xlmpfuncs;
	extern long cpq_proliant_id[];
	extern int cpq_proliant_ids;

	ms_bool_t	rv=MS_FALSE;
	long		system_id;
	void		*addr;
	int		id;

	/* Search 0xfe000-0xfe100 as on PSMv1 Syspro/XL */
	addr = (unsigned char*) os_physmap(0xfe000, 0x100);
	rv = cpq_ckidstr((char *)addr, "COMPAQ", 0x100);
	os_physmap_free(addr, 0x100);
	
	/* Search 0xfffe4-0x1000e4 as on autodetect */
	addr = (unsigned char*) os_physmap(0xfffe4, 0x100);
	rv |= cpq_ckidstr((char *)addr, "COMPAQ", 0x100);
	os_physmap_free(addr, 0x100);

	/* No string found */
        if (rv == MS_FALSE) return rv;

	/*
	 * Find out if we are running on a Systempro, Systempro/XL,
	 * Proliant or Powerpro?
	 */
	cpq_mptype = CPQ_NONE;
	system_id = inl(CPQ_ID_PORT);

	/* These are explicitly NOT SystemPro clones, taken from OSR5 */
	if ((system_id == SP_NOTSPRO) ||
	    (system_id == SP_NOTSPRO1))
		return MS_FALSE;

	if (system_id == SP_XL_ID) cpq_mptype = CPQ_SYSTEMPROXL;

	for (id = 0; id < cpq_proliant_ids; id++) {
		if (cpq_proliant_id[id] == system_id) {
			cpq_mptype = CPQ_PROLIANT;
			break;
		}
	}

	/* Taken from OSR5 */
	if ((ISCPQ) && (ISSYSTEMPRO)) cpq_mptype = CPQ_SYSTEMPRO;

	/*
	 * UW 2.1 defaults all systems that have "COMPAQ" signature
         * inside Bios even though not being CPQ as EISA id.
         * Default is Syspro compatible.
	 */
	if ((ISPOWERPRO) ||
	    (! (ISCPQ))) cpq_mptype = CPQ_SYSTEMPRO_COMPATIBLE;

	/*
	 * Do not program an XL to symmetric mode if the mode 
	 * is masked.
	 */
	if ((cpq_mptype & (CPQ_SYSTEMPROXL | CPQ_PROLIANT)) && 
	    !(cpq_spxl_iomodemask & CPQ_SYMINTR)) {
		cpq_sp_iomode = CPQ_SYMINTR;
		cpq_mpfuncs = &cpq_xlmpfuncs;
		cpq_cpu_max = XL_MAXNUMCPU;
		cpq_platform_name = cpq_platform_sym;
	} else {
		cpq_sp_iomode = CPQ_ASYMINTR;
		cpq_mpfuncs = &cpq_spmpfuncs;
		cpq_cpu_max = SP_MAXNUMCPU;
		cpq_platform_name = cpq_platform_asym;
	}

	return rv;
}

/*
 * STATIC void
 * cpq_init_msparam()
 *
 * Calling/Exit State:
 *
 */
STATIC void
cpq_init_msparam()
{
	os_set_msparam(MSPARAM_PLATFORM_NAME, cpq_platform_name);
	os_set_msparam(MSPARAM_SW_SYSDUMP, &cpq_sw_sysdump);
	os_set_msparam(MSPARAM_TIME_RES, &cpq_time_res);
	os_set_msparam(MSPARAM_TICK_1_RES, &cpq_tick_1_res);
	os_set_msparam(MSPARAM_TICK_1_MAX, &cpq_tick_1_max);
	os_set_msparam(MSPARAM_ISLOT_MAX, &cpq_islot_max);
	os_set_msparam(MSPARAM_SHUTDOWN_CAPS, &cpq_shutdown_caps);
	os_set_msparam(MSPARAM_TOPOLOGY, cpq_topology );
}

/*
 * ms_bool_t
 * cpq_initpsm()
 *
 * Calling/Exit State:
 *      This function is called exactly once as the first entry
 *      into the PSM.
 *
 * Description:
 *
 */
ms_bool_t
cpq_initpsm()
{
	ms_cpu_t n, nproc;
	ms_resource_t *rp;
	ms_islot_t slot;
	unsigned int i;

	/* System detection */
	if(! cpq_find_sys())
		return MS_FALSE;

	/* Execute platform specific initpsm */
	nproc = (*cpq_mpfuncs->cpq_ps_initpsm)();

	/* MSOPs registration */
        if (os_register_msops(cpq_msops) != MS_TRUE)
		return MS_FALSE;

	/*
	 * This is the commit point. If we get here, we must
	 * return TRUE, committing the system to run with this PSM.
	 */

	/* Initialize topology */
	cpq_topology = (ms_topology_t *) os_alloc( sizeof(ms_topology_t));
	cpq_topology->mst_nresource = 0;

	/* Find what PSM cannot fill */
	for (i = 0; i < os_default_topology->mst_nresource; i++ ) {
                if ((os_default_topology->mst_resources[i].msr_type != MSR_CPU)
                && (os_default_topology->mst_resources[i].msr_type != MSR_BUS))
			cpq_topology->mst_nresource++;
	}

	/* Add CPUS and EISA bus */
	cpq_topology->mst_nresource += nproc + 1;

	/* Alloc resources */
	rp = cpq_topology->mst_resources = (ms_resource_t *)
		os_alloc( cpq_topology->mst_nresource * sizeof(ms_resource_t));

	/* Initialize resources: fill CPU description */
	for (n = 0; n < nproc; n++, rp++) {
		rp->msr_cgnum = 0;
		rp->msr_private = MS_FALSE;
		rp->msr_private_cpu = MS_CPU_ANY;
		rp->msr_type = MSR_CPU;
		/* speed not known for the time being */
		rp->msri.msr_cpu.msr_clockspeed = 0;
		rp->msri.msr_cpu.msr_cpuid = n;
	}	

	/* Initialize resources: fill EISA bus description */
	rp->msr_cgnum = 0;
	rp->msr_private = MS_FALSE;
	rp->msr_private_cpu = MS_CPU_ANY;
	rp->msr_type = MSR_BUS;
	rp->msri.msr_bus.msr_bus_type = MSR_BUS_EISA;
	rp->msri.msr_bus.msr_bus_number = 0;
	/* No routing info */
	rp->msri.msr_bus.msr_intr_routing = (msr_routing_t *) 0;
	rp->msri.msr_bus.msr_n_routing = 0;
	rp++;

	/* Now merge with the os_default_topology */
	for (i = 0; i < os_default_topology->mst_nresource; i++ ) {
                if ((os_default_topology->mst_resources[i].msr_type != MSR_CPU)
                && (os_default_topology->mst_resources[i].msr_type != MSR_BUS))
                        *rp++ = os_default_topology->mst_resources[i];
	}

	/* Set all msparam after topology is defined */
	cpq_init_msparam();

	/* Allocate mutexes needed by the platform */
	cpq_attach_lock = os_mutex_alloc();
	cpq_mask_lock = os_mutex_alloc();

	/*
	 * Mask all interrupts off for safety, maybe
	 * a deinit is more appropriate.
	 */
	for (slot = CPQ_FIRST_SLOT; slot < CPQ_MAX_SLOT+1; slot++)
		i8259_intr_mask(slot);

	/*
	 * Allow first psm_time_spin to be processed
	 * through the uncalibrated psm_time toolkit.
	 */
	psm_time_spin_init();

	return MS_TRUE;
}


/*
 * STATIC void
 * cpq_clr_fpbusy(void)
 *	Clear FPU BUSY latch.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_clr_fpbusy(void)
{
	/*
	 * This is mantained for historycal reason, PSMv2 does
	 * not support external FPUs.
	 */
	outb(CPQ_FPU_BUSY_LATCH, 0);	/* Clear FPU BUSY latch */

}


/*
 * STATIC void
 * cpq_init_cpu(void)
 *      Initialize the interrupt control system.  At this point, all
 *      driver interrupts are disabled, since none have been attached.
 * 	Initialize tick 1 timer.
 * 	Initialize free running fine granularity timer.
 *	Take over attached interrupts that can be handled by this CPU.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.  This function
 *	must be called with the IE flag off.
 *
 */
STATIC void
cpq_init_cpu(void)
{
	ms_islot_t i;
	ms_lockstate_t ls, ls1;

	PSM_ASSERT(!is_intr_enabled());

        if (cpq_booteng == MS_CPU_ANY) {

		/*
		 * Save cpu number of this cpu - it will handle global clocks.
		 * Note that this is also a 'first_time' flag so this code is
		 * executed only on the boot engine.
		 */
		cpq_booteng = os_this_cpu;

		/*
		 * Initialize the interrupt slot info array
		 * with appropriate values.
		 */
		for (i = CPQ_FIRST_SLOT; i < CPQ_MAX_SLOT+1; i++) {
			/* No unmask has been done, yet */
			cpq_intr_slot[i].masked = MS_TRUE;
			/* No attach has been done. yet */
			cpq_intr_slot[i].attached_to = MS_CPU_ALL_BUT_ME;
			cpq_intr_slot[i].idtp = os_intr_dist_stray;
		}

		/*
		 * Open TIMER, CASCADE and XINT channels from the beginning.
		 * Platforms will tell us where those interrupts are
		 * attached to.
		 */
		cpq_intr_slot[CPQ_TIMER_SLOT].idtp = os_intr_dist_nop;
		cpq_intr_slot[CPQ_CASC_SLOT].idtp = os_intr_dist_nop;
		cpq_intr_slot[CPQ_XINT_SLOT].idtp = os_intr_dist_nop;

		cpq_intr_slot[CPQ_TIMER_SLOT].masked = MS_FALSE;
		cpq_intr_slot[CPQ_CASC_SLOT].masked = MS_FALSE;
		cpq_intr_slot[CPQ_XINT_SLOT].masked = MS_FALSE;

		/*
	 	 * Claim device interrupt vectors for all devices.
	 	 */
		os_claim_vectors(I8259_VBASE, CPQ_MAX_SLOT+1, cpq_service_int);

		/*
		 * Then use separate handlers for CPQ_TIMER_SLOT, CPQ_XINT_SLOT
		 * and highest vectors of each PIC. 
		 */
		os_claim_vectors(I8259_VBASE+CPQ_TIMER_SLOT, 1,
				 cpq_service_timer);
		os_claim_vectors(I8259_VBASE+CPQ_XINT_SLOT, 1,
				 cpq_service_xint);
		os_claim_vectors(I8259_VBASE+CPQ_SP_SLOT, 1,
				 cpq_service_high_int);
		os_claim_vectors(I8259_VBASE+CPQ_MAX_SLOT, 1,
				 cpq_service_high_int);
        }

	/*
	 * Platform specific cpu_init could reassign interrupt service
	 * to other CPUs. Prevent race conditions with intr attach/detach
         * and mask/unmask.
	 */
	ls = os_mutex_lock(cpq_attach_lock);
	ls1 = os_mutex_lock(cpq_mask_lock);

	/* Execute platform specific initpsm */
	(*cpq_mpfuncs->cpq_ps_init_cpu)();

	/* mark CPU state */
	cpq_engine_online[os_this_cpu] = MS_TRUE;
	cpq_howmany_online++;

	cpq_clr_fpbusy();

	/* Execute the picreload() equivalent */
	if ((ISSYMINTR) || (os_this_cpu == cpq_booteng)) {

		/* mask them all ... */
		for (i = CPQ_FIRST_SLOT; i < CPQ_MAX_SLOT+1; i++) {
			cpq_pintmask[os_this_cpu] |= (1 << i);
			i8259_intr_mask(i);
		}
	}

	/* ... and unmask the needed ones */
	for (i = CPQ_FIRST_SLOT; i < CPQ_MAX_SLOT+1; i++) {
		/* CPQ_XINT_SLOT is set as MS_CPU_ANY */
		if ((cpq_intr_slot[i].attached_to == MS_CPU_ANY) ||
	    	    (cpq_intr_slot[i].attached_to == os_this_cpu))
			if (cpq_intr_slot[i].masked == MS_FALSE) {
				cpq_pintmask[os_this_cpu] &= ~(1 << i);
				i8259_intr_unmask(i);
			}
	}

	/*
	 * The interrupt reassignment or the intr_attach is
	 * consistent only when the sequence
	 * intr_mask/attach/intr_unmask is complete.
	 */
	os_mutex_unlock(cpq_mask_lock, ls1);
	os_mutex_unlock(cpq_attach_lock, ls);
}


/*
 * STATIC ms_bool_t
 * cpq_intr_attach(ms_intr_dist_t *idtp)
 *	Attach the specified interrupt.
 *
 * Calling/Exit State:
 *      Called when the system is being initialized.  This function
 *      must be called with the IE flag off.
 */
STATIC ms_bool_t
cpq_intr_attach(ms_intr_dist_t *idtp)
{
        ms_islot_t slot;
        ms_cpu_t cpu, i, newcpu, oldcpu;
	ms_lockstate_t ls, ls1;
	int mask;

	PSM_ASSERT(!is_intr_enabled());

	cpu = idtp->msi_cpu_dist;
	slot = idtp->msi_slot;

	PSM_ASSERT(slot != CPQ_TIMER_SLOT);
	PSM_ASSERT(slot != CPQ_CASC_SLOT);

	/* Never attach CPQ_TIMER_SLOT */
	if ((slot > cpq_islot_max) ||
	    (slot == CPQ_CASC_SLOT) ||
	    (slot == CPQ_TIMER_SLOT))
		return(MS_FALSE);

	/*
	 * CPQ_XINT_SLOT is a special case: never masked and MS_CPU_ANY
	 */
	if (slot == CPQ_XINT_SLOT) {
		ls = os_mutex_lock(cpq_attach_lock);

		idtp->msi_flags |= (MSI_ORDERED_COMPLETES | MSI_NONMASKABLE);

		cpq_intr_slot[slot].idtp = idtp;

		os_mutex_unlock(cpq_attach_lock, ls);
		return(MS_TRUE);
	}

	/* Prevent other attach/detach/interrupt redistribution */
	ls = os_mutex_lock(cpq_attach_lock);

	/*
	 * Edge-trigger interrupts cannot be moved across CPUs.
	 * If an interrupt occurs while an additional CPU is being
	 * brought online the cold start sequence will wipe out
	 * its acknowledge. Level-trigger interrupts instead will
	 * be reasserted until serviced. Prevent edge-trigger
	 * interrupts from being distributed.
	 */
	if (!(idtp->msi_flags & MSI_ITYPE_CONTINUOUS))
		cpq_intrdistmask[slot] = 1;

	oldcpu = cpq_intr_slot[slot].attached_to;

	/* If already attached, mask it off on all CPUs */
	if (oldcpu != MS_CPU_ALL_BUT_ME) {

		ls1 = os_mutex_lock(cpq_mask_lock);

		/* Mask it logically */
		mask = 1 << slot;
		if ((cpu == MS_CPU_ANY) &&
		    (cpq_intrdistmask[slot] == 0)) {
				/* Mask it logically off on all CPUs */
				for (i = 0; i < CPQ_MAXNUMCPU; i++)
					cpq_lintmask[i] |= mask;
		}
		else cpq_lintmask[oldcpu] |= mask;

		/* Mask it physically */
		if (oldcpu == os_this_cpu) {

			/* Mask it locally */
			cpq_pintmask[os_this_cpu] |= (1 << slot);
			i8259_intr_mask(slot);

		} else {

		    if (cpq_engine_online[oldcpu] == MS_TRUE) {
			/* Spin waiting: because of a special case for XL */
			while (cpq_intr_slot[slot].event2_pending == MS_TRUE) {
				cpq_noop();
			}

			/* Cross-masking through XINT */
			cpq_intr_slot[slot].masked = MS_TRUE;
			cpq_intr_slot[slot].event2_pending = MS_TRUE;

			/* Signal the target only */
			cpq_xpost(oldcpu, MS_EVENT_PSM_2);

			/* Spin waiting for sync */
			while (cpq_intr_slot[slot].event2_pending == MS_TRUE) {
				cpq_noop();
			}
		    }
		}

		os_mutex_unlock(cpq_mask_lock, ls1);
	}

	if (cpu == MS_CPU_ANY) {
		/* Find out the new cpu to attach the interrupt to */
		newcpu = (*cpq_mpfuncs->cpq_ps_assignvec)(slot);
	} else newcpu = cpu;

	PSM_ASSERT (newcpu < cpq_cpu_max);
	PSM_ASSERT (newcpu >= 0);

	/* Attach it */
	cpq_intr_slot[slot].attached_to = newcpu;
	cpq_intr_slot[slot].idtp = idtp;

	/* Set ELTR */
	(*cpq_mpfuncs->cpq_ps_set_eltr)(newcpu, slot, idtp);

	idtp->msi_flags |= MSI_ORDERED_COMPLETES;
	idtp->msi_flags &= ~MSI_NONMASKABLE;

	ls1 = os_mutex_lock(cpq_mask_lock);

	/* Unmask it logically */
	mask = 1 << slot;
	if ((cpu == MS_CPU_ANY) &&
	    (cpq_intrdistmask[slot] == 0)) {
			/* Unmask it logically on all CPUs */
			for (i = 0; i < CPQ_MAXNUMCPU; i++)
				cpq_lintmask[i] &= ~(mask);
	}
	else cpq_lintmask[newcpu] &= ~(mask);

	/* Unmask it physically */
	if (newcpu == os_this_cpu) {

		/* Unmask it locally */
		cpq_pintmask[os_this_cpu] &= ~(1 << slot);
		i8259_intr_unmask(slot);

	} else {

		/* Spin waiting: needed because of a special case for XL */
		while (cpq_intr_slot[slot].event2_pending == MS_TRUE) {
			cpq_noop();
		}

		if (cpq_engine_online[newcpu] == MS_TRUE) {
			/* Cross-masking through XINT */
			cpq_intr_slot[slot].masked = MS_FALSE;
			cpq_intr_slot[slot].event2_pending = MS_TRUE;

			/* Signal the target only */
			cpq_xpost(newcpu, MS_EVENT_PSM_2);

			/* Spin waiting for sync */
			while (cpq_intr_slot[slot].event2_pending == MS_TRUE) {
				cpq_noop();
			}
		}
	}

	os_mutex_unlock(cpq_mask_lock, ls1);

	/* Prevent other attach/detach/interrupt redistribution */
	os_mutex_unlock(cpq_attach_lock, ls);

	return(MS_TRUE);
}


/*
 * STATIC void
 * cpq_intr_detach(ms_intr_dist_t *idtp)
 *      Detach the specified interrupt source from the dispatching
 *      tables and disable distribution of this interrupt source
 *      in the interrupt controller(s).
 *
 *
 * Calling/Exit State:
 *      After this operation returns, there should be no more interrupt
 *      deliveries using the specified interrupt distribution structure
 *      pointer.
 *
 */
STATIC void
cpq_intr_detach(ms_intr_dist_t *idtp)
{
        ms_islot_t slot;
	ms_lockstate_t ls, ls1;
	ms_cpu_t oldcpu, cpu, i;
	int mask;

	PSM_ASSERT(!is_intr_enabled());

	slot = idtp->msi_slot;
	cpu = idtp->msi_cpu_dist;

	PSM_ASSERT(slot != CPQ_TIMER_SLOT);
	PSM_ASSERT(slot != CPQ_CASC_SLOT);
	PSM_ASSERT(slot != CPQ_XINT_SLOT);

	/* Never detach CPQ_CASC_SLOT, CPQ_TIMER_SLOT and CPQ_XINT_SLOT */
	if ((slot > cpq_islot_max) ||
	    (slot == CPQ_XINT_SLOT) ||
	    (slot == CPQ_CASC_SLOT) ||
	    (slot == CPQ_TIMER_SLOT))
		return;

	/* Prevent other attach/detach/interrupt redistribution */
	ls = os_mutex_lock(cpq_attach_lock);

	oldcpu = cpq_intr_slot[slot].attached_to;

	ls1 = os_mutex_lock(cpq_mask_lock);

	/* Mask it logically */
	mask = 1 << slot;
	if ((cpu == MS_CPU_ANY) &&
	    (cpq_intrdistmask[slot] == 0)) {
			/* Mask it logically off on all CPUs */
			for (i = 0; i < CPQ_MAXNUMCPU; i++)
				cpq_lintmask[i] |= mask;
	}
	else cpq_lintmask[oldcpu] |= mask;

	/* Mask it physically */
	if (oldcpu == os_this_cpu) {

		/* Mask it locally */
		cpq_pintmask[os_this_cpu] |= (1 << slot);
		i8259_intr_mask(slot);

	} else {

		/* Spin waiting: because of a special case for XL */
		while (cpq_intr_slot[slot].event2_pending == MS_TRUE) {
			cpq_noop();
		}

		if (cpq_engine_online[oldcpu] == MS_TRUE) {
			/* Cross-masking through XINT */
			cpq_intr_slot[slot].masked = MS_TRUE;
			cpq_intr_slot[slot].event2_pending = MS_TRUE;

			/* Signal the target only */
			cpq_xpost(oldcpu, MS_EVENT_PSM_2);

			/* Spin waiting for sync */
			while (cpq_intr_slot[slot].event2_pending == MS_TRUE) {
				cpq_noop();
			}
		}
	}

	os_mutex_unlock(cpq_mask_lock, ls1);

	/* Detach it */
	cpq_intr_slot[slot].attached_to = MS_CPU_ALL_BUT_ME;
	cpq_intr_slot[slot].idtp = os_intr_dist_stray;

	/* Prevent other attach/detach/interrupt redistribution */
	os_mutex_unlock(cpq_attach_lock, ls);
}


/*
 * STATIC void
 * cpq_intr_mask(ms_intr_dist_t *idtp)
 * 	Mask off (prevent from interrupting) interrupt requests
 *	coming in on the specified islot request line on the
 * 	related CPU.
 *
 * Calling/Exit State:
 *	
 */
STATIC void
cpq_intr_mask(ms_intr_dist_t *idtp)
{
        ms_islot_t slot;
	ms_lockstate_t ls;

	PSM_ASSERT(!is_intr_enabled());

	slot = idtp->msi_slot;

	PSM_ASSERT(slot != CPQ_XINT_SLOT);
	PSM_ASSERT(slot != CPQ_CASC_SLOT);

	/* Never mask CPQ_CASC_SLOT, CPQ_TIMER_SLOT and CPQ_XINT_SLOT */
	if ((slot > cpq_islot_max) ||
	    (slot == CPQ_XINT_SLOT) ||
	    (slot == CPQ_CASC_SLOT) ||
            (slot == CPQ_TIMER_SLOT))
		return;

	ls = os_mutex_lock(cpq_mask_lock);

	/* Mask it logically */
	cpq_lintmask[os_this_cpu] |= (1 << slot);

	/* Mask it physically */
	if (cpq_intr_slot[slot].attached_to == os_this_cpu) {
		cpq_pintmask[os_this_cpu] |= (1 << slot);
		i8259_intr_mask(slot);
	}

	os_mutex_unlock(cpq_mask_lock, ls);
}


/*
 * STATIC void
 * cpq_intr_unmask(ms_intr_dist_t *idtp)
 * 	Unmask (allow to interrupt) interrupt requests coming
 *	in on the specified islot request line on the
 * 	related CPU.
 *
 * Calling/Exit State:
 *
 */
STATIC void
cpq_intr_unmask(ms_intr_dist_t *idtp)
{
        ms_islot_t slot;
	ms_lockstate_t ls;

	PSM_ASSERT(!is_intr_enabled());

	slot = idtp->msi_slot;

	ls = os_mutex_lock(cpq_mask_lock);

	/* Interrupt not attached */
	if (cpq_intr_slot[slot].attached_to == MS_CPU_ALL_BUT_ME) {
		os_mutex_unlock(cpq_mask_lock, ls);
		return;
	}

	/* Unmask it logically */
	cpq_lintmask[os_this_cpu] &= ~(1 << slot);

	/* Unmask it physically */
	if (cpq_intr_slot[slot].attached_to == os_this_cpu) {
		cpq_pintmask[os_this_cpu] &= ~(1 << slot);
		i8259_intr_unmask(slot);
	}

	os_mutex_unlock(cpq_mask_lock, ls);
}


/*
 * STATIC void
 * cpq_intr_complete(ms_intr_dist_t *idtp)
 *
 *
 * Calling/Exit State:
 *
 */
STATIC void
cpq_intr_complete(ms_intr_dist_t *idtp)
{
	PSM_ASSERT(!is_intr_enabled());

        /*
         * Unmask the interrupt if it was masked when the interrupt
         * was recognized.
         */
        if (idtp->msi_flags & MSI_MASK_ON_INTR) {
		cpq_pintmask[os_this_cpu] &= ~(1 << idtp->msi_slot);
		i8259_intr_unmask(idtp->msi_slot);
	} else {
		i8259_eoi(idtp->msi_slot);
	}
}


/*
 * STATIC ms_intr_dist_t *
 * cpq_service_timer(ms_ivec_t vec)
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
cpq_service_timer(ms_ivec_t vec)
{
	unsigned int newlock;

	PSM_ASSERT(!is_intr_enabled());

	/* Tick 1 / 2 management */
	os_post_events(cpq_clock_event);
	
	/* Clock propagation */
	if (ISASYMINTR) {

		PSM_ASSERT(os_this_cpu == cpq_booteng);

		cpq_xpost(MS_CPU_ALL_BUT_ME, MS_EVENT_PSM_1);

		newlock = cpq_rawtime_lock+1;
		if (newlock == 0) newlock++;
		cpq_rawtime_lock = 0;

		/* Do not care wrap arounds */
		cpq_rawtime += os_tick_period.mst_nsec / CPQ_NSECS_IN_USEC;

		cpq_rawtime_lock = newlock;
	}

	/* Treat clock as edge trigger */
	i8259_eoi(CPQ_TIMER_SLOT);

	return os_intr_dist_nop;
}


/*
 * STATIC ms_intr_dist_t *
 * cpq_service_xint(ms_ivec_t vec)
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
cpq_service_xint(ms_ivec_t vec)
{
	ms_event_t eventflags;
	ms_islot_t i;
	struct cpq_slot_info *sip;

	PSM_ASSERT(!is_intr_enabled());

	/* Check IPI */
	if ((*cpq_mpfuncs->cpq_ps_isxcall)() == MS_TRUE) {

		/* Clear IPI */
		(*cpq_mpfuncs->cpq_ps_clear_xintr)(os_this_cpu);

		eventflags = (ms_event_t)
			atomic_fnc(&cpq_xpost_eventflags[os_this_cpu]);

		/* clock propagation */
		if (eventflags & MS_EVENT_PSM_1) {
			eventflags &= ~MS_EVENT_PSM_1;

			if (ISASYMINTR) os_post_events(cpq_clock_event);
			/* else do nothing, it's the idle nudge */
		}

		/* interrupt cross enabling/disabling */
		if (eventflags & MS_EVENT_PSM_2) {
			eventflags &= ~MS_EVENT_PSM_2;

			/* Scan all interrupt slots */
			for (i = CPQ_FIRST_SLOT; i < CPQ_MAX_SLOT+1; i++) {

				/* Skip TIMER, CASC and XINT */
				if ((i == CPQ_TIMER_SLOT) ||
				    (i == CPQ_XINT_SLOT) ||
				    (i == CPQ_CASC_SLOT)) continue;

				/* Driver interrupts only */
				sip = &cpq_intr_slot[i];
				if ((sip->event2_pending == MS_TRUE) &&
				    (sip->attached_to == os_this_cpu)) {
					if (sip->masked == MS_TRUE) {
						cpq_pintmask[os_this_cpu] |=
								     (1 << i);
						i8259_intr_mask(i);
					}
					else {
						cpq_pintmask[os_this_cpu] &=
								     ~(1 << i);
						i8259_intr_unmask(i);
					}

					/* Sync up */
					sip->event2_pending = MS_FALSE;
				}
			}
		}

		/* OS flags */
		if (eventflags) os_post_events (eventflags);
	}

	/* Check FPU intr */
	if ((*cpq_mpfuncs->cpq_ps_fpu_intr)() == MS_TRUE) {

		/* No FPU error handling from PSMv2 on, anyway */

		cpq_clr_fpbusy();
	}

	/* XINT is edge trigger */
	if ((ISSYMINTR) || (os_this_cpu == cpq_booteng))
		i8259_eoi(CPQ_XINT_SLOT);

	return cpq_intr_slot[CPQ_XINT_SLOT].idtp;
}


/*
 * STATIC ms_intr_dist_t *
 * cpq_service_high_int(ms_ivec_t vec)
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
cpq_service_high_int(ms_ivec_t vec)
{
	ms_intr_dist_t *idtp;
	ms_islot_t slot;

	PSM_ASSERT(!is_intr_enabled());

	slot = cpq_vec2slot(vec);

	if (i8259_check_spurious(slot)) {
		idtp = os_intr_dist_nop;
		if (slot == CPQ_MAX_SLOT) i8259_eoi(CPQ_SP_SLOT);
	}
	else {

		idtp = cpq_intr_slot[slot].idtp;
		if (idtp->msi_flags & MSI_MASK_ON_INTR) {
			cpq_pintmask[os_this_cpu] |= (1 << slot);
			i8259_intr_mask(slot);
			i8259_eoi(slot);
		}
	}

	return idtp;
}

/*
 * STATIC ms_intr_dist_t *
 * cpq_service_int(ms_ivec_t vec)
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
cpq_service_int(ms_ivec_t vec)
{
	ms_intr_dist_t *idtp;
	ms_islot_t slot;

	PSM_ASSERT(!is_intr_enabled());

	slot = cpq_vec2slot(vec);

	idtp = cpq_intr_slot[slot].idtp;
	if (idtp->msi_flags & MSI_MASK_ON_INTR) {
		cpq_pintmask[os_this_cpu] |= (1 << slot);
		i8259_intr_mask(slot);
		i8259_eoi(slot);
	}

	return idtp;
}


/*
 * void
 * cpq_xpost(ms_cpu_t, ms_event_t)
 *
 *
 * Calling/Exit State:
 *
 */
void
cpq_xpost(ms_cpu_t cpu, ms_event_t eventmask)
{
	ms_cpu_t i;

	PSM_ASSERT(!is_intr_enabled());
	if (cpu != MS_CPU_ALL_BUT_ME) {
		atomic_or(&cpq_xpost_eventflags[cpu], eventmask);
		(*cpq_mpfuncs->cpq_ps_send_xintr)(cpu);
	} else {
		for (i=0; i<=cpq_cpu_max; i++)
			if (i != os_this_cpu) {
				atomic_or(&cpq_xpost_eventflags[i], eventmask);
				(*cpq_mpfuncs->cpq_ps_send_xintr)(i);
			}
	}
	
}


/*
 * STATIC void
 * cpq_offline_prep(void)
 *
 * Calling/Exit State:
 *
 * Remarks:
 *
 */
STATIC void
cpq_offline_prep(void)
{
	ms_lockstate_t ls, ls1;

	PSM_ASSERT(!is_intr_enabled());
	
	/*
	 * Platform specific offline_prep could reassign interrupt service
	 * to other CPUs. Prevent race conditions with intr_attach/intr_mask.
	 */
	ls = os_mutex_lock(cpq_attach_lock);
	ls1 = os_mutex_lock(cpq_mask_lock);

	/* mark CPU state before calling the platform specific function */
	cpq_engine_online[os_this_cpu] = MS_FALSE;
	cpq_howmany_online--;

	(*cpq_mpfuncs->cpq_ps_offline_prep)();

	os_mutex_unlock(cpq_mask_lock, ls1);
	os_mutex_unlock(cpq_attach_lock, ls);
}


/*
 * STATIC void
 * cpq_offline_self(void)
 *
 * Calling/Exit State:
 *
 * Remarks:
 *
 */
STATIC void
cpq_offline_self(void)
{
	PSM_ASSERT(!is_intr_enabled());

	/* Flush processor cache */
	(*cpq_mpfuncs->cpq_ps_reboot)(os_this_cpu);

	for (;;)
		asm("hlt");
	/* NOTREACHED */
}


/*
 * STATIC void
 * cpq_tick_2(ms_time_t)
 *	Enable/disable the tick 2 clock.
 *
 * Calling/Exit State:
 *
 */

STATIC void
cpq_tick_2(ms_time_t time)
{
	PSM_ASSERT(!is_intr_enabled());

	if (time.mst_sec || time.mst_nsec) {
		PSM_ASSERT(time.mst_sec == os_tick_period.mst_sec && 
			time.mst_nsec == os_tick_period.mst_nsec);
		cpq_clock_event = MS_EVENT_TICK_1 | MS_EVENT_TICK_2;
	} else
		cpq_clock_event = MS_EVENT_TICK_1;
}



/*
 * STATIC void
 * cpq_time_get(ms_rawtime_t *)
 *	Get the current value of a free-running high-resolution
 *	clock counter, in PSM-specific units.  This clock should
 *	not wrap around in less than an hour.
 *
 * Calling/Exit State:
 *
 *
 */

STATIC void
cpq_time_get(ms_rawtime_t *gtime)
{
	*gtime = (*cpq_mpfuncs->cpq_ps_usec_time)();
}


/*
 * STATIC void
 * cpq_time_add(ms_rawtime_t *, ms_rawtime_t *)
 *	Add 2 raw times.
 *
 * Calling/Exit State:
 *
 */
STATIC void
cpq_time_add(ms_rawtime_t *dst, ms_rawtime_t *src)
{
	unsigned int lo;

	dst->msrt_hi += src->msrt_hi;
	lo = dst->msrt_lo + src->msrt_lo;

	if ((lo < dst->msrt_lo) ||
	    (lo < src->msrt_lo)) dst->msrt_hi++;

	dst->msrt_lo = lo;
}


/*
 * STATIC void
 * cpq_time_sub(ms_rawtime_t *, ms_rawtime_t *)
 *	Subtract 2 raw times.
 *
 * Calling/Exit State:
 *
 */
STATIC void
cpq_time_sub(ms_rawtime_t *dst, ms_rawtime_t *src)
{
	unsigned int lo;

	dst->msrt_hi -= src->msrt_hi;
	lo = dst->msrt_lo - src->msrt_lo;
	if (lo > dst->msrt_lo) {
		dst->msrt_hi--;
	}
	dst->msrt_lo = lo;
}


/*
 * STATIC void
 * cpq_time_cvt(ms_time_t *, ms_rawtime_t *)
 *	Convert a PSM-specific high-resolution time value to seconds
 *	or nanoseconds.
 *
 * Calling/Exit State:
 *      - Convert the opaque time stamp to micro second.
 *
 * Remarks:
 *	- Conversions for times larger than two hours are not supported.
 *
 */
STATIC void
cpq_time_cvt(ms_time_t *dst, ms_rawtime_t *src)
{
	unsigned int sec=0, usec=0;

	usec = src->msrt_lo % CPQ_USECS_IN_SEC;

	/* src represent usec */
	if (src->msrt_hi == 1) {
		sec = CPQ_SECS_IN_1HIRT;
		usec += CPQ_USECS_IN_1HIRT;

		/* Wrap around on usec */
		if (usec >= CPQ_USECS_IN_SEC) {
			sec++;
			usec -= CPQ_USECS_IN_SEC;
		}
	}

	sec += src->msrt_lo / CPQ_USECS_IN_SEC;

	dst->mst_sec = sec;
	dst->mst_nsec = usec * CPQ_NSECS_IN_USEC;
}



/*
 * STATIC void
 * cpq_idle_self(void)
 *
 *
 * Calling/Exit State:
 *
 *
 */
STATIC void
cpq_idle_self(void)
{
	int ena;

	PSM_ASSERT(is_intr_enabled());

	if (ISASYMINTR) {
		while(cpq_idle_engine[os_this_cpu] == MS_FALSE)
			cpq_noop();
	} else {

		ena = intr_disable();

		while(cpq_idle_engine[os_this_cpu] == MS_FALSE) {
			asm("sti");
			asm("hlt");
			ena = intr_disable();
		}

		intr_restore(ena);
	}

	cpq_idle_engine[os_this_cpu] = MS_FALSE;
}


/*
 * STATIC void
 * cpq_idle_exit(ms_cpu_t)
 *
 * Calling/Exit State:
 *
 *
 */
STATIC void
cpq_idle_exit(ms_cpu_t cpu)
{
	cpq_idle_engine[cpu] = MS_TRUE;

	if ((ISSYMINTR) && (cpu != os_this_cpu))
		cpq_xpost(cpu, MS_EVENT_PSM_1);
}


/*
 * STATIC void
 * cpq_start_cpu(ms_cpu_t, ms_paddr_t)
 *
 *
 *
 * Calling/Exit State:
 *
 *
 */
STATIC void
cpq_start_cpu(ms_cpu_t cpu, ms_paddr_t reset_code)
{
	struct resetaddr {
		unsigned short	offset;
		unsigned short	segment;
	} start;
	unsigned char *vector, *source;
	int i, ena;

	PSM_ASSERT(is_intr_enabled());
	ena = intr_disable();

	/*
	 * Historically we have never temporarily switched CMOS
	 * to WARM_RESET.
	 */

	/* get the real address seg:offset format */
	start.offset = reset_code & 0x0f;
	start.segment = (reset_code >> 4) & 0xFFFF;

	/* now put the address into warm reset vector (40:67) */
	vector = (unsigned char *)((int) os_page0 + CPQ_RESET_VECT);

	/*
	 * copy byte by byte since the reset vector port is
	 * not word aligned
	 */
	source = (unsigned char *) &start;
	for (i = 0; i < sizeof(struct resetaddr); i++)
		*vector++ = *source++;

        (*cpq_mpfuncs->cpq_ps_online_engine)(cpu);

	intr_restore(ena);
}



/*
 * STATIC void
 * cpq_shutdown(int)
 *	Shutdown and/or reboot the system.  This operation handles
 *	low-level machine state.  Any software state involved in  
 *	shutdown/reboot will have been taken care of by the core
 *	kernel.
 *
 * Calling/Exit State:
 *      Intended use for action values:
 *              action == MS_SD_HALT  
 *              action == MS_SD_POWEROFF
 *              action == MS_SD_AUTOBOOT
 *              action == MS_SD_BOOTPROMPT
 *
 */

STATIC void
cpq_shutdown(int action)
{
	ms_cpu_t cpu;

	PSM_ASSERT(!is_intr_enabled());

	for (cpu = 0; cpu < cpq_cpu_max ; cpu++) {
		if ((cpu == os_this_cpu) || (cpu == cpq_booteng)) continue;

		/*
		 * Prohibit processor from gaining access to M bus and
		 * assert the reset line for non-boot engine.
		 */
		(*cpq_mpfuncs->cpq_ps_reboot)(cpu);
	}

	psm_softreset(os_page0);

	switch (action) {

	case MS_SD_HALT:
		break;

	case MS_SD_AUTOBOOT:
	case MS_SD_BOOTPROMPT:
		psm_sysreset();
		break;

	}
	for(;;)
		asm("   hlt     ");
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

unsigned int cpq_spintime = 20000000;
unsigned int cpq_spintime_fine = 20000;

/*
 * STATIC void
 * cpq_usec_time_debug(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This function helps to debug psm_time_spin() and cpq_time_get().
 *	Default time_spin is twenty seconds.
 *
 *	Within kdb we can modify the cpq_wait_time value.
 */
STATIC void
cpq_usec_time_debug(void)
{
	int i, j;
	ms_rawtime_t time1, time2;

	os_printf ("\n Time spin debug code.\n");
	os_printf (" Change cpq_spintime if not using 20 second default.\n");
	os_printf (" Change cpq_spintime_fine if not using 20 millisecond default.\n");
	
	os_printf (" Current spin time = %d microseconds.\n", cpq_spintime);
	os_printf (" Current fine spin time = %d microseconds.\n",
		   cpq_spintime_fine);
	os_printf (" Testing psm_time_spin().\n");
	os_printf ("  Spin in one go: ");
	psm_time_spin(cpq_spintime);

	os_printf ("Done.\n");
	os_printf ("  Spin splitted in 1000 small spins : ");

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 100; j++)
			psm_time_spin(cpq_spintime_fine);

		os_printf (".");
	}

	os_printf (" Done.\n");

	os_printf (" Testing cpq_time_get().\n");
	os_printf ("  Spin in one go: ");

	time2.msrt_hi = 0;
	time2.msrt_lo = cpq_spintime;

	/* Get current time */
	cpq_time_get(&time1);

	/* time1 start, time2 target */
	cpq_time_add(&time2, &time1);

	/* Now spin */
	if (time2.msrt_hi) {
		while(time2.msrt_hi) {
			while (time1.msrt_lo >= time2.msrt_lo)
				cpq_time_get(&time1);

			time2.msrt_hi--;

			while (time1.msrt_lo < time2.msrt_lo)
				cpq_time_get(&time1);

		}
	} else 
		while (time1.msrt_lo < time2.msrt_lo)
			cpq_time_get(&time1);

	os_printf ("Done.\n");
	os_printf ("  Spin splitted in 1000 small spins : ");

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 100; j++) {

			time2.msrt_hi = 0;
			time2.msrt_lo = cpq_spintime_fine;

			/* Get current time */
			cpq_time_get(&time1);

			/* time1 start, time2 target */
			cpq_time_add(&time2, &time1);

			/* Now spin */
			if (time2.msrt_hi) {
				while(time2.msrt_hi) {
					while (time1.msrt_lo >= time2.msrt_lo)
						cpq_time_get(&time1);

					time2.msrt_hi--;

					while (time1.msrt_lo < time2.msrt_lo)
						cpq_time_get(&time1);

				}
			} else 
				while (time1.msrt_lo < time2.msrt_lo)
					cpq_time_get(&time1);


		}
		os_printf (".");
	}

	os_printf (" Done.\n");

}

/*
 * STATIC void
 * cpq_print_intmap(void)
 *	Debug function to print current interrupt distribution map.
 *
 * Calling/Exit State:
 *
 */

STATIC void
cpq_print_intmap(void)
{
	ms_islot_t i;
	char *inttype;
	char *cpu;
	char *event2;
	char *masked;
	char *numbers[6]={"-", "ALL", "0", "1", "2", "3"};
	char *status[2]={"No", "Yes"};
	char *mask[2]={"Unmask", "Mask"};

	os_printf ("\n Interrupt distribution map.\n");
	os_printf (" SLOT\tTYPE\tCPU\tEVENT\tEVENT2 PENDING\n");

	for (i = CPQ_FIRST_SLOT; i <= CPQ_MAX_SLOT; i++) {
		inttype = "Stat";
		if (cpq_intr_slot[i].idtp->msi_cpu_dist == MS_CPU_ANY)
			inttype = "Dist";

		cpu = numbers[cpq_intr_slot[i].attached_to + 2];
		masked = mask[cpq_intr_slot[i].masked];
		event2 = status[cpq_intr_slot[i].event2_pending];

		if ((i == CPQ_CASC_SLOT) ||
		    (i == CPQ_XINT_SLOT) ||
		    (i == CPQ_TIMER_SLOT)) {
			inttype = "Priv";
			masked = "-";
			event2 = "-";
		}

		os_printf (" %d\t%s\t%s\t%s\t%s\n",
        		   i, inttype, cpu, masked, event2);
	}
}
#endif	/* DEBUG || DEBUG_TOOLS */





msop_func_t cpq_msops[] = {
	{ MSOP_INIT_CPU,	(void *)cpq_init_cpu },
	{ MSOP_INTR_ATTACH,     (void *)cpq_intr_attach },
	{ MSOP_INTR_DETACH,     (void *)cpq_intr_detach },
	{ MSOP_INTR_MASK,       (void *)cpq_intr_mask },
	{ MSOP_INTR_UNMASK,     (void *)cpq_intr_unmask },
	{ MSOP_INTR_COMPLETE,   (void *)cpq_intr_complete },
	{ MSOP_TICK_2,		(void *)cpq_tick_2 },
	{ MSOP_TIME_GET,	(void *)cpq_time_get },
	{ MSOP_TIME_ADD,	(void *)cpq_time_add },
	{ MSOP_TIME_SUB,	(void *)cpq_time_sub },
	{ MSOP_TIME_CVT,	(void *)cpq_time_cvt },
	{ MSOP_TIME_SPIN,	(void *)psm_time_spin },
	{ MSOP_XPOST, 		(void *)cpq_xpost },
	{ MSOP_RTODC,		(void *)psm_mc146818_rtodc },
	{ MSOP_WTODC,		(void *)psm_mc146818_wtodc },
	{ MSOP_IDLE_SELF,       (void *)cpq_idle_self },
	{ MSOP_IDLE_EXIT,       (void *)cpq_idle_exit },
	{ MSOP_START_CPU,     	(void *)cpq_start_cpu },
	{ MSOP_OFFLINE_PREP,    (void *)cpq_offline_prep },
	{ MSOP_OFFLINE_SELF,    (void *)cpq_offline_self },
	{ MSOP_SHUTDOWN,	(void *)cpq_shutdown },
	{ 0,			NULL }
};

