#ident	"@(#)kern-i386at:svc/eisa.c	1.4.2.1"
#ident	"$Header$"

/*
 * EISA bus utility routines.
 */

#include <mem/kmem.h>
#include <svc/eisa.h>
#include <svc/errno.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

#include <io/f_ddi.h>


/*
 * int
 * eisa_verify(void)
 *      Check if eisa machine.
 *
 * Calling/Exit State:
 *	Return -1 if NOT on an EISA machine, otherwise return 0.
 */
int
eisa_verify(void)
{
	char    *vaddr;
	char    *eisa_ptr = EISA_STRING;
	int     i;

	vaddr = (char *)physmap((paddr_t)EISA_STRING_ADDRESS, 4, KM_SLEEP);
	for (i = 0; i < 4; i++) {
		if (*vaddr++ != *eisa_ptr++) {
			physmap_free(vaddr, 4, 0);
			return(-1);
		}
	}

	physmap_free(vaddr, 4, 0);
	return(0);
}


#define NMI_EXTPORT	0x461	/* Extended NMI register */
#define	BUSTCHK		0x40	/* Bus timeout NMI CK */
#define BUSTCHK_ENABLE	0x08	/* Enable 32-bit bus timeout NMI interrupt */	

/*
 * int
 * eisa_nmi_bus_timeout(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Clear the Bus Timeout flip/flop. 
 *
 *	Extended NMI Status and Control port (0x461) bit <6> is 
 *	set (BUS TIMEOUT) if more than 64 BCLKs (8us) have elapsed
 *	from the rising edge. This interrupt is enabled by setting
 *	port 0x461 bit <3> to "1". To reset the bus timeout interrupt,
 *	port 0x461 bit <3> is set to "0" (Disable Bus Timeout NMI) 
 *	and then is set to "1" (Enable Bus Timeout Interrupt). 
 */
int
eisa_nmi_bus_timeout(void)
{
	int	stat;	

	if ((stat = inb(NMI_EXTPORT)) & BUSTCHK) {
		outb(NMI_EXTPORT, inb(NMI_EXTPORT) & ~BUSTCHK_ENABLE);
		outb(NMI_EXTPORT, inb(NMI_EXTPORT) | BUSTCHK_ENABLE);
		/* wait for ten microsec */
		ms_time_spin(10);
		/* Fatal Bus Timeout NMI error. */
		return (NMI_BUS_TIMEOUT);
	}

	return (NMI_UNKNOWN);
}


extern int sanity_clk;

int sanity_ctl	 = SANITY_CTL;		/* EISA sanity ctl word */
int sanity_ctr0	 = SANITY_CTR0;		/* EISA sanity timer */
int sanity_port	 = SANITY_CHECK;	/* EISA sanity enable/disable port */
int sanity_mode = PIT_C0|PIT_ENDSIGMODE|PIT_READMODE;
int sanity_enable = ENABLE_SANITY;
int sanity_reset = RESET_SANITY;
int sanity_latch = PIT_C0|PIT_ENDSIGMODE;
unsigned int sanitynum = SANITY_NUM;	/* interrupt interval for sanitytimer */

/*
 * void
 * eisa_sanity_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
eisa_sanity_init(void)
{
 	uchar_t byte;
	int	efl;

	/*
         * Now set sanity timer if EISA machine and tunable on 
	 */
	if (sanity_clk) {
		/* disable interrupts */
		efl = intr_disable();
		outb(sanity_ctl, sanity_mode);
		byte = sanitynum;
		outb(sanity_ctr0, byte);
		byte = sanitynum >> 8;
		outb(sanity_ctr0, byte);
		outb(sanity_port, sanity_reset);
		outb(sanity_port, sanity_enable);
		/* restore interrupt state */
		intr_restore(efl);
		/* Register the fail-safe sanity timer NMI handler. */
		drv_callback(NMI_ATTACH, eisa_sanity_check, NULL);
	}
}


int system_reboot = 0;		/* flag to cause reboot by second sanity loop */

/*
 * int 
 * eisa_sanity_check(void)
 *	Check if sanity timer caused NMI by latching and then reading ctr0.
 *
 * Calling/Exit State:
 *	Called from the NMI handler.
 *
 * Remarks:
 *	Port 0x461 bit <7> is set (FAILSAFE_NMI) if the fail-safe
 *	timer count has expired before being reset by a software
 *	routine. This interrupt is enabled by setting port 0x461
 *	bit <2> to "1". To reset the interrupt port 0x461 bit <2>
 *	is set "0" (Disable Failsafe Interrupt) and then set it
 *	to "1" (Enable Failsafe Interrupt). 
 *					-- EISA TRM.
 */
int
eisa_sanity_check(void)
{
        uchar_t byte;
        uint_t  leftover;

	if (sanity_clk) {
		byte = inb(sanity_port);
		cmn_err(CE_NOTE, "!sanity NMI byte = %x", byte);
		if (byte & FAILSAFE_NMI)  {
			outb(sanity_ctl, sanity_latch);
			outb(sanity_ctl, sanity_mode);
			byte = inb(sanity_ctr0);
			leftover = inb(sanity_ctr0);
			leftover = leftover << 8 + byte;
			cmn_err(CE_NOTE, "!sanity count = %x", leftover);
			if (system_reboot) {
				outb(sanity_port, sanity_reset);
				return (NMI_REBOOT);
			} else {
				system_reboot++;
				outb(sanity_ctl, sanity_mode);
				byte = sanitynum;
				outb(sanity_ctr0, byte);
				byte = sanitynum >> 8;
				outb(sanity_ctr0, byte);
				outb(sanity_port, sanity_reset);
				outb(sanity_port, sanity_enable);
				cmn_err(CE_NOTE, "!Sanity Timer went off"); 
				return (NMI_BENIGN);
			}
		}
	}

	return (NMI_UNKNOWN);
}


/*
 * void
 * eisa_sanity_halt(void)
 *	Turn off sanity timer.
 *
 * Calling/Exit State:
 *	Called from the NMI handler.
 *
 * Remarks:
 *	If sanity timer is in use, turn off to allow a clean soft reboot.
 */
void
eisa_sanity_halt(void)
{
	if (sanity_clk) {
		outb(SANITY_CHECK, RESET_SANITY);
		/* Unregister the fail-safe sanity timer NMI handler. */
		drv_callback(NMI_DETACH, eisa_sanity_check, NULL);
	}
}


/*
 * void
 * set_elt(int irq, int mode)
 *	Sets mode (edge- or level-triggered) for a given interrupt line.
 *
 * Calling/Exit State:
 *	On entry, irq is the interrupt line whose mode is being set
 *	and mode is the desired mode (either EDGE_TRIG or LEVEL_TRIG).
 *
 * Remarks:
 *	IRQ's 0, 1, 2, 8 and 13 are always set to EDGE_TRIG regardless of
 *	the mode argument passed in.
 */
void
eisa_set_elt(int irq, int mode)
{
	int i, port;

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

	i = inb(port);
	i &= ~(1 << irq);
	i |= (mode << irq);
	outb(port, i);

	return;
}
