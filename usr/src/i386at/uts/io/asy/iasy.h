#ifndef _IO_ASY_IASY_H	/* wrapper symbol for kernel use */
#define _IO_ASY_IASY_H	/* subject to change without notice */


#ident	"@(#)iasy.h	1.5"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/conssw.h>		/* REQUIRED */
#include <io/stream.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/conssw.h>		/* REQUIRED */
#include <sys/stream.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	


/*
 * Definitions for generic async support
 */

#ifndef T_CONNECT
#define T_CONNECT	42	/* Augment tty.h */
#endif

#define	T_TRLVL1	43	/* set rcv trigger level to 1 character	*/
#define	T_TRLVL2	44 	/* set rcv trigger level to 2 character	*/
#define	T_TRLVL3	45	/* set rcv trigger level to 3 character	*/
#define	T_TRLVL4	46 	/* set rcv trigger level to 4 character	*/

#ifdef MERGE386
#define T_FIRSTOPEN	47	
#define T_LASTCLOSE	48	
#endif

#define	IASY_HIWAT	512
#define	IASY_LOWAT	256
#define	IASY_BUFSZ	64	/* Chosen to be about	CLSIZE	*/

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * This is used to remember where the interrupt-time code is for
 * each async line.
 */
struct iasy_hw {
	int  (*proc)();		/* proc routine does most operations 	*/
	void (*hwdep)();	/* Called as last resort for unknown ioctls */
	conssw_t *consswp;	/* Console capable functions		*/
};

/*
 * Synchronization variables per port
 */
struct iasy_sv {
	sv_t	*iasv_drain;	/* drain synchronization varible */
	sv_t	*iasv_carrier;	/* carrier detect synchronization variable */
	sv_t	*iasv_buf;	/* bufcall() synchronization variable */
};

#endif /* _KERNEL || _KMEMUSER */

#define	L_BUF		0
#define	L_BREAK		3

/*
 * Defines for ioctl calls (VP/ix)
 */
#define AIOC			('A'<<8)
#define AIOCINTTYPE		(AIOC|60)	/* set interrupt type */
#define AIOCDOSMODE		(AIOC|61)	/* set DOS mode */
#define AIOCNONDOSMODE		(AIOC|62)	/* reset DOS mode */
#define AIOCSERIALOUT		(AIOC|63)	/* serial device data write */
#define AIOCSERIALIN		(AIOC|64)	/* serial device data read */
#define AIOCSETSS		(AIOC|65)	/* set start/stop chars */
#define AIOCINFO		(AIOC|66)	/* tell usr what device we are */

/*
 * Ioctl alternate names used by VP/ix 
 */
#define VPC_SERIAL_DOS		AIOCDOSMODE	
#define VPC_SERIAL_NONDOS	AIOCNONDOSMODE
#define VPC_SERIAL_INFO		AIOCINFO
#define VPC_SERIAL_OUT		AIOCSERIALOUT
#define VPC_SERIAL_IN		AIOCSERIALIN

#ifdef MERGE386
/*
 * Defines for MERGE ioctl 
 */
#define	COMPPIIOCTL		(AIOC|67)	/* Do com_ppiioctl() */
#endif

/*
 * Define for mouse ioctl to set receive trigger levels 
 */
#define	SETRTRLVL		(AIOC|68)	/* set rcv trigger level */

/*
 * Serial in/out requests 
 */
#define SO_DIVLLSB		1
#define SO_DIVLMSB		2
#define SO_LCR			3
#define SO_MCR			4
#define SI_MSR			1
#define SIO_MASK(elem)		(1 << ((elem) - 1))


#ifdef _KERNEL

#define	SPL		splstr

/* #define	iasychan(dev)		(dev & 0x0f) */

/*
 * L001: Macros changed to take account of 2 extra minors per port. 
 * Added SCO port device macros. Added enum constants to speed lookup.
 * Definitions are: 
 * 	Minor	Originally	Operating Modes		Control Lines 
 *	-----	----------	---------------		-------------
 *	  0	UW tty00s	SW flow, Modem ctrl	DTR, DCD
 *	  1	UW tty00h	HW flow, Modem ctrl	RTS, CTS, DCD. DTR
 *	  2	SCO tty1a	No flow/modem control	None	
 *	  3 	SCO tty1A	Modem control		DTR, DCD
 *
 * UW ports all required that the attached device emulated a modem with 
 * a live remote connection ( DCD asserted ); the minor chose between SW
 * and hardware flow control. SCO provided a non-modem control port (for 
 * basic terminal attachment) which always opened, and a modem control port
 * which required DCD asserted; neither had flow control by default.
 */

#define	IASY_UNIT_TO_TP(id, unit) \
				(struct strtty *)(&(asy_tty[(id)+(unit)]))
#define	IASY_TP_TO_UNIT(id, tp) \
				(int)((tp) - &asy_tty[id])
#define IASY_TP_TO_XTP(tp) 	\
				(struct termiox *)(&(asy_xtty[IASY_TP_TO_UNIT(asyc_id,tp)]))

#define IASY_MPU	4

#define IASY_UNIT_TO_MINOR(unit)	((unit) * IASY_MPU)
#define IASY_MINOR_TO_UNIT(minor)	((minor) / IASY_MPU)

#define IASY_MINOR(dev)			(geteminor(dev))
#define IASY_HWDEV(dev)			((IASY_MINOR(dev) % IASY_MPU) == 1)
#define IASY_SCONOMDM(dev)		((IASY_MINOR(dev) % IASY_MPU) == 2)
#define IASY_SCOMDM(dev)		((IASY_MINOR(dev) % IASY_MPU) == 3)

/* 
 * Get hardware interface type from minor 
 */
 
#define MKHW_T(m)	(m % 4) 

typedef enum { UWSWFC=0, UWHWFC, SCOTRM, SCOMDM } HWIF_T; 

/* 
 * Debugging display defines.
 */

#define	IDSP_SETMODE	0x00000001
#define	IDSP_REG		0x00000002
#define	IDSP_IOCTL		0x00000004
#define IDSP_ENXIO		0x00000008

#define IDSP_DOGETC		0x10000000 


/*
 * Function prototypes.
 */
struct conssw;
struct strtty;

extern int	iasy_input(struct strtty *, unsigned int);
extern int	iasy_output(struct strtty *);
extern void	iasy_hup(struct strtty *);
extern void	iasy_carrier(struct strtty *);
extern int	iasy_ctime(struct strtty *, int);
extern void	iasyhwdep(queue_t *, mblk_t *, struct strtty *);
extern int	iasy_register(minor_t, int, int (*)(), void (*)(),
			      struct conssw *);

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_ASY_IASY_H */
