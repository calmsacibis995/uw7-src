/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/xl.c	1.17.6.1"
#ident	"$Header$"

/*
 * COMPAQ SYSTEMPRO/XL & PROLIANT HARDWARE INFO:
 *	- To do something to the CPU that you are on then just do the
 *	  outb, else write CPU # to INDEXCPU, LOW Addr to INDEXLOW, and 
 *	  HIGH Addr to INDEXHI that you wish to read/write to and then 
 *	  read or write from INDEXDATA. Note that several of the frequently
 *	  used commands are prepackaged into 32bit bundles that can be
 *	  ANDed with cpu, ie.
 *
 *		to PINT CPU 1 at IRQ 13 would be the following
 *
 *		INT13 addr 0x0CC8
 *		Command 0x05
 *
 *		outb(INDEXCPU,0x01)
 *		outb(INDEXLOW,0xC8)
 *		outb(INDEXHI,0x0C)
 *		outb(INDEXDATA,0x05)
 *
 *	  There for the command outl(INDEXCPU,(0x050CC800|(long)cpu))
 *
 *	  Keep in mind that all of the PICs are now local once you are in
 *	  SystemPro/XL mode.
 *
 *	  Set up iplmask so that each CPU can receive clock interrupts and
 *	  program both clock chips.
 *
 *	  See programmers guide for indexed and local io.
 *
 *	- All cpus can receive clock interrupts.
 *
 *	- All cpus access their control port at the same address.
 *
 *	- There can be empty cpu slots.
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>
#include <psm/toolkits/psm_i8254/psm_i8254.h>
#include <psm/toolkits/psm_time/psm_time.h>
#include <psm/toolkits/psm_string.h>
#include <psm/toolkits/psm_eisa.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_assert.h>
#include <psm/compaq/compaq.h>
#include <psm/compaq/xl.h>


#ifdef DEBUG
STATIC int cpq_spxl_debug = 0;
#define DEBUG1_BIT	1			      /* original debug code */
#define DEBUG2_BIT	2			          /* processor start */
#define DEBUG3_BIT	3			                   /* get EV */

#define DEBUG1(a)	if (cpq_spxl_debug & DEBUG1_BIT) os_printf a
#define DEBUG2(a)	if (cpq_spxl_debug & DEBUG2_BIT) os_printf a
#define DEBUG3(a)	if (cpq_spxl_debug & DEBUG3_BIT) os_printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#define DEBUG3(a)
#endif /* DEBUG */

struct {
	char	slot;
	char	step;
	char	type;
} cpq_cqhcpu[XL_MAXNUMCPU];		         /* cpus present (type/step) */



extern void cpq_xpost(ms_cpu_t cpu, ms_event_t eventmask);
extern void cpq_noop(void);
extern ms_bool_t cpq_sp_ecc_intr(void);

extern i8254_params_t		cpq_i8254_params;
extern ms_cpu_t			cpq_booteng;
extern struct cpq_slot_info	cpq_intr_slot[];
extern ms_bool_t 		cpq_engine_online[];
extern ms_cpu_t 		cpq_howmany_online;
extern unsigned int		cpq_lintmask[CPQ_MAXNUMCPU];
extern unsigned int		cpq_pintmask[CPQ_MAXNUMCPU];
extern unsigned char		cpq_mptype;
extern unsigned char		cpq_sp_iomode;



/*
 * Configurable objects -- defined in Space.c
 */
extern ms_cpu_t cpq_sp_maxnumcpu;	         /* max. cpus in SP or SP/XL */
extern int cpq_sp_systypes;		   /* number of SP or SP/XL EISA ids */
extern char **cpq_sp_eisa_id[];		        /* EISA ids for SP and SP/XL */
extern unsigned char cpq_sp_xl_slot[];	            /* slot number for SP/XL */
extern ms_cpu_t cpq_xl_maxnumcpu;	      /* max. cpus in an XL/Proliant */
extern int cpq_intrdistmask[];
extern char cpq_sp_xl_eisabuf[];



/*
 * Global variables
 */
STATIC ms_cpu_t		cpq_intrdist_engnum; /* Last CPU to take over an int */
STATIC unsigned int	*cpq_spxl_nsec;   /* free-running nanosecond counter */
STATIC ms_lockp_t	cpq_spxl_lockp;	   /* lock to protect the index reg. */
STATIC ms_cpu_t		cpq_xl_num_OK_CPUs = 0;	    /* no. of operation cpus */
STATIC char		cpq_cqhcpf;	 /* bitmask of cpus that failed POST */



/*
 * Module functions
 */
