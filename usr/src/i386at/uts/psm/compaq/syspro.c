/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/syspro.c	1.5.4.2"
#ident	"$Header$"

/*
 * COMPAQ SYSTEMPRO HARDWARE INFO:
 *      - All interrupts are distributed only to processor 0.
 *	- Symmetric I/O access.
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>
#include <psm/toolkits/psm_i8254/psm_i8254.h>
#include <psm/toolkits/psm_time/psm_time.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_assert.h>
#include <psm/compaq/compaq.h>
#include <psm/compaq/syspro.h>



/*
 * Configurable objects -- defined in Space.c
 */
extern ms_cpu_t cpq_sp_maxnumcpu;		 /* max. cpus in SP or SP/XL */
extern unsigned short cpq_engine_ctl_port[];
extern unsigned short cpq_engine_ivec_port[];

extern unsigned char 		cpq_mptype;
extern i8254_params_t		cpq_i8254_params;
extern ms_cpu_t			cpq_booteng;
extern unsigned int		cpq_rawtime;		     /* time counter */
extern unsigned int		cpq_rawtime_lock;	  /* asymmetric lock */
extern struct cpq_slot_info	cpq_intr_slot[];

/*
 * Module functions
 */
ms_cpu_t	cpq_sp_initpsm(void);
void		cpq_sp_init_cpu(void);
ms_cpu_t	cpq_sp_assignvec(ms_islot_t);
void		cpq_sp_set_eltr(ms_cpu_t, ms_islot_t, ms_intr_dist_t *);
void		cpq_sp_bootcpu(ms_cpu_t);
void		cpq_sp_cpuintr(ms_cpu_t);
void		cpq_sp_cpuclrintr(ms_cpu_t);
ms_bool_t	cpq_sp_isxcall(void);
ms_bool_t	cpq_sp_fpu_intr(void);
void		cpq_sp_offline_prep(void);
void		cpq_sp_stopcpu(ms_cpu_t);
ms_rawtime_t 	cpq_sp_usec_time(void);


/*
 * Platform specific function table
 */
struct cpq_psmops cpq_spmpfuncs = {
	cpq_sp_initpsm,				/* cpq_ps_initpsm */
	cpq_sp_init_cpu,			/* cpq_ps_init_cpu */
	cpq_sp_assignvec,			/* cpq_ps_assignvec */
	cpq_sp_set_eltr,			/* cpq_ps_set_eltr */
	cpq_sp_bootcpu,				/* cpq_ps_online_engine */
	cpq_sp_cpuintr,				/* cpq_ps_send_xintr */
	cpq_sp_cpuclrintr,			/* cpq_ps_clear_xintr */
	cpq_sp_isxcall,				/* cpq_ps_isxcall */
	cpq_sp_fpu_intr,			/* cpq_ps_fpu_intr */
	cpq_sp_offline_prep,			/* cpq_ps_offline_prep */
	cpq_sp_stopcpu,				/* cpq_ps_reboot */
	cpq_sp_usec_time			/* cpq_ps_usec_time */
};



/*
 * STATIC void
 * cpq_sp_stopcpu(ms_cpu_t engnum)
 *	Stop additional (P2) CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_sp_stopcpu(ms_cpu_t engnum)
{

	if (engnum == os_this_cpu) {
		/* Offline case: flush cache */
		outb(cpq_engine_ctl_port[engnum],
			(SP_INTDIS | SP_CACHEON | SP_CACHEFLUSH));

		/* Give it a small time */
		psm_time_spin(1000);

		/* Turn cache off */
		outb(cpq_engine_ctl_port[engnum], SP_INTDIS);

		/* Give it a small time */
		psm_time_spin(1000);

		/* Put processor to sleep */
		outb(cpq_engine_ctl_port[engnum],
		     (SP_INTDIS | SP_RESET | SP_PHOLD));
	} else
		/* Shutdown or online case */
		outb(cpq_engine_ctl_port[engnum], SP_RESET | SP_PHOLD);
}


