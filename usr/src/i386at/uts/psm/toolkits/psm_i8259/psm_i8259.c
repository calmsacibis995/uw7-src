#ident	"@(#)kern-i386at:psm/toolkits/psm_i8259/psm_i8259.c	1.1.2.3"
#ident  "$Header$"

/*
 * The i8259 toolkit supports the Intel i8259 family of Priority Interrrupt    
 * Controllers (PICs).  This toolkit includes the following routines:
 *
 *  	1) A routine to initialize the i8259 machinery. 
 *  	2) A routine to mask off (prevent from interrupting) interrupt requests
 *		coming in on the islot request line of the PIC(s).
 *	3) A routine to unmask (allow to interrupt) interrupt requests
 *              coming in on the islot request line of the PIC(s).
 *	4) A routine to check whether an interrupt on the indicated vector was
 *		spurious.
 *	5) A routine to de-initialize the i8259 machinery.
 *
 */

#include <svc/psm.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>



/*
 * void
 * i8259_init(unsigned int bustype)
 *      Initialize the i8259 machinery.
 *
 * Calling/Exit State:
 *
 */
void
i8259_init(unsigned int bustype)
{
        /*
         * Initialize the interrupt controller by setting
         * appropriate values in the ICWs and OCWs for both
         * master and slave PICs.
         */
	ms_bool_t combined = MS_FALSE;

	/* 
 	 * The following code was added to detect if we are on an IBM PC 
	 * Server 310, or IBM PC750.  These machines have an interesting 
	 * bus setup which includes the combination of ISA/PCI/MCA.  
	 * In order for UnixWare to work with these machines, we must 
	 * identify the machine so that  interrupts are set up properly.
 	 * Changes have been made to the following files:
 	 *
 	 *      standalone/boot/at386/mip/mc386.c
 	 *      standalone/boot/at386/mip/mip.c
 	 *      uts/io/fd/fd.c
 	 *      uts/svc/bootinfo.h
 	 *      uts/psm/toolkits/psm_i8259/psm_i8259.c
 	 *
 	 * to support these systems.  The fix, as you can see, is comprised of 
 	 * the mip setting a bit (BUS_BRIDGED) in the "machflags" member of 
 	 * the "bootinfo" structure.   This bit indicates we are on the system 
 	 * with the MCA/ISA bus combination.  IBM feels the detection of the 
 	 * system via the mechanism in mc386.c is reliable, and should not 
 	 * conflict with other MCA systems.
 	 * 
 	 * The effected drivers (atup, and fd) check that this bit is set 
 	 * before doing the MCA specific initialization.   If the bit 
 	 * is set, these drivers behave as if their components are on 
 	 * an ISA bus.   Note that the changes to "fd" introduce the supposedly 
 	 * opaque "bootinfo" into the driver set, and make those drivers not DDI
 	 * compliant (but remember, this is a quick fix, and should ultimately
 	 * be replaced with a better solution).
	 *
	 * In PSMv2 BUS_BRIDGED MCA systems are recognized as
	 * (MSR_BUS_MCA && !(MSR_BUS_ISA)) bus type.
 	 */
#define	PIC_LTIM	0x08


	if ((bustype & (1 << MSR_BUS_MCA)) &&
	    !(bustype & (1 << MSR_BUS_ISA)))
		combined = MS_TRUE;

        outb(I8259_IC_1+I8259_ICW1, combined ? I8259_MASTER_ICW1 | PIC_LTIM :
		I8259_MASTER_ICW1);
        outb(I8259_IC_1+I8259_ICW2,I8259_MASTER_ICW2);
        outb(I8259_IC_1+I8259_ICW3,I8259_MASTER_ICW3);
        outb(I8259_IC_1+I8259_ICW4,I8259_MASTER_ICW4);
        outb(I8259_IC_1+I8259_OCW1,I8259_MASTER_OCW1);
        outb(I8259_IC_1+I8259_OCW3,I8259_MASTER_OCW3);
        outb(I8259_IC_2+I8259_ICW1, combined ? I8259_SLAVE_ICW1 | PIC_LTIM :
	     I8259_SLAVE_ICW1);
        outb(I8259_IC_2+I8259_ICW2,I8259_SLAVE_ICW2);
        outb(I8259_IC_2+I8259_ICW3,I8259_SLAVE_ICW3);
        outb(I8259_IC_2+I8259_ICW4,I8259_SLAVE_ICW4);
        outb(I8259_IC_2+I8259_OCW1,I8259_MASTER_OCW1);
        outb(I8259_IC_2+I8259_OCW3,I8259_SLAVE_OCW3);

        /*
         * Enable for the cascade interrupt from the second PIC.
         */

        outb(I8259_IC_1+I8259_OCW1,0xfb);
}


