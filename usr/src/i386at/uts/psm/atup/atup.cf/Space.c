#ident	"@(#)kern-i386at:psm/atup/atup.cf/Space.c	1.1.1.3"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/ipl.h>
#include <sys/pic.h>


/* Initialized data for Programmable Interupt Controllers */

ushort_t cmdport[NPIC]		/* command port addrs for pics */
	= { MCMD_PORT, SCMD_PORT };

ushort_t imrport[NPIC]		/* intr mask port addrs for pics */
	= { MIMR_PORT, SIMR_PORT };

uchar_t masterpic[NPIC]		/* index of this pic's master (for 82380) */
	= { 0, 0 };

uchar_t masterline[NPIC]	/* line on master this slave connected to */
	= { 0, MASTERLINE };

uchar_t curmask[NPIC];		/* current pic masks */

uchar_t iplmask[(PLHI + 1) * NPIC];	/* pic masks for intr priority levels */

uchar_t picbuffered = PICBUFFERED;	/* PICs in buffered mode */

int npic = NPIC;		/* number of pics configured */

/*
 * pseudo irq for soft int
 */
int psm_nirq = NPIC * PIC_NIRQ;

/*
 * service priority level for each interrupt,
 * plus one for irq_softint pseudo interrupt
 */
pl_t svcpri[NPIC * PIC_NIRQ + 1];

struct irqtab irqtab[NPIC * PIC_NIRQ];	/* table of per IRQ information */
