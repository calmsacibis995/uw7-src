#ident	"@(#)kern-i386at:psm/mps/mps.c	1.1.6.1"
#ident  "$Header$"

#include <svc/psm.h>
#include <psm/mps/mps.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_assert.h>
#include <psm/toolkits/at_toolkit/at_toolkit.h>
#include <psm/toolkits/psm_cfgtables/psm_cfgtables.h>
#include <psm/toolkits/psm_apic/psm_apic.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>
#include <psm/toolkits/psm_time/psm_time.h>
#include <psm/toolkits/psm_mc146818/psm_mc146818.h>

extern msop_func_t mps_msops[];

/*
 * Global variables related to the topology and the configuration
 * we find by walking the MPS tables.
 */

STATIC ms_topology_t 	*mps_topology;
STATIC unsigned long    mps_priset[MPS_LEVELS];
STATIC struct mp_info   mps_mpinfo;

STATIC ms_event_t	mps_clock_event = MS_EVENT_TICK_1;
STATIC ms_lockp_t	mps_attach_lock;
STATIC ms_cpu_t		mps_clock_cpunum=MS_CPU_ANY;
STATIC ms_intr_dist_t  	*mps_intr_vect[256];
STATIC struct intr_info *mps_intr_slots;
STATIC ms_cpu_t		mps_cpu_max;
STATIC ms_islot_t	mps_islot_max;
STATIC ms_bool_t	mps_irq13_connected;

STATIC struct {
	volatile unsigned int	lock;
	volatile ms_rawtime_t	time;
} mps_rawtime={0,0,0};

/*
 * Values to pass to os_set_msparam.
 */

STATIC unsigned int    mps_intr_order = MPS_LEVELS-1;
STATIC char	       mps_platform_name[] = "Generic MP";
STATIC ms_bool_t       mps_sw_sysdump = MS_TRUE;
STATIC ms_time_t       mps_time_res = {10000000,0};
STATIC ms_time_t       mps_tick_1_res = {10000000,0};
STATIC ms_time_t       mps_tick_1_max = {10000000,0};
STATIC ms_cpu_t        mps_shutdown_caps =
			      (MS_SD_HALT | MS_SD_AUTOBOOT | MS_SD_BOOTPROMPT);

void mps_intr_detach();
void mps_init_msparam();
ms_islot_t mps_assign_slots();

ms_intr_dist_t * mps_service_int( ms_ivec_t vec );
ms_intr_dist_t * mps_service_xint( ms_ivec_t vec );
ms_intr_dist_t * mps_service_timer( ms_ivec_t vec );
ms_intr_dist_t * mps_service_spur( ms_ivec_t vec );
ms_intr_dist_t * mps_service_stray( ms_ivec_t vec );

#define CPUS                    mps_mpinfo.cpus
#define LAPIC_ADDR              mps_mpinfo.cpus[os_this_cpu].lapic_addr
#define IOAPIC_PARAMS(intrp)    &(mps_mpinfo.ioapics[mps_idt2apic(intrp)])

/*
 * Interrupt slots represent a particular APIC and IntIn line by
 * storing the IntIn line number in the low four bits and the APIC number
 * in the high bits:  slot == apicno << 4 | apiclineno
 *
 * We support 6 maskable priority levels, with 32 possible interrupts
 * within each level.  The APIC treats the high 4 bits of the vector as
 * the priority and the low 4 bits as the interrupt id within the priority
 * level.  We will combine pairs of APIC priority levels into one
 * externally visible priority level, so that each maskable priority level
 * has 32 interrupts within it.  Since there are eight pairs, there should
 * be eight maskable priority levels.  However, the processor has reserved
 * vectors 0x00-0x1f (the bottommost pair) and the PSM uses
 * 0xe0-0xff (the topmost pair) * for its own purposes.
 *
 * The interrupt management policy is captured by the following routines:
 *
 * mps_vec2idt()
 *	Given a vector, return the interrupt associated with it
 *
 * mps_idt2apic()
 *	Given an interrupt, return the APIC to which it is connected.
 *	APICs are identified by their index in the apic_params
 *	structure.
 *
 * mps_idt2line()
 *	Given an interrupt, return the APIC IntIn to which it is connected.
 *
 * mps_idt2flags()
 *      Given an interrupt, return the APIC flags (e.g. polarity).
 */

#define mps_vec2idt(vec)        (mps_intr_vect[vec])
#define mps_idt2apic(intrp)     (mps_intr_slots[intrp->msi_slot].ioapic_num)
#define mps_idt2line(intrp)     (mps_intr_slots[intrp->msi_slot].line)
#define mps_idt2flags(intrp)    (mps_intr_slots[intrp->msi_slot].flags)

/*
 * ms_ivec_t
 * mps_assign_vec(ms_intr_dist_t *intrp)
 *
 * Calling/Exit State:
 *
 * Description:
 *      Assigns a vector appropriate for use with a particular interrupt
 *
 */