ms_cpu_t	cpq_spxl_initpsm(void);
void		cpq_spxl_init_cpu(void);
ms_cpu_t	cpq_spxl_assignvec(ms_islot_t);
void		cpq_spxl_set_eltr(ms_cpu_t, ms_islot_t, ms_intr_dist_t *);
void		cpq_spxl_bootcpu(ms_cpu_t);
void		cpq_spxl_cpuintr(ms_cpu_t);
void		cpq_spxl_cpuclrintr(ms_cpu_t);
ms_bool_t	cpq_spxl_checkint13(void);
ms_bool_t	cpq_spxl_fpu_intr(void);
void		cpq_spxl_offline_prep(void);
void		cpq_spxl_stopcpu(ms_cpu_t);
ms_rawtime_t	cpq_spxl_usec_time(void);
void		cpq_spxl_resetcpu(ms_cpu_t);
ms_bool_t	cpq_spxl_findeng(unsigned char, ms_cpu_t);
char		cpq_spxl_read_indexed(ms_cpu_t, int);
void		cpq_spxl_write_indexed(ms_cpu_t, int, int);
int		cpq_xl_check_evs(void);
int		cpq_xl_get_ev(char *, char *, int *);



/*
 * Platform specific function table
 */
struct cpq_psmops cpq_xlmpfuncs = {
	cpq_spxl_initpsm,			/* cpq_ps_initpsm */
	cpq_spxl_init_cpu,			/* cpq_ps_init_cpu */
	cpq_spxl_assignvec,			/* cpq_ps_assignvec */
	cpq_spxl_set_eltr,			/* cpq_ps_set_eltr */
	cpq_spxl_bootcpu,			/* cpq_ps_online_engine */
	cpq_spxl_cpuintr,			/* cpq_ps_send_xintr */
	cpq_spxl_cpuclrintr,			/* cpq_ps_clear_xintr */
	cpq_spxl_checkint13,			/* cpq_ps_isxcall */
	cpq_spxl_fpu_intr,			/* cpq_ps_fpu_intr */
	cpq_spxl_offline_prep,			/* cpq_ps_offline_prep */
	cpq_spxl_stopcpu,			/* cpq_ps_reboot */
	cpq_spxl_usec_time			/* cpq_ps_usec_time */
};




/*
 * STATIC char
 * cpq_spxl_read_indexed(ms_cpu_t cpu_number, int port_addr)
 *
 * Calling/Exit State:
 *	It is protected by a fast spin lock, since index ports can
 *	be written by other CPUs simultaneously during indexed I/O
 *	transfer.
 */
STATIC char
cpq_spxl_read_indexed(ms_cpu_t cpu_number, int port_addr)
{
	char port_value;
	ms_lockstate_t ls;

	ls = os_mutex_lock(cpq_spxl_lockp);
	outb(XL_INDEXCPU, (char)cpu_number);
	outb(XL_INDEXLOW, (char)port_addr);
	outb(XL_INDEXHI, (char)(port_addr >> 8));
	port_value = inb(XL_INDEXDATA);
	os_mutex_unlock(cpq_spxl_lockp, ls);

	return (port_value);
}


/*
 * STATIC void 
 * cpq_spxl_write_indexed(ms_cpu_t cpu_number, int port_addr, int port_value)
 *
 * Calling/Exit State:
 *	It is protected by a fast spin lock, since index ports can
 *	be written by other CPUs simultaneously during indexed I/O
 *	transfer.
 */
STATIC void
cpq_spxl_write_indexed(ms_cpu_t cpu_number, int port_addr, int port_value)
{
	ms_lockstate_t ls;

	ls = os_mutex_lock(cpq_spxl_lockp);
	outb(XL_INDEXCPU, (char)cpu_number);
	outb(XL_INDEXLOW, (char)port_addr);
	outb(XL_INDEXHI, (char)(port_addr >> 8));
	outb(XL_INDEXDATA, (char)port_value);
	os_mutex_unlock(cpq_spxl_lockp, ls);
}


/*
 * STATIC void
 * cpq_spxl_bootcpu(ms_cpu_t cpu)
 *	Boot additional CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_spxl_bootcpu(ms_cpu_t cpu)
{
	int i;

	PSM_ASSERT(cpu < cpq_xl_maxnumcpu);
	PSM_ASSERT(inb(XL_MODESELECT) & SP_XLMODE);

	/* Once in a while reset is not taken */
	do {
		cpq_spxl_resetcpu(cpu);

		/* Give it a small time */
		psm_time_spin(1000);
	} while (!(cpq_spxl_read_indexed(cpu, XL_STATUSPORT) & XL_SLEEPING));

	outl(XL_INDEXCPU, 
		(long)(XL_START | XL_CACHEON | XL_FLUSH) << 24 | 
		(long)XL_COMMANDPORT << 8 | (long)cpu);
}


