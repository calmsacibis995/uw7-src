#ident	"@(#)kern-i386at:psm/toolkits/psm_cfgtables/psm_cfgtables.c	1.1.4.2"
#ident	"$Header$"


/*
 * Routines for dealing with the MPS (Multi-Processor Spec) and MCS
 * (Multi-Computer Spec) configuration tables.
 *
 * Both tables contain entries describing hardware features available on
 * a particular platform.   
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_cfgtables/psm_cfgtables.h>
#include <psm/toolkits/psm_apic/psm_apic.h>

/*
 * Function prototypes for internal procedures.
 */

STATIC ms_bool_t find_fp(unsigned long, unsigned long, struct mpfp **);
STATIC ms_bool_t find_fpptr(struct mpfp **);
STATIC ms_bool_t mp_table_chk(struct mpconfig *);
STATIC ms_bool_t mc_table_chk(struct mcconfig *);

/*
 * Feature information bytes from the MP table's floating pointer structure 
*/

STATIC char cfgtable_default = 0;
STATIC char cfgtable_imcrp = 0;

extern int mp_entry_sizes[];
extern struct mpconfig *mpc_defaults[];

#define	CFG_DEF_CONF2	2	/* EISA, Neither timer IRQ0 nor DMA chaining */

/*
 * ms_bool_t
 * cfgtable_mpinfo(struct mp_info *mpi)
 *
 * Calling/Exit State:
 */

ms_bool_t
cfgtable_mpinfo(struct mp_info *mpi)
{
	struct mpfp *fp;	
        struct mpconfig *mp;

	if (find_fpptr(&fp)) {
        	mp = (struct mpconfig *)fp->mpaddr;
        	cfgtable_default = fp->mp_feature_byte[0];
        	cfgtable_imcrp = fp->mp_feature_byte[1] & CFG_IMCRP;
	} else
		return (MS_FALSE);

	if (mp == 0) {
        	if (cfgtable_default)
                	mp = mpc_defaults[cfgtable_default];
		else
			return (MS_FALSE);
	}

	if (!cfgtable_readmp(mp, mpi))
		return (MS_FALSE);

	cfgtable_default = 0;

	return (MS_TRUE);
}

/*
 * ms_bool_t
 * cfgtable_readmp(unsigned long *mp, struct mp_info *mp_info)
 *
 *      Walk the MP configuration table entries grabbing information 
 *	needed by the PSM.  If the mp_table parameter is 0 it indicates
 *	that a default MP table defined by ctable_fib1 is to be used.
 *
 *	If the MP table's checksum fails a value of MS_FALSE is returned
 *	to the caller.  
 *
 * Calling/Exit State:
 *      Saves the 2 feature information bytes in table_fib1 and table_fib2
 *      for later use.  Passes back the pointer to the MP config table
 *      itself.  If the table's doesn't exist a value of 0 is passed back.
 */

