#ident	"@(#)kern-i386at:psm/atup/atup.c	1.4.5.2"
#ident  "$Header$"

/*
 * AT PSM/mHAL hardware-specific routines and data.
 *
 * This file contains a set of subroutines and a set of data, all of which
 * are used elsewhere in the kernel but which vary for MP hardware from
 * different vendors. In general, all the routines with an "atup_" prefix
 * should be defined for whatever hardware you use, even if the routine does
 * nothing. The same applies for data (unfortunately the data names may not
 * have a distinctive prefix), even if it might not be used. Some routines
 * and data defined in this file are for specific platform only. Vendors
 * do not need to support these items for their own hardware.
 *
 * The requirements for each item in this file are clearly stated in the
 * PSM/mHAL interface specification document.
 *
 * This PSM/mHAL file supports the uniprocessor system.
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_assert.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>
#include <psm/toolkits/psm_i8254/psm_i8254.h>
#include <psm/toolkits/psm_mc146818/psm_mc146818.h>
#include <psm/toolkits/at_toolkit/at_toolkit.h>
#include <psm/toolkits/psm_time/psm_time.h>
#include <psm/atup/atup.h>

#define atup_vec2slot(v)	(v-I8259_VBASE)		/* vectors start at 32 */

/*
 *
 */

STATIC ms_event_t	atup_clock_event = MS_EVENT_TICK_1;
STATIC ms_intr_dist_t  *atup_intr_vect[AT_MAX_SLOT + 1];
STATIC volatile ms_rawtime_t	atup_rawtime={0,0}; /* elapsed time since boot*/
STATIC i8254_params_t	atup_i8254_params;		/* timer params */
STATIC unsigned int	atup_bustype;		/* One bit for each bus type */
STATIC volatile	int	atup_idleflag;		/* for idle_self, idle_exit */


/*
 * Function prototypes for msops
 */

void            atup_init_cpu(void);

ms_bool_t       atup_intr_attach(ms_intr_dist_t*);
void            atup_intr_detach(ms_intr_dist_t*);
void            atup_intr_mask(ms_intr_dist_t*);
void            atup_intr_unmask(ms_intr_dist_t*);
void            atup_intr_complete(ms_intr_dist_t*);

void            atup_tick_2(ms_time_t);
void            atup_time_get(ms_rawtime_t*);
void            atup_time_add(ms_rawtime_t*, ms_rawtime_t*);
void            atup_time_sub(ms_rawtime_t*, ms_rawtime_t*);
void            atup_time_cvt(ms_time_t*, ms_rawtime_t*);

void            atup_idle_self(void);
void            atup_idle_exit(ms_cpu_t);
void            atup_shutdown(int);
void            atup_offline_prep(void);
void            atup_offline_self(void);
void            atup_online(ms_cpu_t, void(*reset_code)());
void            atup_show_state(void);

/*
 * Machine-specific function pointer array.
 */

msop_func_t atup_msops[] = {
        { MSOP_INIT_CPU,        (void *)atup_init_cpu },
        { MSOP_INTR_ATTACH,     (void *)atup_intr_attach },
        { MSOP_INTR_DETACH,     (void *)atup_intr_detach },
        { MSOP_INTR_MASK,       (void *)atup_intr_mask },
        { MSOP_INTR_UNMASK,     (void *)atup_intr_unmask },
        { MSOP_INTR_COMPLETE,   (void *)atup_intr_complete },
        { MSOP_TICK_2,          (void *)atup_tick_2 },
        { MSOP_TIME_GET,        (void *)atup_time_get },
        { MSOP_TIME_ADD,        (void *)atup_time_add },
        { MSOP_TIME_SUB,        (void *)atup_time_sub },
        { MSOP_TIME_CVT,        (void *)atup_time_cvt },
        { MSOP_TIME_SPIN,       (void *)psm_time_spin },
        { MSOP_IDLE_SELF,       (void *)atup_idle_self },
        { MSOP_IDLE_EXIT,       (void *)atup_idle_exit },
        { MSOP_RTODC,           (void *)psm_mc146818_rtodc },
        { MSOP_WTODC,           (void *)psm_mc146818_wtodc },
        { MSOP_OFFLINE_PREP,    (void *)atup_offline_prep },
        { MSOP_OFFLINE_SELF,    (void *)atup_offline_self },
        { MSOP_SHUTDOWN,        (void *)atup_shutdown },
        { MSOP_SHOW_STATE,      (void *)atup_show_state },
        { 0,                    0 }
};

/*
 * Values to pass to os_set_msparam.
 */