/*
 * STATIC void
 * cpq_spxl_resetcpu(ms_cpu_t cpu)
 *	Put the processor to a known state and put it to sleep.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_spxl_resetcpu(ms_cpu_t cpu)
{
	outl(XL_INDEXCPU, (long)(XL_RESET | XL_SLEEP) << 24 | 
		(long) XL_COMMANDPORT << 8 | (long)cpu);
}


/*
 * STATIC void
 * cpq_spxl_stopcpu(ms_cpu_t cpu)
 *	Stop additonal CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_spxl_stopcpu(ms_cpu_t cpu)
{

	if (cpu == os_this_cpu) {
		/* Offline case: flush cache
		 * Some XLs require all this not to be done
		 * in one go.
		 */
		outb(XL_COMMANDPORT, XL_FLUSH);

		/* Give it a small time */
		psm_time_spin(1000);

		/* Turn cache off */
		outb(XL_COMMANDPORT, XL_CACHEOFF);

		/* Give it a small time */
		psm_time_spin(1000);

		/* Put us to sleep */
		outb(XL_COMMANDPORT, XL_SLEEP);
	} else
		/* Shutdown case */
		outl(XL_INDEXCPU,
			(long)(XL_SLEEP | XL_FLUSH | XL_CACHEOFF) << 24 | 
			(long)XL_COMMANDPORT << 8 | (long)cpu);
}


/*
 * STATIC void
 * cpq_spxl_cpuintr(ms_cpu_t cpu)
 *	Interrupt a CPU.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_spxl_cpuintr(ms_cpu_t cpu)
{
	/* Indexed I/O to another CPU. */
	outl(XL_INDEXCPU, XL_PINT13 | (long)cpu);
}


/*
 * STATIC void
 * cpq_spxl_cpuclrintr(ms_cpu_t cpu)
 *	To clear a CPU interrupt
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_spxl_cpuclrintr(ms_cpu_t cpu)
{
	/* Do this locally. */
	outb(XL_INT13, XL_PINTCLR);
}


/*
 * STATIC ms_bool_t
 * cpq_spxl_checkint13(void)
 *	Check to see if IRQ13 is FP or PINT
 *
 * Calling/Exit State:
 *	None.
 */
STATIC ms_bool_t
cpq_spxl_checkint13(void)
{
	if ((inb(XL_STATUSPORT) & XL_NCPERR) || (cpq_sp_ecc_intr())) {
		/* This is a FP IRQ. */
		return (MS_FALSE);
	} else {
		/* This is IPI IRQ */
		return (MS_TRUE);
	}
}

/*
 * STATIC ms_bool_t
 * cpq_spxl_fpu_intr(void)
 *
 * Calling/Exit State:
 *	Return MS_TRUE if it is floating-point interrupt, otherwise 
 *	return MS_FALSE.
 */
STATIC ms_bool_t
cpq_spxl_fpu_intr(void)
{
	return ((inb(XL_STATUSPORT) & XL_NCPERR) ? MS_TRUE : MS_FALSE);
}

/*
 * STATIC void
 * cpq_spxl_init_cpu(void)
 *	Initialize programmable interrupt timer.
 *	Initialize programmable interrupt controller.
 *	Initialize microsecond performance counter.
 *	Set logical and physical CPU number.
 *	Take over attached interrupts that can be handled.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 *
 * Remarks:
 *	We use the logical CPU assignment port to assign a logical
 *	CPU number for a physical CPU. We specify the physical CPU
 *	to receive the assignment by writing to the CPU index port.
 */