/*
 * STATIC void
 * cpq_sp_init_cpu(void)
 *	Initialize programmable interrupt timer.
 *	Initialize programmable interrupt controller.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 */
STATIC void
cpq_sp_init_cpu(void)
{
	int dummy;

	if (os_this_cpu == cpq_booteng) {

		/* Initialize the i8254 programmable interrupt timer. */
		i8254_init(&cpq_i8254_params, I8254_CTR0_PORT, os_tick_period);

		/*
	 	 * os_alloc (called by i8254_init) is forcing a sti.
         	 * We don't want interrupts enabled at this time.
	 	 */
		dummy = intr_disable();

		/* Initialize the i8259 based interrupt controller. */
		i8259_init(1 << MSR_BUS_EISA);

		/*
		 * Set TIMER, CASCADE and XINT asymmetry. 
		 * Other interrupts are examined by cpq_ps_assignvec.
		 */
		cpq_intr_slot[CPQ_TIMER_SLOT].attached_to = cpq_booteng;
		cpq_intr_slot[CPQ_CASC_SLOT].attached_to = cpq_booteng;
		cpq_intr_slot[CPQ_XINT_SLOT].attached_to = cpq_booteng;
	} else {

		/* Initialize P2's interrupt number to CPQ_XINT_SLOT. */
		outb(cpq_engine_ivec_port[os_this_cpu],
		     I8259_VBASE+CPQ_XINT_SLOT);
	}

	/* Clear all interrupts */
	outb(cpq_engine_ctl_port[os_this_cpu],
		inb(cpq_engine_ctl_port[os_this_cpu]) & ~(SP_PINT | SP_INTDIS));
}


/*
 * STATIC ms_cpu_t
 * cpq_sp_initpsm(void)
 *
 * Calling/Exit State:
 *	Returns number of engines available in the system.
 *
 */
STATIC ms_cpu_t
cpq_sp_initpsm(void)
{
	if (inb(cpq_engine_ctl_port[1]) != 0xFF)
		/* number of engines equal 2 */
		return(2);
	else
		/* number of engines equal 1 */
		return(1);
}


/*
 * STATIC void
 * cpq_sp_bootcpu(ms_cpu_t engnum)
 *	Boot additional CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_sp_bootcpu(ms_cpu_t engnum)
{
	unsigned char	data;
	int		i;


	PSM_ASSERT(engnum < cpq_sp_maxnumcpu);
	PSM_ASSERT(os_this_cpu == 0);

	cpq_sp_stopcpu(engnum);

	/* Give it a small time */
	psm_time_spin(1000);

	/* Disable engine's interrupt */
	outb(cpq_engine_ctl_port[engnum], (SP_INTDIS | SP_PHOLD));

	/* Give it a small time */
	psm_time_spin(1000);

	/* Turn Cache on */
	outb(cpq_engine_ctl_port[engnum], (SP_INTDIS | SP_CACHEON | SP_PHOLD));

	/* Give it a small time */
	psm_time_spin(1000);

	/* Clear engine's Mbus access bit to let it run */
	outb(cpq_engine_ctl_port[engnum], (SP_INTDIS | SP_CACHEON));

}

/*
 * STATIC void
 * cpq_sp_offline_prep(void)
 *
 * Calling/Exit State:
 *
 */
STATIC void
cpq_sp_offline_prep(void)
{

	PSM_ASSERT (os_this_cpu != cpq_booteng);

	/* Disable engine's interrupt */
	outb(cpq_engine_ctl_port[os_this_cpu], (SP_INTDIS | SP_CACHEON));

	/* Give it a small time */
	psm_time_spin(1000);
}


/*
 * STATIC ms_bool_t 
 * cpq_sp_isxcall(void)
 *
 * Calling/Exit State:
 *	Return MS_TRUE if the cross processor interrupt bit is asserted
 *	in the current engines processor control port, otherwise
 *	return MS_FALSE.
 */
