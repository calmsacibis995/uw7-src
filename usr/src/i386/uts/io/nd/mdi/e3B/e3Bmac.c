#ident "@(#)e3Bmac.c	28.2"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

#ifdef _KERNEL_HEADERS

#include <io/nd/mdi/e3B/e3B.h>

#else

#include "e3B.h"

#endif

static ushort io_addrs[] = { 0x250, 0x280, 0x2a0, 0x2e0, 0x300, 0x310, 0x330, 0x350 };
static unchar io_BCR[] =   { 0x08, 0x04, 0x02, 0x01, 0x80, 0x40, 0x20, 0x10 };

static unsigned int e3Bimsk[] =	{
					/* vec no 0 */ 0,
					/* vec no 1 */ 0,
					/* vec no 2 */ IRQ2,
					/* vec no 3 */ IRQ3,
					/* vec no 4 */ IRQ4,
					/* vec no 5 */ IRQ5,
					0,0,0,IRQ2,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,IRQ2
					};
#define intlev_to_msk(l)	(e3Bimsk[l])

static unsigned int e3Bdmask[] = {DRQ1, DRQ1, DRQ2, DRQ3};
#define dchan_to_msk(c)		(e3Bdmask[c])

extern struct e3Bdevice *e3Bdev;
extern unsigned int e3B_nunit;

/* probe routine for 3c503 - it's rude and crude but works */
int
e3Bpresent(iobase, e)
unsigned char e[];
{
	register int i;
	unsigned int numio;

	/* determine if card is present */
	numio = sizeof(io_addrs)/sizeof(short);
	for (i = 0; i < numio; i++) {
		if (iobase == io_addrs[i])
			break;
	}
	if (i >= numio)
		return (0);
	if ((inb(BCFR_IO(iobase))) != io_BCR[i])
		return (0);

	/* reset ethernet controller */
	outb(CTRL_IO(iobase), XSEL|SRST);	/* initialize gate array */
	outb(CTRL_IO(iobase), XSEL);		/* toggle software reset bit */
	outb(CTRL_IO(iobase), EALO|XSEL);	/* enable Ethernet address prom */

	for (i = 0; i < 6; i++) {
		e[i] = inb(iobase+i);
	}
	return(1);
}

e3Bhwinit(unit) 
register unit;
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	register int i;
	int	     s;
	int t;

	/* set the io address of the board */

	STRLOG(ENETM_ID, 0, 9, SL_TRACE, "e3Bhwinit");

	/* reset ethernet controller */
	outb(CTRL, XSEL|SRST);		/* initialize gate array */
	outb(CTRL, XSEL);		/* toggle software reset bit */
	outb(CTRL, EALO|XSEL);		/* enable Ethernet address prom */

	/* recognize board type */
	outb(CTRL, XSEL);
	outb(CR, RD2);			/* page 0 registers */ 
	outb(DCR, 0);			/* write a zero */
	outb(CR, PS1|RD2);		/* page 2 registers */
	t = inb(DCR)&WTS;		/* if WTS on then it is a 16-bit brd */
	outb(CR, RD2);			/* page 0 registers */ 
	if (t) {
		device->type16 = 1;
		device->rx_buflim = RX_BUFLIM16;
	} else {
		device->rx_buflim = RX_BUFLIM;
	}

	/* reset ethernet controller */
	outb(CTRL, XSEL|SRST);		/* initialize gate array */
	outb(CTRL, XSEL);		/* toggle software reset bit */
	outb(CTRL, EALO|XSEL);		/* enable Ethernet address prom */

	if (device->xcvr) 
		outb(CTRL, 0);		/* external transceiver */
	else
		outb(CTRL, XSEL);	/* on-board transceiver */

	outb(PSTR, RX_BUFBASE);
	outb(PSPR, device->rx_buflim);	/* 5K or 13k of receive space */
					/* Set interrupt level and DMA channel */
	outb(IDCFR, intlev_to_msk(device->irq));	
	outb(DQTR, 8);			/* 8 bytes/DMA transfer */
	outb(DAMSB, TX_BUFBASE);	/* 3k (2 x 1.5K) transmit buffers */
	outb(DALSB, 0); 

	s = splstr();
	e3Bstrtnic(unit, 1);		/* start NIC */
	device->txbufaddr[0] = TX_BUFBASE;
	device->txbufaddr[1] = TX_BUFBASE + TX_BUFSIZE;
	splx(s);

	return(OK);
}

