#ident	"@(#)i8237A.c	1.16"

/*      Copyright (c) 1988, 1989 Intel Corp.
 *        All Rights Reserved
 *
 *      INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *      This software is supplied under the terms of a license
 *      agreement or nondisclosure agreement with Intel Corpo-
 *      ration and may not be copied or disclosed except in
 *      accordance with the terms of that agreement.
 */
/* Modification History
 * L000		georgep@sco.com		9-July-1997
 * - from stevbam@sco.com, removed references to Dma_addr_28
 *   as part of topology parse changes.
 * L001		brendank@sco.com	8-Jan-1997
 * - Some EISA bus systems cannot DMA into the full 32 bit address space.
 *   To get round this, DMA is restricted to the bottom 16M of memory,
 *   just as in ISA bus machines.
 */

#include <io/dma.h>
#include <mem/kmem.h>
#include <svc/bootinfo.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/types.h>

#include <io/ddi_i386at.h>


extern uchar_t inb(int);
extern void outb(int, uchar_t);

#define	DMACHIER	1
#define	DMACPL		plhi

#ifdef DMA_DEBUG
#ifdef INKERNEL
#define dprintf(x)      cmn_err(CE_NOTE, x)
#else
#define dprintf(x)      printf x
#endif
#else
#define dprintf(x)
#endif

/*
 * macros to get Bytes from longs
 */
#define BYTE0(x)	((x) & 0x0FF)
#define BYTE1(x)	(((x) >> 8) & 0x0FF)
#define BYTE2(x)	(((x) >> 16) & 0x0FF)
#define BYTE3(x)	(((x) >> 24) & 0x0FF)

#define TOLONG(x, y)	((x) | ((y) << 16))

#define	D37A_DMA_DISABLE(chan) \
		(outb(chan_addr[(chan)].mask_reg, DMA_SETMSK | (chan) & 0x03))

#define	D37A_DMA_ENABLE(chan) \
		(outb(chan_addr[(chan)].mask_reg, (chan) & 0x03))

/*
 * Defines for MCA DMA controllers.
 */

#define	DMCA_DMA_DISABLE(chan) \
		(outb(MCADMA_CTL, MCADMA_SMK + (chan)))	/* set mask reg. */

#define	DMCA_DMA_ENABLE(chan) \
		(outb(MCADMA_CTL, MCADMA_CMK + (chan)))	/* reset mask reg. */

/*
 * MCA DMA extended mode definitions.
 */
#define MCADMA_CTL	0x18	/* function register            */
#define MCADMA_DAT	0x1A	/* execute function register    */

#define MCADMA_SMK	0x90	/* set mask register            */
#define MCADMA_MAR	0x20	/* memory address register      */
#define MCADMA_TCR	0x40	/* transfer count register      */
#define MCADMA_WMR	0x70	/* write mode register          */
#define MCADMA_CMK	0xA0	/* clear mask register          */

#define MCADMA_RD	0x44	/* 16-bit read mode             */
#define MCADMA_WR	0x4C	/* 16-bit write mode            */


STATIC void	dMCA_prog_extmode(ulong_t, ulong_t, int, int);
STATIC void	dEISA_setchain(int);
STATIC void	d37A_write_target(unsigned long, int);
STATIC unsigned long d37A_read_target(int);
STATIC void	d37A_write_count(unsigned long, int);
STATIC unsigned long d37A_read_count(int);
STATIC void	d37A_mode_set(struct dma_cb *, int);

/*
 * data structures for programming the DMAC
 */
struct d37A_chan_reg_addr chan_addr[] = { D37A_BASE_REGS_VALUES };

int	Eisa_dma = FALSE;
uint_t	d37A_bustype;

struct dma_buf *eisa_curbuf[NCHANS];

/*
 * DMA transfer size for each channel. It is protected by channel
 * pseudo sleep lock. 
 */
uchar_t	d37A_targ_path[NCHANS];	

/*
 * Spin lock to protect concurrent requests to program the 
 * DMA controller.
 */
lock_t	*d37A_dmaclk;

LKINFO_DECL(d37A_dmaclk_lkinfo, "DMA:DMAC:DMA controller spin lock", 0);