ms_bool_t
cfgtable_readmp(struct mpconfig *mp, struct mp_info *mpi)
{
	union mpcentry *ep;
	unsigned char type;
        unsigned int nentries, ncpus, nbuses, napics, nintrs, npcis, nnmis; 
	unsigned int i, j, table_size, polarity;
	char *end_of_tbl;
	char apicid;
	long *lapic_addr;
	struct mpconfig *vmp;

        mpi->imcrp = cfgtable_imcrp;

	if (!cfgtable_default) {
        	vmp = (struct mpconfig *)
			os_physmap((ms_paddr_t)mp, sizeof(struct mpchdr));

		os_physmap_free((void *)vmp, sizeof(struct mpchdr));

		table_size = vmp->hdr.tbl_len + vmp->hdr.ext_len;
        	vmp = (struct mpconfig *)
			os_physmap((ms_paddr_t)mp, table_size);

        	if (!mp_table_chk(vmp)) {
			os_physmap_free((void *)vmp, table_size);
                	return (MS_FALSE);
		}

		for (i=0; i < sizeof(vmp->hdr.oem_id); i++)
			mpi->platform_name[i] = vmp->hdr.oem_id[i];
		while (mpi->platform_name[--i] == ' ');
		mpi->platform_name[i+1] = ' ';
		i += 2;

		for (j=0; j < sizeof(vmp->hdr.product_id); i++, j++)
			mpi->platform_name[i] = vmp->hdr.product_id[j];
		while (mpi->platform_name[--i] == ' ');
		mpi->platform_name[i+1] = 0;
		
	} else  {
	
		vmp = mp;
		table_size = vmp->hdr.tbl_len + vmp->hdr.ext_len;

		mpi->platform_name[0] = 0;
	} 

        lapic_addr = (long *)os_physmap(vmp->hdr.loc_apic_adr, CFG_APIC_SIZE);
	nentries = vmp->hdr.num_entry;
	end_of_tbl = (char *)((char *)vmp + table_size);

#ifdef DEBUG
	os_printf("Number of MP entries is %d\n", nentries);
#endif

        ep = vmp->entry;
        type = ep->bytes[0];

	ncpus = napics = nbuses = nintrs = npcis = nnmis = 0;
        for (i=0; i < nentries; i++) {
         	switch(type) {
            		case CFG_ET_PROC:
                		ncpus++;
                		break;

            		case CFG_ET_BUS:
                        	nbuses++;
                                if (ep->b.name[0] == 'P' &&
                                    ep->b.name[1] == 'C' &&
                                    ep->b.name[2] == 'I' &&
                                    ep->b.name[3] == ' ' &&
                                    ep->b.name[4] == ' ' &&
                                    ep->b.name[5] == ' ')
					npcis = 1;
                		break;

            		case CFG_ET_IOAPIC:
                		napics++;
                		break;

            		case CFG_ET_I_INTR:
                        	nintrs++;	/* Fall through */

         		case CFG_ET_L_INTR:
             			if( ep->i.intr_type == CFG_INT_NMI )
                        		nnmis++;
	
         	}
                ep = (union mpcentry *)((char *)ep + mp_entry_sizes[type]);
		type = ep->bytes[0];
        }

#ifdef DEBUG
	os_printf("Found %d CPUs  %d Buses  %d APICs  and %d I/O ints\n",
			ncpus, nbuses, napics, nintrs);
#endif

	mpi->num_cpus = ncpus;
	mpi->cpus = (struct cpu_info *)
				os_alloc(ncpus * sizeof(struct cpu_info));
	mpi->num_buses = nbuses;
	mpi->num_pcis = npcis;
	mpi->buses = (struct bus_info *)
				os_alloc(nbuses * sizeof(struct bus_info));
	mpi->num_ioapics = napics;
	mpi->ioapics = (struct ioapic_info *)
				os_alloc(napics * sizeof(struct ioapic_info));
	mpi->num_intrs = nintrs;
	mpi->intrs = (struct intr_info *)
				os_alloc(nintrs * sizeof(struct intr_info));
    	mpi->num_nmis = nnmis;
    	mpi->nmis = (struct nmi_info *)
				os_alloc(nnmis * sizeof(struct nmi_info));

	mpi->mc.mc_table = 0;

	ncpus = napics = nbuses = nintrs = nnmis= 0;
        ep = vmp->entry;
        type = ep->bytes[0];
        for (i=0; i < nentries; i++) {
         	switch(type) {
            		case CFG_ET_PROC:
				if (ep->p.cpu_flags & CFG_CPU_ENABLE) {
				    mpi->cpus[ncpus].lapic_pid = ep->p.apic_id;
				    mpi->cpus[ncpus].lapic_addr = lapic_addr;
				    mpi->cpus[ncpus].idleflag = 0;
				    ncpus++;
				}
				break;
			case CFG_ET_BUS:
				mpi->buses[nbuses].bus_id = ep->b.bus_id;

				if (ep->b.name[0] == 'P' &&
		    		    ep->b.name[1] == 'C' &&
		        	    ep->b.name[2] == 'I' &&
		    		    ep->b.name[3] == ' ' &&
		    		    ep->b.name[4] == ' ' &&
		    		    ep->b.name[5] == ' ') 
				    mpi->buses[nbuses].bus_type = CFG_BUS_PCI;
			
                		else if (ep->b.name[0] == 'E' &&
                         		 ep->b.name[1] == 'I' &&
                         		 ep->b.name[2] == 'S' &&
                         		 ep->b.name[3] == 'A' &&
                         		 ep->b.name[4] == ' ' &&
                         		 ep->b.name[5] == ' ') 
                        	    mpi->buses[nbuses].bus_type = CFG_BUS_EISA;

                		else if (ep->b.name[0] == 'I' &&
                         		 ep->b.name[1] == 'S' &&
                         		 ep->b.name[2] == 'A' &&
                         		 ep->b.name[3] == ' ' &&
                         		 ep->b.name[4] == ' ' &&
                         		 ep->b.name[5] == ' ') 
                        	    mpi->buses[nbuses].bus_type = CFG_BUS_ISA;

                		else if (ep->b.name[0] == 'M' &&
                         		 ep->b.name[1] == 'C' &&
                         		 ep->b.name[2] == 'A' &&
                         		 ep->b.name[3] == ' ' &&
                         		 ep->b.name[4] == ' ' &&
                         		 ep->b.name[5] == ' ')
                                    mpi->buses[nbuses].bus_type = CFG_BUS_MCA;
				else
				    mpi->buses[nbuses].bus_type = CFG_BUS_OTHER;

				nbuses++;
				break;

        		case CFG_ET_IOAPIC:
                		if (ep->a.ioapic_flags & CFG_IOAPIC_ENABLE) {
                        	   mpi->ioapics[napics].ioapic_id = 
								ep->a.apic_id;
				   mpi->ioapics[napics].ioapic_addr = 
                        		  (long *)os_physmap(ep->a.io_apic_adr,
 							     CFG_APIC_SIZE);
				   napics++;
				}
				break;

        		case CFG_ET_I_INTR:
                		if (ep->i.intr_type == CFG_INT_INT) {
				    mpi->intrs[nintrs].line = ep->i.dest_line;
				    mpi->intrs[nintrs].irq = ep->i.src_irq;
				    mpi->intrs[nintrs].bus_id = ep->i.src_bus;
                		    mpi->intrs[nintrs].flags = ep->i.intr_flags;

				    apicid = ep->i.dest_apicid;
				    for (j=0; j < napics; j++) {
					if (mpi->ioapics[j].ioapic_id == apicid)
					    mpi->intrs[nintrs].ioapic_num = j;	
				    }	
			
				    for (j=0; j < nbuses; j++) {
					if (mpi->buses[j].bus_id 
					    == mpi->intrs[nintrs].bus_id)
				      		mpi->intrs[nintrs].bus_type = 
							mpi->buses[j].bus_type;
				    }

                                    if ((ep->i.intr_flags & CFG_INT_POMASK) ==
                                         CFG_INT_POHIGH)
                                        polarity = CFG_INT_FL_POHIGH;
                                    else if ((ep->i.intr_flags & CFG_INT_POMASK)
					      == CFG_INT_POLOW)
                                        polarity = CFG_INT_FL_POLOW;
				    else {
                                    	if (mpi->intrs[nintrs].bus_type !=
                                            CFG_BUS_PCI)
                                            polarity = CFG_INT_FL_POHIGH;
					else
                                    	    polarity = CFG_INT_FL_POLOW;
				    }

                                    mpi->intrs[nintrs].flags &=
                                        		  ~(CFG_INT_FL_POMASK);
                                    mpi->intrs[nintrs].flags |= polarity;

				    nintrs++;
				} /* Fall through */

         		case CFG_ET_L_INTR:
            			if (ep->i.intr_type == CFG_INT_NMI ) {
                		    mpi->nmis[nnmis].line = ep->i.dest_line;
                		    mpi->nmis[nnmis].bus_id = ep->i.src_bus;
                		    mpi->nmis[nnmis].flags = ep->i.intr_flags;
                		    if( type == CFG_ET_L_INTR ) {
                    		    	for( j=0; j < ncpus; j++ ) {
                        		    if( mpi->cpus[j].lapic_pid == 
					   	ep->i.dest_apicid ) {
                                		    mpi->nmis[nnmis].apic = j;
                                		    break;
                        		    }
                     		    	}
                    			if (ep->i.dest_apicid == 0xff)
                        		    mpi->nmis[nnmis].apic = 0xff;
                     			mpi->nmis[nnmis].local = MS_TRUE;
                		    } else {
                    			for( j=0; j < napics; j++) {
                        		    if (mpi->ioapics[j].ioapic_id == 
						ep->i.dest_apicid) {
                                		    mpi->nmis[nnmis].apic = j;
                                		    break;
                        		    }
                    			}
                    			mpi->nmis[nnmis].local = MS_FALSE;
                		    }
                		    nnmis++;
            			}
            			break;

		}
                ep = (union mpcentry *)((char *)ep + mp_entry_sizes[type]);
                type = ep->bytes[0];
	}

	while ((char *)ep < end_of_tbl) {
		switch(type) {
			case CFG_ET_MC_PTR:
				mpi->mc.node_id = ep->m.node_id;
				mpi->mc.mc_table = ep->m.table_ptr;
		}
                ep = (union mpcentry *)((char *)ep + ep->bytes[1]);
                type = ep->bytes[0];
	}
			

	if (!cfgtable_default)
		os_physmap_free(vmp, table_size);

	return (MS_TRUE);
}