ms_ivec_t mps_assign_vec(ms_intr_dist_t *intrp)
{
        unsigned long s, i;

        for(i=0,s=mps_priset[intrp->msi_order]; (s&1)&&i<MPS_INTS_PER_LEVEL;
            s>>=1,i++)
                ;
        if( i < MPS_INTS_PER_LEVEL ) {
                mps_priset[intrp->msi_order] |= 1 << i;
                return (MPS_FIRST_VEC+intrp->msi_order*MPS_INTS_PER_LEVEL + i);
        } else
                return MPS_SPUR_VEC;
}

/*
 * void
 * mps_free_vec(ms_ivec_t vec)
 *
 * Calling/Exit State:
 *
 * Description:
 *      Frees a vector assigned by mps_assign_vec()
 *
 */
void mps_free_vec(ms_ivec_t vec)
{
        mps_priset[(vec- MPS_FIRST_VEC) / MPS_INTS_PER_LEVEL]
                &= ~(1 << ((vec - MPS_FIRST_VEC) % MPS_INTS_PER_LEVEL));
}

#ifdef CCNUMA
ms_topology_t * mps_topo_split(ms_topology_t * tp);
#endif

/*
 * ms_bool_t
 * mps_initpsm()
 *
 * Calling/Exit State:
 *      This function is called exactly once as the first entry
 *      into the PSM.
 *
 * Description:
 *
 * After verifying, through the presence of a valid MPSpec table, that
 * this is an MPSpec-compliant platform, scan the table to find the 
 * system configuration, and initialize accordingly.
 *
 * We make two passes through the table.  The first discovers what's there
 * so that we can size the various data structures appropriately; the second
 * actually processes the entries.
 *
 * Some points worth noting:
 *
 * When we build the topology structure, we want one entry for the entire
 * PCI hierarchy; MPS machines have a compliant PCI or no PCI.  Thus,
 * the number of PCI bridges is one if there are one or more PCI buses,
 * and zero otherwise.  One should not be confused because the topology
 * structure contains "bus" entries; it's the host bridge that we're
 * describing.  Note also that multiple-compliant-bridge machines such
 * as the Intel XXpress are treated as if they have single PCI hierarchy,
 * just as the PCI BIOS presents it.
 *
 * Interrupt rouuting entries are developed only for PCI buses, as the PCI
 * driver is currently the only driver with either the need or the
 * ability to deal with PSM_supplied interrupt routings.
 *
 * The explicit check for a connection for EISA IRQ13 caters to those
 * machines in which the IESA chipset does not take IRQ13 off-chip; in these,
 * IRQ13 is perforce connected to the 8259 PIC, whose interrupt in turn is
 * routed to whatever APIC IntIn is identified as an external interrupt,
 * and IRQ13 must be handled through the 8259 PIC.
 */