/*
 * STATIC void
 * dMCA_prog_extmode(ulong_t addr, ulong_t len, int rw, int chan)
 *	Set up the MCA DMA controller to do the requested transfer
 *	while in extended mode (16-bit transfer).
 *
 * caller:	d37A_prog_chan()
 * calls:	none
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	The MCA DMA controller extended mode supports:
 *		- Each channel programmable to byte or word transfer.
 *		- Extended operations:
 *			- Extended program control: This allows certain
 *			  register to be read which are only writeable
 *			  in 8237 compatible mode. 
 *			- Extended mode register: This allows 8-bit or
 *			  16-bit transfer size.
 *		- Extended mode register is activated whenever a DMA
 *		  channel requests a DMA data transfer.
 *		- Extended mode does not include block, cascade or demand
 *		  transfer modes as does the ISA DMA controller.
 *		- The MCA DMAC does not support memory-to-memory transfers,
 *		  thus it cannot use the dma_swsetup, dma_stop and dma_swstart
 *		  interfaces in extended mode.
 *
 *	The system microprocessor uses the following steps to write or
 *	or read from any of the DMA internal registers:
 *
 *	1. Write to the function register (0x18) to indicate the
 *	   the function and the channel number. Note the internal
 *	   Byte Pointer register is always reset to 0 when an
 *	   out to address 0x18 is detected. The format of the
 *	   function register is
 *
 *	        7		 4          3		     0
 *		-----------------------------------------------
 *		| program command | reserved | channel number | 
 *		-----------------------------------------------
 *
 *	2. Write the function by doing an In/Out to address 0x1A. 
 *	   Note the byte pointer automatically increments by 1 and
 *	   points to the next byte every time the port address 0x1A
 *	   is used.
 */
STATIC void
dMCA_prog_extmode(ulong_t addr, ulong_t len, int rw, int chan)
{
        pl_t    opl;

	ASSERT(rw == DMA_CMD_READ || rw == DMA_CMD_WRITE);

	opl = LOCK(d37A_dmaclk, DMACPL);

	outb(MCADMA_CTL, MCADMA_SMK + chan);	/* set mask register */
	outb(MCADMA_CTL, MCADMA_MAR + chan);	/* set memory address reg */
	outb(MCADMA_DAT, BYTE0(addr));		/* set low byte of address */
	outb(MCADMA_DAT, BYTE1(addr));		/* set 2nd byte of address */
	outb(MCADMA_DAT, BYTE2(addr));		/* set page register */
	outb(MCADMA_CTL, MCADMA_TCR + chan);	/* set transfer count reg */
	outb(MCADMA_DAT, BYTE0(len));		/* set low byte of count */
	outb(MCADMA_DAT, BYTE1(len));		/* set high byte of count */
	outb(MCADMA_CTL, MCADMA_WMR + chan);	/* set transfer mode reg */
	outb(MCADMA_DAT, rw ? MCADMA_WR : MCADMA_RD);/* set R/W 16-bit mode */

	/* Do not reset the mask register. */

	UNLOCK(d37A_dmaclk, opl);
}


/*
 * uchar_t
 * d37A_get_best_mode(struct dma_cb *dmacbptr)
 *	stub routine - determine optimum transfer method
 *
 * caller:	dma_get_best_mode()
 * calls:	
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
uchar_t
d37A_get_best_mode(struct dma_cb *dmacbptr)
{
	return((uchar_t)DMA_CYCLES_2);
}


/*
 * void
 * d37A_intr(int lev)
 *	Stub routine.
 *
 * caller:	dma_intr()
 * calls:	dEISA_setchain()
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
d37A_intr(int lev)
{
	int	i, st;

	if (Eisa_dma == TRUE) {
		pl_t opl = LOCK(d37A_dmaclk, DMACPL);

		st = inb(EISA_DMAIS) & 0xef;    /* channel 4 can't interrupt */
		i = 0;
		while (st) {
			if (st & 1)
				dEISA_setchain(i);
			st >>= 1;
			i++;
		}

		UNLOCK(d37A_dmaclk, opl);
	}
	return;
}