/*
 * ms_bool_t
 * find_fpptr(struct mpfp **fp)
 *
 *	Search for MP configuration table's floating pointer structure.  
 *
 *	If the floating pointer structure is found and it checks out to be
 *	okay a value of MS_TRUE is returned to the caller.  If the 
 *	structure contains a pointer to an MP table it's value is passed back
 *	in the mp_table parameter.
 *	
 *	If the floating pointer structure is okay and there is no MP table
 *	then mp_table is set to 0.  This indicates that a default MP table
 *	has been specified and will be used.
 *
 * PC+MP floating pointer signature "_MP_" search order:
 *	1. First 1k bytes of EBDA (Extended BIOS data Area)	
 *	   EBDA pointer is defined at 2-byte location (40:0Eh)
 *	   Standard EBDA segment located at 639K	
 *	   i.e., (40:0Eh) = C0h; (40:0Fh) = 9F;	
 *	2. Last 1k bytes of the base memory if EBDA undefined	
 *	   i.e., 639k-640k for system with 640k base memory
 *	3. ROM space 0F0000h-0FFFFFFh if nothing found in RAM
 *
 * Calling/Exit State:
 *
 */

STATIC
ms_bool_t
find_fpptr(struct mpfp **fp)
{
	unsigned long ebda_addr, bmem_addr; 
	unsigned int ebda_base, bmem_base;

	ebda_addr = (unsigned long)os_page0 + CFG_EBDA_PTR;
	ebda_base = (int) ((*(unsigned short *)ebda_addr) << 4);
	if (ebda_base > 639*1024 || ebda_base < 510*1024)
		ebda_base = 0;		/* EBDA undefined */

	bmem_addr = (unsigned long)os_page0 + CFG_BMEM_PTR;
	bmem_base = (int) (((*(unsigned short *)bmem_addr)-1) * 1024);
	if (bmem_base > 639*1024 || bmem_base < 510*1024)
		bmem_base = 0;		/* Base memory undefined */

	if (ebda_base) {
		if (!find_fp(ebda_base, ebda_base+1023, fp ))
			if (!find_fp(0xf0000, 0xfffff, fp)) 
				return MS_FALSE;
	} else {
		if (!bmem_base || !find_fp(bmem_base, bmem_base+1023, fp))
			if (!find_fp(0xf0000, 0xfffff, fp))
				return MS_FALSE;
	}

	return MS_TRUE;
}