STATIC char            atup_platform_name[] = "Generic AT";
STATIC ms_bool_t       atup_sw_sysdump = MS_TRUE;
STATIC ms_time_t       atup_time_res = {10000000,0};
STATIC ms_time_t       atup_tick_1_res = {100000,0};
STATIC ms_time_t       atup_tick_1_max = {27000000,0};
STATIC ms_islot_t      atup_islot_max = AT_MAX_SLOT;
STATIC unsigned        atup_shutdown_caps = (MS_SD_HALT | MS_SD_AUTOBOOT | MS_SD_BOOTPROMPT);

/*
 * Function prototypes for internal procedures.
 */

STATIC ms_topology_t	* atup_merge_topologies();
STATIC ms_intr_dist_t   *atup_service_int(ms_ivec_t);
STATIC ms_intr_dist_t   *atup_service_timer(ms_ivec_t);
STATIC ms_intr_dist_t   *atup_service_spur(ms_ivec_t);
STATIC void             atup_init_msparam();

/*
 * void
 * atup_initpsm()
 *
 * Calling/Exit State:
 *      This function is called exactly once as the first entry
 *      into the PSM.
 *
 * Description:
 *      atup_initpsm does several tasks:
 *              - performs sufficient hardware initialization and
 *                detection to be able to determine operating
 *                parameters (such as number of CPUs).
 *              - registers MSOP entry points which can be called
 *                at a later time by the core kernel.
 *              - checks and set operating parameters (such as
 *                number of CPUs) which are relevant to the machine
 *                specific hardware it controls.  
 */
ms_bool_t
atup_initpsm()
{
	ms_islot_t	slot;

        /*
         * Register the machine-specific operations. If this fails,
         * the psm cannot be loaded and a return code of MS_FALSE
         * will be passed to the kernel.   This is the last valid
         * juncture in which to pass back a return value of MS_FALSE.
         */
        if (os_register_msops(atup_msops) != MS_TRUE)
                return (MS_FALSE);

	atup_init_msparam();

	psm_time_spin_init();

        /*
         * Mask all interrupts until the kernel is ready
	 * to handle them, at which point the kernel will
	 * call xxx_intr_init.
         */
        for (slot=0; slot<=AT_MAX_SLOT; slot++)
        	i8259_intr_mask(slot);

	return(MS_TRUE);
}


/*
 * STATIC void
 * atup_init_cpu(void)
 *      Initialize a cpu when it is coming online.
 *
 * Calling/Exit State:
 *	Called once on each cpu.
 *	Must be called with the IE flag off.
 *
 */
STATIC void
atup_init_cpu(void)
{
	unsigned i;
	int dummy;

        PSM_ASSERT(!is_intr_enabled());

	i8254_init(&atup_i8254_params, I8254_CTR0_PORT, os_tick_period);

	/*
	 * os_alloc (called by i8254_init) is forcing a sti.
	 * We don't want interrupts enabled at this time.
	 */
	dummy = intr_disable();

	i8259_init(atup_bustype);

	/*
 	 * Claim vectors. all are assigned to the IO interrupt handler
	 * except for the clock vector and the spurrious interrupt
	 * vectors.
 	 */
        os_claim_vectors(I8259_VBASE, AT_MAX_SLOT+1, atup_service_int);
        os_claim_vectors(I8259_VBASE+AT_TIMER_SLOT, 1, atup_service_timer);
        for (i = 1; i <= I8259_MAX_ICS; i++)
          os_claim_vectors(I8259_VBASE+(i*I8259_NIRQ)-1, 1, atup_service_spur);

        i8259_intr_unmask(AT_TIMER_SLOT);

	/*
  	 * Initialize the interrupt distribution vector pointer array
 	 * with appropriate values.
  	 */
	for (i = 0; i <= AT_MAX_SLOT; i++)
		atup_intr_vect[i] = os_intr_dist_stray;
}

/*
 * STATIC ms_bool_t
 * atup_intr_attach(ms_intr_dist_t *idtp)
 *      Enter the specified interrupt source into the interrupt
 *      distribution vector pointer array and (if not already
 *      enabled) enable distribution of this interrupt source in
 *      the interrupt controller(s).  If the interrupt was masked
 *      upon entry to this function, it becomes unmasked.
 *
 * Calling/Exit State:
 *      Called when the system is being initialized.  This function
 *      must be called with the IE flag off.
 *
 */