/*
 * STATIC void
 * dEISA_setchain(int chan)
 *	Set next buffer address/count from chain.
 *
 * caller:	d37A_intr()
 * calls:	d37A_write_target(), d37A_write_count()
 *
 * Calling/Exit State:
 *	- DMA controller spin lock is held on entry & exit.
 */
STATIC void
dEISA_setchain(int chan)
{
	struct dma_buf *dp;

	dprintf(("dEISA_setchain(%d)\n", chan));

	if ((dp = eisa_curbuf[chan]) == (struct dma_buf *)0) {
		/*
		 * clear chain enable bit
		 */
		outb(chan_addr[chan].scm_reg, chan);
		return;
	}
	dprintf(("next buffer:%xbytes @%x\n", dp->count, dp->address));
	outb(chan_addr[chan].scm_reg, chan | EISA_ENCM);
	d37A_write_target(dp->address, chan);
	outb(chan_addr[chan].hpage_reg, BYTE3((long)dp->address));
	if (Eisa_dma) {
		d37A_write_count(((dp->count_hi << 16) | dp->count), chan);
	} else {
		d37A_write_count(dp->count, chan);
	}
	outb(chan_addr[chan].scm_reg, chan | EISA_ENCM | EISA_CMOK);
	eisa_curbuf[chan] = dp->next_buf;
	return;
}


/*
 * STATIC void
 * d37A_write_target(ulong_t targ_addr, int chan)
 *	Write the target address into the Base Target Register
 *	for the indicated channel.
 *
 * caller:	d37A_prog_chan(), d37A_dma_swsetup(), dEISA_setchain()
 * calls:	none
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit if called 
 *	  from d37A_prog_chan().
 *	- DMA controller spin lock is held on entry & exit.
 */
STATIC void
d37A_write_target(ulong_t targ_addr, int chan)
{
	dprintf(("d37A_write_target: writing Target Address %x to channel %d\n",
			targ_addr, chan));

	/* write the target device address, one byte at a time */

	outb(chan_addr[chan].ff_reg, 0);        /* set flipflop */
	drv_usecwait(10);
	/* write the base address */
	outb(chan_addr[chan].addr_reg, BYTE0(targ_addr));
	drv_usecwait(10);
	outb(chan_addr[chan].addr_reg, BYTE1(targ_addr));
	drv_usecwait(10);
	/* set page register */
	outb(chan_addr[chan].page_reg, BYTE2(targ_addr));

	/* Set High Page Register */
	if (Eisa_dma == TRUE) {
		outb(chan_addr[chan].hpage_reg, BYTE3(targ_addr));
	}

	drv_usecwait(10);
}


/*
 * STATIC ulong_t
 * d37A_read_target(int chan)
 *	Read the target address from the Current Target Register
 *	for the indicated channel.
 *
 * caller:	d37A_get_chan_stat()
 * calls:	none
 *
 * Calling/Exit State:
 *	- DMA controller spin lock is held on entry & exit.
 */
STATIC ulong_t
d37A_read_target(int chan)
{
	ulong_t	targ_addr;

	dprintf(("d37A_read_target: reading channel %d's Target Address.\n",
	    chan));

	/* read the target address a byte at a time */

	outb(chan_addr[chan].ff_reg, 0);        /* set flipflop */
	/* read the address */
	targ_addr = inb(chan_addr[chan].addr_reg);
	targ_addr |= inb(chan_addr[chan].addr_reg) << 8;
	targ_addr |= inb(chan_addr[chan].page_reg) << 16;

	/*
         * Always read the high byte, for compatible accesses
         * it will be 0 anyway
         */
	if (Eisa_dma == TRUE) {
		targ_addr |= inb(chan_addr[chan].hpage_reg) << 24;
	}

	dprintf(("d37A_read_target: channel %d's Target Address= %x.\n",
			chan, (ulong_t) targ_addr));

	return(targ_addr);
}


