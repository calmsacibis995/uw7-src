#ifndef _PSM_PSM_APIC_PSM_APIC_H     /* wrapper symbol for kernel use */
#define _PSM_PSM_APIC_PSM_APIC_H     /* subject to change without notice */

#ident	"@(#)kern-i386at:psm/toolkits/psm_apic/psm_apic.h	1.1.3.1"
#ident	"$Header$"


#if defined(_PSM)
#if _PSM != 2
#error "Unsupported PSM version"
#endif
#else
#error "Header file not valid for Core OS"
#endif

#ifndef __PSM_SYMREF

struct __PSM_dummy_st {
	int *__dummy1__;
	const struct __PSM_dummy_st *__dummy2__;
};
#define __PSM_SYMREF(symbol) \
	extern int symbol; \
	static const struct __PSM_dummy_st __dummy_PSM = \
		{ &symbol, &__dummy_PSM }

#if !defined(_PSM)
__PSM_SYMREF(No_PSM_version_defined);
#pragma weak No_PSM_version_defined
#else
#define __PSM_ver(x) _PSM_##x
#define _PSM_ver(x) __PSM_ver(x)
__PSM_SYMREF(_PSM_ver(_PSM));
#endif

#endif  /* __PSM_SYMREF */


#include <svc/psm.h>
#include <psm/toolkits/psm_cfgtables/psm_cfgtables.h>

/*
 * Definitions for Intel Advanced Programmable Interrupt Controller.
 */
#define _82489APIC_MASK 	0xF0
#define APIC_RESET_VECTOR 	0x467
#define APIC_NIRQ		16	/* number of IRQ lines per apic */

/*
 * Offsets for I/O unit registers.
 */
#define	APIC_IO_REG	0
#define APIC_IO_DATA	4

/*
 * Offset of local unit registers. Each local unit register is 32 bits wide
 * and are aligned at 128-bit (16 byte) boundaries.
 */
#define APIC_ID		(4*2)		/* local unit ID reg. */
#define APIC_VERS	(4*3)		/* local version reg. */
#define APIC_TASKPRI    (4*8)		/* local task priority reg. */
#define APIC_EOI	(4*0xb)		/* local EOI register */
#define APIC_LOGICAL	(4*0xd)		/* local logical destination reg. */
#define APIC_DESTFMT	(4*0xe)		/* local destination format reg. */
#define APIC_SPUR	(4*0xf)		/* local spurious intr. vector reg. */
#define APIC_ERROR_STATUS (4*0x28)	/* apic error status register */
#define APIC_ICMD	(4*0x30)	/* local unit intr. cmd. reg. (31:0) */
#define APIC_ICMD2	(4*0x31)	/* local unit intr. cmd. reg. (63:32) */
#define APIC_LVT_TIMER	(4*0x32)	/* local vector table (timer) */
#define APIC_LVT_I0	(4*0x35)	/* local vector table (local int 0) */
#define APIC_LVT_I1	(4*0x36)	/* local vector table (local int 1) */
#define	APIC_LVT_ERROR	(4*0x37)	/* local vector table (error) */
#define APIC_ICOUNT	(4*0x38) 	/* initial count reg. */
#define APIC_CCOUNT	(4*0x39)	/* current count reg. */
#define APIC_DIVIDECR	(4*0x3e)	/* divide configuration register */

/*
 * Select values for I/O registers. 
 */
#define APIC_AIR_ID	0
#define APIC_AIR_VERS	0x01
#define APIC_AIR_RDT	0x10
#define APIC_AIR_RDT2	0x11

/*
 * Various values for registers in I/O unit (IOU) and Local unit (LOU).
 */
#define APIC_MASK		0x10000		/* bit in RDT to mask an intr */
#define APIC_UNMASK		0xfffeffff	/* bit in RDT to unmask intr */
#define APIC_TOALL		0x7fffffff
#define APIC_LOGDEST(c)		(0x01000000 << (c)) /* log dest. if ON */
#define APIC_ALLDEST		0x7f000000	/* log dest. All cpus */
#define APIC_IM_OFF		0x80000000	/* log dest. if OFF */
#define APIC_FIXED		0		/* fixed delivery mode */
#define APIC_LOPRI		0x100		/* low priority delivery mode */
#define APIC_NMI		0x400		/* deliver the NMI interrupt */
#define APIC_RESET		0x500		/* RESET the processor */
#define APIC_STARTUP		0x600
#define APIC_EXTINT		0x700
#define APIC_PENDING		0x1000
#define APIC_PDEST		0
#define APIC_LDEST		0x800
#define APIC_POLOW		0x2000
#define APIC_POHIGH		0
#define APIC_ASSERT		0x4000
#define APIC_DEASSERT		0
#define APIC_EDGE		0
#define APIC_LEVEL		0x8000
#define APIC_XTOSELF		0x40000
#define APIC_XTOALL		0x80000
#define	APIC_LOU_ENABLE		0x100		/* enable local unit */
#define	APIC_LOU_DISABLE	0x000		/* disable local unit */
#define	APIC_XTOOTHERS		0x000C0000
#define APIC_PERIODIC		0x00020000	/* set local timer to periodic*/
#define APIC_EDGE_LEVEL_MASK 	0x8000
#define APIC_DELIV_MODE_MASK 	0x700
#define APIC_VECTOR_MASK	0xFF
#define APIC_IO_ID_SHIFT	24

#define APIC_DIVIDE_BY_2	0

#define APIC_VERS_INTEGRATED    0x10


/* 
 * Function Prototypes
 */

void    apic_local_init(struct cpu_info *);
void	apic_local_disable(volatile long *);
void    apic_io_init(struct ioapic_info *, ms_cpu_t);
void    apic_deinit(struct ioapic_info *);
void    apic_intr_set_mode(struct ioapic_info *, int, int, int, int);
void    apic_intr_mask(struct ioapic_info *, int);
void    apic_intr_unmask(struct ioapic_info *, int);
void    apic_eoi(volatile long *);
void	apic_xintr(volatile long *, unsigned long, unsigned char);
void	apic_timer_init(volatile long *, ms_time_t, unsigned int);
void	apic_reset_cpu(volatile long *, unsigned char, ms_paddr_t);
void  	apic_get_ticks(struct cpu_info *,  ms_time_t *, ms_time_t *);

#endif /* _PSM_PSM_APIC_PSM_APIC_H */