STATIC void
cpq_spxl_init_cpu(void)
{
	ms_islot_t i, j, k, ncpuany, ntotake;
	ms_islot_t dist_slot[CPQ_MAX_SLOT+1], which_slot[CPQ_MAX_SLOT-1];
	ms_cpu_t cpu;
	ms_intr_dist_t *idtp;
	int dummy;

	/*
	 * Initialize the i8254 programmable interrupt timer.
         */
	i8254_init(&cpq_i8254_params, I8254_CTR0_PORT, os_tick_period);

	/*
	 * os_alloc inside i8254_init is forcing a sti.
         * We don't want interrupts enabled at this time.
	 */
	dummy = intr_disable();

	/* Initialize the i8259 based interrupt controller. */
	i8259_init(1 << MSR_BUS_EISA);

	if (os_this_cpu == cpq_booteng) {

		/*
		 * Set TIMER, CASCADE and XINT symmetry. 
		 * Other interrupts are examined by cpq_ps_assignvec.
		 */
		cpq_intr_slot[CPQ_TIMER_SLOT].attached_to = MS_CPU_ANY;
		cpq_intr_slot[CPQ_CASC_SLOT].attached_to = MS_CPU_ANY;
		cpq_intr_slot[CPQ_XINT_SLOT].attached_to = MS_CPU_ANY;

		/* Initialize cpq_intrdist_engnum */
		cpq_intrdist_engnum = os_this_cpu;

		/* Map in the microsecond performance counter. */
		cpq_spxl_nsec = (unsigned int *)
			os_physmap(XL_NSEC_PADDR, 8);
		if (cpq_spxl_nsec == NULL) {
			/*
			 *+ Not enough virtual memory available to map 
			 *+ in a free-running counter.
			 */
			os_printf("\nWARNING: cpq_spxl_init_cpu: Unable to map in a free-flowing counter");
		}
	}

	/* one-to-one mapping between logical CPU and physical CPU */
	outb(XL_INDEXPORT, os_this_cpu);	/* write physical CPU no */
	outb(XL_ASSIGNPORT, os_this_cpu);	/* write logical CPU no */

	/* Enable IPI */
	outb(XL_INT13, XL_PINTCLR | XL_PINTENABLE);

	/* Only once on system life, cpq_booteng is never offlined */
	if (os_this_cpu == cpq_booteng) return;

	/*
	 * This non-boot CPU can now take over the burden of some
	 * MS_CPU_ANY interrupts. 
	 */

	PSM_ASSERT(!is_intr_enabled());

	/* Find out how many are distributed */
	for (i = CPQ_FIRST_SLOT, ncpuany = 0; i < CPQ_MAX_SLOT+1; i++) {

		/* Skip CPQ_CASC_SLOT, CPQ_TIMER_SLOT and CPQ_XINT_SLOT */
		if ((i == CPQ_TIMER_SLOT) || (i == CPQ_XINT_SLOT) ||
		    (i == CPQ_CASC_SLOT)) continue;

		if ((cpq_intr_slot[i].idtp->msi_cpu_dist == MS_CPU_ANY) &&
		    (cpq_intrdistmask[i] == 0)) {
			dist_slot[ncpuany++] = i;
		}
	}

	/* Find out how many can be taken, add ourself as online */
	ntotake = (ncpuany + 1) / (cpq_howmany_online + 1);

	/*
	 * Take them in round robin from the one after
	 * cpq_intrdist_engnum, that was the last one a
	 * distributable interrupt was assigned to.
	 * Adjust which_slot[] accordingly.
	 */
	cpu = (cpq_intrdist_engnum + 1) % XL_MAXNUMCPU;

	j = 0;
	while (j < ntotake) {
		/* Find next online */
		while ((cpq_engine_online[cpu] == MS_FALSE) ||
		       (cpu == os_this_cpu))
			cpu = (cpu + 1) % XL_MAXNUMCPU;

		for (i = 0; i < ncpuany; i++) {
			if (cpq_intr_slot[dist_slot[i]].attached_to == cpu) {
				which_slot[j] = dist_slot[i];
				for (k = 0; k < j; k++)
					if (which_slot[k] == dist_slot[i])
						break;
				/* If first appearence, take it */
				if (k == j)
				{
					j++;
					break;
				}
			}
		}

		/* Move forward */
		cpu = (cpu + 1) % XL_MAXNUMCPU;
	}

	/* Based on which_slot[], take them */
	for (i = 0; i < ntotake; i++) {
		j = which_slot[i];

		idtp = cpq_intr_slot[j].idtp;

		if (!(cpq_pintmask[cpq_intr_slot[j].attached_to] & (1 << j))) {
			/* Spin waiting: beware of async event2 in offlining */
			while (cpq_intr_slot[j].event2_pending == MS_TRUE) {
				cpq_noop();
			}

			/* Cross-masking through XINT */
			cpq_intr_slot[j].masked = MS_TRUE;
			cpq_intr_slot[j].event2_pending = MS_TRUE;

			cpq_xpost(cpq_intr_slot[j].attached_to, MS_EVENT_PSM_2);

			/* Spin waiting for sync */
			while (cpq_intr_slot[j].event2_pending == MS_TRUE) {
				cpq_noop();
			}
		}

		/* Attach it */
		cpq_intr_slot[j].attached_to = os_this_cpu;

		/* Set ELTR locally */
		cpq_spxl_set_eltr(os_this_cpu, j, idtp);

		/* Set interrupt as unmasked */
		cpq_intr_slot[j].masked = MS_FALSE;
	}

	/* We are the last one */
 	if (ntotake) cpq_intrdist_engnum = os_this_cpu;
}


/*
 * STATIC ms_cpu_t
 * cpq_spxl_initpsm(void)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC ms_cpu_t
cpq_spxl_initpsm(void)
{
	cpq_spxl_lockp = os_mutex_alloc();

	/* Turn SysproXL mode on */
	outb(XL_MODESELECT, inb(XL_MODESELECT) | SP_XLMODE);

	return (cpq_spxl_numeng());
}