/*
 * STATIC void
 * d37A_write_count(ulong_t count, int chan)
 *	Write the transfer count into the Base Count Register for
 *	the indicated channel.
 *
 * caller:	d37A_prog_chan(), d37A_dma_swsetup(), dEISA_setchain()
 * calls:	none
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit if called 
 *	  from d37A_prog_chan().
 *	- DMA controller spin lock is held on entry & exit.
 */
STATIC void
d37A_write_count(ulong_t count, int chan)
{
	/* write the transfer byte count, one byte at a time */

	dprintf(("d37A_write_count: writing Count %x to channel %d.\n",
	    count, chan));

	/* check validity of channel number */

	outb(chan_addr[chan].ff_reg, 0);        /* set flipflop */
	drv_usecwait(10);
	/* write the word count */
	outb(chan_addr[chan].cnt_reg, BYTE0(count));
	drv_usecwait(10);
	outb(chan_addr[chan].cnt_reg, BYTE1(count));
	drv_usecwait(10);

	if (Eisa_dma == TRUE)
		outb(chan_addr[chan].hcnt_reg, BYTE2(count));
}


/*
 * STATIC ulong_t
 * d37A_read_count(int chan)
 *	Read the transfer count from the Current Count Register for
 *	the indicated channel.
 *
 * caller:	d37A_get_chan_stat()
 * calls:	d37A macros
 *
 * Calling/Exit State:
 *	- DMA controller spin lock is held on entry & exit.
 */
STATIC ulong_t	
d37A_read_count(int chan)
{
	ulong_t	count;

	dprintf(("d37A_read_count: reading channel %d's Count.\n", chan));

	/* read the transfer byte count, a byte at a time */

	outb(chan_addr[chan].ff_reg, 0);        /* set flipflop */
	/* read the count */
	count = inb(chan_addr[chan].cnt_reg);
	count |= inb(chan_addr[chan].cnt_reg) << 8;

	if (Eisa_dma == TRUE) {
		count |= inb(chan_addr[chan].hcnt_reg) << 16;
	}

	dprintf(("d37A_read_count: channel %d's Count= %x.\n", chan, count));

	return(count);
}


/*
 * void
 * d37A_init(void)
 *	Initializes the 8237A.
 *
 * caller:	dma_init()
 * calls:	none
 *
 * Calling/Exit State:
 *	None.
 */
void
d37A_init(void)
{
	d37A_dmaclk = LOCK_ALLOC(DMACHIER, DMACPL, &d37A_dmaclk_lkinfo,
					KM_NOSLEEP);
	if (d37A_dmaclk == NULL)
		/*
		 *+ There is not enough memory to allocate for
		 *+ i8237A DMA controller synchronization primitives.
		 *+ Check memory configured in your system.
		 */
		cmn_err(CE_PANIC,
			"d37A_init: Not enough memory");

	/*
         * Determine bus type (EISA or otherwise).
         */

	if ((drv_gethardware(IOBUS_TYPE, &d37A_bustype)) < 0) {
		/*
		 *+ Unknown I/O bus type.
		 */
		cmn_err(CE_WARN,
			"d37A_init: Unknown I/O bus type");
	}

        if (d37A_bustype == BUS_EISA)
                Eisa_dma = TRUE;

        return;
}


/*
 * STATIC void
 * d37A_mode_set(struct dma_cb *dmacbptr, int chan)
 *	Program the Mode registers of the DMAC for a subsequent
 *	hardware-initiated transfer. 
 *
 * caller:	d37A_prog_chan()
 * calls:	none
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit.
 *	- DMA controller spin lock is held on entry & exit.
 */
