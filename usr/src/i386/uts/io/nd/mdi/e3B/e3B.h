#ifndef _MDI_E3B_H
#define _MDI_E3B_H

#ident "@(#)e3B.h	28.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *      Copyright 1990 Interactive Systems Corporation,(ISC)
 *      All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/* Header for 3com Ethernet board */

#ifdef _KERNEL_HEADERS
 
#include <util/types.h>
#include <util/param.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <io/nd/sys/mdi.h>
#include <util/mod/moddefs.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <util/debug.h>
#include <io/ddi.h>

#else

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/mdi.h>
#include <sys/moddefs.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/debug.h>
#include <sys/ddi.h>

#endif

#ifndef UNIXWARE
#include <bool.h>
#endif

#define	E3COM_NMCADDR	16
#define	E3COM_ADDR	6

#define	MCON		0	/* turn on multicast mode */
#define	MCOFF		1	/* turn off multicast mode */

/* LAN Controller, Control Register bits 3=0, 2=0 */

#define CR	(device->base)		/* Command register (r/w) */
#define		XSTP	0x01			/* Stop */
#define		STA	0x02			/* Start */
#define		TXP	0x04			/* Transmit packet */
#define		RD0	0x08			/* Remote DMA command */
#define		RD1	0x10
#define		RD2	0x20
#define		PS0	0x40			/* Page select */
#define		PS1	0x80
/* page 0 registers */
#define PSTART	(device->base+0x01)	/* Page start register (w) */
#define PSTOP	(device->base+0x02)	/* Page stop register (w) */
#define BNRY	(device->base+0x03)	/* Boundary pointer (r/w) */
#define TSR	(device->base+0x04)	/* Transmit status (r) */
#define		PTX	0x01			/* Packet transmitted */
#define		NDT	0x02			/* Non-deferred transmission */
#define		COL	0x04			/* Transmit collided */
#define		ABT	0x08			/* Transmit aborted */
#define		CRS	0x10			/* Carrier sense lost */
#define 	FU	0x20			/* FIFO underrun */
#define		CDH	0x40			/* Collision-detect heartbeat */
#define		OWC	0x80			/* Out-of-windown collision */
#define TPSR	(device->base+0x04)	/* Transmit page start (w) */
#define NCR	(device->base+0x05)	/* # collisions (r) */
#define TBCR0	(device->base+0x05)	/* Transmit byte count (w) */
#define TBCR1	(device->base+0x06)
#define ISR	(device->base+0x07)	/* Interrupt status (r/w) */
#define		IPRX	0x01		 	/* Packet received */
#define		IPTX	0x02			/* Packet transmitted */
#define		IRXE	0x04			/* Receive error */
#define		ITXE	0x08			/* Transmit error */
#define		IOVW	0x10			/* Overwrite warning */
#define		ICNT	0x20			/* Counter overflow */
#define		IRDC	0x40			/* Remote DMA complete */
#define		IRST	0x80			/* Reset status */
#define RSAR0	(device->base+0x08)	/* Remote start address (w) */
#define RSAR1	(device->base+0x09)
#define RBCR0	(device->base+0x0a)	/* Remote byte count (w) */
#define RBCR1	(device->base+0x0b)
#define RSR	(device->base+0x0c)	/* Receive status (r) */
#define		PRX	0x01			/* Pakcet received intact */
#define		CRCE	0x02			/* CRC error */
#define		FAE	0x04			/* Frame alignment error */
#define		FO	0x08			/* FIFO overrun */
#define		MPA	0x10			/* Missed packet */
#define		PHY	0x20			/* Physical/multicast address */
#define		DIS	0x40			/* Receiver disabled */
#define		DFR	0x80			/* Deferring */
#define RCR	(device->base+0x0c)	/* Receive configuration (w) */
#define		SEP	0x01			/* Save errored packets */
#define		AR	0x02			/* Accept runt packets */
#define		AB	0x04			/* Accept broadcast */
#define		AM	0x08			/* Accept multicast */
#define		PRO	0x10			/* Promiscuous physical */
#define		MON	0x20			/* Monitor mode */
#define CNTR0	(device->base+0x0d)	/* Frame alignment errors (r) */
#define TCR	(device->base+0x0d)	/* Transmit configuration (w) */
#define		CRC	0x01			/* Inhibit CRC */
#define		LB0	0x02			/* Encoded loopback control */
#define		LB1	0x04
#define		ATD	0x08			/* Auto-transmit disable */
#define		OFST	0x10			/* Collision offset enable */
#define CNTR1	(device->base+0x0e)	/* CRC errors (r) */
#define DCR	(device->base+0x0e)	/* Data configuration (w) */
#define		WTS	0x01			/* Word transfer select */
#define		BOS	0x02			/* Byte order select */
#define		LAS	0x04			/* Long address select */
#define		BMS	0x08			/* Burst mode select */
#define		ARM	0x10			/* Autoinitialize remote */
#define		FT0	0x20			/* FIFO threshold select */
#define		FT1	0x40
#define CNTR2	(device->base+0x0f)	/* Missed packet errors (r) */
#define IMR	(device->base+0x0f)	/* Interrupt mask (w) */
#define		PRXE	0x01			/* Packet received */
#define		PTXE	0x02			/* Packet transmitted */
#define		RXEE	0x04			/* Receive error */
#define		TXEE	0x08			/* Transmit error */
#define		OVWE	0x10			/* Overwrite warning */
#define		CNTE	0x20			/* Counter overflow */
#define		RDCE	0x40			/* DMA complete */
/* page 1 registers */
#define PAR0	(device->base+0x01)	/* Physical address (r/w) */
#define CURR	(device->base+0x07)	/* Current page (r/w) */
#define MAR0	(device->base+0x08)	/* Multicast address (r/w) */