ms_bool_t
mps_initpsm()
{
        unsigned i, n, m, nr, apic, pciints;
	ms_resource_t *rp;
	msr_routing_t *rpp;
	ms_topology_t * hi_topo;
	char *cp;
#ifdef CCNUMA
	unsigned int ncg, cpus_per_cg;
#endif
	
        if (!cfgtable_mpinfo(&mps_mpinfo))
                return MS_FALSE;

	mps_islot_max = mps_assign_slots();
	mps_cpu_max = mps_mpinfo.num_cpus - 1;

        if (os_register_msops(mps_msops) != MS_TRUE)
		return  MS_FALSE;

	/*
	 * This is the commit point. If we get here, we must
	 * return TRUE, committing the system to run with this PSM.
	 * Next we do our allocations based on what we've seen in the
	 * table on the first pass, and then make a second pass to fill
	 * in the allocated data structures.
	 */

	mps_topology = (ms_topology_t *) os_alloc( sizeof(ms_topology_t));

	nr  = os_count_himem_resources();
	
	mps_topology->mst_nresource = mps_mpinfo.num_cpus +
		os_default_topology->mst_nresource + nr - 1;

#ifdef CCNUMA	
	if (cp=os_get_option("NCG")) {

		switch( cp[0] ) {
		case '2':
			ncg = 2;
			break;
			
		case '4':
			ncg = 4;
			break;
		default:
			ncg = 0;
			break;
		}
	}

	mps_topology->mst_nresource += ncg;

	cpus_per_cg = mps_mpinfo.num_cpus / ncg;
#endif
	
        rp = mps_topology->mst_resources = (ms_resource_t *)
		os_alloc(mps_topology->mst_nresource * sizeof(ms_resource_t));

#ifdef CCNUMA
	/* Fill in the CG resources */
	for(n = 0; n < ncg; n++, rp++) {
                rp->msr_cgnum = n;
                rp->msr_private = MS_FALSE;
                rp->msr_private_cpu = MS_CPU_ANY;
                rp->msr_type = MSR_CG;
		/* Make cgid = cgnum */
                rp->msri.msr_cg.msr_cgid = n;
	}
#endif
		
	for (n = 0; n < mps_mpinfo.num_cpus; n++, rp++) {
#ifdef CCNUMA
		rp->msr_cgnum = n / cpus_per_cg;
#else
		rp->msr_cgnum = 0;
#endif
		rp->msr_private = MS_FALSE;
		rp->msr_private_cpu = MS_CPU_ANY;
		rp->msr_type = MSR_CPU;
                rp->msri.msr_cpu.msr_cpuid = n;
		rp->msri.msr_cpu.msr_clockspeed = 0;

		CPUS[n].lapic_lid = APIC_LOGDEST(n);
		CPUS[n].lapic_svec = MPS_SPUR_VEC;
		CPUS[n].lapic_vec0 = APIC_MASK;
		CPUS[n].lapic_vec1 = APIC_MASK;
	}

        for(n = 0; n < os_default_topology->mst_nresource; n++ ) {
                if(os_default_topology->mst_resources[n].msr_type == MSR_MEMORY)
                        *rp++ = os_default_topology->mst_resources[n];
        }
	
	/* Detect high memory, before copying the topology structure */
        if(nr) {
                os_fill_himem_resources(rp);
		rp += nr;
	}

        for(n = 0; n < os_default_topology->mst_nresource; n++ ) {
                if(os_default_topology->mst_resources[n].msr_type == MSR_MEMORY)
                        continue;
                if(os_default_topology->mst_resources[n].msr_type == MSR_CPU )
                        continue;
                *rp = os_default_topology->mst_resources[n];

                if( rp->msr_type == MSR_BUS &&
                    rp->msri.msr_bus.msr_bus_type == MSR_BUS_PCI ) {
                	pciints = mps_islot_max - MPS_NUM_IRQS + 1;
                	rp->msri.msr_bus.msr_n_routing = pciints;
                	rpp = (msr_routing_t *)
					os_alloc(pciints*sizeof(msr_routing_t));
                	rp->msri.msr_bus.msr_intr_routing = rpp;

                	for (m = MPS_NUM_IRQS; m <= mps_islot_max; m++, rpp++) {
                            rpp->msr_islot = m;

			    /*
			     * If an islot is already in use for
			     * that Interrupt Line, override it.
			     */
			    for (i = MPS_NUM_IRQS; i < m; i++) {
				if ((mps_intr_slots[m].ioapic_num ==
				     mps_intr_slots[i].ioapic_num)
				&&  (mps_intr_slots[m].line ==
				     mps_intr_slots[i].line)) {
                            		rpp->msr_islot = i;
					break;
				}
			    }

                            rpp->msr_isource = (mps_intr_slots[m].bus_id << 7)
                                                | mps_intr_slots[m].irq;
			}
		}
                rp++;
        }

#ifdef CCNUMA
	mps_topology = mps_topo_split(mps_topology);
#endif

        mps_irq13_connected = cfgtable_eisa(&mps_mpinfo);

        apic_local_init(&CPUS[0]);
        psm_time_spin_init();
        apic_get_ticks(&CPUS[0], &mps_tick_1_res, &mps_tick_1_max);
        mps_time_res = mps_tick_1_res;

        mps_init_msparam();

	for( n = 0; n < mps_mpinfo.num_ioapics; n++ )
		apic_io_init(&mps_mpinfo.ioapics[n], 0);

        for (n = 0; n < mps_mpinfo.num_nmis; n++) {
            if( !mps_mpinfo.nmis[n].local ) {
                apic = mps_mpinfo.nmis[n].apic;
                apic_intr_set_mode( &mps_mpinfo.ioapics[apic],
                                     mps_mpinfo.nmis[n].line,
                                     APIC_NMI | APIC_LEVEL,
                                     0, 0 );
                break;
            }
        }

	/* i8259 is not used for Bus Standard interrupt handling */
	i8259_init(1 << MSR_BUS_NONSTD);

	mps_attach_lock = os_mutex_alloc();

	return( MS_TRUE );
}

#ifdef CCNUMA
#define ONE_MEG (1024*1024)
/*
 * Split the memory equally among all the CGs and modify the topology
 * structure to reflect the splitting.
 *
 * Currently handles two way splitting only.
 */
