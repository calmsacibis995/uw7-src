#ifndef _PSM_CBUS_CBUS_H
#define _PSM_CBUS_CBUS_H

#ident	"@(#)kern-i386at:psm/cbus/cbus.h	1.2.2.1"
#ident	"$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Header Name:

    cbus.h

Abstract:

    this module provides the C-bus hardware architecture definitions.

--*/

//
// general type defines.
//
typedef	char					CHAR;
typedef unsigned char			UCHAR;
typedef char *					PCHAR;
typedef unsigned char *			PUCHAR;
typedef short					SHORT;
typedef unsigned short			USHORT;
typedef unsigned short *		PUSHORT;
typedef long					LONG;
typedef unsigned long			ULONG;
typedef unsigned long *			PULONG;

#define NULL					0

#define MIN(a, b)				((a) < (b) ? (a) : (b))

//
// general AT-bus defines.
//
#define ELCR_MASK				0xDEF8	// ELCR mask bits:
										// mask for valid bits of edge/level
										// control register (ELCR) in 82357 ISP:
										// ie: ensure irqlines 0, 1, 2, 8 and 13
										// are always marked edge, as the I/O
										// register does not have them set
										// correctly.  all other bits in the
										// I/O register will be valid without us
										// having to set them.

#define PIC1_ELCR_PORT			0x4D0	// ISP edge/level control register lo
#define PIC2_ELCR_PORT			0x4D1	// ISP edge/level control register hi

#define	CMOS_ADDR				0x70

#define	MULTI_IO_PORT			0x61
#define	MEMORY_PARITY_BIT		0x80	// for use with the MULTI_IO_PORT
#define	IO_CHANNEL_CHECK_BIT	0x40	// for use with the MULTI_IO_PORT

//
// general PSM defines.
//
#define CBUS_MAXIRQS			16		// max # of interrupt lines
#define CBUS_IDT_VECTORS		256
#define CBUS_FIRST_VECTOR		0x20
#define CBUS_LAST_VECTOR		(CBUS_FIRST_VECTOR + CBUS_MAXIRQS - 1)
#define CBUS_TIMER_VECTOR		(CBUS_FIRST_VECTOR + CBUS_MAXIRQS)
#define CBUS_TIMER_SLOT			0
#define	CBUS_CASC_SLOT			2
#define CBUS_NMIFLT				2
#define CBUS_NANOSECONDS_IN_SECOND	(1000000000)
#define	CBUS_RESET_VECTOR		0x467

typedef struct _cbus_msop_rawtime_t {
	volatile ULONG			Lock;
	volatile ms_rawtime_t	Time;
} CBUS_MSOP_RAWTIME_T, *PCBUS_MSOP_RAWTIME;

typedef struct _cbus_msop_cpu_info {
	volatile unsigned int	EventFlags;
	volatile int			IdleFlag;
	UCHAR					CpuOnLine;
	UCHAR					Unused[3];
} CBUS_MSOP_CPU_INFO_T, *PCBUS_MSOP_CPU_INFO;

//
// general C-bus defines.
//
#define CBUS_MAXCPUS			14
#define CBUS_ATMB				16		// 16MB AT bus address space
#define CBUS_MAXRAMBOARDS		4		// max # of cbus memory cards
#define CBUS_MAXSLOTS			16		// max # of slots in cbus
#define CBUS_RRD_RAM			0xE0000	// address of configuration
#define CBUS_RRD_RAM_SIZE		0x8000	// size of configuration
#define CBUS_CACHE_LINE			16		// size of cache line
#define CBUS_CACHE_SHIFT		4		// shift value for above
#define CBUS_CACHE_SIZE			0x100000// 1 Mb caches
#define CBUS_FLUSH386			0xFFFF	// 64K for 386 cbus cache flush
#define CBUS_FLUSH486			0x3FFFF	// 256K for 486 cbus cache flush
#define CBUS_FLUSH_ALL			0xFFFFF	// 1MB works for all C-bus
#define CBUS_SCBASE				0x80000
#define CBUS_SCLEN				0x41000
#define CBUS_SCIO				0x40000	// offset from SMP_SCBASE
#define CBUS_MAX_IDS			64
#define CBUS_CONTEND_CPUID		0x0
#define CBUS_LOWCPUID			0x1
#define CBUS_HICPUID			0xF
#define CBUS_ALL_CPUID			0xF
#define	CBUS_BRIDGE_ID			0xE
#define	CBUS_BRIDGE_STATREG		0xfdf800L
#define CBUS_ATB_STATREG		0xf1
#define	CBUS_BS_ARBVALUE		0xf
#define CBUS_STARTVEC			0x7fffff0
#define CBUS_MB(n)				(1024L * 1024L * (ULONG)(n))
#define CBUS_BUSIO				CBUS_MB(64)
#define CBUS_MEM				CBUS_MB(64)
#define CBUS_IO					CBUS_MB(128)
#define CBUS_SLOTID2CIO(id, a)	(((ULONG)(id)<<18)|(ULONG)(a)|0x1000000)
#define CBUS_COUTB(addr, reg, value) (((PUCHAR)(addr))[reg] = (UCHAR)value)

//
// C-bus I/O space addresses.
//
#define CBUS_CRESET				0x00	// clear reset
#define CBUS_SRESET				0x10	// set reset
#define	CBUS_CONTEND			0x20	// contend during arbitration
#define	CBUS_SETIDA				0x30	// set id during arbitration
#define CBUS_CSWI				0x40	// clear software interrupt
#define CBUS_SSWI				0x50	// set software interrupt
#define CBUS_CNMI				0x60	// clear NMI
#define CBUS_SNMI				0x70	// set NMI
#define CBUS_SLED				0x80	// set LED
#define CBUS_CLED				0x90	// clear LED

#ifdef CBUS_I
//
// C-bus bridge window mapping.
//
#define	CBUS_BRIDGE_WINDOWBASE	0xf00000L
#define	CBUS_BRIDGE_MAPBASE		0xfdf000L
#define	CBUS_BRIDGE_NMAPS		223
#define	CBUS_BRIDGE_MAPSIZE		(B_NMAPS * sizeof(USHORT))
#define	CBUS_BRIDGE_WINDOWSZ	0x1000L
#define CBUS_CADDR2MAP(caddr)	((caddr) >> 12)
#define CBUS_MAPOFFSET(caddr)	((caddr) & (~B_WINDOWSZ))

// 
// C-bus shadow register.
//
#define CBUS_SHADOW_REGISTER	0xB0	// offset of shadow register
#define CBUS_DISABLE_SHADOWING	0x0		// disable ROM BIOS shadowing

//
// defines for the C-bus ECC control register.
//
#define	CBUS_EDAC_SAEN			0x80
#define CBUS_EDAC_1MB			0x40
#define	CBUS_EDAC_WDIS			0x20
#define	CBUS_EDAC_EN			0x10
#define	CBUS_EDAC_SAMASK		0x0f

#endif // CBUS_I

#endif // _PSM_CBUS_CBUS_H