/*
 * ms_bool_t 
 * find_fp(unsigned long begin, unsigned long end, struct mpfp **fptr)
 *
 *	This routine looks for the MP floating pointer structure
 *	from physical address <begin> to <end>.  If the structure's
 *	found and it's checksum computes okay the pointer to the
 *	structure is passed back to the caller with a return value
 *	of MS_TRUE.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC
ms_bool_t
find_fp(unsigned long begin, unsigned long end, struct mpfp **fptr)
{
	unsigned long vbegin;
	struct mpfp  *fp, *endp;
	ms_size_t sz;
        char *cp;
        unsigned char sum;
        unsigned int i;
		
	sz = end - begin + 1;
	vbegin = (unsigned long)os_physmap((ms_paddr_t)begin, sz);
	fp = (struct mpfp *)vbegin;
	endp = (struct mpfp *)(vbegin + sz - sizeof(*fp));

	for (; fp <= (struct mpfp *)endp; fp++) {
		if (fp->sig[0] == '_' && fp->sig[1] == 'M' &&
		    fp->sig[2] == 'P' && fp->sig[3] == '_') {

			sum = 0;
			cp = (char *)fp;
			for (i = 0; i < sizeof(*fp); i++)
				sum += *cp++;
			if (sum == 0) {
				*fptr = fp;
				return MS_TRUE;
			}
		}
	}
	os_physmap_free((void *)vbegin, sz);
	*fptr = 0;
	return MS_FALSE;
}

/*
 * ms_bool_t
 * cfgtable_readmc(struct mcconfig *mc, struct mc_info *mci)
 *
 *      Walk the MC configuration table entries grabbing information 
 *	needed by the PSM.
 *
 * Calling/Exit State:
 */