ms_topology_t *
mps_topo_split(ms_topology_t * tp)
{
	unsigned split, i, j, nr;
	ms_topology_t *tp2;
	ms_resource_t *rp;
	msr_memory_t *srp;
	char *cp;
	ms_paddr_t end, percg_mem;
	ms_paddr_t physmem = 0;
	ms_paddr_t node_start = 0;	
	ms_paddr_t rounding = ONE_MEG;
	ms_memsize_t size;

	if( !(cp=os_get_option("NCG")) )
		return tp;

	switch( cp[0] ) {
	 case '2':
		split = 2;
		break;

	 default:
		return tp;
	}

	os_printf( "Splitting %d ways\n", split );
 	nr = tp->mst_nresource;
	os_printf( "Initial resource count is %d\n", nr );

	/* Compute physmem as the max physical address */
	for( i = 0; i < tp->mst_nresource; i++ ) {
		if( tp->mst_resources[i].msr_type != MSR_MEMORY )
			continue;
		end = tp->mst_resources[i].msri.msr_memory.msr_address +
			tp->mst_resources[i].msri.msr_memory.msr_size;

		if (end > physmem)
			physmem = end;
	}

	
	/*
	 * Per CG memory is physmem/ncg - rounded up to the next megabyte.
	 */
	percg_mem = ((physmem / split) + ONE_MEG - 1)
		      & ~((ms_paddr_t) (ONE_MEG -1));

#ifdef DEBUG
	os_printf("Percg memory: %x\n", percg_mem);
#endif	
	
	for( i = 0; i < tp->mst_nresource; i++ ) {
		if( tp->mst_resources[i].msr_type != MSR_MEMORY )
			continue;
		if (tp->mst_resources[i].msri.msr_memory.msr_address +
		    tp->mst_resources[i].msri.msr_memory.msr_size > percg_mem)
			nr += split - 1;
	}

	os_printf( "Final resource count is %d\n", nr );
	
	tp2 = (ms_topology_t *) os_alloc( sizeof(ms_topology_t) );
	tp2->mst_nresource = nr;
	rp = tp2->mst_resources = (ms_resource_t *)
		os_alloc( nr * sizeof(ms_resource_t));

	for( i = 0; i < nr; i++ ) {
		switch( tp->mst_resources[i].msr_type ) {
		 case MSR_CPU:
			*rp = tp->mst_resources[i];
			rp->msr_cgnum = rp->msri.msr_cpu.msr_cpuid % split;
			rp++;
			break;

		 case MSR_MEMORY:
			srp = &tp->mst_resources[i].msri.msr_memory;
			end = srp->msr_address + srp->msr_size;
			if (end > percg_mem) {
#ifdef DEBUG
				os_printf("Initially: %Lx, %Lx\n",
					  srp->msr_address,
					  srp->msr_size);
#endif					  
				size = srp->msr_size - (end - percg_mem); 
				for( j = 0; j < split; j++ ) {
				    *rp = tp->mst_resources[i];
				    rp->msr_private = MS_FALSE;
				    rp->msr_cgnum = j;
				    if (j == 0) {
					    rp->msri.msr_memory.msr_size =
						    size;
#ifdef DEBUG
					    os_printf("0: %Lx, %Lx\n",
						      rp->msri.msr_memory.msr_address,
						rp->msri.msr_memory.msr_size);
#endif

				    } else /* j == 1 */ {

					    rp->msri.msr_memory.msr_address =
						    percg_mem;
					    rp->msri.msr_memory.msr_size =
						    end - percg_mem;
#ifdef DEBUG
					    os_printf("1: %Lx, %Lx\n",
						      rp->msri.msr_memory.msr_address,
						rp->msri.msr_memory.msr_size);
#endif

				    }
				    rp++;
				}
				break;
			} /* Fall through */
		 default:
			*rp++ = tp->mst_resources[i];
			break;
		}
	}
	return tp2;
}
#endif

/*
 * STATIC void
 * mps_init_cpu(void)
 *      Initialize the interrupt control system.  At this point,
 *      all interrupts are disabled, since none have been attached.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.  This function
 *	must be called with the IE flag off.
 */