STATIC void
d37A_mode_set(struct dma_cb *dmacbptr, int chan)
{
	uchar_t	mode;

	mode = chan & 0x03;

	if (dmacbptr->command == DMA_CMD_VRFY)
		mode |= DMA_VR << 2;
	else if (dmacbptr->command == DMA_CMD_READ)
		mode |= DMA_RD << 2;
	else
		mode |= DMA_WR << 2;

	if (dmacbptr->bufprocess == DMA_BUF_AUTO)
		mode |= 1 << 4;

	if (dmacbptr->targ_step == DMA_STEP_DEC)
		mode |= 1 << 5;

	if (dmacbptr->trans_type == DMA_TRANS_SNGL)
		mode |= 1 << 6;
	else if (dmacbptr->trans_type == DMA_TRANS_BLCK)
		mode |= 2 << 6;
	else if (dmacbptr->trans_type == DMA_TRANS_CSCD)
		mode |= 3 << 6;

	dprintf(("set mode: chan = %d mode = %x, mode_reg = %x \n",
			chan, mode, chan_addr[chan].mode_reg));
	outb(chan_addr[chan].mode_reg, mode);

	if (Eisa_dma) {
		mode &= 3;
		switch (dmacbptr->targ_path) {
		case DMA_PATH_16:
			mode |= EISA_DMA_16 << 2;
			break;
		case DMA_PATH_8:
			/* mode |= EISA_DMA_8 << 2; */
			break;
		case DMA_PATH_32:
			mode |= EISA_DMA_32 << 2;
			break;
		case DMA_PATH_16B:
			mode |= EISA_DMA_16B << 2;
			break;
		}
		mode |= dmacbptr->cycles << 4;
		outb(chan_addr[chan].xmode_reg, mode);
		dprintf(("ext mode: chan = %d, mode = %x, xmode_reg = %x\n", 
				chan, mode, chan_addr[chan].xmode_reg));
	}
}


/*
 * int
 * d37A_prog_chan(struct dma_cb *dmacbptr, int chan)
 *	Program the Mode registers and the Base registers of the
 *	DMAC for a subsequent hardware-initiated transfer. 
 *
 * caller:	dma_prog_chan()
 * calls:	d37A_write_target(), d37A_write_count(), dEISA_setchain,
 *		d37A_mode_set
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit.
 */
