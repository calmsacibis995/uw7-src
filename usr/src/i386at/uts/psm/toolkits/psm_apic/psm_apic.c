/*      Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.    */
/*      Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.  */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.       */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */

#ident	"@(#)kern-i386at:psm/toolkits/psm_apic/psm_apic.c	1.5.2.1"
#ident	"$Header$"

/*
 * Support for Intel Advanced Programmable Interrupt Controller.
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_apic/psm_apic.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_time/psm_time.h>
#include <psm/toolkits/psm_mc146818/psm_mc146818.h>
#include <psm/toolkits/psm_cfgtables/psm_cfgtables.h>

STATIC int      apic_findspeed(volatile long *);


/*
 * MACRO
 * APIC_INTR_SEND_PENDING(lp)
 *
 * Calling/Exit State:
 *	<lp> is the pointer to apic_local_addr.
 */
#define	APIC_INTR_SEND_PENDING(lp) { \
	int limit = 25000; \
	while ((lp)[APIC_ICMD] & APIC_PENDING) \
		if (limit-- <= 0) \
			break; \
}

/*
 * void
 * apic_local_init(struct cpu_info *cpu)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	It is called during initialization for each processor.  The
 *	processor's local APIC is initialized.
 *
 * Remarks:
 */
void
apic_local_init(struct cpu_info *cpu)
{
	volatile long *lp;

	lp = cpu->lapic_addr;

	lp[APIC_TASKPRI] = 0;		/* ZZZ - task priority not used yet */	

	/*
	 * Mask the injection of local interrupts. The local interrupts
	 * are equivalent to interrupts generated thru I/O subsystem. In
	 * the case of I/O subsystem the interrupts are steered to processor
	 * through the Redirection Table of the I/O unit, whereas the local
	 * interrupts are steered thru the Local Vector Table (LVT) of the
	 * local unit.
	 */
        lp[APIC_LVT_I0] = cpu->lapic_vec0;
        lp[APIC_LVT_I1] = cpu->lapic_vec1;
	lp[APIC_LVT_TIMER] = APIC_MASK;
	if ((lp[APIC_VERS] & _82489APIC_MASK) >= APIC_VERS_INTEGRATED)
		lp[APIC_LVT_ERROR] = APIC_MASK;

	/*
	 * Interrupt Destination can be either addressed physically or
	 * logically. When the interrupt message addresses the destination
	 * physically, each 82489DX in the ICC bus compares the address
	 * with its own unit ID. When the message addresses the destination
	 * using logical addressing scheme each Local Unit in the ICC bus
	 * compares the logical address in the interrupt messsage with its
	 * own Logical Destination Register. All the 32 bits of the Dest-
	 * ination Format Register of all 82489DX connected to ICC bus
	 * should be written with all "1's" to enable the addressing scheme.
	 */

	lp[APIC_DESTFMT] = -1;
	lp[APIC_LOGICAL] = cpu->lapic_lid;

	/*
	 * Enable local unit.
	 */
	lp[APIC_SPUR] = APIC_LOU_ENABLE | cpu->lapic_svec;
}

/*
 * void
 * apic_local_disable(volatile long *lp)
 *	Disable the local APIC to keep it from accepting any new IO
 *	interrupts or from generating any local (clock, etc) interrupts.
 *
 * Calling/Exit State:
 *	None.
 */
void
apic_local_disable(volatile long *lp)
{
	int 		tmp;
	long		entry;

	lp[APIC_LOGICAL] = APIC_IM_OFF;
	lp[APIC_LVT_I0] = lp[APIC_LVT_I0] | APIC_MASK;
	lp[APIC_LVT_I1] = lp[APIC_LVT_I1] | APIC_MASK;
	lp[APIC_LVT_TIMER] = lp[APIC_LVT_TIMER] | APIC_MASK;
	lp[APIC_SPUR] = APIC_LOU_DISABLE;
	tmp = lp[APIC_SPUR];			/* ZZZ - make sure writes don't get posted */
}

/*
 * void
 * apic_io_init(struct ioapic_info *ioapic, ms_cpu_t cpu)
 *
 * Calling/Exit State:
 *	Called only on boot engine.
 */