/*
 * STATIC void
 * cpq_spxl_offline_prep(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Do any necessary work to redistribute interrupts.
 */
STATIC void
cpq_spxl_offline_prep(void)
{
	ms_islot_t i, j, ncpuany, which_slot[CPQ_MAX_SLOT-1];
	ms_cpu_t cpu, which_cpu[CPQ_MAX_SLOT-1];
	ms_intr_dist_t *idtp;

	PSM_ASSERT(os_this_cpu != cpq_booteng);

	/* Find out which interrupts have to be redistributed */
	for (i = CPQ_FIRST_SLOT, ncpuany = 0; i < CPQ_MAX_SLOT+1; i++) {

		/* Skip CPQ_CASC_SLOT, CPQ_TIMER_SLOT and CPQ_XINT_SLOT */
		if ((i == CPQ_TIMER_SLOT) || (i == CPQ_XINT_SLOT) ||
		    (i == CPQ_CASC_SLOT)) continue;

		if (cpq_intr_slot[i].attached_to == os_this_cpu) {
			PSM_ASSERT(cpq_intr_slot[i].idtp->msi_cpu_dist ==
			       MS_CPU_ANY);

			which_slot[ncpuany++] = i;
		}
	}

	/* Distribute them in round robin from last CPU.  */
	cpu = (cpq_intrdist_engnum + 1) % XL_MAXNUMCPU;

	for (i = 0; i < ncpuany; i++) {

		/* Search next CPU online, we see ourself as offline */
		while (cpq_engine_online[cpu] == MS_FALSE)
			cpu = (cpu + 1) % XL_MAXNUMCPU;

		which_cpu[i] = cpu;

		/* Move forward */
		cpu = (cpu + 1) % XL_MAXNUMCPU;
	}

	/* Pair which_slot[]/which_cpu[] is known, now redistribute */
	for (i = 0; i < ncpuany; i++) {

		j = which_slot[i];

		idtp = cpq_intr_slot[j].idtp;

		/* Mask the interrupt off locally */
		cpq_pintmask[os_this_cpu] |= (1 << j);
		i8259_intr_mask(j);

		/* Attach it */
		cpq_intr_slot[j].attached_to = which_cpu[i];

		/* Set ELTR on target */
		cpq_spxl_set_eltr(which_cpu[i], j, idtp);
	
		/*
		 * Reload the current mask on target, this is
		 * working as cross-CPU operation.
		 */

		/* Spin waiting: beware of async event2 in offlining */
		while (cpq_intr_slot[j].event2_pending == MS_TRUE) {
			cpq_noop();
		}

		if (cpq_lintmask[which_cpu[i]] & (1 << j))
			cpq_intr_slot[j].masked = MS_TRUE;
		else
			cpq_intr_slot[j].masked = MS_FALSE;

		cpq_intr_slot[j].event2_pending = MS_TRUE;

		cpq_xpost(cpq_intr_slot[j].attached_to, MS_EVENT_PSM_2);

		/*
		 * Do not wait spinning, the CPU putting ourself
		 * offline could not take MS_EVENT_PSM_2 being
		 * at disabled interrupts while waiting for our ack.
		 */
	}

	/* This is the last one */
 	if (ncpuany) cpq_intrdist_engnum = which_cpu[ncpuany-1];

	/* Disable IPI */
	outb(XL_INT13, XL_PINTCLR | XL_PINTDISABLE);

	/* Deinit the local 8259 */
	i8259_deinit();
}


STATIC unsigned int cpq_spxl_lop;
STATIC unsigned int cpq_spxl_hop;
STATIC unsigned int cpq_spxl_usecs;
STATIC unsigned int cpq_spxl_latchcounter[2];

/*
 * STATIC ms_rawtime_t
 * cpq_spxl_usec_time(void)
 *
 * Calling/Exit State:
 *	- Return the current time (free-running counter value)
 *	  in microseconds.
 *
 * Remarks:
 *	The documentation for the 48-bit free-running counter 
 *	available on the Compaq SysproXL and ProLiant machines 
 *	does not adequately describe the timebase for the counter.
 *	The documentation describes the counter as having nanosecond
 *	resolution, when infact the counter appears to be driven by a
 *	33MHz clock and thus would have a 30ns resolution. Consequently
 *	the values previously returned by the compaq cpq_spxl_usec_time() 
 *	routine (which converts this counter into a microsecond value)
 *	are off by factor of 33.  This is fixed by dividing the low
 *	order 32 bits (nanosecond counter) by 33 instead of 1000. 
 *
 *	There appears to be a hardware bug, where sometimes the value
 *	returned in the low 32 bits of the counter is 0x45f8. Reading
 *	the counter again in this case gives the correct value. This
 *	results in a more reliable microsecond timer because bad values
 *	are now sampled out.
 *
 *	In our previous scheme we read the nanosecond counter and 
 *	divided it by 1000 to return the microsecond counter value.
 *	This scheme only used lower 32 bits of the 48 bit counter.
 *	Converting that to microsecond lost 5 bits of resolution.
 *	The counter then wrapped in 2^27 bits or 134 seconds. Since 
 *	there are 16 more bits of counter available in the hardware, 
 *	we can read this counter also and shift in the low 5 bits 
 *	of the upper 16 bits, and not loose any resolution. This
 *	results in a higher resolution timer because we now do not
 *	discard the high 16 bits (32-48) of the 48 bit counter.
 */