void
e3Bwatchdog(unit)
{
	register struct e3Bdevice *device = &e3Bdev[unit];

	device->macstats.mac_timeouts++;
	e3Brestrtnic(unit, 0);
	if (!(device->flags & E3BBUSY)) {	/* should be BUSY */
		device->tid = 0;
		return;
	}
#ifdef E3B_DEBUG
	cmn_err(CE_CONT, "e3Bwatchdog retransmit");
#endif
	outb(TCR, 0);			/* retry current transmission */
	device->tid = itimeout(e3Bwatchdog, (void *)unit, TX_TIMEOUT, plstr);  /* chip bug */
	outb(CR, RD2|TXP|STA);		/* transmit */
}

e3Brestrtnic(unit, flag)
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	static int restarting;

	STRLOG(ENETM_ID, 0, 9, SL_TRACE, "Restarting 3c503 NIC");
	if (restarting)
		return;
	restarting = 1;
	device->macstats.mac_tx_errors++;
	outb(CR, RD2|XSTP);			/* stop NIC first */
/*MAF
	while (!(inb(ISR) & IRST))
		;
*/
	e3Bstrtnic(unit, flag);
	restarting = 0;
}

static
e3Bstrtnic(unit, flag)
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	unsigned char *eaddr = device->eaddr;
	int	 i;

	outb(CR, RD2|XSTP);		/* page 0 registers */ 
	outb(DCR, FT1|BMS);		/* 8 bytes/DMA burst */ 

	outb(RCR, AB|AM);		/* accept  broadcast and multicast */

	outb(TCR, LB0);			/* loopback mode */
	if (flag)
		outb(BNRY, device->rx_buflim-1);	/* CURR - 1 */
	outb(PSTART, RX_BUFBASE);	/* same as PSTR */
	outb(PSTOP, device->rx_buflim);	/* same as PSTOP */
	outb(ISR, 0xff);		/* reset interrupt status */
	outb(IMR, E3BIMASK);		/* enable interrupts */
	outb(CR, PS0|RD2|XSTP);		/* page 1 registers */
	for (i=0; i<E3COM_ADDR; i++)
		outb(PAR0+i, eaddr[i]);    /* set ethernet address */
	if (flag)
		outb(CURR, RX_BUFBASE);		/* current page */
	outb(CR, RD2|XSTP);		/* page 0 registers */
	outb(CR, RD2|STA);		/* activate NIC */ 
	outb(TCR, 0);			/* exit loopback mode */
}

/*
 * e3Bintr - Ethernet interface interrupt
 */

