/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/xl.h	1.2.3.1"
#ident	"$Header$"


#define	CPQ_SYMINTR	0x02		/* symmetric interrupt */
#define XL_MAXNUMCPU	4		/* max CPU # */

/*
 * CPU Mode Select Port (0C67h)
 *	bit0-4	Reserved
 *	bit5	Mode Select
 *	bit6-7	Reserved
 */
#define	SP_XLMODE	0x20		/* 0=Systempro, 1=Systempro_xl       */

/*
 * CPU Command Port (0C6Ah) Write only
 *	bit0	SLEEP
 *	bit1	AWAKE
 *	bit2	CACHEON
 *	bit3	CACHEOFF
 *	bit4	FLUSH
 *	bit5	RESET
 *	bit6-7	Reserved
 */
#define	XL_SLEEP	0x1		/* Put processor in sleep (STOP)      */
#define	XL_START	0x2		/* Start processor (Out of sleep)     */
#define	XL_CACHEON	0x4		/* enable caching		      */
#define	XL_CACHEOFF	0x8		/* Disable and flush cache	      */
#define	XL_FLUSH	0x10		/* flush processor cache	      */
#define	XL_RESET	0x20		/* reset processor		      */

/*
 * CPU Status Port (0C6Ah) Read only
 *	bit0	Always Read 0
 *	bit1	NCPIN
 *	bit2	CACHEON
 *	bit3	SLEEP
 *	bit4	Always Read 0
 *	bit5	NCPERR
 *	bit6-7	Reserved
 */
#define XL_NCPIN	0x2		/* 1 = Numeric Coprocessor Installed  */
#define	XL_CACHE	0x4		/* 1 = caching enabled  	      */
#define	XL_SLEEPING	0x8		/* 1 = CPU is Sleeping		      */
#define	XL_NCPERR	0x20		/* Numeric Coprocessor error occurred */

/* MP INT(x) Control/Status	*/
#define	XL_INT0		0x0CB0		/* MP INT0			*/
#define	XL_INT13	0x0CC8		/* MP INT13			*/

/* MP INTx Control/Status	*/
#define	XL_PINT		0x1		/* CPU PINT Requested at level x*/
#define	XL_PINTCLR	0x2		/* Clear CPU PINT at level x	*/
#define	XL_PINTENABLE	0x4		/* Enable CPU PINT at level x	*/
#define XL_PINTDISABLE	0x8		/* Disable CPU PINT at level x	*/

/* For optimization define PINT's and CLRPINT's here */
#define XL_PINT0	0x010CB000	/* CPU PINT at IRQ0                  */
#define XL_PINT13	0x010CC800	/* CPU PINT at IRQ13                 */
#define XL_CLRPINT0	0x020CB000	/* CPU CLEAR PINT at IRQ0	     */
#define XL_CLRPINT13	0x020CC800	/* CPU CLEAR PINT at IRQ13	     */
#define XL_DISPINT0	0x0A0CB000	/* CPU CLEAR & Disable PINT at IRQ0  */
#define XL_DISPINT13	0x0A0CC800	/* CPU CLEAR & Disable PINT at IRQ13 */

/* Define PINT levels for CPU's	*/
#define	XL_P1INTR	13		/* interrupt vector for P1	 */
#define	XL_PnINTR	13		/* interrupt vector for Pn	 */

#define	BASE_PROCESSOR	0

/* Systempro_xl MP Registers */
#define	XL_MODESELECT	0x0C67		/* CPU Mode Select Port		*/
#define XL_COMMANDPORT	0x0C6A		/* CPU Command Port (write)	*/
#define XL_STATUSPORT	0x0C6A		/* CPU Status Port (read)	*/
#define XL_WHOAMIPORT	0x0C70		/* Who am I Port		*/
#define XL_INDEXCPU	0x0C74		/* Index Port			*/ 
#define XL_INDEXLOW	0x0C75		/* Index Address Register (Low)	*/ 
#define XL_INDEXHI	0x0C76		/* Index Address Register (Hi)	*/ 
#define XL_INDEXDATA	0x0C77		/* Index Data Port		*/ 
#define	XL_INDEXPORT	0x0C74		/* Index Port */
#define	XL_ASSIGNPORT	0x0C71		/* Logical CPU Assignment Port */

#define	XL_NSEC_PADDR	0x80C00030	/* free-running nanosecond counter */
#define	XL_USEC_PADDR	0x80C00032	/* free-running microsecond counter */