/* Ethernet Address PROM, Control Register bits 3=0, 2=1 */

#define	EADDR	(device->base) 		/* Address (read & write)*/

/* Gate Array  */

#define PSTR	(device->base+0x400)	/* Page Start Register (r/w) */
#define PSPR	(device->base+0x401)	/* Page Stop Register (r/w) */
#define DQTR	(device->base+0x402)	/* Drq Timer Register (r/w) */
#define BCFR	(device->base+0x403)	/* Base Configuration Register (ro) */
#define BCFR_IO(io)	(io+0x403)	/* Base Configuration Register (ro) */
#define PCFR	(device->base+0x404)	/* EPROM Configuration Register (ro) */
#define GACFR	(device->base+0x405)	/* Ga Configuration Register (r/w) */
#define	MBS0	0x01			/* Memory bank select 0 */
#define	MBS1	0x02			/* Memory bank select 1 */
#define	MBS2	0x04			/* Memory bank select 2 */
#define	REL	0x08			/* RAM select */
#define	TEST	0x10			/* Test */
#define	OWS	0x20			/* Zero wait state */
#define	TCM	0x40			/* Terminal count mask */
#define	NIM	0x80			/* NIC int mask */
#define CTRL	(device->base+0x406)	/* Control Register (r/w) */
#define	CTRL_IO(io)	(io+0x406)	/* Control Register (r/w) */
#define	SRST	0x01			/* Software reset */
#define	XSEL	0x02			/* Transceiver select */
#define	EALO	0x04			/* Ethernet address low */
#define	EAHI	0x08			/* Ethernet address high */
#define	SHARE	0x10			/* Interrupt share */
#define	DBSEL	0x20			/* Double buffer select */
#define	DDIR	0x40			/* DMA direction */
#define	START	0x80			/* Start DMA controller */
#define STREG	(device->base+0x407)	/* Status Register (ro) */
/* #define	REV	0x07			/* Ga revision */
#define	DIP	0x08			/* DMA in progress */
#define	DTC	0x10			/* DMA terminal count */
#define	OFLW	0x20			/* Overflow */
#define	UFLW	0x40			/* Underflow */
#define	DPRDY	0x80			/* Data port ready */
#define IDCFR	(device->base+0x408)	/* Interrupt/DMA Configuration Register (r/w) */
#define	DRQ1	0x01			/* DMA request 1 */
#define	DRQ2	0x02			/* DMA request 2 */
#define	DRQ3	0x04			/* DMA request 3 */

