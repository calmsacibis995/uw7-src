#ifndef _SVC_PIC_H	/* wrapper symbol for kernel use */
#define _SVC_PIC_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/pic.h	1.6"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif


/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Definitions for 8259 Programmable Interrupt Controller */

#define PIC_NEEDICW4    0x01            /* ICW4 needed */
#define PIC_ICW1BASE    0x10            /* base for ICW1 */
#define PIC_86MODE      0x01            /* MCS 86 mode */
#define PIC_AUTOEOI     0x02            /* do auto eoi's */
#define PIC_SLAVEBUF    0x08            /* put slave in buffered mode */
#define PIC_MASTERBUF   0x0C            /* put master in bnuffered mode */
#define PIC_SPFMODE     0x10            /* special fully nested mode */
#define PIC_READISR     0x0B            /* Read the ISR */
#define PIC_NSEOI       0x20            /* Non-specific EOI command */

#define PIC_VECTBASE    0x40            /* Vectors for external interrupts */
					/* start at 64.                    */


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Structure which maintains information about IRQ lines.  A table
 *	of these structures is initialized by picinit and is referenced
 *	during interrupt handling.  The use of this table helps to reduce
 *	computations during interrupt handling, thus speeding up the
 *	interrupt path.
 */
struct irqtab
	{
	ushort_t irq_cmdport;	/* PIC command port I/O address */
	uchar_t	irq_flags;	/* flags - see below */
	};

/*
 * flags for irqtab
 */
#define	IRQ_CHKSPUR		0x01	/* could be a spurious interrupt */
#define	IRQ_ONSLAVE		0x02	/* comes from a slave PIC */

#endif

#define	PIC_NIRQ		8	/* number of IRQ lines per pic */
#define	PIC_IRQSPUR		0x80	/* IRQ for spurious interrupts */

/*
 * Interrupt configuration information specific to a particular computer.
 * These constants are used to initialize tables in modules/pic/space.c.
 * NOTE: The master pic must always be pic zero.
 */




#define NPIC    2                       /* 2 PICs */
/* Port addresses */
#define MCMD_PORT       0x20            /* master command port */
#define MIMR_PORT       0x21            /* master intr mask register port */
#define SCMD_PORT       0xA0            /* slave command port */
#define SIMR_PORT       0xA1            /* slave intr mask register port */
#define MASTERLINE      0x02            /* slave on IR2 of master PIC */
#define SLAVEBASE       8               /* slave IR0 interrupt number */
#define PICBUFFERED     0               /* PICs not in buffered mode */
#define I82380          0               /* i82380 chip not used */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_PIC_H */