int
d37A_prog_chan(struct dma_cb *dmacbptr, int chan)
{
	ulong_t	addr;
	long	tcount;
	pl_t	opl;

	/* prepare for a hardware operation on the specified channel */

	dprintf(("d37A_prog_chan: programming channel %d, dmacbptr= %x.\n",
		    chan, (ulong_t) dmacbptr));

	ASSERT(dmacbptr->trans_type != DMA_TRANS_CSCD);

	if (chan == DMA_CH4) {
		dprintf(("d37A_prog_chan: channel %d not supported.\n", chan));
		return(FALSE);
	}

	if (d37A_bustype == BUS_ISA) {
		if (dmacbptr->targ_type == DMA_TYPE_MEM) {
			dprintf(("d37A_prog_chan: "
				 "memory to memory mode not supported.\n"));
			return(FALSE);
		}

		switch (chan) {
		case DMA_CH0:
		case DMA_CH1:
		case DMA_CH2:
		case DMA_CH3:
			if (dmacbptr->targ_path != DMA_PATH_8) {
				dprintf(("d37A_prog_chan: "
					 "channel %d not programmed.\n", chan));
				return(FALSE);
			}
			break;
		case DMA_CH5:
		case DMA_CH6:
		case DMA_CH7:
			if (dmacbptr->targ_path != DMA_PATH_16) {
				dprintf(("d37A_prog_chan: "
					 "channel %d not programmed.\n", chan));
				return(FALSE);
			}
			break;
		} /* end switch */

	} else if (d37A_bustype == BUS_MCA) {

		if (dmacbptr->targ_type == DMA_TYPE_MEM) {
			dprintf(("d37A_prog_chan: "
				 "memory to memory mode not supported.\n"));
			return(FALSE);
		}

		switch (dmacbptr->targ_path) {
		case DMA_PATH_8:
			break;
		case DMA_PATH_16:
			/*
			 * "16-bit I/O, count by word" mode requires
			 * the count to be one less than the number 
			 * of 16 bit words to be transferred. IOW,
			 *	count = count / 2 - 1
			 */
			if (dmacbptr->targbufs->count_hi == 0) {
				dmacbptr->targbufs->count >>= 1;
				--dmacbptr->targbufs->count;
			} else {
				tcount = (dmacbptr->targbufs->count_hi << 16) |
					  dmacbptr->targbufs->count;
				tcount = (tcount >> 1) - 1;
				dmacbptr->targbufs->count = tcount & 0xFFFF;
				dmacbptr->targbufs->count_hi = (tcount >> 16) & 0xFFFF;
			}

			/*
			 * save the channel target path to get correct channel
			 * count state. 
			 */
			d37A_targ_path[chan] = dmacbptr->targ_path;

			dMCA_prog_extmode(
				dmacbptr->targbufs->address,
				((dmacbptr->targbufs->count_hi << 16) |
				 (dmacbptr->targbufs->count)),
				dmacbptr->command,
				chan);

			return(TRUE);

		default:
			dprintf(("d37A_prog_chan: channel %d target path not supported.\n", chan));
			return(FALSE);

		} /* end switch */
	}

	/* save the channel target path to get correct channel count state */
	d37A_targ_path[chan] = dmacbptr->targ_path;

	switch (dmacbptr->targ_path) {
	case DMA_PATH_8:
	case DMA_PATH_16B:
	case DMA_PATH_32:
		/*
		 * All of the "count by byte" modes require the count
		 * to be one less than the number of bytes to be transferred.
		 */
		if (dmacbptr->targbufs->count_hi == 0) {
			--dmacbptr->targbufs->count;
		} else {
			tcount = ((dmacbptr->targbufs->count_hi << 16) |
				   dmacbptr->targbufs->count) - 1;
			dmacbptr->targbufs->count = tcount & 0xFFFF;
			dmacbptr->targbufs->count_hi = (tcount >> 16) & 0xFFFF;
		}
		break;

	case DMA_PATH_16:
		/*
		 * The base address register must be programmed to an even
		 * address for "16-bit I/O, count by word (address shifted)"
		 * mode.  The count must also be an even number of bytes.
		 */
		if (((addr = dmacbptr->targbufs->address) |
		      dmacbptr->targbufs->count) & 0x01) {
			dprintf(("d37A_prog_chan: channel %d not programmed.\n",
				chan));
			return(FALSE);
		}

		/*
		 * "16-bit I/O, count by word" mode requires the count to be
		 * one less than the number of 16 bit words to be transferred.
		 * In other words, 
		 *		count = (count / 2) - 1
		 */
		if (dmacbptr->targbufs->count_hi == 0) {
			dmacbptr->targbufs->count >>= 1;
			--dmacbptr->targbufs->count;
		} else {
			tcount = (dmacbptr->targbufs->count_hi << 16) |
				  dmacbptr->targbufs->count;
			tcount = (tcount >> 1) - 1;
			dmacbptr->targbufs->count = tcount & 0xFFFF;
			dmacbptr->targbufs->count_hi = (tcount >> 16) & 0xFFFF;
		}

		/*
		 * Shift the address bits as appropriate for "16-bit I/O,
		 * count by word" mode.
		 */
		addr = (addr & ~0x1ffff) | ((addr & 0x1ffff) >> 1);
		dmacbptr->targbufs->address = (paddr_t)addr;
		break;

	default:
		dprintf(("d37A_prog_chan: channel %d target path not supported.\n", chan));
		return(FALSE);
	}

	opl = LOCK(d37A_dmaclk, DMACPL);

	/* keep the channel quiet while programming it */
	D37A_DMA_DISABLE(chan);

	d37A_mode_set(dmacbptr, chan);

	d37A_write_target(dmacbptr->targbufs->address, chan);

	if (dmacbptr->targbufs->count_hi == 0) {
		d37A_write_count(dmacbptr->targbufs->count, chan);
	} else {
		d37A_write_count(((dmacbptr->targbufs->count_hi << 16) |
				   dmacbptr->targbufs->count), chan);
	}

	if (Eisa_dma && dmacbptr->bufprocess == DMA_BUF_CHAIN) {
		eisa_curbuf[chan] = dmacbptr->targbufs;
		dEISA_setchain(chan);
	}

	UNLOCK(d37A_dmaclk, opl);

	return(TRUE);
}


/*
 * void
 * d37A_dma_enable(int chan)
 *	Enable to DMAC to respond to hardware requests for DMA
 *	service on the specified channel.
 *
 * caller:	dma_enable()
 * calls:	none
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit.
 */
