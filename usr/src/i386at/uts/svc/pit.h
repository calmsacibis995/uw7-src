#ifndef _SVC_PIT_H	/* wrapper symbol for kernel use */
#define _SVC_PIT_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/pit.h	1.5"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */

/* Definitions for 8254 Programmable Interrupt Timer ports on AT 386 */
#define	PITCTR0_PORT	0x40		/* counter 0 port */	
#define	PITCTR1_PORT	0x41		/* counter 1 port */	
#define	PITCTR2_PORT	0x42		/* counter 2 port */	
#define	PITCTL_PORT	0x43		/* PIT control port */
#define	PITAUX_PORT	0x61		/* PIT auxiliary port */
#define SANITY_CTR0	0x48		/* sanity timer counter */
#define SANITY_CTL	0x4B		/* sanity control word */
#define SANITY_CHECK	0x461		/* bit 7 set if sanity timer went off*/
#define FAILSAFE_NMI	0x80		/* to test if sanity timer went off */
#define ENABLE_SANITY	0x04		/* Enables sanity clock NMI ints */
#define RESET_SANITY	0x00		/* resets sanity NMI interrupt */

/* Definitions for 8254 commands */

/* Following are used for Timer 0 */
#define PIT_C0          0x00            /* select counter 0 */
#define	PIT_LOADMODE	0x30		/* load least significant byte followed
					 * by most significant byte */
#define PIT_NDIVMODE	0x04		/*divide by N counter */
#define	PIT_SQUAREMODE	0x06		/* square-wave mode */
#define	PIT_ENDSIGMODE	0x00		/* assert OUT at end-of-count mode*/

/* Used for Timer 1. Used for delay calculations in countdown mode */
#define PIT_C1          0x40            /* select counter 1 */
#define	PIT_READMODE	0x30		/* read or load least significant byte
					 * followed by most significant byte */
#define	PIT_RATEMODE	0x06		/* square-wave mode for USART */

#define	CLKNUM	(1193167/HZ)		/* clock speed for timer */
#define SANITY_NUM	0xFFFF		/* Sanity timer goes off every .2 secs*/
/* bits used in auxiliary control port for timer 2 */
#define	PITAUX_GATE2	0x01		/* aux port, PIT gate 2 input */
#define	PITAUX_OUT2	0x02		/* aux port, PIT clock out 2 enable */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_PIT_H */