e3Bintr(lev) 
register lev;
{
	register struct e3Bdevice *device;
	register mac_stats_eth_t *e3Bmacstats;
	register mblk_t *mp;
	register unsigned char c, c2;
	register uint len;
	register i,j;
	int nxtpkt, curpkt;
	int board = int_to_board(lev);	/* key off interrupt level */
	int x;

	device = &e3Bdev[board];

	if (device->unit == (uint_t) -1) {
		return; /* adapter not configured */
	}

	if (device->up_queue == (queue_t *)0) {
		return; /* adapter reset */
	}

	e3Bmacstats = &(device->macstats);

	outb(IMR, 0);		/* disable interrupts */

	if ((c = inb(ISR)) == 0) {
		e3Bmacstats->mac_spur_intr++;
		return;
	}
	outb(ISR, c);		/* clear status */
	if (c & (IPTX|ITXE)) {
		e3Bcktsr(board);
		e3Bstrtout(board);
	}
	if (c & IRXE) {		/* Receive error */
		c2 = inb(RSR);
		STRLOG(ENETM_ID, 0, 9, SL_TRACE, "receive error status = %x", c2);
		if (c2 & CRCE) {
			e3Bmacstats->mac_badsum++;
		}
		if (c2 & FAE) {
			e3Bmacstats->mac_align++;
		}
		if (c2 & FO) {
			e3Bmacstats->mac_baddma++;
		}
		if (c2 & MPA) {
			e3Bmacstats->mac_no_resource++;
		}
		if (c2 & ~(CRCE|FAE|FO|MPA)) {		/* Huh? */
			e3Bmacstats->mac_spur_intr++;
		}
	}
	if (c & ICNT) { 	/* Counter overflow */
		(void)inb(CNTR0);	 /* clear counters */
		(void)inb(CNTR1);
		(void)inb(CNTR2);
	}
	if (c & (IPRX|IOVW)) {	/* Packet received */
		if (c & IOVW) {
			outb(CR, RD2|XSTP);
			outb(RBCR0, 0);
			outb(RBCR1, 0);
			outb(TSR, LB0);
			outb(CR, RD2|STA);
		}
		nxtpkt = NXT_RXBUF(inb(BNRY));
		while (nxtpkt != CURRXBUF(j)) {
			outb(DAMSB, nxtpkt);
			outb(DALSB, 0);
			outb(CTRL, (device->xcvr ? 0 : XSEL)|START);
			while(!(inb(STREG) & DPRDY))
				/* wait for board to fill register */;
			c2 = inb(RFMSB);
			curpkt = nxtpkt;
			nxtpkt = inb(RFMSB);
			len = inw(RFMSB);
			STRLOG(ENETM_ID, 0, 9, SL_TRACE, 
				"packet received, s %x cn %x l %x", 
				 c2, (curpkt<<8)|nxtpkt, len);
			if (e3Bpktsft(curpkt, nxtpkt, len, device)) {
				STRLOG(ENETM_ID, 0, 9, SL_TRACE, 
					"shifted packet rcvd");
				outb(CTRL, (device->xcvr ? 0 : XSEL));
				e3Bmacstats->mac_align++;
				e3Brestrtnic(board, 1);
				return;
			}
			if (c2 & (DIS|DFR)) {
				STRLOG(ENETM_ID, 0, 9, SL_TRACE, 
				"packet rcvd erronously, status=%x", c2);
				e3Bmacstats->mac_no_resource++;
				outb(CTRL, (device->xcvr ? 0 : XSEL));
				outb(BNRY, PRV_RXBUF(nxtpkt));	/* update read ptr */
				continue;
			}
			len -= 4;	/* dump the CRC */
			if ((len < E3COM_MINPACK) || (len > E3COM_MAXPACK)) {
				e3Bmacstats->mac_badlen++;
				STRLOG(ENETM_ID, 0, 9, SL_TRACE, 
					"bad length %d recvd", len);
				outb(CTRL, (device->xcvr ? 0 : XSEL));
				outb(BNRY, PRV_RXBUF(nxtpkt));	/* update read ptr */
				continue;
			}

			STRLOG(ENETM_ID, 0, 9, SL_TRACE, 
				"allocating buffer size %d", len);
			if (!(mp = allocb(len+2, BPRI_HI))) {
				e3Bmacstats->mac_frame_nosr++;
				STRLOG(ENETM_ID, 0, 9, SL_TRACE, 
					"allocb length %d failed", len);
				outb(CTRL, (device->xcvr ? 0 : XSEL));
				outb(BNRY, PRV_RXBUF(nxtpkt));	/* update read ptr */
				continue;
			}
			/* Align user data on 4 byte boundary for speed */
			mp->b_rptr += 2;
			e3Bioin(mp->b_rptr, len, RFMSB, STREG, device->type16);
			outb(CTRL, (device->xcvr ? 0 : XSEL));
			outb(BNRY, PRV_RXBUF(nxtpkt));	/* update read ptr */


			/* adjust length of write pointer */
			mp->b_wptr = mp->b_rptr + len;

			/* is it a broadcast/multicast address? */
			if (*mp->b_rptr & 0x01 &&
				e3Bchktbl(mp->b_rptr, board)) {
				freeb(mp);
				continue;
			}

			if (device->up_queue) {
				putnext(device->up_queue, mp);
			} else {
				freeb(mp);
			}
		}
	}
	if (c & IOVW) {		/* Overwrite warning */
		e3Bmacstats->mac_baddma++;
		outb(TCR, 0);
	}
	outb(IMR, E3BIMASK);
}