STATIC void
mps_init_cpu(void) 
{
	int		i, lint0, lint1;
	struct cpu_info *cpup;
        struct nmi_info *nmip;

	PSM_ASSERT(!is_intr_enabled());

        nmip = mps_mpinfo.nmis;
	cpup = &mps_mpinfo.cpus[os_this_cpu];
        for (i = 0; i < mps_mpinfo.num_nmis; i++ ) {
                if( (nmip[i].apic == os_this_cpu || nmip[i].apic == 0xff)
                                                        && nmip[i].local ) {
                        if( nmip[i].line == 0 )
                                cpup->lapic_vec0 = APIC_NMI | APIC_LEVEL;
                        else
                                cpup->lapic_vec1 = APIC_NMI | APIC_LEVEL;
                        break;
                }
        }
        apic_local_init(cpup);

        if (mps_clock_cpunum == MS_CPU_ANY) {

		/*
		 * Save cpu number of this cpu - it will handle global clocks.
		 * Note that this is also a 'first_time' flag so this code is
		 * executed only on the boot engine.
		 */
		mps_clock_cpunum = os_this_cpu;

        	for (i = 0; i < 256; i++)
                	mps_intr_vect[i] = os_intr_dist_stray;

		/*
		 * Transition from PIC mode to Symmetric I/O interrupt
		 * mode.
		 */
                if (mps_mpinfo.imcrp) {
                        outb(MPS_IMCR_ADDR, 0x70);
                        outb(MPS_IMCR_DATA, 1);
                }

		/*
		 * Assign all vectors to mps_service_stray. Then assign the ones
		 * we really use to the appropriate routines. This lets us 
		 * return the right idtp for unused entries.
		 */
                os_claim_vectors(MPS_FIRST_VEC, 256 - MPS_FIRST_VEC,
                                 mps_service_stray);
                os_claim_vectors(MPS_FIRST_VEC, MPS_TIMER_VEC - MPS_FIRST_VEC,
                                 mps_service_int);
                os_claim_vectors(MPS_TIMER_VEC, 1, mps_service_timer);
                os_claim_vectors(MPS_XINT_VEC, 1, mps_service_xint);
                os_claim_vectors(MPS_SPUR_VEC, 1, mps_service_spur);
        }

	apic_timer_init(LAPIC_ADDR, os_tick_period, MPS_TIMER_VEC);
}



/*
 * STATIC ms_bool_t
 * mps_intr_attach(ms_intr_dist_t *)
 *      Enter the specified interrupt source into the dispatch
 *      tables and (if not already enabled) enable distribution
 *	of interrupts for this slot in the interrupt controller(s).
 *
 * Calling/Exit State:
 *      Called when the system is being initialized.  This function
 *      must be called with the IE flag off.
 */
STATIC ms_bool_t
mps_intr_attach(ms_intr_dist_t *idtp)
{
        ms_cpu_t cpu;
	ms_ivec_t vec;
	ms_lockstate_t ls;
	unsigned int i, mode;

	PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(idtp->msi_slot <= mps_islot_max);

	if ((mps_irq13_connected == MS_FALSE) &&
            (idtp->msi_slot == I8259_IRQ13))
		return MS_FALSE;

	ls = os_mutex_lock(mps_attach_lock);
	vec = mps_assign_vec( idtp );
	if( vec == MPS_SPUR_VEC ) {
		os_mutex_unlock(mps_attach_lock, ls);
		return MS_FALSE;
	}

       	for (i = 0; i < 256; i++) {
               	if (mps_intr_vect[i] == idtp)
			mps_intr_detach( idtp );
	}

	idtp->msi_mspec = (void *) vec;
	mps_intr_vect[vec] = idtp;
	os_mutex_unlock(mps_attach_lock, ls);

	cpu = idtp->msi_cpu_dist;
        if ((mps_idt2flags(idtp) & CFG_INT_FL_POMASK) == CFG_INT_FL_POHIGH)
                mode = APIC_POHIGH;
	else
        	mode = APIC_POLOW;

        if (idtp->msi_flags & MSI_ITYPE_CONTINUOUS) 
		mode |= APIC_LEVEL;
	else
		mode |= APIC_EDGE;

        apic_intr_set_mode(IOAPIC_PARAMS(idtp), mps_idt2line(idtp),
                           mode, vec, cpu);

	idtp->msi_flags |= MSI_ORDERED_COMPLETES;
	idtp->msi_flags &= ~MSI_NONMASKABLE;
        apic_intr_unmask(IOAPIC_PARAMS(idtp), mps_idt2line(idtp));
	return(MS_TRUE);
}


/*
 * STATIC void
 * mps_intr_detach(ms_intr_dist_t *)
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
mps_intr_detach(ms_intr_dist_t *idtp)
{
	ms_ivec_t	vec;
	ms_lockstate_t	ls;

	PSM_ASSERT(!is_intr_enabled());

        apic_intr_mask(IOAPIC_PARAMS(idtp), mps_idt2line(idtp));
	vec = (ms_ivec_t) idtp->msi_mspec;

	ls = os_mutex_lock(mps_attach_lock);
	mps_free_vec(vec);

	mps_intr_vect[vec] = os_intr_dist_stray;
	os_mutex_unlock(mps_attach_lock, ls);


}


/*
 * STATIC void
 * mps_intr_mask(ms_intr_dist_t *)
 * 	Mask off (prevent from interrupting) interrupt requests
 *	coming in on the specified islot request line on the
 * 	specified APIC.
 *
 * Calling/Exit State:
 *	
 */
STATIC void
mps_intr_mask(ms_intr_dist_t *idtp)
{
	ms_ivec_t	vec;

	PSM_ASSERT(!is_intr_enabled());

        apic_intr_mask(IOAPIC_PARAMS(idtp), mps_idt2line(idtp));
}