STATIC ms_rawtime_t
cpq_spxl_usec_time(void)
{
	unsigned int ntries;
	ms_rawtime_t rawtime;


	if (cpq_spxl_nsec == NULL) {
		rawtime.msrt_lo = 0;
		rawtime.msrt_hi = 0;
		return (rawtime);
	}

	/*
	 * Read all 64-bits of the performance counter, so that the
	 * memory-mapped counter is latched/updated and read later.
	 */
	for (ntries = 0; ntries < 4; ntries++) {
		cpq_spxl_latchcounter[0] = cpq_spxl_nsec[0];
		cpq_spxl_latchcounter[1] = cpq_spxl_nsec[1];
		cpq_spxl_lop = cpq_spxl_nsec[0];
		cpq_spxl_hop = cpq_spxl_nsec[1];
		/* we somtimes get back bad values */
		if (cpq_spxl_hop >= cpq_spxl_latchcounter[1] && 
		    cpq_spxl_lop >= cpq_spxl_latchcounter[0] &&
		    cpq_spxl_lop != 0x45f8)
			break;
	}

	/*
	 * The input clock seems to be a 33Mhz clock. We divide
	 * divide this by 33 to get a microsecond clock. We shift
	 * in some of the high order bits to keep the full range
	 * of the counter.  We accomplish both (without using a
	 * divide) by using the formula:
	 *	x/33 = x/32 - x/32^2 + x/32^3 - x/32^4
	 */

	/* convert to microseconds */
	cpq_spxl_usecs = ((cpq_spxl_hop << 27) | (cpq_spxl_lop >> 5));
	/* x/33 = x/32 - x/32^2 + x/32^3 - x/32^4 */
	cpq_spxl_usecs = cpq_spxl_usecs - (cpq_spxl_usecs >> 5) + 
			(cpq_spxl_usecs >> 10) - (cpq_spxl_usecs >> 15);

	/* return the microsec. value */
	/* low rawtime = 2^32 microseconds = 1 hour, 11 minutes */
	rawtime.msrt_lo = cpq_spxl_usecs;
	rawtime.msrt_hi = 0;
	return (rawtime);
}


/*
 * STATIC ms_cpu_t
 * cpq_spxl_assignvec(ms_islot_t vec)
 *
 * Calling/Exit State:
 *	<vec> is the interrupt vector number that is to be redistributed.
 *
 *	Returns the engine number to which the interrupt is assigned.
 *
 * Remarks:
 *	Assign interrupt vector of the multithreaded drivers
 *	to the processors based on a round-robin scheme.
 *
 */
STATIC ms_cpu_t 
cpq_spxl_assignvec(ms_islot_t vec)
{
	ms_cpu_t cpu, i;

	
	PSM_ASSERT(vec <= CPQ_MAX_SLOT);
	PSM_ASSERT(vec != CPQ_XINT_SLOT);
	PSM_ASSERT(vec != CPQ_CASC_SLOT);
	PSM_ASSERT(vec != CPQ_TIMER_SLOT);

	/* Check space.c static assignment */
	if (cpq_intrdistmask[vec] == 0) {
		/* It's distributable */
		cpu = (cpq_intrdist_engnum + 1) % XL_MAXNUMCPU;
		while (cpq_engine_online[cpu] == MS_FALSE)
			cpu = (cpu + 1) % XL_MAXNUMCPU;

		/* This is the last one */
		cpq_intrdist_engnum = cpu;
	} else
		/* Bound it based on space.c */
		cpu = cpq_intrdistmask[vec] - 1;

	return cpu;
}


/*
 * STATIC ms_bool_t
 * cpq_spxl_findeng(unsigned char slot, ms_cpu_t engnum)
 *
 * Calling/Exit State:
 *	- <slot> is the eisa socket no. of the cpu board id.
 *	- <engnum> is the processor index in the <cpq_sp_eisa_id> table
 *	  to find the list of EISA id strings.
 */