e3Bcktsr(unit)
int unit;
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	register mac_stats_eth_t *e3Bmacstats = &device->macstats;
	unsigned char c;

	while (!((c = inb(TSR)) & (PTX|ABT|FU)))
		;

	if (c & PTX) {
	} else
		if (c & ABT) {
			e3Bmacstats->mac_xs_coll++;
		} else
			if (c & FU) {
				e3Bmacstats->mac_baddma++;
			}
	if (c & COL) {
		int n = inb(NCR);

		if (n >= 1 && n <= 16) {
		    n--;
		    e3Bmacstats->mac_colltable[n]++;
		}
	} else
		if ((c & NDT) == 0)
			e3Bmacstats->mac_frame_def++;
	if (c & CRS) {
		e3Bmacstats->mac_carrier++;
	}
	if (c & OWC) {
		e3Bmacstats->mac_oframe_coll++;
	}
	if (c & CDH)
		e3Bmacstats->mac_sqetesterrors++;
	return(c);
}

e3Bstrtout(unit)
int unit;
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	queue_t *q;
	int      i;
	int	 s;
	unsigned char nxttxbuf;
	uint len;

	nxttxbuf = device->curtxbuf;
	device->txbufstate[nxttxbuf] = TX_FREE;
	nxttxbuf ^= 1;
	if (device->txbufstate[nxttxbuf] != TX_FREE) {
		outb(TCR, 0);
		outb(TPSR, device->txbufaddr[nxttxbuf]);
		len = device->txbuflen[nxttxbuf];
		outb(TBCR0, len & 0xff);
		outb(TBCR1, len >> 8);
		outb(CR, RD2|TXP|STA);		/* transmit */
		device->curtxbuf = nxttxbuf;
		/* restart timeout for the chip bug */
		untimeout(device->tid);
		device->tid = itimeout(e3Bwatchdog, (void *)unit,
			TX_TIMEOUT, plstr);
	} else {
		if (device->flags & E3BBUSY) {
			untimeout(device->tid);		/* call off watchdog */
			device->tid = 0;
			device->flags &= ~E3BBUSY;
		}
	}
	/* we need to qenable only if writer is waiting */
	if (device->flags & E3BWAITO) {
		device->flags &= ~E3BWAITO;
		e3Bdequeue(WR(device->up_queue));
	}
}

e3Bpktsft(curpkt, nxtpkt, len, device)
int curpkt;
int nxtpkt;
int len;
register struct e3Bdevice *device;
{
	curpkt += (len>>8) + 1;
	if (curpkt >= (int)device->rx_buflim)
		curpkt -= device->rx_buflim - RX_BUFBASE;
	if (nxtpkt == curpkt || nxtpkt == NXT_RXBUF(curpkt))
		return(0);
	else
		return(1);
}

/*
 * check if send windows are available, basically avoid putbq's if possible
 * as they are extremely expensive
 */
e3Boktoput(unit)
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	register int nxttxbuf;

	nxttxbuf = device->curtxbuf;
	if (device->txbufstate[0] && device->txbufstate[1]) {
		device->flags |= E3BWAITO;	/* caller waits */
		return(NOT_OK);
	}
	return(OK);
}

/*
 * e3Bhwput - copy to board and send packet
 *
 */

static char e3Bzerobuf[E3COM_MINPACK];		/* preinitialized to zero */