/*
 * STATIC void
 * mps_intr_unmask(ms_intr_dist_t *)
 * 	Unmask (allow to interrupt) interrupt requests coming
 *	in on the specified islot request line on the
 * 	specified PIC.
 *
 * Calling/Exit State:
 *
 */
STATIC void
mps_intr_unmask(ms_intr_dist_t *idtp)
{
    	ms_ivec_t	vec;

    	PSM_ASSERT(!is_intr_enabled());

    	apic_intr_unmask(IOAPIC_PARAMS(idtp), mps_idt2line(idtp));
}


/*
 * STATIC void
 * mps_intr_complete(ms_intr_dist_t *)
 *
 *
 * Calling/Exit State:
 *
 */
STATIC void
mps_intr_complete(ms_intr_dist_t *idtp)
{

	PSM_ASSERT(!is_intr_enabled());
        apic_eoi(LAPIC_ADDR);
}


/*
 * STATIC ms_intr_dist_t *
 * mps_service_int( ms_ivec_t )
 *	Service an interrupt from an attached slot.
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
mps_service_int( ms_ivec_t vec )
{
	PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(vec >= MPS_FIRST_VEC && vec < MPS_TIMER_VEC);

	return (mps_intr_vect[vec]);
}


/*
 * STATIC ms_intr_dist_t *
 * mps_service_timer( ms_ivec_t )
 *	Service a timer interrupt.
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
mps_service_timer( ms_ivec_t vec )
{
	unsigned int	lo, newlock;

	PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(vec == MPS_TIMER_VEC);

	if (os_this_cpu == mps_clock_cpunum) {
		newlock = mps_rawtime.lock+1;
		mps_rawtime.lock = 0;
		lo = mps_rawtime.time.msrt_lo + os_tick_period.mst_nsec;
		if (lo >= 1000000000) {
			lo -= 1000000000;
			mps_rawtime.time.msrt_hi++;
		}
		mps_rawtime.time.msrt_lo = lo;
		mps_rawtime.lock = newlock ? newlock : 1;
	}

	os_post_events(mps_clock_event);
        apic_eoi(LAPIC_ADDR);
	return (os_intr_dist_nop);
}


/*
 * STATIC ms_intr_dist_t *
 * mps_service_xint( ms_ivec_t )
 *	Service a cross interrupt.
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
mps_service_xint( ms_ivec_t vec )
{
	ms_event_t	eventflags;

	PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(vec == MPS_XINT_VEC);

	eventflags = (ms_event_t) 
		     atomic_fnc(&CPUS[os_this_cpu].eventflags);

        apic_eoi(LAPIC_ADDR);

	eventflags &= ~(MS_EVENT_PSM_1 | MS_EVENT_PSM_2);
	if (eventflags) os_post_events (eventflags);

	return (os_intr_dist_nop);
}


/*
 * STATIC ms_intr_dist_t *
 * mps_service_spur( ms_ivec_t )
 *	Service a spurious interrupt - uses a special APIC vector for interrupts
 *	that go away before being recognized.
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
mps_service_spur( ms_ivec_t vec )
{
	PSM_ASSERT(vec == MPS_SPUR_VEC);

	return (os_intr_dist_nop);
}


/*
 * STATIC ms_intr_dist_t *
 * mps_service_stray( ms_ivec_t )
 *	Service an interrupt from a vector that is currently unused and
 *	unassigned. We have no idea why these interrupts ever occur except
 *	possibly from race conditions where interrupts are being
 *	detached while they are occuring.
 *
 * Calling/Exit State:
 *
 */
STATIC ms_intr_dist_t *
mps_service_stray( ms_ivec_t vec )
{
	PSM_ASSERT(mps_intr_vect[vec] == os_intr_dist_stray);

	os_intr_dist_stray->msi_slot = 0xffff; /* ZZZ ?? no slot associated with vec */
	apic_eoi(LAPIC_ADDR);
	return (os_intr_dist_stray);
}


/*
 * STATIC void
 * mps_xpost(ms_cpu_t, ms_event_t)
 *
 *
 * Calling/Exit State:
 *
 */
STATIC void
mps_xpost(ms_cpu_t cpu, ms_event_t eventmask )
{
	PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(cpu != MS_CPU_ANY);

	if (cpu != MS_CPU_ALL_BUT_ME) {
		atomic_or(&CPUS[cpu].eventflags, eventmask);
		apic_xintr(LAPIC_ADDR, CPUS[cpu].lapic_lid, MPS_XINT_VEC);
	} else {
		for (cpu=0; cpu<=mps_cpu_max; cpu++)
			if (cpu != os_this_cpu) {
			    atomic_or(&CPUS[cpu].eventflags, 
				      eventmask);
			    apic_xintr(LAPIC_ADDR, CPUS[cpu].lapic_lid, 
					MPS_XINT_VEC);
			}
	}
}