STATIC ms_bool_t
atup_intr_attach(ms_intr_dist_t *idtp)
{
        int             i;

        PSM_ASSERT(!is_intr_enabled());
        PSM_ASSERT(idtp->msi_slot <= AT_MAX_SLOT
                   && idtp->msi_slot != AT_TIMER_SLOT
                   && idtp->msi_slot != AT_CASCADE_SLOT);

        for (i = 0; i <= AT_MAX_SLOT; i++) {
                if (atup_intr_vect[i] == idtp)
                        atup_intr_detach( idtp );
        }

	atup_intr_vect[idtp->msi_slot] = idtp;

	idtp->msi_flags |= MSI_ORDERED_COMPLETES;
	idtp->msi_flags &= ~MSI_NONMASKABLE;

        i8259_intr_attach(idtp, atup_bustype); 

        i8259_intr_unmask(idtp->msi_slot); 

	return(MS_TRUE);
}

/*
 * STATIC void
 * atup_intr_detach(ms_intr_dist_t *idtp)
 *      Detach the specified interrupt source from the dispatching
 *      tables and disable distribution of this interrupt source
 *      in the interrupt controller(s).
 *
 *
 * Calling/Exit State:
 *      After this operation returns, there should be no more interrupt
 *      deliveries using the specified interrupt distribution structure
 *      pointer.
 */
STATIC void
atup_intr_detach(ms_intr_dist_t *idtp)
{
        PSM_ASSERT(!is_intr_enabled());

        i8259_intr_mask(idtp->msi_slot);

	/*
 	 * Use interrupt slot number as an index into interrupt distribution 
 	 * vector array to disable the ms_intr_dist_t pointer for this interrupt
 	 * vector.
 	 */

	atup_intr_vect[idtp->msi_slot] = os_intr_dist_stray;


}

/*
 * STATIC void
 * atup_intr_mask(ms_intr_dist_t *idtp)
 * 	Mask off (prevent from interrupting) interrupt requests
 *	coming in on the specified islot request line on the
 * 	specified PIC.  Never mask off interrupt slot 0, since
 *	this is where clock interrupts arrive.
 *
 * Calling/Exit State:
 *	
 */
STATIC void
atup_intr_mask(ms_intr_dist_t *idtp)
{
        if( idtp->msi_slot != AT_TIMER_SLOT ) 
		i8259_intr_mask(idtp->msi_slot);
}

/*
 * STATIC void
 * atup_intr_unmask(ms_intr_dist_t *idtp)
 * 	Unmask (allow to interrupt) interrupt requests coming
 *	in on the specified islot request line on the
 * 	specified PIC.
 *
 * Calling/Exit State:
 *
 */
STATIC void
atup_intr_unmask(ms_intr_dist_t *idtp)
{
        PSM_ASSERT(!is_intr_enabled());

	i8259_intr_unmask(idtp->msi_slot);
}

/*
 * STATIC void
 * atup_intr_complete(ms_intr_dist_t *idtp)
 *
 *
 * Calling/Exit State:
 *
 */
STATIC void
atup_intr_complete(ms_intr_dist_t *idtp)
{
        PSM_ASSERT(!is_intr_enabled());

	/*
 	 * Unmask the interrupt if it was masked when the interrupt
	 * was recognized.
 	 */
	if( idtp->msi_flags & MSI_MASK_ON_INTR)
                i8259_intr_unmask( idtp->msi_slot );
}


/*
 * void 
 * atup_service_int( ms_ivec_t vec )
 *
 * Calling/Exit State:
 *
 */
ms_intr_dist_t *
atup_service_int( ms_ivec_t vec )
{
	ms_intr_dist_t	*idtp;
	ms_islot_t	slot;

        PSM_ASSERT(!is_intr_enabled());

	slot = atup_vec2slot( vec );

        PSM_ASSERT(slot <= AT_MAX_SLOT);

        idtp = atup_intr_vect[slot];
        if( idtp->msi_flags & MSI_MASK_ON_INTR )
                i8259_intr_mask( slot );

        i8259_eoi( slot );

        return idtp;
}

/*
 * void 
 * atup_service_timer( ms_ivec_t vec )
 *
 * Calling/Exit State:
 *
 */
ms_intr_dist_t *
atup_service_timer( ms_ivec_t vec )
{
	unsigned int lo;
	unsigned char flg;

        PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(vec == 32);

	lo = atup_rawtime.msrt_lo + os_tick_period.mst_nsec;
	if (lo >= 1000000000) {
		lo -= 1000000000;
		atup_rawtime.msrt_hi++;
	}
	atup_rawtime.msrt_lo = lo;

        if (atup_bustype & (1 << MSR_BUS_MCA)) {
                flg = inb(I8254_AUX_PORT);
                flg |= 0x80;
                outb(I8254_AUX_PORT,flg);
        }

	os_post_events(atup_clock_event);

	i8259_eoi(AT_TIMER_SLOT);
	return (os_intr_dist_nop);
}