STATIC ms_bool_t
cpq_spxl_findeng(unsigned char slot, ms_cpu_t engnum)
{
	char eid[4];		/* compressed EISA id */
	char *cbuf;		/* character buffer */
	int systype;


	eisa_boardid(slot, eid);
	cbuf = eisa_uncompress(eid);

	DEBUG1(("cpq_spxl_findeng: eisa uncompressed id = %s\n", cbuf));

	for (systype = 0; systype < cpq_sp_systypes; systype++) {
		if (strncmp(cbuf, cpq_sp_eisa_id[systype][engnum], 7) == 0) {
			DEBUG2(("cpq_spxl_findeng: Found processor %d in eisa slot %x\n", engnum, slot));
			return(MS_TRUE);
		}
	}

	return(MS_FALSE);
}


/*
 * STATIC ms_cpu_t 
 * cpq_spxl_numeng(void)
 *	Find number of processor boards in the system.
 *
 * Calling/Exit State:
 *	Called by the platform specific psminit.
 */
STATIC ms_cpu_t
cpq_spxl_numeng(void)
{
	ms_cpu_t neng = 0;
	ms_cpu_t i;
	int error = 0;

	if (cpq_mptype & CPQ_SYSTEMPROXL) {
		int slot;

		/*
		 * Check only the slots listed in cpq_sp_xl_slot for the
		 * "CPU" type/subtype string.
		 */
		for (i = 0; i < cpq_sp_maxnumcpu; i++) {
		    if (eisa_read_nvm(cpq_sp_xl_slot[i],
				      (unsigned char *) cpq_sp_xl_eisabuf,
				      &error))
			if (eisa_parse_devconfig((void *) cpq_sp_xl_eisabuf,
						 (void *)"CPU", EISA_SLOT_DATA,
						 0))
					neng++;
		}

		if (neng == cpq_sp_maxnumcpu)
			return neng;

		/*
		 * Check all the slots for the "CPU" type/subtype string.
		 * This is necessary because the slot number listed in 
		 * cpq_sp_xl_slot could be different from the actual slot no.
		 * of the processor board.
		 */
		if (neng < cpq_sp_maxnumcpu) {
		    neng = 0;
		    for (slot = 0; slot < EISA_MAX_SLOTS; slot++) {
			if (eisa_read_nvm(slot,
					  (unsigned char *)cpq_sp_xl_eisabuf,
					  &error))
			    if (eisa_parse_devconfig((void *)cpq_sp_xl_eisabuf,
						     (void *)"CPU",
						     EISA_SLOT_DATA, 0))
				neng++;
		    }

		    if (neng == cpq_sp_maxnumcpu)
			return neng;
		}

		/*
		 * If we cannot find "CPU" string in EISA NVRAM, check the
		 * slots for the boardid listed in cpq_sp_eisa_id.
		 */
		if (neng == 0) {
			neng = BASE_PROCESSOR + 1;
			for (i = 1; i < cpq_sp_maxnumcpu; i++) {
				for (slot = 1; slot < EISA_MAX_SLOTS; slot++) {
					if (cpq_spxl_findeng(slot, neng)) {
						neng++;
						break;
					}
				}
			}
		}

		for (i = BASE_PROCESSOR + 1; i < neng; i++) {
			cpq_spxl_resetcpu(i);
		}

		PSM_ASSERT(neng <= cpq_sp_maxnumcpu);
		return (neng);

	} else if (cpq_mptype & CPQ_PROLIANT) {
		ms_cpu_t	cpu;

		/* Do this if it is a proliant */

		cpq_xl_check_evs();

		for (cpu = 1; cpu < cpq_xl_num_OK_CPUs; cpu++) {
			/*
			 * Is it a i486 or Pentium?
			 */
			DEBUG2(("CPU %d type = %s\n", cpu,
			    (cpq_cqhcpu[cpu].type == 4) ? "486" : "Pentium"));
		}

		neng = cpq_xl_num_OK_CPUs;

		for (i = BASE_PROCESSOR + 1; i < neng; i++) {
			cpq_spxl_resetcpu(i);
		}

		return (neng);
	}
}

#define OK			0
#define NOT_OK 			(-1)
#define ROM_CALL_ERROR 		0x01
#define CALL_NOT_SUPPORTED 	0x86
#define EV_NOT_FOUND 		0x88
#define GET_EV 			0xD8A4

/*
 * STATIC int 
 * cpq_xl_check_evs(void)
 *	Check environment variables found in EISA NVM.
 *
 * Calling/Exit State:
 *	Returns OK if ASR functionality is in an EV, else return NOT_OK
 *
 *	Called from cpq_spxl_numeng().
 *
 * Remarks:
 *	"CQHCPU" EV returns the number of cpus present in the system.
 *	"CQHCPF" EV returns the state of cpus that are present, but have
 *		 failed the POST (Power On Self Test). A "1" indicates
 *		 that a cpu has failed post and a "0" indicates that a cpu
 *		 has passed post.
 */