/*
 * STATIC void
 * mps_offline_prep(void)
 *
 * Calling/Exit State:
 *
 * Remarks:
 *
 */
STATIC void
mps_offline_prep(void)
{

	/*
	 * Disable all interrupts from being accepted by this cpu. This
	 * includes clock interrupts as well as timer & spurious interrupts
	 * Note that pending interrupt may be queued for us - the base
	 * kernel must go to spl0 & let these interrupts be processed.
	 */
	PSM_ASSERT(!is_intr_enabled());
	apic_local_disable(LAPIC_ADDR);
}


/*
 * STATIC void
 * mps_offline_self(void)
 *
 * Calling/Exit State:
 *
 * Remarks:
 *
 */
STATIC void
mps_offline_self(void)
{
	PSM_ASSERT(!is_intr_enabled());
	for (;;)
		asm("hlt");
	/* NOTREACHED */
}



/*
 * STATIC void
 * msp_tick2(ms_time_t)
 *	Enable/disable the tick 2 clock.
 *
 * Calling/Exit State:
 *
 */

STATIC void
mps_tick_2(ms_time_t time)
{
	PSM_ASSERT(!is_intr_enabled());
	if (time.mst_sec || time.mst_nsec) {
		PSM_ASSERT(time.mst_sec == os_tick_period.mst_sec && 
			time.mst_nsec == os_tick_period.mst_nsec);
		mps_clock_event = MS_EVENT_TICK_1 | MS_EVENT_TICK_2;
	} else
		mps_clock_event = MS_EVENT_TICK_1;
}


/*
 * STATIC void
 * mps_time_get(ms_rawtime_t *)
 *	Get the current value of a free-running high-resolution
 *	clock counter, in PSM-specific units.  This clocks should
 *	not wrap around in less than an hour.
 *
 * Calling/Exit State:
 *	- Save the current time stamp that is opaque to the base kernel
 *	  in ms_rawtime_t.	  
 */

STATIC void
mps_time_get(ms_rawtime_t *gtime)
{
	unsigned int	oldlock;
	do {
		oldlock = mps_rawtime.lock;
		*gtime = mps_rawtime.time;
	} while (oldlock == 0 || oldlock != mps_rawtime.lock);
}


/*
 * STATIC void
 * mps_time_add(ms_rawtime_t *, ms_rawtime_t *)
 *	Add 2 raw times.
 *
 * Calling/Exit State:
 *
 */
STATIC void
mps_time_add(ms_rawtime_t *dst, ms_rawtime_t *src)
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
 * mps_time_sub(ms_rawtime_t *, ms_rawtime_t *)
 *	Subtract 2 raw times.
 *
 * Calling/Exit State:
 *
 */
STATIC void
mps_time_sub(ms_rawtime_t *dst, ms_rawtime_t *src)
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
 * mps_time_cvt(ms_time_t *, ms_rawtime_t *)
 *	Convert a PSM-specific high-resolution time value to seconds
 *	or nanoseconds.
 *
 * Calling/Exit State:
 *      - Convert the opaque time stamp to micro second.
 *
 */
STATIC void
mps_time_cvt(ms_time_t *dst, ms_rawtime_t *src)
{
	dst->mst_sec = src->msrt_hi;
	dst->mst_nsec = src->msrt_lo;
}


/*
 * STATIC void
 * mps_idle_self(void)
 *
 *
 *
 * Calling/Exit State:
 *      
 *
 */
STATIC void
mps_idle_self(void)
{
	int ena;

	PSM_ASSERT(is_intr_enabled());

	ena = intr_disable();

	while(CPUS[os_this_cpu].idleflag == 0) {
		asm("sti");
		asm("hlt");
		ena = intr_disable();
	}

	intr_restore(ena);

	CPUS[os_this_cpu].idleflag = 0;
}


/*
 * STATIC void
 * mps_idle_exit(ms_cpu_t cpu)
 *
 *
 *
 * Calling/Exit State:
 *      
 *
 */
STATIC void
mps_idle_exit(ms_cpu_t cpu)
{
	CPUS[cpu].idleflag = 1;

	if (cpu != os_this_cpu)
		mps_xpost (cpu, MS_EVENT_PSM_1);

}


/*
 * STATIC void
 * mps_start_cpu(ms_cpu_t, ms_paddr_t )
 *
 *
 *
 * Calling/Exit State:
 *
 *
 */
STATIC void
mps_start_cpu(ms_cpu_t cpu, ms_paddr_t reset_code)
{
	int	ena;

	PSM_ASSERT(is_intr_enabled());
	ena = intr_disable();
	psm_warmreset();
	apic_reset_cpu(LAPIC_ADDR, CPUS[cpu].lapic_pid, reset_code);
	intr_restore(ena);
}




/*
 * STATIC void
 * mps_init_msparam()
 *
 * Calling/Exit State:
 *
 */