/*
 * void 
 * atup_service_spur( ms_ivec_t vec )
 *
 * Calling/Exit State:
 *
 */
ms_intr_dist_t *
atup_service_spur( ms_ivec_t vec )
{
       ms_intr_dist_t  *idtp;
        ms_islot_t      slot;

        PSM_ASSERT(!is_intr_enabled());

        slot = atup_vec2slot( vec );

        PSM_ASSERT(slot <= AT_MAX_SLOT);
        PSM_ASSERT((slot % I8259_NIRQ) == (I8259_NIRQ - 1));

        if( i8259_check_spurious(slot) ) {
                idtp = os_intr_dist_nop;
                if ((slot / I8259_NIRQ) != 0)
                        i8259_eoi(I8259_NIRQ-1);
        } else {
                idtp = atup_intr_vect[slot];
                if( idtp->msi_flags & MSI_MASK_ON_INTR )
                        i8259_intr_mask( slot );
                i8259_eoi( slot );
        }
        return idtp;
}



/*
 * STATIC void
 * atup_offline_prep(void)
 *
 * Calling/Exit State:
 *
 * Remarks:
 *
 */
STATIC void
atup_offline_prep(void)
{
        ms_islot_t      slot;

        for (slot=0; slot<=AT_MAX_SLOT; slot++)
                i8259_intr_mask(slot);
}


/*
 * STATIC void
 * atup_offline_self(void)
 *
 * Calling/Exit State:
 *
 * Remarks:
 *
 */
STATIC void
atup_offline_self(void)
{
	for(;;)
		asm("hlt");
	/* NOTREACHED */
}



/*
 * STATIC void
 * atup_tick2(ms_time_t)
 *	Enable/disable the tick 2 clock.
 *
 * Calling/Exit State:
 *
 */

STATIC void
atup_tick_2(ms_time_t time)
{
	if (time.mst_sec || time.mst_nsec)
		atup_clock_event = MS_EVENT_TICK_1 | MS_EVENT_TICK_2;
	else
		atup_clock_event = MS_EVENT_TICK_1;
}


/*
 * STATIC void
 * atup_time_get(ms_rawtime_t *)
 *	Get the current value of a free-running high-resolution
 *	clock counter, in PSM-specific units.  This clocks should
 *	not wrap around in less than an hour.
 *
 * Calling/Exit State:
 *	- Save the current time stamp that is opaque to the base kernel
 *	  in ms_rawtime_t.	  
 */

STATIC void
atup_time_get(ms_rawtime_t *gtime)
{
	unsigned int	hi, hi2;

	/*
	 * Pick up both halves of the rawtime. Protect against the
	 * (unlikely) case where an interrupt occurs between the 2
	 * acceses to the words & changes both hi & lo.
	 */
	do {
		hi = atup_rawtime.msrt_hi;
		gtime->msrt_lo = atup_rawtime.msrt_lo;
		hi2 = atup_rawtime.msrt_hi;
	} while (hi != hi2);
	gtime->msrt_hi = hi;
}


/*
 * STATIC void
 * atup_time_add(ms_rawtime_t *, ms_rawtime_t *)
 *	Add 2 raw times.
 *
 * Calling/Exit State:
 *
 */
STATIC void
atup_time_add(ms_rawtime_t *dst, ms_rawtime_t *src)
{
	unsigned int	lo;
	dst->msrt_hi += src->msrt_hi;
	lo = dst->msrt_lo + src->msrt_lo;
	if (lo >= 1000000000) {
		lo -= 1000000000;
		dst->msrt_hi++;
	}
	dst->msrt_lo = lo;
}


/*
 * STATIC void
 * atup_time_sub(ms_rawtime_t *, ms_rawtime_t *)
 *	Subtract 2 raw times.
 *
 * Calling/Exit State:
 *
 */
STATIC void
atup_time_sub(ms_rawtime_t *dst, ms_rawtime_t *src)
{
	int	lo;
	dst->msrt_hi -= src->msrt_hi;
	lo = (int)dst->msrt_lo - (int)src->msrt_lo;
	if (lo < 0) {
		lo += 1000000000;
		dst->msrt_hi--;
	}
	dst->msrt_lo = (unsigned int)lo;
}