/*
 * void
 * i8259_intr_mask(ms_islot_t islot)
 *	Mask off (prevent from interrupting) interrupt requests coming in on
 * 	the islot request line of the PIC(s).
 *
 * Calling/Exit State:
 *
 */

void
i8259_intr_mask(ms_islot_t islot)
{
        int pic, line, mask;

        pic = islot / I8259_NIRQ;
        line = islot % I8259_NIRQ;

        if ( pic == 0 ) {
            mask = inb(I8259_IC_1+I8259_OCW1) | (1 << line);
            outb(I8259_IC_1+I8259_OCW1, mask);
        }   else {
            mask = inb(I8259_IC_2+I8259_OCW1) | (1 << line);
            outb(I8259_IC_2+I8259_OCW1,mask);
        }
}


/*
 * void
 * i8259_intr_unmask(ms_islot_t islot)
 *	Unmask (allow to interrupt) interrupt requests coming in on
 * 	the islot request line of the PIC(s).
 *
 * Calling/Exit State:
 *
 */

void
i8259_intr_unmask(ms_islot_t islot)
{
        int pic, line, mask;

        pic = islot / I8259_NIRQ;
        line = islot % I8259_NIRQ;

        if ( pic == 0 ) {
            mask = inb(I8259_IC_1+I8259_OCW1) & ~(1 << line);
            outb(I8259_IC_1+I8259_OCW1, mask);
        }   else {
            mask = inb(I8259_IC_2+I8259_OCW1) & ~(1 << line);
            outb(I8259_IC_2+I8259_OCW1, mask);
        }
}

/*
 * void
 * i8259_intr_attach(ms_intr_dist_t *idtp, unsigned int bustype)
 *      
 *     
 *
 * Calling/Exit State:
 *
 */

void
i8259_intr_attach(ms_intr_dist_t *idtp, unsigned int bustype)
{
        int pic, line, mask;
	ms_islot_t islot;

	if (!(bustype & (1 << MSR_BUS_EISA))) return;

	islot = idtp->msi_slot;

	switch (islot) {
	case I8259_IRQ0:
	case I8259_IRQ1:
	case I8259_IRQ2:
	case I8259_IRQ8:
	case I8259_IRQ13:
		return;
	}

        pic = islot / I8259_NIRQ;
        line = islot % I8259_NIRQ;

        if(idtp->msi_flags & MSI_ITYPE_CONTINUOUS) {
        	if(pic == 0) {
			mask = inb(I8259_IRQ_SET_0_EDGE_LEVEL) | (1 << line);
			outb(I8259_IRQ_SET_0_EDGE_LEVEL, mask);
		} else {
			mask = inb(I8259_IRQ_SET_1_EDGE_LEVEL) | (1 << line);
			outb(I8259_IRQ_SET_1_EDGE_LEVEL, mask);
		}
	}
}

/*
 * ms_bool_t
 * i8259_check_spurious(ms_slot_t islot)
 *	Check to see if an interrupt on the indicated vector was spurious.
 *	
 * Calling/Exit State:
 *
 */
ms_bool_t
i8259_check_spurious(ms_islot_t islot)
{
        int	mask = 1;
        int	pic, line;

        pic = islot / I8259_NIRQ;
        line = islot % I8259_NIRQ;

        /*
         * Check to determine whether the interrupt came in on interrupt line
         * 7 (master or slave PIC).  If not, there is no need to make any 
         * further checks since this cannot be a spurious interrupt.
         */

        if (line == 7 ) {
                /*
                 * Read the Interrupt Service Register (ISR) from the PIC.
                 */

                if (pic == 0)
                        mask = inb(I8259_IC_1+I8259_OCW3) & 0x80;
                else
                        mask = inb(I8259_IC_2+I8259_OCW3) & 0x80;

        }

        if (mask) 
                return(MS_FALSE);
        else
                return(MS_TRUE);

}

/*
 * void
 * i8259_eoi(void)
 *	
 *
 * Calling/Exit State:
 *
 */
void
i8259_eoi(ms_islot_t islot)
{
        int pic;

	pic = islot / I8259_NIRQ;
        if( pic == 1 )
        	outb(I8259_IC_2,I8259_EOI);
        outb(I8259_IC_1,I8259_EOI);
}


/*
 * void
 * i8259_deinit(void)
 *	De-initialize the i8259 machinery.
 *
 * Calling/Exit State:
 *
 */
void
i8259_deinit(void)
{
	outb(I8259_IC_1+I8259_OCW1, 255);
	outb(I8259_IC_2+I8259_OCW1, 255);
}