void
apic_io_init(struct ioapic_info *ioapic, ms_cpu_t cpu)
{
	volatile long * ip;
	unsigned int i;
	unsigned long nentries;

	ip = ioapic->ioapic_addr;

	/* set I/O APIC id */
	ip[APIC_IO_REG] = APIC_AIR_ID;
	ip[APIC_IO_DATA] = ioapic->ioapic_id << APIC_IO_ID_SHIFT;

	/* mask all slots */
	ip[APIC_IO_REG] = APIC_AIR_VERS;
	nentries = ((ip[APIC_IO_DATA] >> 16) & 0x000000ff) + 1;
	for (i = 0; i < nentries; i++) {
	    	ip[APIC_IO_REG] = APIC_AIR_RDT + 2*i;
	    	ip[APIC_IO_DATA] = APIC_MASK | APIC_LDEST;
                ip[APIC_IO_REG] = APIC_AIR_RDT2 + 2*i;
                ip[APIC_IO_DATA] = APIC_LOGDEST(cpu); 
	}
	ioapic->ioapic_lock = os_mutex_alloc();
}

/*
 * void
 * apic_deinit(struct ioapic_info *ioapic)
 *
 * Calling/Exit State:
 *	Called only on boot engine.
 */
void
apic_deinit(struct ioapic_info *ioapic)
{
	volatile long * ip;
	unsigned int i;
	unsigned long nentries;

	ip = ioapic->ioapic_addr;

	/* mask all slots */
	ip[APIC_IO_REG] = APIC_AIR_VERS;
	nentries = ((ip[APIC_IO_DATA] >> 16) & 0x000000ff) + 1;
	for (i = 0; i < nentries; i++) {
	    	ip[APIC_IO_REG] = APIC_AIR_RDT + 2*i;
	    	ip[APIC_IO_DATA] = APIC_MASK;
	}

}

/*
 * void
 * apic_intr_set_mode(struct ioapic_info *ioapic, int line,
 *		      int mode, int vec, int cpu)
 *
 * Calling/Exit State:
 *      None.
 */
void
apic_intr_set_mode(struct ioapic_info *ioapic, int line, 
		   int mode, int vec, int cpu)
{
        volatile long 	*ip;
        long 		entry, rdt2;
	ms_lockstate_t	lstate;

        ip = ioapic->ioapic_addr;
	lstate = os_mutex_lock(ioapic->ioapic_lock);

		/*
		 * Write id register twice.
		 * We have observed one particular configuration
		 * which requires the second write to work properly - an
		 * old XXpress motherboard with 166 MHz processors.
		 */
        ip[APIC_IO_REG] = APIC_AIR_RDT + 2*line;
        ip[APIC_IO_REG] = APIC_AIR_RDT + 2*line;
        entry = ip[APIC_IO_DATA];
	entry &=  ~(APIC_DELIV_MODE_MASK|APIC_EDGE_LEVEL_MASK|APIC_VECTOR_MASK);
	if (cpu < 0) {
		entry |= APIC_LOPRI;
		rdt2 = APIC_ALLDEST;
	} else {
		entry |= APIC_FIXED;
		rdt2 = APIC_LOGDEST(cpu);
	}
        ip[APIC_IO_DATA] = entry | mode | vec;

        ip[APIC_IO_REG] = APIC_AIR_RDT2 + 2*line;
        ip[APIC_IO_DATA] = rdt2;
	os_mutex_unlock(ioapic->ioapic_lock, lstate);
}

/*
 * void
 * apic_intr_mask(struct ioapic_info *ioapic, int line)
 *
 * Calling/Exit State:
 *	None.
 */
void
apic_intr_mask(struct ioapic_info *ioapic, int line)
{
	volatile long	*ip;
	long		entry;
	ms_lockstate_t	lstate;

	ip = ioapic->ioapic_addr;
	lstate = os_mutex_lock(ioapic->ioapic_lock);
	ip[APIC_IO_REG] = APIC_AIR_RDT + 2 * line;
	entry = ip[APIC_IO_DATA];
	ip[APIC_IO_DATA] = entry | APIC_MASK; 
	entry = ip[APIC_IO_DATA];                 /* ZZZ read after mask needed?   */
	os_mutex_unlock(ioapic->ioapic_lock, lstate);
}