/*
 * STATIC void
 * atup_time_cvt(ms_time_t *, ms_rawtime_t *)
 *	Convert a PSM-specific high-resolution time value to seconds
 *	or nanoseconds.
 *
 * Calling/Exit State:
 *      - Convert the opaque time stamp to micro second.
 *
 */
STATIC void
atup_time_cvt(ms_time_t *dst, ms_rawtime_t *src)
{
	dst->mst_sec = src->msrt_hi;
	dst->mst_nsec = src->msrt_lo;
}


/*
 * STATIC void
 * atup_idle_self(void)
 *	Idle the current cpu.
 *
 * Calling/Exit State:
 *
 */
STATIC void
atup_idle_self(void)
{
	int ena;

	PSM_ASSERT(is_intr_enabled());

	ena = intr_disable();

	while (atup_idleflag == 0) {
		asm("sti");
		asm("hlt");
		ena = intr_disable();
	}

	intr_restore(ena);

	atup_idleflag = 0;
}


/*
 * STATIC void
 * atup_idle_exit(ms_cpu_t cpu)
 *	Wake the specified cpu from its idle loop.
 *
 * Calling/Exit State:
 *
 */
/*ARGSUSED*/
STATIC void
atup_idle_exit(ms_cpu_t cpu)
{
	atup_idleflag = 1;
}


/*
 * STATIC void
 * atup_init_msparam(void)
 *
 * Calling/Exit State:
 *
 */
STATIC void
atup_init_msparam()
{
        os_set_msparam(MSPARAM_PLATFORM_NAME, &atup_platform_name);
        os_set_msparam(MSPARAM_SW_SYSDUMP, &atup_sw_sysdump);
        os_set_msparam(MSPARAM_TIME_RES, &atup_time_res);
        os_set_msparam(MSPARAM_TICK_1_RES, &atup_tick_1_res);
        os_set_msparam(MSPARAM_TICK_1_MAX, &atup_tick_1_max);
        os_set_msparam(MSPARAM_ISLOT_MAX, &atup_islot_max);
        os_set_msparam(MSPARAM_SHUTDOWN_CAPS, &atup_shutdown_caps);
	os_set_msparam(MSPARAM_TOPOLOGY, (void *) atup_merge_topologies() );
}


/*
 * STATIC ms_topology_t *
 * atup_merge_topologies(void)
 *
 * Calling/Exit State:
 *
 */
STATIC ms_topology_t * atup_merge_topologies()
{
	static ms_topology_t topo;
	ms_resource_t * rp;
	char *cp;
	unsigned i, nr;

	atup_bustype = 0;

	for (i = 0, rp = os_default_topology->mst_resources;
	     i < os_default_topology->mst_nresource; i++, rp++) {
		if (rp->msr_type == MSR_BUS)
			atup_bustype |= (1 << rp->msri.msr_bus.msr_bus_type);
	}

	nr = os_count_himem_resources();
	
	if(nr == 0)
		return os_default_topology;

	topo.mst_nresource = os_default_topology->mst_nresource + nr;
	rp = topo.mst_resources =
		os_alloc((topo.mst_nresource) * sizeof(ms_resource_t)); 
	
	for ( i = 0; i < os_default_topology->mst_nresource; i++ )
		*rp++ = os_default_topology->mst_resources[i];

	os_fill_himem_resources(rp);
		
	return &topo;

}

/*
 * void
 * atup_shutdown(int action)
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
atup_shutdown(int action)
{
        psm_softreset(os_page0);

	atup_offline_prep();
	switch (action) {

        case MS_SD_HALT:
                break;

        case MS_SD_AUTOBOOT:
	case MS_SD_BOOTPROMPT:
		psm_sysreset();
		break;
	}

	for(;;)
		asm("hlt");
}

/*
 * STATIC void
 * atup_show_state(void)
 *      Print out the platform-specific state, if any, which might be
 *      useful in a system crash analysis. Also free up the interrupt
 *      controller from In Service interrupts. Level interrupts can
 *      be a problem since atup_show_state() is called with interrupt
 *      disabled at the cpu. It means no one is masking that interrupt
 *      until a detach is done. PIC will not stop from presenting the
 *      first level interrupt undefinitely vanishing the loop for
 *      MAX_SLOT. For this reason ms_show_state() needs to be called
 *      at least after the set of detach of unwanted interrupts.
 *
 * Calling/Exit State:
 *      Upon exit of this function, the system crash will commence with
 *      a subsequent call to atup_shutdown to shutdown the system.
 */

STATIC void
atup_show_state()
{
        ms_islot_t      slot;

        for (slot=0; slot<=AT_MAX_SLOT; slot++)
                i8259_eoi(slot);
}