ms_bool_t
cfgtable_readmc(struct mcconfig *mc, struct mc_info *mci)
{
	union mpcentry *ep;
        unsigned int nentries, nnodes, nbridges, nmem_spaces, i;
	unsigned char type;
	struct mcconfig *vmc;
	int table_size;

        vmc = (struct mcconfig *)

                        os_physmap((ms_paddr_t)mc, sizeof(struct mcchdr));

        table_size = vmc->hdr.tbl_len;
        os_physmap_free((void *)vmc, sizeof(struct mcchdr));

        vmc = (struct mcconfig *) os_physmap((ms_paddr_t)mc, table_size);

        if (!mc_table_chk(vmc)) {
                os_physmap_free((void *)vmc, table_size);
                return (MS_FALSE);
	}

	nentries = vmc->hdr.num_entries;
        ep = vmc->entry;
        type = ep->bytes[0];

	nnodes = nbridges = nmem_spaces = 0;
        for (i=0; i < nentries; i++) {
         	switch(type) {
            		case CFG_ET_NODE:
                                if (ep->n.flags & CFG_NODE_ENABLED_MASK)
                                        nnodes++;
                		break;

            		case CFG_ET_MC_BRIDGE:
                		nbridges++;
                		break;

            		case CFG_ET_MC_ADDR:
                        	nmem_spaces++;
                		break;
         	}
                ep = (union mpcentry *)((char *)ep + ep->bytes[1]);
		type = ep->bytes[0];
        }

	mci->num_nodes = nnodes;
	mci->nodes = (struct node_info *)
				os_alloc(nnodes * sizeof(struct node_info));
	mci->num_mc_bridges = nbridges;
	mci->mc_bridges = (struct bridge_info *)
				os_alloc(nbridges * sizeof(struct bridge_info));
	mci->num_mem_spaces = nmem_spaces;
	mci->mem_spaces = (struct mem_info *)
				os_alloc(nmem_spaces * sizeof(struct mem_info));

        ep = vmc->entry;
        type = ep->bytes[0];
	nnodes = nbridges = nmem_spaces = 0;
        for (i=0; i < nentries; i++) {
         	switch(type) {
            		case CFG_ET_NODE:
                                if (ep->n.flags & CFG_NODE_ENABLED_MASK) {
                                    mci->nodes[nnodes].node_id = ep->n.node_id;
                                    mci->nodes[nnodes].bsp_id = ep->n.apic_id;
                                    mci->nodes[nnodes].local_mp =
                                                                ep->n.table_ptr;
                                    if (ep->n.flags & CFG_NODE_CONSOLE_MASK)
                                          mci->nodes[nnodes].console = MS_TRUE;
                                    else
                                          mci->nodes[nnodes].console = MS_FALSE;

                                    if (ep->n.flags & CFG_NODE_BSN_MASK)
                                          mci->nodes[nnodes].bsn = MS_TRUE;
                                    else
                                          mci->nodes[nnodes].bsn = MS_FALSE;

				    nnodes++;
				}
				break;

			case CFG_ET_MC_BRIDGE:
				mci->mc_bridges[nbridges].node_id = 
							ep->br.node_id;
				mci->mc_bridges[nbridges].bridge_id = 
							ep->br.bridge_id;
				nbridges++;
				break;

			case CFG_ET_MC_ADDR:
                                mci->mem_spaces[nmem_spaces].node_id =
                                                        ep->ad.node_id;
                                mci->mem_spaces[nmem_spaces].resource_type =
                                                        ep->ad.resource_type;
                                mci->mem_spaces[nmem_spaces].resource_id =
                                                        ep->ad.resource_id;
                                mci->mem_spaces[nmem_spaces].memory_type =
                                                        ep->ad.mem_type;
                                mci->mem_spaces[nmem_spaces].flags =
                                                        ep->ad.flags;
                                mci->mem_spaces[nmem_spaces].global_addr =
                                                        ep->ad.global_addr;
                                mci->mem_spaces[nmem_spaces].local_addr =
                                                        ep->ad.local_addr;
                                mci->mem_spaces[nmem_spaces].size =
                                                        ep->ad.addr_size;
				nmem_spaces++;
				break;
		}
                ep = (union mpcentry *)((char *)ep + ep->bytes[1]);
                type = ep->bytes[0];
	}

        os_physmap_free((void *)vmc, table_size);

	return (MS_TRUE);
}