STATIC int
cpq_xl_check_evs(void)
{
	int length;
	int itmp;
	int err_code = 1;


	length = 24;

	if (cpq_xl_get_ev("CQHCPU", (char *)cpq_cqhcpu, &length) != OK)
		err_code--;
	
	/* length / sizeof(struct cpq_cqhcpu) */
	cpq_xl_num_OK_CPUs = length / 3;

	DEBUG1(("cpq_xl_check_evs: length=0x%x, cpus=0x%x\n", length,
		 cpq_xl_num_OK_CPUs));
	
	length = 1;

	if (cpq_xl_get_ev("CQHCPF", &cpq_cqhcpf, &length) != OK)
		err_code--;

	DEBUG1(("cpq_xl_check_evs: length=%d, cqhcpf=%x\n", length,
		cpq_cqhcpf));

	for (itmp = 0; itmp < cpq_xl_maxnumcpu; itmp++) {
		if ((cpq_cqhcpf >> itmp) & 0x01) {
			cpq_xl_num_OK_CPUs--;
			/*
			 *+ The processor is unoperational. It failed a
			 *+ Power On Self Test (POST).
			 */
			DEBUG2(("\nWARNING: Processor slot %d failed", itmp));
		}
	}

	return(err_code);
}


/*
 * STATIC int
 * cpq_xl_get_ev(char *ev_name_string, char *rbuf, int *length)
 *	Get an environment variable (EV) from EISA NV MEM.
 *
 * Calling/Exit State:
 *	Intput:
 *		ev_name_string	- points to the NULL terminated EV name string
 *
 *	Output:
 *		rbuf		- points to target buffer for EV value
 *		length		- length of EV string
 *
 *	Return:
 *		OK if EV completed successfully, else return NOT_OK
 */
STATIC int
cpq_xl_get_ev(char *ev_name_string, char *rbuf, int *length)
{
	regs reg;
	int status = OK;


	bzero(&reg, sizeof(regs));
	reg.eax.eax = GET_EV;
	reg.ebx.ebx = 0x0;
	reg.ecx.ecx = *length;
	reg.edx.edx = 0x0;
	reg.esi.esi = (unsigned long) ev_name_string;
	reg.edi.edi = (unsigned long) rbuf;

	/* make INT 15 ROM call */
	status = eisa_rom_call(&reg);

	/* save the count of data in the return buffer */
	*length = reg.ecx.ecx;

	/* check for error conditions */
	if ((reg.eflags & ROM_CALL_ERROR) == 1) {

		if (reg.eax.byte.ah == CALL_NOT_SUPPORTED) {
			/*
			 *+ Call not supported.
			 */
			DEBUG3(("\nWARNING: cpq_xl_get_ev: get EV call not supported"));
			status = NOT_OK;

		} else if (reg.eax.byte.ah == EV_NOT_FOUND) {
			/*
			 *+ Unsupported event.
			 */
			DEBUG3(("\nWARNING: cpq_xl_get_ev: EV %s not found", ev_name_string));
			status = NOT_OK;
		}

	} else if (reg.eax.byte.ah != 0x0) {
		/*
		 *+ ROM call error.
		 */
		DEBUG3(("\nWARNING: cpq_xl_get_ev: error with get EV ROM call"));
		status = NOT_OK;
	}

	return(status);
}


/*
 * STATIC void
 * cpq_spxl_set_eltr(ms_cpu_t engnum, ms_islot_t irq, ms_intr_dist_t *idtp)
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
 */
STATIC void
cpq_spxl_set_eltr(ms_cpu_t engnum, ms_islot_t irq, ms_intr_dist_t *idtp)
{
	int i, port, mode;


	/* ELTR is the EISA one on os_this_cpu */
	if (engnum == os_this_cpu) {
		i8259_intr_attach(idtp, (1 << MSR_BUS_EISA)); 
		return;
	}

	mode = (idtp->msi_flags & MSI_ITYPE_CONTINUOUS) ?
		CPQ_LEVEL_TRIGGER : CPQ_EDGE_TRIGGER;

	switch (irq) {
	case 0:
	case 1:
	case 2:
	case 8:
	case 13:
		return;
	}

	if (irq > 7) {
		irq -= 8;
		port = ELCR_PORT1;
	} else {
		port = ELCR_PORT0;
	}

	i = cpq_spxl_read_indexed(engnum, port);
	i &= ~(1 << irq);
	i |= (mode << irq);
	cpq_spxl_write_indexed(engnum, port, i);

	return;
}