void
d37A_dma_enable(int chan)
{
	pl_t	opl;

	dprintf(("d37A_dma_enable:chan = %x ,mask_reg = %x, val = %x\n", 
			chan, chan_addr[chan].mask_reg, chan & 0x03));

	opl = LOCK(d37A_dmaclk, DMACPL);

	if (d37A_bustype == BUS_MCA && d37A_targ_path[chan] == DMA_PATH_16) {
		/* enable a HW request to be seen */
		DMCA_DMA_ENABLE(chan);
		UNLOCK(d37A_dmaclk, opl);
		return;
	}

	/* enable a HW request to be seen */
	D37A_DMA_ENABLE(chan);

	UNLOCK(d37A_dmaclk, opl);
}


/*
 * void
 * d37A_dma_disable(int chan)
 *	Prevent the DMAC from responding to external hardware
 *	requests for DMA service on the given channel.
 *
 * caller:	dma_disable()
 * calls:	d37A macros
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit.
 */
void
d37A_dma_disable(int chan)
{
	pl_t	opl;

	dprintf(("d37A_dma_disable:chan = %x, mask_reg = %x, val = %x\n",
		chan, chan_addr[chan].mask_reg, DMA_SETMSK | chan & 0x03));

	opl = LOCK(d37A_dmaclk, DMACPL);

	if (d37A_bustype == BUS_MCA && d37A_targ_path[chan] == DMA_PATH_16) {
		/* mask off subsequent HW requests */
		DMCA_DMA_DISABLE(chan);
		UNLOCK(d37A_dmaclk, opl);
		return;
	}

	/* mask off subsequent HW requests */
	D37A_DMA_DISABLE(chan);

	UNLOCK(d37A_dmaclk, opl);
}


/*
 * int
 * d37A_cascade(int chan)
 *	Program the Mode registers of the DMAC for cascade mode.
 *
 * caller:	dma_cascade()
 * calls:	d37A_mode_set
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit.
 */
int
d37A_cascade(int chan)
{
	static struct dma_cb cascade_cb = {
		NULL, NULL, NULL,
		0, DMA_TYPE_IO, 0, 0, 0,
		DMA_TRANS_CSCD, 0, 0,
		DMA_CYCLES_1
	};
	pl_t	opl;

	dprintf(("d37A_cascade: programming channel %d for cascade mode.\n",
		    chan));

	if (chan == DMA_CH4) {
		dprintf(("d37A_cascade: channel %d not supported.\n", chan));
		return(FALSE);
	}

	opl = LOCK(d37A_dmaclk, DMACPL);

	/* keep the channel quiet while programming it */
	D37A_DMA_DISABLE(chan);
	d37A_mode_set(&cascade_cb, chan);
	D37A_DMA_ENABLE(chan);

	UNLOCK(d37A_dmaclk, opl);

	return(TRUE);
}


/*
 * int
 * d37A_dma_swsetup(struct dma_cb *dmacbptr, int chan)
 *	Program the Mode registers and the Base register for the
 *	specified channel.
 *
 * caller:	dma_swsetup()
 * calls:	d37A_prog_chan()
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit.
 */
int
d37A_dma_swsetup(struct dma_cb *dmacbptr, int chan)
{
	dprintf(("d37A_dma_swsetup: set up channel %d, dmacbptr= %x.\n",
		chan, (ulong_t) dmacbptr));

	if (d37A_bustype == BUS_MCA) {
		dprintf(("d37A_dma_swsetup: MCA based systems cannot program the DMA for software initiated requests.\n"));
		return(FALSE);
	}

	/* MUST BE IN BLOCK MODE FOR SOFTWARE INITIATED REQUESTS */
	dmacbptr->trans_type = DMA_TRANS_BLCK;

	/* prepare for a software operation on the specified channel */
	return(d37A_prog_chan(dmacbptr, chan));
}


/*
 * void
 * d37A_dma_swstart(int chan)
 *	SW start transfer setup on the indicated channel.
 *
 * caller:	dma_swstart()
 * calls:	d37A_dma_enable(), d37A macros
 *
 * Calling/Exit State:
 *	None.
 */