STATIC void
mps_init_msparam()
{
	os_set_msparam(MSPARAM_INTR_ORDER_MAX, &mps_intr_order);

	if (mps_mpinfo.platform_name[0])
		os_set_msparam(MSPARAM_PLATFORM_NAME,
			       &mps_mpinfo.platform_name[0]);
	else
		os_set_msparam(MSPARAM_PLATFORM_NAME, &mps_platform_name);

	os_set_msparam(MSPARAM_SW_SYSDUMP, &mps_sw_sysdump);
	os_set_msparam(MSPARAM_TIME_RES, &mps_time_res);
	os_set_msparam(MSPARAM_TICK_1_RES, &mps_tick_1_res);
	os_set_msparam(MSPARAM_TICK_1_MAX, &mps_tick_1_max);
	os_set_msparam(MSPARAM_ISLOT_MAX, &mps_islot_max);
	os_set_msparam(MSPARAM_SHUTDOWN_CAPS, &mps_shutdown_caps);
	os_set_msparam(MSPARAM_TOPOLOGY, mps_topology );
}



/*
 * STATIC void
 * mps_show_state(void)
 *
 *	Print platform specific panic info.
 *	ZZZ - send EOIs for possibly unacknowleged interrupts.
 *
 * Calling/Exit State:
 *      
 *
 */
STATIC void
mps_show_state()
{
	int		i;
	for (i=0; i<=MPS_LEVELS; i++)
		apic_eoi(LAPIC_ADDR);
}


/*
 * STATIC void
 * mps_shutdown(int)
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
mps_shutdown(int action)
{
	int n;

	PSM_ASSERT(!is_intr_enabled());

	apic_local_disable(LAPIC_ADDR);

	for( n = 0; n < mps_mpinfo.num_ioapics; n++ )
		apic_deinit(&mps_mpinfo.ioapics[n]);

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
		asm("hlt");
}

msop_func_t mps_msops[] = {
        { MSOP_INIT_CPU,       (void *)mps_init_cpu },
        { MSOP_INTR_ATTACH,     (void *)mps_intr_attach },
        { MSOP_INTR_DETACH,     (void *)mps_intr_detach },
        { MSOP_INTR_MASK,       (void *)mps_intr_mask },
        { MSOP_INTR_UNMASK,     (void *)mps_intr_unmask },
        { MSOP_INTR_COMPLETE,   (void *)mps_intr_complete },
	{ MSOP_TICK_2,		(void *)mps_tick_2 },
	{ MSOP_TIME_GET,	(void *)mps_time_get },
	{ MSOP_TIME_ADD,	(void *)mps_time_add },
	{ MSOP_TIME_SUB,	(void *)mps_time_sub },
	{ MSOP_TIME_CVT,	(void *)mps_time_cvt },
	{ MSOP_TIME_SPIN,	(void *)psm_time_spin },
        { MSOP_XPOST, 		(void *)mps_xpost },
        { MSOP_RTODC,           (void *)psm_mc146818_rtodc },
        { MSOP_WTODC,           (void *)psm_mc146818_wtodc },
        { MSOP_IDLE_SELF,       (void *)mps_idle_self },
        { MSOP_IDLE_EXIT,       (void *)mps_idle_exit },
        { MSOP_START_CPU,     	(void *)mps_start_cpu },
        { MSOP_OFFLINE_PREP,    (void *)mps_offline_prep },
        { MSOP_OFFLINE_SELF,    (void *)mps_offline_self },
        { MSOP_SHUTDOWN,        (void *)mps_shutdown },
        { MSOP_SHOW_STATE,      (void *)mps_show_state },
        { 0,                    NULL }
};

STATIC ms_islot_t
mps_assign_slots()
{
	unsigned int i, slots, pcislots;
	unsigned char irq;

	for (i = 0, pcislots = 0; i < mps_mpinfo.num_intrs; i++) {
		if (mps_mpinfo.intrs[i].bus_type == CFG_BUS_PCI)
			pcislots++;
	}

	slots = MPS_NUM_IRQS + pcislots;
	mps_intr_slots = (struct intr_info *)
				os_alloc(sizeof(struct intr_info) * slots);

	for (i = 0, pcislots = MPS_NUM_IRQS; i < mps_mpinfo.num_intrs; i++) {
		switch (mps_mpinfo.intrs[i].bus_type) {
			case CFG_BUS_PCI:
			    mps_intr_slots[pcislots++] = mps_mpinfo.intrs[i];
			    break;

			case CFG_BUS_ISA:
			case CFG_BUS_EISA:
			case CFG_BUS_MCA:
			    irq = mps_mpinfo.intrs[i].irq;
			    mps_intr_slots[irq] = mps_mpinfo.intrs[i];
			    break;

		}
	}

	return (pcislots - 1);
}
			

