#ident	"@(#)kern-i386at:svc/cbus.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef _SYS_MP_CBUS_H
#define _SYS_MP_COROLLARY_H
#ifdef CISCCS
#ident "@(#)kern-atmp-coro:svc/cbus.h	1.2"
#endif
#ifdef __STDC__
#pragma comment(exestr, "@(#) cbus.h 23.2 91/03/03 ")
#else
#ident "@(#)kern-atmp-coro:svc/cbus.h	1.2"
#endif

/*
 *	Corollary cbus architecture specific system defines
 */
#define BUSIO	MB(64)

#define SLOTID2CIO(slotid, a)	(((ulong)(slotid) << 18) | (ulong)(a) | 0x1000000)

#define coutb(addr, c)	(*((unsigned char *) phystokv(addr) + BUSIO) = c)

#define CONTEND_CPUID	0x0
#define ALL_CPUID	0xf

/*
 *  C-Bus I/O space addresses
 */
#define CI_CRESET	0x00		/* clear reset */
#define CI_SRESET	0x10		/* set reset */
#define	CI_CONTEND	0x20		/* contend during arbitration */
#define	CI_SETIDA	0x30		/* set id during arbitration */
#define CI_CSWI		0x40		/* clear software interrupt */
#define CI_SSWI		0x50		/* set software interrupt */
#define CI_CNMI		0x60		/* clear NMI */
#define CI_SNMI		0x70		/* set NMI */
#define CI_CLED		0x90		/* clear LED */
#define CI_SLED		0x80		/* set LED */

/*
 *  AT Bus I/O space addresses
 */

#define AT_ARBVALUE		0xf1		/* arbitration value register */

/*
 * support for the Corollary cbus architecture
 */
#define ATMB		16		/* 16MB AT bus address space */

#define MAXRAMBOARDS	4		/* max # of cbus memory cards */
#define MAXSLOTS	16		/* max # of slots in cbus */

#define RRD_RAM		0xE0000		/* address of configuration passed */

#define CACHE_LINE	16		/* size of cache line */
#define CACHE_SHIFT	4		/* shift value for above */
#define FLUSH386	0xFFFF		/* 64K for 386 cbus cache flush */
#define FLUSH486	0x3FFFF		/* 256K for 486 cbus cache flush */

#define SMP_SCBASE	0x80000
#define SMP_SCLEN	0x41000
#define SMP_SCIO	0x40000		/* offset from SMP_SCBASE */

#define CBUSMEM		MB(64)

#define CBUSIO		MB(128)

#define LOWCPUID	0x1
#define	B_ID		0xE
#define HICPUID		0xF

#define	B_STATREG	0xfdf800L
#define ATB_STATREG	0xf1
#define	BS_ARBVALUE	0xf

#define CBUS_STARTVEC	0x7fffff0

/*
 *	Corollary cbus bridge window mapping
 */
#define	B_WINDOWBASE	0xf00000L
#define	B_MAPBASE	0xfdf000L
#define	B_NMAPS		223
#define	B_MAPSIZE	(B_NMAPS * sizeof(short))
#define	B_WINDOWSZ	0x1000L

#define CADDR2MAP(caddr) ((caddr) >> 12)
#define MAPOFFSET(caddr) ((caddr) & (~B_WINDOWSZ))

#define EDAC_EN		0x10

extern int 			(*cbus_init_table[])();

#if defined(__cplusplus)
	}
#endif

#endif /* _SYS_MP_CBUS_H */