/*
 * void
 * apic_intr_unmask(struct ioapic_info *ioapic, int line)
 *
 * Calling/Exit State:
 *      None.
 */
void
apic_intr_unmask(struct ioapic_info *ioapic, int line)
{
        volatile long	*ip;
	long		entry;
	ms_lockstate_t	lstate;

        ip = ioapic->ioapic_addr;
	lstate = os_mutex_lock(ioapic->ioapic_lock);
        ip[APIC_IO_REG] = APIC_AIR_RDT + 2 * line;
        entry = ip[APIC_IO_DATA];
        ip[APIC_IO_DATA] = entry & APIC_UNMASK; 
	os_mutex_unlock(ioapic->ioapic_lock, lstate);
}

/*
 * void
 * apic_eoi(volatile long *lp)
 *
 * Calling/Exit State:
 *      None.
 */
void
apic_eoi(volatile long *lp)
{
	lp[APIC_EOI] = 0;	
}

/*
 * void
 * apic_xintr(volatile long *lp, unsigned long target, unsigned char vector)
 *
 * Calling/Exit State:
 *      Assumes that interrupts have been disabled before this
 *	function is called.
 */
void
apic_xintr(volatile long *lp, unsigned long target, unsigned char vector)
{
        /* Wait while an interrupt is already pending. */
        APIC_INTR_SEND_PENDING(lp);
        lp[APIC_ICMD2] = target;
        APIC_INTR_SEND_PENDING(lp);
        /* Send an interrupt. */
        lp[APIC_ICMD] = APIC_EDGE | APIC_LDEST | APIC_FIXED | vector;
        /* Wait till an interrupt is send. */
        APIC_INTR_SEND_PENDING(lp);
}

/*
 * void
 * apic_timer_init(volatile long *lp,  ms_time_t period, unsigned int vector)
 *
 * Calling/Exit State:
 *      None.
 */
void
apic_timer_init(volatile long *lp, ms_time_t period, unsigned int vector)
{
        unsigned int    clk, usec, count;

        clk = apic_findspeed(lp);

        /*
         * Now calibrate the delay loop used for busy-waiting.
         */
        count = lp[APIC_CCOUNT];
        psm_time_spin(100000);
        count = 2 * (count - lp[APIC_CCOUNT]);

        if (clk/10000)
                usec = (count*100) / (clk/10000);
        else
                usec = 0;        /* 0 means calibration failed - see toolkit */

        psm_time_spin_adjust(100000, usec);

        /*
         * clknum = (clk/2) * period_in_sec.
         * Rest of formula is to prevent overflow/underflow.
         */
        lp[APIC_LVT_TIMER] = APIC_PERIODIC | vector;
        lp[APIC_DIVIDECR] = APIC_DIVIDE_BY_2;
        lp[APIC_ICOUNT] = ((clk/10000) * (period.mst_nsec/100))/2000;
}

/*
 * void
 * apic_reset_cpu(volatile long *lp, unsigned char lid, ms_paddr_t resetcode)
 *
 * Calling/Exit State:
 *	None.
 */
void
apic_reset_cpu(volatile long *lp, unsigned char lid, ms_paddr_t resetcode)
{
        struct resetaddr {
                unsigned short  offset;
                unsigned short  segment;
        } start;
        long addr;
        char    *vector, *source;
        register      i;

        addr = resetcode;

        /* get the real address seg:offset format */
        start.offset = addr & 0x0f;
        start.segment = (addr >> 4) & 0xFFFF;

        /*
         * now put the address into warm reset vector (40:67)
         * since interrupts have been disabled, we cannot use
         * os_physmap() to accomplish this task.
         */
        vector = (char *)((int)os_page0 + APIC_RESET_VECTOR);

        /*
         * Copy byte by byte since the reset vector port is
         * not word aligned.
         */
        source = (char *) &start;
        for (i = 0; i < sizeof(struct resetaddr); i++)
                *vector++ = *source++;

       	/* Wait while an interrupt is already pending. */
       	APIC_INTR_SEND_PENDING(lp);
       	lp[APIC_ICMD2] = (lid << 24);
       	lp[APIC_ICMD] = APIC_LEVEL | APIC_ASSERT | APIC_FIXED | APIC_RESET;

       	/* Wait while an interrupt is already pending. */
       	APIC_INTR_SEND_PENDING(lp);
       	lp[APIC_ICMD2] = (lid << 24);
       	lp[APIC_ICMD] = APIC_LEVEL | APIC_DEASSERT | APIC_FIXED | APIC_RESET;

	psm_time_spin(200);

        APIC_INTR_SEND_PENDING(lp);
        lp[APIC_ICMD2] = (lid << 24);
        lp[APIC_ICMD] = APIC_EDGE | APIC_STARTUP | APIC_FIXED | (addr >> 12);

	psm_time_spin(200);

        APIC_INTR_SEND_PENDING(lp);
        lp[APIC_ICMD2] = (lid << 24);
        lp[APIC_ICMD] = APIC_EDGE | APIC_STARTUP | APIC_FIXED | (addr >> 12);
        APIC_INTR_SEND_PENDING(lp);

	psm_time_spin(200);
}

