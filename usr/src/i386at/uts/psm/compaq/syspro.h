/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/syspro.h	1.6.4.1"
#ident	"$Header$"


#define	CPQ_ASYMINTR	0x01		/* asymmetric i/o */
#define SP_MAXNUMCPU	2		/* max CPU # */

/*
 * Systempro Compatible Definitions
 */

/*
 * Bits of Processor control port (0x0c6a and 0xfc6a)
 *
 * Processor Control and Status Ports for Systempro compatible mode are 
 * are defined in compaq.cf/Space.c file.
 */
#define SP_RESET	0x01		/* reset processor */
#define SP_FPU387PRES	0x02		/* 387 is present */
#define SP_CACHEON	0x04		/* enable caching */
#define SP_PHOLD	0x08		/* put processor in HOLD */
#define SP_CACHEFLUSH	0x10		/* flush processor cache */
#define SP_FPU387ERR	0x20		/* 387 floating point unit error */
#define SP_PINT		0x40		/* cause an interrupt on this cpu */
#define SP_INTDIS	0x80		/* disable interrupt on this cpu */

#define SP_WHOAMI_PORT	0x0C70          /* WHO-AM-I port */

/*
 * Extended IRQ13 Control and Status Port.
 *
 * The IRQ13 interrupt level is shared by the numerric coprocessor 
 * error interrupt, DMA chaining, and correctable error interrupt.
 * This register is active in both the Compaq Systempro compatible
 * programming mode and the symmetric mode.
 */
#define	SP_INT13_XSTATUS_PORT	0x0CC9

/*
 * Bit description of extended irq13 control and status port (0xcc9)
 */
#define	SP_INT13_NCP_ERROR_ACTIVE  0x01	/* numeric coprocessor error intr */
#define	SP_INT13_NCP_ENABLE	   0x02	/* status -- active and enabled */
#define	SP_INT13_DMA_CHAIN_ACTIVE  0x04	/* DMA chaning interrupt status -- */
#define	SP_INT13_DMA_CHAIN_ENABLE  0x08	/* active and enabled */
#define	SP_INT13_ECC_MEMERR_ACTIVE 0x10	/* ECC correctable memory error intr */
#define	SP_INT13_ECC_MEMERR_ENABLE 0x20	/* status -- active and enabled */

#define SP_RAM_RELOC_REG	0x80C00000	/* RAM Relocation Register */
#define SP_ENG1_CACHE_REG	0x80C00002	/* P1 Cache Control Register */