/*
 * ms_bool_t
 * mp_table_chk(struct mpconfig *mp)
 *
 *	Check the MP config table to ensure it's header contains the
 *	right signature, it's size is within bounds, and it's checksum
 *	is correct.
 *
 * Calling/Exit State:
 *	Return MS_FALSE to the caller is anything's wrong with the table.
 */
STATIC
ms_bool_t
mp_table_chk(struct mpconfig *mp)
{
	unsigned int i, n;
	char *cp;
	unsigned char sum;

	if (mp->hdr.sign[0] != 'P' || mp->hdr.sign[1] != 'C' ||
	    mp->hdr.sign[2] != 'M' || mp->hdr.sign[3] != 'P')
		return MS_FALSE;

	/* check table length */
	n = mp->hdr.tbl_len;
	if (n < sizeof(struct mpchdr) || n > 0x1000)
		return MS_FALSE;

	/* calculate checksum */
	sum = 0;
	cp = (char *)mp;
	for (i = 0; i < n; i++)
		sum += *cp++;
	if (sum != 0) 
		return MS_FALSE;
	return MS_TRUE;
}

/*
 * ms_bool_t
 * mc_table_chk(struct mcconfig *mc)
 *
 *	Check the MC config table to ensure it's header contains the
 *	right signature, it's size is within bounds, and it's checksum
 *	is correct.
 *
 * Calling/Exit State:
 *	Return MS_FALSE to the caller is anything's wrong with the table.
 */
STATIC
ms_bool_t
mc_table_chk(struct mcconfig *mc)
{
	unsigned int i, n;
	unsigned long *cp;
	unsigned long sum;

	if (mc->hdr.sign[0] != 'S' || mc->hdr.sign[1] != 'M' ||
	    mc->hdr.sign[2] != 'M' || mc->hdr.sign[3] != 'C')
		return MS_FALSE;

	/* check table length */
	n = mc->hdr.tbl_len;
	if (n < sizeof(struct mcchdr))
		return MS_FALSE;

	/* calculate checksum */
	sum = 0;
	cp = (unsigned long *)mc;
	for (i = 0; i < n / 4; i++)
		sum += *cp++;
	if (sum != 0) 
		return MS_FALSE;
	return MS_TRUE;
}

/*
 * void
 * cfgtable_pci(struct mp_info *mpi, msr_bus_t *busp, int first_slot)
 *
 *	Encode PCI interrupts listed in an MP configuration table.  
 *	
 * Calling/Exit State:
 *	
 */
void
cfgtable_pci(struct mp_info *mpip, msr_bus_t *busp, int first_slot)
{
        unsigned int num_intrs, ioapic_num, i;
        msr_routing_t *rpp;
        struct intr_info *intrip;

        num_intrs = 0;
        intrip = mpip->intrs;
        for (i = 0; i < mpip->num_intrs; i++, intrip++) {
                if (intrip->bus_type == CFG_BUS_PCI)
                        num_intrs++;
        }

	busp->msr_n_routing = num_intrs;
        busp->msr_intr_routing = 
		(msr_routing_t *)os_alloc(num_intrs * sizeof(msr_routing_t));
        rpp = busp->msr_intr_routing;
        intrip = mpip->intrs;
        for (i = 0; i < mpip->num_intrs; i++, intrip++) {
                if (intrip->bus_type == CFG_BUS_PCI) {
                        ioapic_num = intrip->ioapic_num;
                        rpp->msr_islot = (ioapic_num * APIC_NIRQ) + 
					 intrip->line + first_slot;
                        rpp->msr_isource =
                                      (intrip->bus_id << 7) | intrip->irq;
                        rpp++;
                }
        }
}
	
/*
 * ms_bool_t
 * cfgtable_eisa(struct mp_info *mpi)
 *
 *	Check to see if IRQ 13 from an EISA bus is connected to an APIC.
 *	
 * Calling/Exit State:
 *	Return a flag indicating whether IRQ 13 is connected.
 */
ms_bool_t
cfgtable_eisa(struct mp_info *mpip)
{
        unsigned int i;
        struct intr_info *intrip;

        intrip = mpip->intrs;
        for (i = 0; i < mpip->num_intrs; i++, intrip++) {
                if (intrip->bus_type == CFG_BUS_EISA && intrip->irq == 13)
                        return (MS_TRUE);
        }

	return (MS_FALSE);
}
	