e3Bhwput(unit, m)
register unit;
mblk_t *m;
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	register mblk_t *mp;
	mac_stats_eth_t *e3Bmacstats = &(device->macstats);
	register uint len = 0;
	register uint tlen;
	unsigned char nxttxbuf;
	int oaddr;

	nxttxbuf = device->curtxbuf;
	if (device->txbufstate[nxttxbuf] != TX_FREE) {
		nxttxbuf ^= 1;
		if (device->txbufstate[nxttxbuf] != TX_FREE) {
			cmn_err(CE_WARN, "e3Bhwput: No free tx bufs");
			freemsg(m);
			return;
		}
	}
	outb(DAMSB, device->txbufaddr[nxttxbuf]);
	outb(DALSB, 0);
	outb(CTRL, (device->xcvr ? 0 : XSEL)|START|DDIR);
	oaddr = 0;		/* fake it, only care about odd/even align */
	for (mp=m; mp; mp=mp->b_cont) {
		len += (tlen = mp->b_wptr - mp->b_rptr);
		if (len > E3COM_MAXPACK)
			break;
if (tlen)
		e3Bioout(mp->b_rptr, tlen, RFMSB, STREG, device->type16, oaddr);
		oaddr += tlen;
	}
	freemsg(m);
	if (len > E3COM_MAXPACK) {
		/* drop the packet */
		return;
	} else if (len < E3COM_MINPACK) {
		e3Bioout(e3Bzerobuf, E3COM_MINPACK - len, RFMSB, STREG,
			device->type16, oaddr);
		len = E3COM_MINPACK;
	}

	STRLOG(ENETM_ID, 0, 9, SL_TRACE, "e3Bhwput, len = %d", len);
	outb(CTRL, (device->xcvr ? 0 : XSEL));

	device->txbufstate[nxttxbuf] = TX_LOADED;
	device->txbuflen[nxttxbuf] = len;

	if (nxttxbuf != device->curtxbuf) {
		return;
	}
	device->txbufstate[nxttxbuf] = TX_TXING;
	device->flags |= E3BBUSY;
	outb(TCR, 0);
	outb(TPSR, device->txbufaddr[nxttxbuf]);
	outb(TBCR0, len & 0xff);
	outb(TBCR1, len >> 8);
	untimeout(device->tid);
	device->tid = itimeout(e3Bwatchdog, (void *)unit, TX_TIMEOUT, plstr);  /* chip bug */
	outb(CR, RD2|TXP|STA);		/* transmit */
	return;
}

e3Bhwclose(unit)
int unit;
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	int i;

	STRLOG(ENETM_ID, 0, 9, SL_TRACE, "e3Bhwclose");

	/* reset ethernet controller */
	outb(CR, RD2|XSTP);
	outb(CTRL, XSEL|SRST);
}

static 
int_to_board(lev)
{
	int i;
	struct e3Bdevice *device = e3Bdev;

	for (i = 0; i < e3B_nunit; i++, device++)
		if (device->irq == lev)
			break;
	return(i);
}

int
e3Bchktbl(addr, unit)
unsigned char *addr;
int unit;
{
	extern unsigned char e3B_broad[];
	struct e3Bdevice *device = &e3Bdev[unit];
	int i;

	if (device->flags & E3BALLMCA)
		return(0);		/* FOUND */
	if (!e3Bstrncmp(e3B_broad, addr, E3COM_ADDR))
		return(0);		/* FOUND */
	if (mdi_valid_mca(device->dlpi_cookie, addr))
		return(0);		/* FOUND */

	return (1);			/* FAIL */
}

/*
 * e3Bmcset:
 *	Enable reception of all multicast addresses
 *	Address filtering must be done at pack receive time
 *
 * Lock State:
 *	Should be called at splstr
 */
e3Bmcset(unit, mode)
int unit;
int mode;
{
	register struct e3Bdevice *device = &e3Bdev[unit];
	int i;
	unsigned char val;

	val = (mode == MCOFF) ? 0 : 0xff;

	outb(CR, PS0|RD2|STA);		/* page 1 registers */
	for (i = 0; i < 8; i++)
		outb((MAR0 + i), val);
	outb(CR, RD2|STA);		/* page 0 registers */
}