#define	IRQ2	0x10			/* Interrupt request 2 */
#define	IRQ3	0x20			/* Interrupt request 3 */
#define	IRQ4	0x40			/* Interrupt request 4 */
#define	IRQ5	0x80			/* Interrupt request 5 */

#define DAMSB	(device->base+0x409)	/* DMA Address Register MSB (r/w) */
#define DALSB	(device->base+0x40a)	/* DMA Address Register LSB (r/w) */
#define VPTR2	(device->base+0x40b)	/* Vector Pointer Register 2 (r/w) */
#define VPTR1	(device->base+0x40c)	/* Vector Pointer Register 1 (r/w) */
#define VPTR0	(device->base+0x40d)	/* Vector Pointer Register 0 (r/w) */
#define RFMSB	(device->base+0x40e)	/* Register File Access MSB (r/w) */
#define RFLSB	(device->base+0x40f)	/* Register File Access LSB (r/w) */

#define	EDEVSIZ	(0x10 * sizeof(char))
/*
 * Total buffer size, is therefore 1566. We allow 2000.
 */

#define TX_BUFBASE	0x20
#define TX_BUFSIZE	0x06
#define RX_BUFBASE	0x2c
#define RX_BUFLIM	0x40
#define RX_BUFLIM16	0x60
#define NXT_RXBUF(p)	((p)==device->rx_buflim-1 ? RX_BUFBASE : (p)+1)
#define PRV_RXBUF(p)	((p)==RX_BUFBASE ? device->rx_buflim-1 : (p)-1)
#define CURRXBUF(t)	(outb(CR, PS0|RD2|STA), t=inb(CURR), outb(CR, RD2|STA), t)

#define E3COM_MINPACK    60     /* minimum output packet length */
#define E3COM_MAXPACK  1514	/* maximum output packet length */

/* transfer limits */

#define E3BETHERMIN		(E3COM_MINPACK)
#define E3BETHERMTU		(E3COM_MAXPACK)

struct e3Bdevice {
	queue_t			*up_queue;
	unsigned int		base;
	unsigned int		irq;
	unsigned int		xcvr;
	int			open;		/* device open already? */
	unsigned char		type16;
	unsigned char		rx_buflim;
	unsigned int		flags;
	toid_t			tid;		/* TX timeout */
	mac_stats_eth_t		macstats;	/* stats */

	unsigned char		eaddr[E3COM_ADDR]; /* H/W address */
	void			*dlpi_cookie;	/* passed from DLPI BIND_REQ */
	unsigned int		mccnt;

	/* dual transmit buffer stuff */
	unsigned char		curtxbuf;
	unsigned char		txbufstate[2];
	unsigned char		txbufaddr[2];
	unsigned short		txbuflen[2];
	uint_t			unit;		/* from mdi_get_unit */
	unsigned char		present;	/* from mdi_get_unit */
};

/* device flags */
#define E3BBUSY		0x01
#define E3BWAITO	0x02
#define E3BALLMCA	0x04

/* miscellany */
#define OK	1
#define NOT_OK	0

#define TX_TIMEOUT	(5*HZ)
#define WATCHDOG2	(5*HZ)

#define E3BIMASK (PRXE|PTXE|RXEE|TXEE|OVWE|CNTE)

#ifndef _INKERNEL
#define N3C503UNIT	4	/* maximum number of boards possible */
#endif

#define	DIAGON		1
#define	DIAGINTR	2

#ifdef DEBUG
#define E3B_DEBUG
#endif

#ifdef E3B_DEBUG
#define E3BDIAG(M,S)	if (e3Bdiag & (M)) S
#else
#define E3BDIAG(M,S)
#endif

#define E3B_INITED	(1)
#define E3B_ACTIVE	(2)
#define E3B_DOWN	(4)

#if !defined(ENETM_ID)
#define ENETM_ID	101
#endif

/* two transmit buffer state flags */
#define TX_FREE		0
#define TX_LOADED	1
#define TX_TXING	2

#endif /* _MDI_E3B_H */