void
d37A_dma_swstart(int chan)
{
	pl_t	opl;

	dprintf(("d37A_dma_swstart: software start channel %d.\n", chan));

	opl = LOCK(d37A_dmaclk, DMACPL);

	/* enable the channel and kick it into gear */
	D37A_DMA_ENABLE(chan);

	/* set request bit */
	outb(chan_addr[chan].reqt_reg, DMA_SETMSK | chan);

	UNLOCK(d37A_dmaclk, opl);
}


/*
 * void
 * d37A_dma_stop(int chan)
 *	Stop any activity on the indicated channel.
 *
 * caller:	dma_stop()
 * calls:	d37A_dma_disable() 
 *
 * Calling/Exit State:
 *	None.
 */
void
d37A_dma_stop(int chan)
{
	pl_t	opl;

	dprintf(("d37A_dma_stop: stop channel %d.\n", chan));

	opl = LOCK(d37A_dmaclk, DMACPL);

	/* stop whatever is going on channel chan */
	D37A_DMA_DISABLE(chan);

	/* reset request bit */
	outb(chan_addr[chan].reqt_reg, chan & 0x03);

	UNLOCK(d37A_dmaclk, opl);
}


/*
 * void
 * d37A_get_chan_stat(struct dma_stat *dmastat, int chan)
 *	Retrieve the Current Address and Count registers for the
 *	specified channel.
 *
 * caller:	dma_get_chan_stat()
 * calls:	d37A_read_target(), d37A_read_count()
 *
 * Calling/Exit State:
 *	- channel pseudo sleep lock held on entry & exit.
 */
void
d37A_get_chan_stat(struct dma_stat *dmastat, int chan)
{
	ulong_t	tcount;
	pl_t	opl;

	/* read the Current Registers for channel chan */

	dprintf(("d37A_get_chan_stat: retrieve channel %d's status.\n", chan));

	opl = LOCK(d37A_dmaclk, DMACPL);

	dmastat->targaddr = d37A_read_target(chan);

	tcount = d37A_read_count(chan);

	UNLOCK(d37A_dmaclk, opl);

	switch (d37A_targ_path[chan]) {
	case DMA_PATH_8:
	case DMA_PATH_16B:
	case DMA_PATH_32:
		/*
		 * Remember, we decremented the user supplied count
		 * for these channels, so we have to add one back
		 * to provide a consistent view to the user.
		 */
		tcount = tcount + 1;
		break;

	case DMA_PATH_16:
		/*
		 * For 16-bit count by word mode we first have to 
		 * multiply the count by 2 and then increment the
		 * the user supplied count to provide a consistent
		 * view to the user.
		 */
		tcount = (tcount << 1) + 1;
		break;

	default:
		tcount = 0;
		break;
	}

	dmastat->count = tcount & 0xFFFF;
	dmastat->count_hi = (tcount << 16) & 0xFFFF;

	dprintf(("d37A_get_chan_stat: channel %d's status:\n", chan));
	dprintf(("d37A_get_chan_stat:\ttarget=    %x\n", dmastat->targaddr));
	dprintf(("d37A_get_chan_stat:\tcount=     %x\n",
				TOLONG(dmastat->count, dmastat->count_hi)));
}


/*
 * void
 * d37A_physreq(int chan, int datapath, physreq_t *preqp)
 *	Apply constraints appropriate for the given channel and path size
 *	to the physical requirements structure at (*preqp).
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
d37A_physreq(int chan, int datapath, physreq_t *preqp)
{
	/* require physically-contiguous memory */
	preqp->phys_flags |= PREQ_PHYSCONTIG;

	/* scatter/gather not supported */
	preqp->phys_max_scgth = 0;

	if (Eisa_dma) {
		preqp->phys_dmasize = 24;	/* L001 */
		return;
	}

	if (datapath == DMA_PATH_8) {
		preqp->phys_align = 1;
		preqp->phys_boundary = 64 * 1024;
		preqp->phys_dmasize = 24;
	} else if (datapath == DMA_PATH_16) {
		preqp->phys_dmasize = 24;
		preqp->phys_boundary = 128 * 1024;
		preqp->phys_align = 2;
	}
}