/*
 * void
 * apic_get_ticks(struct cpu_info *cpu,  ms_time_t *resolution, ms_time_t *max)
 *
 * Calling/Exit State:
 *      None.
 */
void
apic_get_ticks(struct cpu_info *cpu,  ms_time_t *resolution, ms_time_t *max)
{
        unsigned int    clk;
        volatile long *lp;

        lp = cpu->lapic_addr;
        clk = apic_findspeed(lp);

        resolution->mst_nsec = (100000/(clk/10000)) * 2 + 1;
        resolution->mst_sec = 0;

        max->mst_nsec = 0;
        max->mst_sec = resolution->mst_nsec;
}

/*
 * STATIC int
 * apic_findspeed(volatile long *lp)
 *      Return the bus clock speed of the clock driving the local APIC.
 *
 * Calling/Exit State:
 *      None.
 */

STATIC int
apic_findspeed(volatile long *lp)
{
        int             sec, osec;
        unsigned int    busclk;

        lp[APIC_LVT_TIMER] = APIC_MASK;             /* one-shot & masked */
        lp[APIC_DIVIDECR] = APIC_DIVIDE_BY_2;

	/*
	 * wait for start of a new second (ie sec changes)
	 * then count how many apic clock ticks occur in a full second.
	 * The bus speed is equal to 2X clock ticks since apic is in 
	 * divide-by-2 mode.
	 */
	sec = osec = psm_mc146818_getsec();
	while (osec == sec)
		sec=psm_mc146818_getsec();

	lp[APIC_ICOUNT] = 0x10000000;
	while (sec == (osec=psm_mc146818_getsec()))
		;

	busclk = 2 * (0x10000000 - lp[APIC_CCOUNT]);

	return (busclk);

}


#ifdef JOEFLAGS
apic_io_dump(struct ioapic_info *ioapic) {
        int     i,j,val,*valp;
	volatile long *ip;

	ip = ioapic->ioapic_addr;
        os_printf ("RDT: ");
        for (i=0, j=0; i<16; i++) {
                ip[APIC_IO_REG] = APIC_AIR_RDT + 2*i;
                val = ip[APIC_IO_DATA];
                os_printf (" %08x", val);
                j++;
                if(j == 8) {
                        j = 0;
                        os_printf("\n     ");
                }
        }
        os_printf("\n");

        os_printf ("RDT2:");
        for (i=0, j=0; i<16; i++) {
                ip[APIC_IO_REG] = APIC_AIR_RDT2 + 2*i;
                val = ip[APIC_IO_DATA];
                os_printf (" %08x", val);
                j++;
                if(j == 8) {
                        j = 0;
                        os_printf("\n     ");
                }
        }
        os_printf("\n");
}

apic_local_dump(char *ip) {
        int     i,j,val,*valp;

        os_printf("IRR: ");
        for (i=0; i<8; i++) {
                valp = (int*)(ip+0x200+i*16);
                os_printf (" %08x", *valp);
        }
        os_printf("\n");

        os_printf("ISR: ");
        for (i=0; i<8; i++) {
                valp = (int*)(ip+0x100+i*16);
                os_printf (" %08x", *valp);
        }
        os_printf("\n");

        os_printf("TMR: ");
        for (i=0; i<8; i++) {
                valp = (int*)(ip+0x180+i*16);
                os_printf (" %08x", *valp);
        }
        os_printf("\n");
}
#endif
