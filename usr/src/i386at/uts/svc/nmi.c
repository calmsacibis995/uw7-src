#ident	"@(#)kern-i386at:svc/nmi.c	1.4.1.1"
#ident	"$Header$"

/*
 * Machine-dependent Non-Maskable Interrupt (NMI) handler.
 */

#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <proc/disp.h>
#include <proc/regset.h>
#include <proc/user.h>
#include <svc/eisa.h>
#include <svc/errno.h>
#include <svc/pit.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <util/param_p.h>
#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>

#include <io/f_ddi.h>
#include <io/ddi_i386at.h>

#define PORT_B		0x61	/* System Port B */
#define IOCHK_DISABLE	0x08	/* Disable I/O CH CK */
#define PCHK_DISABLE	0x04	/* Disable motherboard parity check */
#define IOCHK		0x40	/* I/O CH CK */
#define PCHK		0x80	/* Motherboard parity check */

#define NMI_EXTPORT	0x461	/* Extended NMI register */
#define	BUSTCHK		0x40	/* Bus timeout NMI CK */
#define BUSTCHK_ENABLE	0x08	/* Enable 32-bit bus timeout NMI interrupt */	

#define CMOS_PORT	0x70	/* CMOS Port */
#define NMI_ENABLE	0x0F	/* Enable NMI interrupt */
#define NMI_DISABLE	0x8F	/* Disable NMI interrupt */


/*
 * NMI handlers.
 */
paddr_t	nmi_addr;		/* address at which parity error occurred */

extern void _nmi_hook(int *);


/*
 * void
 * nmi_init(void)
 *	Handle an NMI interrupt.
 * 
 * Calling/Exit State:
 *	None.
 */
nmi_init(void)
{
	if (nmi_handler == NULL)
		nmi_handler = _nmi_hook;

        if (eisa_verify() == 0)
        	drv_callback(NMI_ATTACH, eisa_nmi_bus_timeout, NULL);

}


/*
 * void
 * nmi(...)
 *	Handle an NMI interrupt.
 * 
 * Calling/Exit State:
 *	The arguments are the saved registers which will be restored
 *	on return from this routine.
 *
 * Description:
 *      Currently, NMIs are presented to the processor in these situations:
 *
 *		- [Software NMI] 
 *		- [Access error from access to reserved processor
 *			LOCAL address]
 *		- Access error:
 *			- Bus Timeout
 *			- ECC Uncorrectable error
 *			- Parity error from System Memory
 *			- Assertion of IOCHK# (only expansion board assert this)
 *			- Fail-safe Timer Timeout
 *		- [Cache Parity error (these hold the processor & freeze
 *                                      the bus)]
 */
/* ARGSUSED */
void
nmi(volatile uint edi, volatile uint esi, volatile uint ebp,
    volatile uint unused, volatile uint ebx, volatile uint edx,
    volatile uint ecx, volatile uint eax, volatile uint es, volatile uint ds,
    volatile uint eip, volatile uint cs, volatile uint flags, volatile uint sp,
    volatile uint ss)
{
	if (nmi_handler == NULL)
		nmi_handler = _nmi_hook;
	(*nmi_handler)((int *)&eax);
}


/*
 * void 
 * _nmi_catch(int *r0ptr)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Latch the physical address at which the parity error occurred.
 */
void
_nmi_catch(int *r0ptr)
{
	nmi_addr = kvtophys(r0ptr[T_ESI]);
}