STATIC ms_bool_t
cpq_sp_isxcall(void)
{
	if (inb(cpq_engine_ctl_port[os_this_cpu]) & SP_PINT)
		return (MS_TRUE);
	else
		return (MS_FALSE);
}


/*
 * STATIC void
 * cpq_sp_cpuclrintr(ms_cpu_t engnum)
 *	To clear a CPU interrupt
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_sp_cpuclrintr(ms_cpu_t engnum)
{
	outb(cpq_engine_ctl_port[engnum],
	     inb(cpq_engine_ctl_port[engnum]) & ~SP_PINT);
}


/*
 * STATIC void
 * cpq_sp_cpuintr(ms_cpu_t engnum)
 *	Interrupt a CPU.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_sp_cpuintr(ms_cpu_t engnum)
{
	outb(cpq_engine_ctl_port[engnum],
	     inb(cpq_engine_ctl_port[engnum]) | SP_PINT);
}


/*
 * STATIC ms_cpu_t 
 * cpq_sp_assignvec(ms_islot_t vec)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC ms_cpu_t
cpq_sp_assignvec(ms_islot_t vec)
{
	return cpq_booteng;
}



/*
 * STATIC ms_rawtime_t
 * cpq_sp_usec_time(void)
 *
 * Calling/Exit State:
 *	- Return the current time (free-running counter value)
 *	  in microseconds.
 */
STATIC ms_rawtime_t
cpq_sp_usec_time(void)
{
	unsigned int	usecs, oldlock;
	int		newtimestamp;
	ms_rawtime_t	rawtime;

	do {
		oldlock = cpq_rawtime_lock;
		newtimestamp = i8254_get_time(&cpq_i8254_params);
	
		usecs = cpq_rawtime +
			(newtimestamp *
			 (os_tick_period.mst_nsec / CPQ_NSECS_IN_USEC) /
			 cpq_i8254_params.clkval);

	} while (oldlock == 0 || oldlock != cpq_rawtime_lock);

	rawtime.msrt_hi = 0;
	rawtime.msrt_lo = usecs;

	return (rawtime);
}



/*
 * ms_bool_t
 * cpq_sp_ecc_intr(void)
 *
 * Calling/Exit State:
 *	Returns MS_TRUE if it is an ECC interrupt, otherwise return MS_FALSE.
 */
ms_bool_t
cpq_sp_ecc_intr(void)
{
	if ((cpq_mptype != CPQ_SYSTEMPRO_COMPATIBLE) &&
	    (inb(SP_INT13_XSTATUS_PORT) & SP_INT13_ECC_MEMERR_ACTIVE))
		return MS_TRUE;
	else
		return MS_FALSE;
}


/*
 * STATIC ms_bool_t
 * cpq_sp_fpu_intr(void)
 *
 * Calling/Exit State:
 *	Return MS_TRUE if it is a floating-point interrupt, otherwise 
 *	return MS_FALSE.
 */
STATIC ms_bool_t
cpq_sp_fpu_intr(void)
{
	return ((inb(cpq_engine_ctl_port[os_this_cpu])
		 & SP_FPU387ERR) ? MS_TRUE : MS_FALSE);
}


/*
 * STATIC void
 * cpq_sp_set_eltr(ms_cpu_t engnum, ms_islot_t irq, ms_intr_dist_t *idtp)
 *	Sets mode (edge- or level-triggered) for a given interrupt line
 *	on the engnum.
 *
 * Calling/Exit State:
 *	On entry, irq is the interrupt line whose mode is being set
 *	and mode is the desired mode (either EDGE_TRIG or LEVEL_TRIG).
 *
 * Remarks:
 *	IRQ's 0, 1, 2, 8 and 13 are always set to EDGE_TRIG regardless of
 *	the mode argument passed in.
 *
 * Note:
 */
STATIC void
cpq_sp_set_eltr(ms_cpu_t engnum, ms_islot_t irq, ms_intr_dist_t *idtp)
{
	/* ELTR is the global EISA one on syspro */
	i8259_intr_attach(idtp, (1 << MSR_BUS_EISA)); 
}

