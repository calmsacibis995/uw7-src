#ident "@(#)e3Dmac.c	28.1"
#ident "$Header$"

/*
 *		Copyright (C) The Santa Cruz Operation, 1993-1997.
 *		This Module contains Proprietary Information of
 *		The Santa Cruz Operation and should be treated
 *		as Confidential.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.	The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */

/*
 *	(C) Copyright 1991 SCO Canada, Inc.
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.	The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	SCO Canada, Inc.	This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by SCO Canada, Inc.
 *
 */

#ifdef _KERNEL_HEADERS

#include <io/nd/mdi/e3D/e3D.h>

#else

#include "e3D.h"

#endif

extern int e3Ddevflag;

STATIC void	e3Dstartru();

/*
 * Valid RAM configuration widow base and window size settings.
 */
struct e3Draminfo {
	ushort	ram_setting;	/* ram configuration register bits */
	caddr_t	ram_base;	/* ram window base */
	u_long	ram_size;	/* ram window size */
};
STATIC struct e3Draminfo	e3Draminfo[] = {
	{ HRAM_0C0_16KB, (caddr_t) 0x0C0000, 16*1024, },
	{ HRAM_0C0_32KB, (caddr_t) 0x0C0000, 32*1024, },
	{ HRAM_0C0_48KB, (caddr_t) 0x0C0000, 48*1024, },
	{ HRAM_0C0_64KB, (caddr_t) 0x0C0000, 64*1024, },
	
	{ HRAM_0C8_16KB, (caddr_t) 0x0C8000, 16*1024, },
	{ HRAM_0C8_32KB, (caddr_t) 0x0C8000, 32*1024, },
	{ HRAM_0C8_48KB, (caddr_t) 0x0C8000, 48*1024, },
	{ HRAM_0C8_64KB, (caddr_t) 0x0C8000, 64*1024, },
	
	{ HRAM_0D0_16KB, (caddr_t) 0x0D0000, 16*1024, },
	{ HRAM_0D0_32KB, (caddr_t) 0x0D0000, 32*1024, },
	{ HRAM_0D0_48KB, (caddr_t) 0x0D0000, 48*1024, },
	{ HRAM_0D0_64KB, (caddr_t) 0x0D0000, 64*1024, },
	{ HRAM_0D8_16KB, (caddr_t) 0x0D8000, 16*1024, },
	{ HRAM_0D8_32KB, (caddr_t) 0x0D8000, 32*1024, },
	
	{ HRAM_F00_64KB, (caddr_t) 0xF00000, 64*1024, },
	{ HRAM_F20_64KB, (caddr_t) 0xF20000, 64*1024, },
	{ HRAM_F40_64KB, (caddr_t) 0xF40000, 64*1024, },
	{ HRAM_F60_64KB, (caddr_t) 0xF60000, 64*1024, },
	{ HRAM_F80_64KB, (caddr_t) 0xF80000, 64*1024, },
};
#define	RAMINFO_CNT (sizeof(e3Draminfo)/sizeof(struct e3Draminfo))

STATIC struct e3Drominfo {
	u_short rom_setting;	/* rom configuration register bits */
	u_long	rom_size;		/* rom window size */
};

STATIC struct e3Drominfo	e3Drominfo[] = {
	{ HROM_NONE, 0, },
	{ HROM_08KB, 8*1024, },
	{ HROM_16KB, 16*1024, },
	{ HROM_32KB, 32*1024, },
};
#define ROMINFO_CNT (sizeof(e3Drominfo)/sizeof(struct e3Drominfo))


/*
 * Valid interrupt request level settings.
 */
struct e3Dirinfo {
	ushort	ir_setting;	/* interrupt configuration register bits */
	ushort	ir_level;	/* interrupt request level */
};
STATIC struct e3Dirinfo e3Dirinfo[] = {
	{ HINT_IRL3,	3, },
	{ HINT_IRL5,	5, },
	{ HINT_IRL7,	7, },
	{ HINT_IRL9,	9, },
	{ HINT_IRL10,	10, },
	{ HINT_IRL11,	11, },
	{ HINT_IRL12,	12, },
	{ HINT_IRL15,	15, },
};
#define	IRINFO_CNT (sizeof(e3Dirinfo)/sizeof(struct e3Dirinfo))

/*
 * Function: e3Dtest_init_complete
 * 
 * Purpose:
 *	Return whether 82586 has completed initialization.
 */
STATIC int
e3Dtest_init_complete(struct scb *scb)
{
	return (scb->scb_status == RESET_OK_BITS ? TRUE : FALSE);
}

/*
 * Function: e3Dtest_cmd_clear
 * 
 * Purpose:
 *	Return whether the SCB command word has cleared, signifying
 *	that the 82586 has no more pending control commands.
 */
STATIC int
e3Dtest_cmd_clear(struct scb *scb)
{
	return (scb->scb_command == 0 ? TRUE : FALSE);
}

/*
 * Function: e3Dtest_cmd_complete
 *
 * Purpose:
 *	Return whether the command status indicates that the command
 *	has completed.
 */
STATIC int
e3Dtest_cmd_complete(union cbu *cbu)
{
	return ((cbu->cbl.cbl_status & CBL_C_BIT) ? TRUE : FALSE);
}

/*
 * Function: e3Dtest_ru_ready
 * 
 * Purpose: 
 *	Return whether the receive unit is ready.
 */
STATIC int
e3Dtest_ru_ready(struct scb *scb)
{
	return (((scb->scb_status & SCB_RUS_MASK) == SCB_RUS_READY) 
		? TRUE : FALSE);
}

/*
 * Function: e3Dpoll
 *
 * Purpose:
 *	continue to call (*func)() until it returns TRUE, or timeout_length
 *	useconds have elapsed.
 *
 * Parameters:
 *	func:		a routine which returns TRUE when
 *			its condition is satisfied, FALSE otherwise
 *	arg:		argument passed to func, unconditionally
 *	timeout_length:	positive timeout value in ticks (1/100th)
 *
 * Returns:
 *	TRUE:		func returned TRUE before timeout_length
 *	FALSE:		timeout_length expired before func returned TRUE
 *
 * Notes:
 * 1) The test below "while (lbolt - start_time <= timeout_length)" means that 
 *	this spin loop will execute for a maximum time of at least 
 *	"timeout_length" ticks.	The "at least" part is the residual time left 
 *	in the current tick.	Note: lbolt can be negative, so the test was
 *	changed to use subtraction, which in 2's complement arithmetic yields
 *	the desired result.
 *
 * 2) The motivation behind this routine is to give a processor-speed
 *	independent method of polling a hardware register for a bounded 
 *	period of time.	The assumption is that if e3Dpoll() returns
 *	FALSE, the h/w has gone belly up, so the only thing left to do
 *	is to shut the h/w down, and do some sort of error handling.
 */
STATIC int
e3Dpoll(int (*func)(), void *arg, int timeout_length)
{
	clock_t start_time=TICKS();
	
	while (TICKS() - start_time <= timeout_length) {
		if ((*func)(arg) == TRUE)
			return (TRUE);
	}

	return (FALSE);
}

/*
 * Function: e3Dmemvalid
 *
 * Purpose:
 *	Test the newly mapped in memory to see if we can write/read
 *	to it.
 *
 * Returns:
 *	TRUE if memory valid, FALSE otherwise.
 */
STATIC int
e3Dmemvalid(caddr_t startp, int len)
{
	register ushort *cp;
	register ushort *end;
	
	cp = (ushort *) startp;
	end = cp + (len / sizeof(ushort));
	while (cp < end)
		*(cp++) = MEMTST_PATTERN;
	
	cp = (ushort *) startp;
	while (cp < end) {
		if (*(cp++) != MEMTST_PATTERN) {
			DEBUG00(CE_CONT, "memtst failed i %d => %x\n",
				cp - (ushort *) startp, *cp);
			return (FALSE);
		}
	}
	
	cp = (ushort *) startp;
	while (cp < end)
		*(cp++) = 0;
	return (TRUE);
}

/*
 * Function: e3Disvalid_3COM_sig
 *
 * Purpose:
 *	Ensure that the signature is a valid 3COM signature.
 *
 * Returns:
 *	TRUE if valid, FALSE otherwise.
 *
 */
STATIC int
e3Disvalid_3COM_sig(e3Ddev_t *dv)
{
	static char 	e3Dsig[] = "*3COM*";
	int		i;
	
	DEBUG00(CE_CONT, "signature x%b x%b x%b x%b x%b x%b\n", 
		inb(ED_NM_DATA(dv)+0),
		inb(ED_NM_DATA(dv)+1), inb(ED_NM_DATA(dv)+2), 
		inb(ED_NM_DATA(dv)+3), inb(ED_NM_DATA(dv)+4), 
		inb(ED_NM_DATA(dv)+5));
	
	for (i = 0; i < HNM_SIZE; i++) {
		if ((inb(ED_NM_DATA(dv) + i) & 0xff) != e3Dsig[i]) {
			DEBUG00(CE_CONT, "e3D: Incorrect 3COM signature\n");
			return (FALSE);
		}
	}
	
	return (TRUE);
}

/*
 * Function: e3Dfind_ram_setting
 *
 * Purpose:
 *	Return the shared ram information for the given ram setting.
 *
 * Returns:
 *	Pointer to ram information structure if found, NULL otherwise.
 */
STATIC struct e3Draminfo *
e3Dget_ram_info(int ram_setting)
{
	int	i;
	
	for (i = 0; i < RAMINFO_CNT; i++)
		if (ram_setting == e3Draminfo[i].ram_setting)
			return (e3Draminfo + i);
	return (NULL);
}

/*
 * Function: e3Dget_irq
 *
 * Purpose:
 *	Find the irq for the given interrupt setting.
 *
 * Returns:
 *	The irq level if found, -1 otherwise.
 */
STATIC int
e3Dget_irq(int ir_setting)
{
	int	i;
	
	for (i = 0; i < IRINFO_CNT; i++)
		if (ir_setting == e3Dirinfo[i].ir_setting)
			return (e3Dirinfo[i].ir_level);
	
	return (-1);
}

/*
 * Function: e3Dread_eaddr
 *
 * Purpose:
 *	Read the ethernet mac address from the card's EEPROM.
 */
STATIC void
e3Dread_eaddr(e3Ddev_t *dv, macaddr_t ea)
{
	int	i;
	
	SELECT_EADDR(dv);
	for (i = 0; i < sizeof(macaddr_t); i++)
		ea[i] = inb(ED_NM_DATA(dv) + i);
	SELECT_SIGNAT(dv);
}

/*
 * Function: e3Dpresent
 *
 * Purpose: 
 *	This is the probe/config routine for the e3D (3COM 507) network
 *	adpater.  This routine expects the I/O base address to be set in  
 *	the e3Ddev_t pointer passed in.	This base address is used to check for
 *	the board's presence, then this routine reads the rest of the
 *	configuration from the adpater.
 *
 * Returns:
 *	Fills in information in the e3Ddev_t structure:
 *	ex_int
 *	ex_base
 *	ex_memsize
 *	ex_rambase
 *	ex_creg2
 *	ex_romr2
 *	ex_ramr2
 *	ex_intr2
 *	ex_eaddr
 *
 *	Returns TRUE if board is present/OK, FALSE otherwise.
 *
 * Notes:
 *
 *	At this point the board must be already configured
 *	with the parameters read from the on-board EEPROM and
 *	must be in the RUN state. The control register must be 
 *	in the default state (all zeroes).
 *
 *	The driver completely relies on the configuration
 *	parameters stored in the on-board EEPROM.
 */
int
e3Dpresent(e3Ddev_t *dv, int fromverify)
{
	int			ir_level, i;
	int			ir_setting;
	struct e3Draminfo	*ram_info;
	int			ram_setting;
	u_short			rom_size;		/* rom window size */
	struct e3Drominfo	*romp;
	
	dv->ex_rambase = 0;
	
	/* make sure that this is an e3D card we're talking to
	* Since the verify routine is called multiple times only whine
	* if we're in the init routine
	*/
	if (e3Disvalid_3COM_sig(dv) == FALSE) {
		if (fromverify == 0) {
			cmn_err(CE_WARN,"e3D: NO BOARD PRESENT AT 0x%x",
				dv->ex_ioaddr);
		}
		return (FALSE);
	}
	
	/* initialize copies of board registers */
	DEBUG00(CE_CONT, "probe: ED_CNTRL is at x%x\n", ED_CNTRL(dv));
	dv->ex_creg2 = ((u_char) inb(ED_CNTRL(dv))) | HCTL_RST; 
	dv->ex_romr2 = (u_char) inb(ED_ROM_CONF(dv));
	dv->ex_ramr2 = (u_char) inb(ED_RAM_CONF(dv));
	dv->ex_intr2 = (u_char) inb(ED_INT_CONF(dv));
	DEBUG00(CE_CONT, "cr x%b romr x%b ramr x%b icr x%b\n", dv->ex_creg2,
		dv->ex_romr2, dv->ex_ramr2, dv->ex_intr2);
	
	/*
	* Initialize the Control Register
	* The 8 bit setting only takes effect if Standard mode is
	* used in the setup program, otherwise 16-bit operation takes place
	* automatically.	Essentially, this setup is required to support
	* the 8/16 bit mode toggle in the setup program.
	*/
	DEVICE_8BIT(dv);
	
	/* initialize the ram setting */
	ram_setting = GET_RAMCONF(dv);
	if ((ram_info = e3Dget_ram_info(ram_setting)) == NULL) {
		cmn_err(CE_WARN, "e3D: INVALID RAM CONFIGURATION RC5-RC0=0x%b", 
			ram_setting);
		return (FALSE);
	}
	dv->ex_base = ram_info->ram_base;
	dv->ex_memsize = ram_info->ram_size;
	DEBUG00(CE_CONT, "base x%x size %d\n", dv->ex_base, dv->ex_memsize);
	
	/* map the board memory into virtual address space */
	/* NOTES on devmem_mapin:
	 * - it is the replacement for physmap
	 * - the memory block number is the nth value of MEMADDR in the resmgr.
	 *   For ISA devices MEMADDR is usually not multivalued but it is for 
	 *   some PCI/EISA devices.  Use the "resdump" command to ndcfg to see
	 *   the memory block number you're referring to.  We always use 0.
	 * - the offset is simply the offset within the memory block. We use 0.
	 * - devmem_mapin uses KM_SLEEP.
	 * - if coming from verify routine then MEMADDR won't be set at the key
	 *   so set it ourselves before the call to devmem_mapin so that
	 *   it will succeed
	 * - if not coming from verify routine then MEMADDR had better be set
	 */
	if (fromverify == 1) {
		struct cm_args cm_args;
		cm_range_t  range;

		range.startaddr = (long) dv->ex_base;
		range.endaddr = (long) (dv->ex_base + dv->ex_memsize - 1);
		cm_args.cm_key = dv->rmkey;
		cm_args.cm_n = 0;   /* to match devmem_mapin call below */
		cm_args.cm_param = CM_MEMADDR;
		cm_args.cm_val = &range;
		cm_args.cm_vallen = sizeof(cm_range_t);

		cm_begin_trans(cm_args.cm_key, RM_RDWR);
		/* if by some fluke it was there before delete it */
		(void) cm_delval(&cm_args);  
		if (cm_addval(&cm_args)) {
			cm_abort_trans(cm_args.cm_key);
			cmn_err(CE_WARN,"e3Dpresent: cm_addval for MEMADDR "
					"at key %d failed ", cm_args.cm_key);
			return(FALSE);
		}
		cm_end_trans(cm_args.cm_key);
	}

	/* devmem_size(D3) does not exist yet in the kernel so we can't call it 
	 * here - but we should.  It will subtract the range represented by the 
	 * nth MEMADDR value, returning the difference as a size_t.
	 */
#if 1
	if ((dv->ex_rambase=(caddr_t) devmem_mapin(dv->rmkey, 0, 0, 
				dv->ex_memsize)) == NULL) {
#endif
#if 0
	 if ((dv->ex_rambase=(caddr_t) physmap((paddr_t)dv->ex_base,dv->ex_memsize,
	 				KM_SLEEP)) == NULL) openbrace
#endif
	/* equivalent with physmap, except it isn't legal with ddi8 any more
	 * note that as of 1 October 1997 devmem_mapin doesn't work properly so
	 * you must use physmap directly (add $interface base to Master too).
	 * Note that this should be fixed by FCS.
	 * if ((dv->ex_rambase=(caddr_t) physmap((paddr_t)dv->ex_base,dv->ex_memsize,
	 * 				KM_SLEEP)) == NULL) openbrace
	 */
		cmn_err(CE_WARN, "e3Dpresent: devmem_mapin(0x%x) failed", dv->ex_base);
		return (FALSE);
	}
	DEBUG00(CE_CONT, "rambase x%x\n", dv->ex_rambase);

	/* retrieve extra parameters needed by verify routine for later on */
	dv->ex_bnc = dv->ex_romr2 & HROM_BNC;
	dv->ex_sad = (inb(ED_RAM_CONF(dv)) & HRAM_SAD);
	rom_size = GET_ROMSIZE(dv);
	dv->ex_rombase = (caddr_t) GET_ROMADDR(dv);
	for (romp = e3Drominfo; romp < &e3Drominfo[ROMINFO_CNT]; romp++) {
		if (rom_size == romp->rom_setting) {
			dv->ex_romsize = romp->rom_size;
			break;
		}
	}
	dv->ex_zerows = GET_0WS(dv);

	/* Get the board's part number. */
	SELECT_PARTNO(dv);
	for (i = 0; i < 6; i++) {
		dv->ex_partno[i] = inb(ED_NM_DATA(dv)+i);
	}
	
	/*
	 * verify board's memory, unless we are from e3D_verify -- there may be
	 * a conflict with another board and ndcfg will sort this out later by 
	 * peering into the resmgr to prevent the user from choosing this range.
	 */
	if (fromverify == 0) {
		DEBUG00(CE_CONT, "Started memory test\n");
		if (e3Dmemvalid(dv->ex_rambase, dv->ex_memsize) == FALSE) {
			cmn_err( CE_WARN, "e3D: MEMORY TEST FAILED "
				"(0x%x - 0x%x)", dv->ex_base,
				dv->ex_base + dv->ex_memsize - 1);
			return (FALSE);
		}
		DEBUG00(CE_CONT, "Memory tests passed\n");
	} else {
		DEBUG00(CE_CONT, "Skipping memory tests\n");
	}
	
	/* read the interrupt level */
	ir_setting = GET_INTLEVEL(dv);
	if ((ir_level = e3Dget_irq(ir_setting)) == -1) {
		cmn_err(CE_WARN,"e3D: INVALID INTERRUPT LEVEL IL3-IL0=0x%b",
			 ir_setting);
		return (FALSE);
	}
	dv->ex_int = ir_level;
	
	e3Dread_eaddr(dv, dv->ex_eaddr);
	DEBUG00(CE_CONT, "eaddr x%b x%b x%b x%b x%b x%b\n",
		dv->ex_eaddr[0], dv->ex_eaddr[1], dv->ex_eaddr[2], 
		dv->ex_eaddr[3], dv->ex_eaddr[4], dv->ex_eaddr[5]);
	
	return (TRUE);
}

/*
 * Function: e3Dhwinit
 * 
 * Purpose:
 *	Set up the 82586 data structures in the shared memory,
 *	initialize the hardware to a known, running state.
 */
int
e3Dhwinit(e3Ddev_t *dv)
{
	struct	control		*cntl;
	struct	scp		*scp;
	struct	iscp		*iscp;
	struct	scb		*scb;
	union	cbu		*cbu;
	struct	rfd		*rfd;
	struct	rbd		*rbd;
	struct	tcb		*tcb;
	struct	tbd		*tbd;
	register int		i;
	long			memleft;
	caddr_t			memfree;
	int			n16kblocks;
	
	DEBUG01(CE_CONT, "hwinit: ram 0x%x len %d\n", 
		dv->ex_rambase, dv->ex_memsize);
	ASSERT(dv->ex_memsize <= BRD_MAXWINSIZE);
	ASSERT(dv->ex_memsize >= BRD_MINWINSIZE);
	
	dv->ex_flags = 0;
	
	/*
	 * Depending on the size of the memory window, determine the
	 * location and count of the control structures for the 82586.
	 * SCP is always at the end of the memory window.
	 */
	memfree = dv->ex_rambase;
	memleft = dv->ex_memsize - sizeof(struct scp);
	dv->ex_control = (struct control *)memfree;
	memfree += sizeof(struct control);
	memleft -= sizeof(struct control);
#ifdef DUMP_82586_REGS
	dv->ex_dumpbuf = (dump_buf *)memfree;
	memfree += sizeof(dump_buf);
	memleft -= sizeof(dump_buf);
#endif
	n16kblocks = dv->ex_memsize / BRD_MINWINSIZE;
	dv->ex_ntcb = TCB_MIN * n16kblocks;
	dv->ex_tcb = (struct tcb *)memfree;
	memfree += sizeof(struct tcb) * dv->ex_ntcb;
	memleft -= sizeof(struct tcb) * dv->ex_ntcb;
	dv->ex_ntbd = TBD_MIN * n16kblocks;
	dv->ex_tbd = (struct tbd *)memfree;
	memfree += sizeof(struct tbd) * dv->ex_ntbd;
	memleft -= sizeof(struct tbd) * dv->ex_ntbd;
	dv->ex_ntbuff = dv->ex_ntbd;
	dv->ex_tbuff = (struct tbuff *)memfree;
	memfree += sizeof(struct tbuff) * dv->ex_ntbuff;
	memleft -= sizeof(struct tbuff) * dv->ex_ntbuff;
	dv->ex_nrfd = RFD_MIN * n16kblocks;
	dv->ex_rfd = (struct rfd *)memfree;
	memfree += sizeof(struct rfd) * dv->ex_nrfd;
	memleft -= sizeof(struct rfd) * dv->ex_nrfd;
	dv->ex_nrbd = memleft / (sizeof(struct rbd)+sizeof(struct rbuff));
	dv->ex_rbd = (struct rbd *)memfree;
	memfree += sizeof(struct rbd) * dv->ex_nrbd;
	memleft -= sizeof(struct rbd) * dv->ex_nrbd;
	dv->ex_nrbuff = dv->ex_nrbd;
	dv->ex_rbuff = (struct rbuff *)memfree;
	memfree += sizeof(struct rbuff) * dv->ex_nrbuff;
	memleft -= sizeof(struct rbuff) * dv->ex_nrbuff;
	
	DEBUG01(CE_CONT, "cntl=0x%x cntlsz=%d n16k=%d\n", dv->ex_control,
		sizeof(struct control), n16kblocks);
#ifdef DUMP_82586_REGS
	DEBUG01(CE_CONT, "dump=0x%x dumpsz=%d\n", dv->ex_dumpbuf, sizeof(dump_buf));
#endif
	DEBUG01(CE_CONT, "ntcb=%d tcb=0x%x tcbsz=%d\n", dv->ex_ntcb,
		dv->ex_tcb, sizeof(struct tcb));
	DEBUG01(CE_CONT, "ntbd=%d tbd=0x%x tbdsz=%d\n", dv->ex_ntbd,
		dv->ex_tbd, sizeof(struct tbd));
	DEBUG01(CE_CONT, "ntbuff=%d tbuff=0x%x tbuffsz=%d\n", dv->ex_ntbuff,
		dv->ex_tbuff, sizeof(struct tbuff));
	DEBUG01(CE_CONT, "nrfd=%d rfd=0x%x rfdsz=%d\n", dv->ex_nrfd,
		dv->ex_rfd, sizeof(struct rfd));
	DEBUG01(CE_CONT, "nrbd=%d rbd=0x%x rbdsz=%d\n", dv->ex_nrbd,
		dv->ex_rbd, sizeof(struct rbd));
	DEBUG01(CE_CONT, "nrbuff=%d rbuff=0x%x rbuffsz=%d\n", dv->ex_nrbuff,
		dv->ex_rbuff, sizeof(struct rbuff));
	DEBUG01(CE_CONT, "memfree=x%x memleft=%d\n", memfree, memleft);
	
	/*
	* Now we must set up the control structures for the 82586 on
	* the board, and kick it to life.
	* The last byte of the RAM window accesible by the host always
	* appears at address 0xFFFF (highest) in the 82586's address space.
	*/
	
	/* Set up the System Control Pointer */
	scp = (struct scp *) BADDR_TO_VADDR(dv, SCP_BADDR);
	bzero(scp, sizeof(struct scp));
	scp->scp_sysbus = SYSBUS_16BIT;
	scp->scp_iscpaddrl = VADDR_TO_BADDR(dv, dv->ex_control);
	scp->scp_iscpaddrh = 0;
	
	DEBUG01(CE_CONT, "scp=x%x bus=x%x adrl=x%x adrh=x%x\n", scp, scp->scp_sysbus,
		scp->scp_iscpaddrl, scp->scp_iscpaddrh);
	
	cntl = dv->ex_control;
	/* bzero(cntl, sizeof (struct control)): sizeof was size 0x401c, larger than
	 * our devmem_mapin/physmap'ed memory, causing panic when memory = 16k
	 * we lower E3D_NMCADDR in e3D.h to bring sizeof struct control back down
	 * to under 16k and bzero out the entire physmap area
	 * instead of first 16k to eliminate test pattern garbage
	 * old way: bzero(cntl, sizeof (struct control));
	 * the old way worked fine when the ramsize is > 16k
	 */
	bzero(cntl, dv->ex_memsize);
	
	/* Set up the Immediate System Control Pointer */
	iscp = &cntl->iscp;
	iscp->iscp_busy = 1;
	iscp->iscp_reserved = 0;
	iscp->iscp_scboffset = sizeof(struct iscp);
	iscp->iscp_scbbasel = VADDR_TO_BADDR(dv, cntl);
	iscp->iscp_scbbaseh = 0;
	
	DEBUG01(CE_CONT, "iscp=x%x ofs=x%x basel=x%x baseh=x%x\n", iscp,
		iscp->iscp_scboffset, iscp->iscp_scbbasel, 
		iscp->iscp_scbbaseh);
	
	/* Set up the System Control Block */
	/*
	 * It should be noted that we are setting things up with 2 command
	 * buffer chains.
	 *
	 * The Command Block chain is only one element long, and is used by
	 * the driver itself to perform some board action (like setting the
	 * ethernet address), or multicast set-up.
	 *
	 * The Transmit Control block chain is a linked list of transmit
	 * command buffers.	This is the chain that is used for putting
	 * packets onto the ethernet.
	 *
	 * We initialize the scb pointing to the CBL since we are soon going
	 * to issue DIAGNOSE and IASETUP commands and will need our special
	 * buffer.
	 */
	scb = &cntl->scb;
	scb->scb_cbl_ofst = VADDR_TO_BOFF(dv, &cntl->cbu);
	scb->scb_rfd_ofst = VADDR_TO_BOFF(dv, dv->ex_rfd);
	
	DEBUG01(CE_CONT, "scb=x%x cblofs=x%x rfdofs=x%x\n", scb, scb->scb_cbl_ofst,
		scb->scb_rfd_ofst);
	
	/* Set up the Command Block */
	cbu = &cntl->cbu;
	cbu->cbl.cbl_command = CBL_EL_BIT;
	cbu->cbl.cbl_link = VADDR_TO_BOFF(dv, cbu);
	
	/* Set up kernel pointer */
	dv->ex_cbuhead = cbu;
	
	/* Set up the Transmit Command Blocks */
	tcb = dv->ex_tcb;
	for (i = 0; i < dv->ex_ntcb; i++, tcb++) {
		tcb->tcb_status = 0;
		tcb->tcb_command = CMD_TRANSMIT | TCB_EL_BIT | TCB_I_BIT;
		tcb->tcb_link = VADDR_TO_BOFF(dv, tcb+1);
		tcb->tcb_tbdoffset = VADDR_TO_BOFF(dv, &dv->ex_tbd[i]);
		tcb->tcb_len = 0;
	}
	/* Now back up to the last TCB, and link it to the first */
	tcb--;
	tcb->tcb_link = VADDR_TO_BOFF(dv, dv->ex_tcb);
	
	/* Set the kernel xmit list pointers */
	dv->ex_tcbhead = dv->ex_tcb;
	dv->ex_tcbnext = dv->ex_tcb;
	dv->ex_tcbtail = dv->ex_tcb + dv->ex_ntcb - 1;
	
	/* Set up the Transmit Buffer Descriptors */
	tbd = dv->ex_tbd;
	for (i = 0; i < dv->ex_ntbd; i++, tbd++) {
		tbd->tbd_count = TBD_EOF_BIT;
		tbd->tbd_link = NULL_OFFSET;
		tbd->tbd_xbuffpl = VADDR_TO_BADDR(dv, dv->ex_tbuff + i);
		tbd->tbd_xbuffph = 0;
	}
	
	/* Set up the Receive Frame Descriptors */
	rfd = dv->ex_rfd;
	for (i = 0; i < dv->ex_nrfd; i++, rfd++) {
		rfd->rfd_status = 0;
		rfd->rfd_cntl = 0;
		rfd->rfd_link = VADDR_TO_BOFF(dv, rfd+1);
		rfd->rfd_rbd_ofst = NULL_OFFSET;
	}
	
	/*
	* Now we want to set the End of List flag in the last rfd, and
	* make the rbd_ofst field of the first rfd point to the list of
	* rbd's as required by the 82586 spec.
	*/
	rfd--;
	rfd->rfd_link = VADDR_TO_BOFF(dv, dv->ex_rfd);
	rfd->rfd_cntl = RFD_EL_BIT;
	rfd = dv->ex_rfd;
	rfd->rfd_rbd_ofst = VADDR_TO_BOFF(dv, dv->ex_rbd);
	
	/* Set the kernel frame descriptor list pointers */
	dv->ex_rfdhead = dv->ex_rfd;
	dv->ex_rfdtail = dv->ex_rfd + dv->ex_nrfd - 1;
	
	/* Set up the Receive Buffer Descriptors */
	rbd = dv->ex_rbd;
	for (i = 0; i < dv->ex_nrbd; i++, rbd++) {
		rbd->rbd_status = 0;
		rbd->rbd_link = VADDR_TO_BOFF(dv, rbd+1);
		rbd->rbd_rbaddrl = VADDR_TO_BADDR(dv, dv->ex_rbuff + i);
		rbd->rbd_rbaddrh = 0;
		rbd->rbd_rbuffsize = sizeof(struct rbuff);
	}
	/* Terminate the list */
	rbd--;
	rbd->rbd_link = VADDR_TO_BOFF(dv, dv->ex_rbd);
	rbd->rbd_rbuffsize |= RBD_EL_BIT;
	
	/* Set the kernel buffer descriptor list pointers */
	dv->ex_rbdhead = dv->ex_rbd;
	dv->ex_rbdtail = dv->ex_rbd + dv->ex_nrbd - 1;
	
	DEBUG07(CE_CONT, "rlst: fh %x ft %x bh %x bf %x\n", dv->ex_rfdhead,
		dv->ex_rfdtail, dv->ex_rbdhead, dv->ex_rbdtail);
	
	/* Finally.	We now have a set of valid 82586 data structures. */
	
	/* start up the 586 */
	SEND_586CA(dv);
	
	scb = &cntl->scb;
	if (e3Dpoll(e3Dtest_init_complete, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: 82586 INIT failed. iscp.busy x%x "
				"Status x%x\n",
				iscp->iscp_busy, scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	scb->scb_command = RESET_OK_BITS;
	SEND_586CA(dv);
	
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: 82586 INIT failed. Status x%x\n", 
			scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	
	DEBUG01(CE_CONT, "586 is up and running!\n");
	
	/*
	 * Run the 82586 internal self test to insure it is working.
	 */
	cbu->diag.diag_command = CMD_DIAGNOSE | CBL_EL_BIT;
	cbu->diag.diag_status = 0;
	
	scb->scb_command = SCB_CUC_START;
	SEND_586CA(dv);
	
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: DIAGNOSE timeout. Status x%x\n", 
			scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	
	DEBUG01(CE_CONT, "diag_status x%x\n", cbu->diag.diag_status);
	
	if (e3Dpoll(e3Dtest_cmd_complete, cbu, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: DIAGNOSE failed. diagstatus x%x "
				"scbstatus x%x\n",
				cbu->diag.diag_status, scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	DEBUG01(CE_CONT, "DIAGNOSE done diag_status x%x scbstatus x%x\n",
		cbu->diag.diag_status, scb->scb_status);
	scb->scb_command = CMD_UNIT_ACK;
	SEND_586CA(dv);
	
	
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: DIAGNOSE ack failed. Status x%x\n",
			scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	
	/*
	 * set the ethernet address in the 82586
	 */
	cbu->ias.ias_command = CMD_IASETUP | CBL_EL_BIT;
	cbu->ias.ias_status = 0;
	
	bcopy((caddr_t) dv->ex_eaddr, (caddr_t) cbu->ias.ias_addr,
		sizeof(cbu->ias.ias_addr));
	DEBUG01(CE_CONT, "iasetup addr %s\n", e3Daddrstr(cbu->ias.ias_addr));
	
	scb->scb_command = SCB_CUC_START;
	SEND_586CA(dv);
	
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: IASETUP timeout. Status x%x\n", 
			scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	
	DEBUG01(CE_CONT, "ias_status x%x\n", cbu->ias.ias_status);
	
	if (e3Dpoll(e3Dtest_cmd_complete, cbu, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: IASETUP failed. iastatus x%x "
				"scbstatus x%x\n",
			cbu->ias.ias_status, scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	DEBUG01(CE_CONT, "IASETUP done iastatus x%x scbstatus x%x\n",
		cbu->ias.ias_status, scb->scb_status);
	scb->scb_command = CMD_UNIT_ACK;
	SEND_586CA(dv);
	
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: IASETUP ack failed. Status x%x\n",
			 scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	
	/* The chip is now running - prepare for xmits */
	scb->scb_cbl_ofst = VADDR_TO_BOFF(dv, dv->ex_tcb);
	
	/* Start reception of frames */
	scb->scb_command = SCB_RUC_START;
	SEND_586CA(dv);
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: RUC_START cmdclr timeout. Status x%x\n",
			scb->scb_status);
		RESET_586(dv);
		return (FALSE);
	}
	DEBUG01(CE_CONT, "hwinit done scbstatus x%x\n", scb->scb_status);
	CLR_586INT(dv);
	ENABLE_586INT(dv);
	
#ifdef DUMP_82586_REGS
	e3Ddumpregs(dv);
#endif
	return (TRUE);
}

/*
 * Function: e3Dhwclose
 *
 * Purpose:
 *	Shut the board down.
 */
void
e3Dhwclose(e3Ddev_t *dv)
{
	/* Disable interrupts from ethernet controller */
	DISABL_586INT(dv);
	RESET_586(dv);
}

/*
 * Function: e3Dstartcu
 *
 * Purpose:
 *	Start the 82586 command unit.
 */
STATIC void
e3Dstartcu(e3Ddev_t *dv)
{
	register struct scb *scb;
	
	DEBUG04(CE_CONT, "e3Dstartcu: flags %x\n", dv->ex_flags);
	
	scb = &dv->ex_control->scb;
	scb->scb_cbl_ofst = VADDR_TO_BOFF(dv, (dv->ex_flags & ED_CBLREADY) 
					? dv->ex_cbuhead 
					: (union cbu *) dv->ex_tcbhead);
	DEBUG04(CE_CONT, "head %x -> off %x\n", 
		(dv->ex_flags & ED_CBLREADY) 
		? dv->ex_cbuhead	: (union cbu *) dv->ex_tcbhead, 
		scb->scb_cbl_ofst);
	
	scb->scb_command = SCB_CUC_START;
	SEND_586CA(dv);
	
	dv->ex_flags |= ED_CUWORKING;
	
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3Dstartcu cmd start timeout, marking card down\n");
		dv->ex_hwinited = 0;	/* mark card down */
		return;
	}
}

/*
 * Function: e3Dstartru
 * 
 * Purpose:
 *	Start the 82586's receive unit.
 */
STATIC void
e3Dstartru(e3Ddev_t *dv, struct scb *scb)
{
	struct rfd	*rfd;
	struct rbd	*rbd;
	
	DEBUG07(CE_CONT, "e3Dstartru status %x\n", scb->scb_status);
	
	rfd = dv->ex_rfdhead;
	if (rfd->rfd_cntl & RFD_EL_BIT) {
		/* Less than 2 rfd's, don't start recv unit */
		DEBUG07(CE_CONT, "not enough rfd's\n");
		return;
	}
	ASSERT((rfd->rfd_cntl & RFD_EL_BIT) == 0);
	
	rbd = dv->ex_rbdhead;
	if (rbd->rbd_rbuffsize & RBD_EL_BIT) {
		/* Less than 2 rbd's, don't start recv unit */
		DEBUG07(CE_CONT, "not enough rbd's\n");
		return;
	}
	ASSERT((rbd->rbd_rbuffsize & RBD_EL_BIT) == 0);
	
	DEBUG07(CE_CONT, "rbd_ofst set to %x -> %x\n", rbd, VADDR_TO_BOFF(dv, rbd));
	rfd->rfd_rbd_ofst = VADDR_TO_BOFF(dv, rbd);
	
	DEBUG07(CE_CONT, "rfd_ofst set to %x -> %x\n", rfd, VADDR_TO_BOFF(dv, rfd));
	scb->scb_rfd_ofst = VADDR_TO_BOFF(dv, rfd);
	scb->scb_command = SCB_RUC_START;
	SEND_586CA(dv);
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3Dstartru: cmd clear timeout");
		dv->ex_hwinited = 0;	/* mark card down */
		return;
	}
	
	if (e3Dpoll(e3Dtest_ru_ready, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3Dstartru: ru ready timeout");
		dv->ex_hwinited = 0;	/* mark card down */
		return;
	}
}

/*
 * Function: e3Doktoput
 *
 * Purpose:
 *	Routine to check whether there are any free blocks on the transmit
 *	queue.	If there are, it is OK to call e3Dhwput().	The main reason
 *	for the routine is to avoid doing getq()'s and putbq()'s unnecessarily.
 *
 * Returns:
 *	TRUE if ok to call e3Dhwput(), FALSE otherwise.
 *
 * Note:
 *	You must splstr() around the calls.
 */
int
e3Doktoput(e3Ddev_t *dev)
{
	if (dev->ex_flags & ED_BUFSBUSY)
		return (FALSE);
	return (TRUE);
}

/*
 * Function: e3Dhwput
 *
 * Purpose:
 *	Copy frame to board and send it.
 *
 * Note:
 *	Must call e3Doktoput() before calling to
 *	ensure that there are buffers available.
 */
void
e3Dhwput(e3Ddev_t *dv, mblk_t *m)
{
	register u_char		*dp;
	register e_frame_t	*eh;
	register uint 		len;
	mac_stats_eth_t		*mst;
	register struct tbd	*tbd;
	register struct tcb	*tcb;
	register uint 		tlen;
	
	ASSERT(dv->issuspended == 0);
	ASSERT(!(dv->ex_flags & ED_BUFSBUSY)); 
	
	mst = &dv->ex_macstats;
	
	DEBUG08(CE_CONT, "hwput(x%x) len %d\n", m, m->b_wptr - m->b_rptr);
	ASSERT(m->b_wptr - m->b_rptr >= sizeof(e_frame_t));
	
	tcb = dv->ex_tcbnext;
	tcb->tcb_status = 0;
	eh = (e_frame_t *) m->b_rptr;
	m->b_rptr += sizeof (e_frame_t);
	
	/* the first message block probably has only the Ethernet header */
	if ((m->b_wptr - m->b_rptr) == 0)
		m = m->b_cont;
	
	tbd = (struct tbd *) BOFF_TO_VADDR(dv, tcb->tcb_tbdoffset);
	dp = (u_char *) BADDR_TO_VADDR(dv, tbd->tbd_xbuffpl);
	
	len = 0;	
	while (m) {
		len += (tlen = m->b_wptr - m->b_rptr);
		if (len > E3D_MAXPACK)
			break;
		bcopy((caddr_t) m->b_rptr, (caddr_t) dp, tlen);
		DEBUG08(CE_CONT, "data %x %x %x %x %x\n", dp[0], dp[1], dp[2], dp[3],
			dp[4]);
		dp += tlen;
		m = m->b_cont;
	}
	
	DEBUG08(CE_CONT, "hwput len %d\n", len);
	if (len > E3D_MAXPACK) {
		return;
	}
	
	/* the frame size does not include the frame header */
	if (len < E3D_MINPACK) {
		bzero((caddr_t) dp, E3D_MINPACK - len);
		len = E3D_MINPACK;
	}
	
	bcopy((caddr_t) eh->eh_daddr, (caddr_t) tcb->tcb_dstaddr,
		sizeof(eh->eh_daddr));
	tcb->tcb_len = eh->eh_type;
	DEBUG08(CE_CONT, "hwput typ %x dst %s\n", tcb->tcb_len,
		e3Daddrstr(tcb->tcb_dstaddr));
	tbd->tbd_count = len | TBD_EOF_BIT;
	
	DEBUG08(CE_CONT, "head %x next %x tail %x\n", dv->ex_tcbhead,
		dv->ex_tcbnext, dv->ex_tcbtail);
	
	if (!(dv->ex_flags & ED_CUWORKING))
		e3Dstartcu(dv);
	
	/* 
	 * If this is the last transmit buffer, mark transmit buffers full,
	 * otherwise, advance the next available transmit buffer pointer.
	 */
	if (tcb == dv->ex_tcbtail)
		dv->ex_flags |= ED_BUFSBUSY;
	else
		dv->ex_tcbnext = (struct tcb *)BOFF_TO_VADDR(dv, tcb->tcb_link);
	
	return;
}

/*
 * Function: e3Dintr
 *
 * Purpose:
 *	Interrupt service routine for the e3D.	We are interrupted
 *	whenever a command is completed and whenever a frame is received.
 *	We loop checking for valid receive frame descriptors (corresponding
 *	to a valid, received frame) in the rfd queue.	We also check for
 *	having completed a command.	A command will be a transmit, or
 *	an mcsetup.
 * DDI8 notes:  note new return values.  The kernel internally considers
 *              all ddi < 7 drivers to return ISTAT_ASSUMED
 * DDI8 SUSPEND note:  we may get interrupts if we're almost-suspended:
 *                     - outstanding command completion interrupt
 *                     - interrupt from board that wasn't disabled yet.
 */
int
e3Dintr(void *idata)
{
	u_long				done_work = 0;
	register e3Ddev_t		*dv;
	register mac_stats_eth_t	*mst;
	register mblk_t 		*mp;
	register struct scb		*scb;
	register			status;
	int				unit;
	
	dv = (e3Ddev_t *)idata;
	mst = &dv->ex_macstats;
	scb = &dv->ex_control->scb;
	
	DEBUG03(CE_CONT, "e3Dintr(0%o): \n", dv->ex_int);
	
	if (!dv->ex_hwinited)	/* card is down */
		return(ISTAT_NONE);
	
	ASSERT(scb->scb_command == 0);
	
	while (TRUE) { /* check for another frame received, transmitted */
		CLR_586INT(dv);
		
		status = scb->scb_status;
		DEBUG03(CE_CONT, "check_again status x%x\n", status);
		
		/* Ack the 586 interrupt */
		scb->scb_command = status & SCB_INT_BITS;
		SEND_586CA(dv);
		if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
			cmn_err(CE_WARN,"e3Dintr: interrupt ack timeout\n");
			dv->ex_hwinited = 0;	/* mark card down */
			return(ISTAT_ASSUMED);
		}
		
		if (status & RECVUNIT_INT) { /* We have received a frame */
			register struct rfd *rfd;
			register struct rbd *rp, *rbd;
			register uint len;
			register u_char *sp, *dp;
			register e_frame_t *eh;
			register s;

			done_work = 1;
			/* read the error counters */
			if (scb->scb_rsc_errs) {
				mst->mac_no_resource += scb->scb_rsc_errs;
				scb->scb_rsc_errs = 0;
			}
			if (scb->scb_crc_errs) {
				mst->mac_badsum += scb->scb_crc_errs;
				scb->scb_crc_errs = 0;
			}
			if (scb->scb_aln_errs) {
				mst->mac_align += scb->scb_aln_errs;
				scb->scb_aln_errs = 0;
			}
			if (scb->scb_ovr_errs) {
				mst->mac_baddma += scb->scb_ovr_errs;
				scb->scb_ovr_errs = 0;
			}

			while (TRUE) { /* continue processing frames */
#ifdef CHECK_82586_CRASHED
				register struct iscp *iscp = &dv->ex_control->iscp;

				ASSERT(iscp->iscp_reserved == 0);
				ASSERT(iscp->iscp_scboffset == sizeof(struct iscp));
				ASSERT(iscp->iscp_scbbasel ==
					VADDR_TO_BADDR(dv, dv->ex_control));
#endif
		
				mp = NULL;
				rfd = dv->ex_rfdhead;
				DEBUG04(CE_CONT, "recv: rfd x%x\n", rfd);
				s = rfd->rfd_status;
				if (!(s & RFD_C_BIT)) {
					/* no more frames ready right now */
					DEBUG04(CE_CONT, "C_BIT clr");
					if ((scb->scb_status & SCB_RUS_MASK) !=
						SCB_RUS_READY) {
						e3Dstartru(dv, scb);
					}
					break; /* out of while (TRUE) */
				}
		
				/* Now we are sure we have a frame */
				dv->ex_rfdhead = (struct rfd *)BOFF_TO_VADDR(dv, rfd->rfd_link);
		
				rbd = (struct rbd *) BOFF_TO_VADDR(dv, rfd->rfd_rbd_ofst);
				DEBUG04(CE_CONT, "rbd x%x\n", rbd);
		
				if (!(s & RFD_OK_BIT)) {
					DEBUG04(CE_CONT, "recv err x%x\n", s);
					if (s & RFD_SHORT_BIT) 
						mst->mac_badlen++;
				}
		
				/* Now find and process the data */
				if (rfd->rfd_rbd_ofst == NULL_OFFSET)
					goto return_rfds;
				ASSERT(rbd == dv->ex_rbdhead);
		
				/*
				 * check for broadcast and filter multicast
				 * addresses - the 82586 does not have perfect
				 * multicast address filtering
				 */
				if (ISBROADCAST(rfd->rfd_dstaddr))
					;
				else if (ISMULTICAST(rfd->rfd_dstaddr))
					if (mdi_valid_mca(dv->dlpi_cookie, rfd->rfd_dstaddr) == -1)
						goto return_rbds_clear;
		
				rp = rbd;
				len = 0;
				for (;;) {
					ASSERT(rp->rbd_status & RBD_F_BIT);
					ASSERT(rp->rbd_status & RBD_PKTLEN_BITS);
					len += rp->rbd_status & RBD_PKTLEN_BITS;
					if (rp->rbd_status & RBD_EOF_BIT)
						break;
					ASSERT((rp->rbd_rbuffsize & RBD_EL_BIT) == 0);
					rp = (struct rbd *) BOFF_TO_VADDR(dv, rp->rbd_link);
				}
				DEBUG04(CE_CONT, "len %d\n", len);
				/* frame header in rfd, not part of rx frame */
				len += sizeof(e_frame_t);
		
				if ((mp = allocb(len, BPRI_HI)) == NULL) {
					DEBUG04(CE_CONT, "allocb failed\n");
					mst->mac_frame_nosr++;
					goto return_rbds_clear;
				}
				mp->b_wptr = mp->b_rptr + len;
		
				/* Copy frame header from rfd to mblock */
				eh = (e_frame_t *) mp->b_rptr;
				bcopy ((caddr_t) rfd->rfd_dstaddr,
					(caddr_t) eh->eh_daddr,
					sizeof(eh->eh_daddr));
				bcopy ((caddr_t) rfd->rfd_srcaddr,
					(caddr_t) eh->eh_saddr,
					sizeof(eh->eh_saddr));
				eh->eh_type = rfd->rfd_length;
				dp = mp->b_rptr + sizeof (e_frame_t);
				DEBUG06(CE_CONT, "src %s ", e3Daddrstr(eh->eh_saddr));
				DEBUG06(CE_CONT, "dst %s ", e3Daddrstr(eh->eh_daddr));
				DEBUG06(CE_CONT, "typ/len x%x\n", eh->eh_type);
		
				/* Copy data from receive buffer to mblock */
				rp = rbd;
				for (;;) {
					sp = (u_char *) BADDR_TO_VADDR(dv, rp->rbd_rbaddrl);
					ASSERT(rp->rbd_status & RBD_F_BIT);
					len = rp->rbd_status & RBD_PKTLEN_BITS;
					DEBUG04(CE_CONT, "copy %d from %x to %x\n", len, sp, dp);
					bcopy ((caddr_t) sp, (caddr_t) dp, len);
					dp += len;
					if (rp->rbd_status & RBD_EOF_BIT)
						break;
					rp->rbd_status = 0;
					rp = (struct rbd *) BOFF_TO_VADDR(dv, rp->rbd_link);
				}
				goto return_rbds;
		
return_rbds_clear:
				ASSERT(rfd->rfd_rbd_ofst != NULL_OFFSET);
				ASSERT(rbd == dv->ex_rbdhead);
				rp = rbd;
				while (!(rp->rbd_status & RBD_EOF_BIT)) {
					rp->rbd_status = 0;
					ASSERT((rp->rbd_rbuffsize & RBD_EL_BIT) == 0);
					rp = (struct rbd *) BOFF_TO_VADDR(dv, rp->rbd_link);
				}
		
return_rbds:
				ASSERT(rp->rbd_status & RBD_EOF_BIT);
				ASSERT(dv->ex_rbdtail->rbd_link == VADDR_TO_BOFF(dv, rbd));
				dv->ex_rbdhead = (struct rbd *) BOFF_TO_VADDR(dv, rp->rbd_link);
		
				/*
				 * Return the used rbd's to the available list.
				 * The link offsets are already in place. Set
				 * the EL bit in the new last rbd and clear the
				 * EL bit in the old last rbd.  rbd = first
				 * used rbd; rp = last used rbd
				 *
				 * The 82586 has problems if the next rbd offset
				 * in the last rbd is changed by the driver.
				 * The 82586 seems to (1) load the offset to the
				 * next rbd and then (2) look at the EL bit.
				 * It does not reload the offset after looking
				 * at the EL bit.  If the last rbd on the list
				 * has (for example) 0xffff as the next rbd
				 * offset, and the offset is changed and the EL
				 * bit cleared between (1) and (2), then the
				 * 82586 will use 0xffff as the next rbd offset.
				 */
				rp->rbd_status = 0;
		
				DEBUG04(CE_CONT, "Return rbds rbd=%x rp=%x\n", rbd, rp);
				ASSERT(dv->ex_rbdtail->rbd_rbuffsize & RBD_EL_BIT);
				rp->rbd_rbuffsize |= RBD_EL_BIT;
				dv->ex_rbdtail->rbd_rbuffsize &= ~RBD_EL_BIT;
				dv->ex_rbdtail = rp;

return_rfds:
				/*
				 * Return used rfd's to the available list.
				 * The link offsets are already in place. Set
				 * the EL bit in the new last rfd and clear the
				 * EL bit in the old last rfd.  rfd = the used
				 * frame descriptor
				 */
				DEBUG04(CE_CONT, "Return rfd %x\n", rfd);
				ASSERT(rfd->rfd_status & RFD_C_BIT);
				rfd->rfd_status = 0;
				rfd->rfd_cntl = RFD_EL_BIT;
				rfd->rfd_rbd_ofst = NULL_OFFSET;

				ASSERT(dv->ex_rfdtail->rfd_cntl & RFD_EL_BIT);
				ASSERT(dv->ex_rfdtail->rfd_link == VADDR_TO_BOFF(dv, rfd));
				dv->ex_rfdtail->rfd_cntl = 0;
				dv->ex_rfdtail = rfd;
				DEBUG07(CE_CONT, "rlst: fh %x ft %x bh %x bf %x\n",
					dv->ex_rfdhead,
					dv->ex_rfdtail, dv->ex_rbdhead,
					dv->ex_rbdtail);
		
				if (mp == NULL) { /* error - frame not filled */
					DEBUG04(CE_CONT, "mp == NULL, looping\n");
				} else if (dv->ex_up_queue) {
					putnext(dv->ex_up_queue, mp);
				} else {
					freeb(mp);
				}
			} /* while (TRUE) - continue processing frames */
		} /* if (status & RECVUNIT_INT) - We have received a frame */
		
		if (status & CMDUNIT_INT) { /* a command completed */
			register union cbu	*cbu;
			queue_t			*q;
			register int 		s;

			done_work = 1;
	
			cbu = (union cbu *) BOFF_TO_VADDR(dv, scb->scb_cbl_ofst);
	
			if ((cbu->cbl.cbl_command & CBL_CMD_MASK) == CMD_TRANSMIT) {
				/* transmit command completed */
				ASSERT(&cbu->tcb == dv->ex_tcbhead);
				ASSERT(cbu->tcb.tcb_status & TCB_C_BIT);
				DEBUG05(CE_CONT, "xmit intr tcb x%x\n", &cbu->tcb);
				dv->ex_tcbhead = (struct tcb *) 
				BOFF_TO_VADDR(dv, dv->ex_tcbhead->tcb_link);
				s = cbu->tcb.tcb_status;
				if (s & TCB_OK_BIT) {
					/* transmission successful */
					if (s & TCB_COLLSNS_CNT)
						mst->mac_colltable[(s & TCB_COLLSNS_CNT) - 1]++;
				} else { /* transmission failure */
					DEBUG05(CE_CONT, "xmit err status x%x\n", s);
					if (s & TCB_CSLOST)
						mst->mac_carrier++;
					if (s & TCB_DMAURUN)
						mst->mac_baddma++;
					if (s & TCB_DEFER)
						mst->mac_frame_def++;
					if (s & TCB_MAXCOLLSNS) {
						if (s & TCB_COLLSNS_CNT)
							mst->mac_colltable[(s & TCB_COLLSNS_CNT) - 1]++;
						else
							mst->mac_colltable[15]++;
						mst->mac_xs_coll++;
					}
				}
				dv->ex_tcbtail = &cbu->tcb;
				dv->ex_flags &= ~ED_CUWORKING;
				/* transmit a frame if ready for transmitting */
				if (dv->ex_tcbhead != dv->ex_tcbnext) /* tx queue not empty */
					e3Dstartcu(dv);
		
				/* were all tx buffers full ? */
				if (dv->ex_flags & ED_BUFSBUSY) {
					/* not any more since this on is free */
					dv->ex_flags &= ~ED_BUFSBUSY;
					dv->ex_tcbnext = &cbu->tcb;
					q = WR(dv->ex_up_queue);
					if (q->q_first) {
						e3Dhwput(dv, mp = getq(q));
						freemsg(mp);
					}
				}
			} else { /* other type of command completed */
				ASSERT(cbu == dv->ex_cbuhead);
				ASSERT(cbu->cbl.cbl_status & CBL_C_BIT);
				DEBUG05(CE_CONT, "cmnd intr cbl x%x\n", &cbu->cbl);
		
				/* reply to ioctl request */
				if (dv->ex_iocq) {
					int release_b_cont = 1;

					if (cbu->cbl.cbl_status & CBL_OK_BIT) {
						if (cbu->ias.ias_command & CMD_IASETUP) {
							/* DLPI expects address returned in b_cont */
							release_b_cont = 0;
							/* old way was to write it back to Space.c:
							 * bcopy((caddr_t) cbu->ias.ias_addr,
							 *	e3Deth_addr[unit],
							 *	sizeof(cbu->ias.ias_addr));
							 */
							bcopy((caddr_t) cbu->ias.ias_addr,
								dv->ex_eaddr,
								sizeof(cbu->ias.ias_addr));
						}
						if (cbu->mcs.mcs_command & CMD_MULTICAST) {
							/* DLPI expects address returned in b_cont */
							release_b_cont = 0;
						}
					}
					e3Dioctlack(dv, dv->ex_iocq, dv->ex_iocmp, 
						(cbu->cbl.cbl_status & CBL_OK_BIT) ? OK : NOT_OK,
						release_b_cont);
					dv->ex_iocq = (queue_t *) 0;
					dv->ex_iocmp = (mblk_t *) 0;
				}
		
				dv->ex_flags &= ~(ED_CUWORKING | ED_CBLREADY);
				if (dv->ex_flags & ED_CBLSBUSY)
					dv->ex_flags &= ~ED_CBLSBUSY;
			}
			if (!(dv->ex_flags & ED_CUWORKING)) {
				if ((dv->ex_tcbhead != dv->ex_tcbnext ||
				   (dv->ex_flags & ED_CBLREADY)))
					e3Dstartcu(dv);
			}
		} /* if (status & CMDUNIT_INT) - a command completed */
		
		if (!(status & SCB_INT_BITS)) {	/* no more interrupts */
			if (!done_work) {
				DEBUG03(CE_CONT, "e3Dintr: spurious x%x\n", status);
				mst->mac_spur_intr++;
				return(ISTAT_NONE);
			}
			break;
		}
		
	} /* while (TRUE) - checking for frames received/command complete */
	
	return(ISTAT_ASSERTED);
}

/*
 * Purpose: e3Dmacaddrset
 *
 * Purpose:
 *	Set board's ethernet address.
 */
int
e3Dmacaddrset(e3Ddev_t *dv, queue_t *q, mblk_t *mp)
{
	macaddr_t	*eaddr;
	struct iasetup	*ias = &dv->ex_control->cbu.ias;
	int		s;
	register int	i;
	
	if (dv->ex_flags & ED_CBLSBUSY)
		return (NOT_OK);
	
	/* there is only one cbl */
	dv->ex_iocq = q;
	dv->ex_iocmp = mp;
	dv->ex_flags |= (ED_CBLSBUSY | ED_CBLREADY);
	
	eaddr = (macaddr_t *)mp->b_cont->b_rptr;
	bcopy((caddr_t) eaddr, (caddr_t) ias->ias_addr, sizeof(ias->ias_addr));
	
	/*
	* Now we pass the address to the 586
	*/
	ias->ias_command = CMD_IASETUP | CBL_EL_BIT | CBL_I_BIT;
	ias->ias_status = 0;
	
	if (!(dv->ex_flags & ED_CUWORKING))
		e3Dstartcu(dv);
	
	return (OK);
}

/*
 * Purpose: e3Dmcaddrset
 *
 * Purpose:
 *	Set board's multicast address.
 */
int
e3Dmcaddrset(e3Ddev_t *dv, queue_t *q, mblk_t *mp)
{
	mblk_t		*mmp;
	mac_mcast_t	*mcast;
	macaddr_t	*mc;
	struct mcsetup	*mcs = &dv->ex_control->cbu.mcs;
	int		s;
	register int	i;
	
	if (dv->ex_flags & ED_CBLSBUSY)
		return (NOT_OK);
	if (! (mmp = mdi_get_mctable(dv->dlpi_cookie)))
		return(NOT_OK);
	
	/* there is only one cbl */
	dv->ex_iocq = q;
	dv->ex_iocmp = mp;
	dv->ex_flags |= (ED_CBLSBUSY | ED_CBLREADY);
	
	bzero ((caddr_t) mcs->mcs_list, sizeof (mcs->mcs_list));
	mcs->mcs_cnt = 0;
	
	mcast = (mac_mcast_t *)mmp->b_rptr;
	mc = (macaddr_t *)((ulong)mcast + mcast->mac_mca_offset);
	DEBUG10(CE_CONT, "mcaddrset unit X mccnt %d\n", dv->mccnt);
	for (i = 0; i < mcast->mac_mca_count; i++, mc++) {
		bcopy((caddr_t) mc, mcs->mcs_list[i], sizeof(macaddr_t));
		mcs->mcs_cnt += sizeof(macaddr_t);
		DEBUG10(CE_CONT, "addr[%d] %s\n", i, e3Daddrstr(mcs->mcs_list[i]));
	}
	bcopy((caddr_t)mp->b_cont->b_rptr, mcs->mcs_list[i], sizeof(macaddr_t));
	mcs->mcs_cnt += sizeof(macaddr_t);
	dv->mccnt = i + 1;
	
	freemsg(mmp);

	/*
	* Now we pass the address list to the 586
	*/
	mcs->mcs_command = CMD_MULTICAST | CBL_EL_BIT | CBL_I_BIT;
	mcs->mcs_status = 0;
	
	if (!(dv->ex_flags & ED_CUWORKING))
		e3Dstartcu(dv);
	
	return (OK);
}

/*
 * Function: e3Drunstate
 * 
 * Purpose:
 *	Bring all the installed adapters to the RUN state.
 *
 * Notes:
 *	There is no way to reset an individual E3D 507 adapter.
 *	The RST bit in the Interrupt Configuraton Register only
 *	restarts the board configuration logic. After that you
 *	have to write to the ID port and all installed adapters
 *	react to writes to the ID port. The only thing that can
 *	be done is to bring all adapters to the CONFIG state
 *	or to the RUN state.
 */
void
e3Drunstate()
{
	ushort al, cx;
	
	DEBUG00(CE_CONT, "e3Drunstate called\n");
	outb(ED_ID_PORT, 0);
	for (al = 0xff, cx = 0; cx < 255; cx++) {
		outb(ED_ID_PORT, al & 0xff);
		al <<= 1;
		if (al & 0x100)
		al ^= 0xe7;
	}
	outb(ED_ID_PORT, 0);
}

/* Function: e3Dloadstate()
 *
 * Purpose:
 *	Bring all the installed adapters to the LOAD state
 *
 * Notes:
 *	See e3Drunstate
 *
 */
void
e3Dloadstate()
{
	ushort al, cx;
	
	DEBUG00(CE_CONT, "e3Dloadstate called\n");
	outb(ED_ID_PORT, 0);
	for (al = 0xff, cx = 0; cx < 255; cx++) {
		outb(ED_ID_PORT, al & 0xff);
		al <<= 1;
		if (al & 0x100)
		al ^= 0xe7;
	}

	for (al = 0xff, cx = 0; cx < 255; cx++) {
		outb(ED_ID_PORT, al & 0xff);
		al <<= 1;
		if (al & 0x100)
		al ^= 0xe7;
	}
}

/*
 * dump the internal registers of 82586
 */
#ifdef DUMP_82586_REGS
int
e3Ddumpregs(e3Ddev_t *dv) 
{
	struct scb		*scb = &dv->ex_control->scb;
	struct dump		*dmp = &dv->ex_control->cbu.dump;
	register dump_buf	*dumpbuf = dv->ex_dumpbuf;
	register int		i;
	ushort			old_offset;
	int			s, rcode = OK;
	
	DEBUG01(CE_CONT, "e3Ddumpregs: buf x%x\n", dumpbuf);
	
	if (dumpbuf == (dump_buf *)NULL)
		return (rcode);

	s = splstr();
	
	/*
	* Set up scb command list to point to our special command buffer
	*/
	old_offset = scb->scb_cbl_ofst;
	scb->scb_cbl_ofst = VADDR_TO_BOFF(dv, dmp);
	
	dmp->dmp_command = CMD_DUMP | CBL_EL_BIT;
	dmp->dmp_status = 0;
	dmp->dmp_bufoffset = VADDR_TO_BOFF(dv, dumpbuf);
	DEBUG01(CE_CONT, "dump offs x%x\n", dmp->dmp_bufoffset);
	
	scb->scb_command = SCB_CUC_START;
	SEND_586CA(dv);
	
	if (e3Dpoll(e3Dtest_cmd_clear, scb, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: DUMP timeout. Status x%x\n", scb->scb_status);
		RESET_586(dv);
		rcode = NOT_OK;
		goto out;
	}
	
	DEBUG01(CE_CONT, "dmp_status x%x\n", dmp->dmp_status);
	
	if (e3Dpoll(e3Dtest_cmd_complete, (union cbu *) dmp, E3D_POLL) == FALSE) {
		cmn_err(CE_WARN,"e3D: DUMP failed. dmpstatus x%x scbstatus x%x\n",
			dmp->dmp_status, scb->scb_status);
		RESET_586(dv);
		rcode = NOT_OK;
		goto out;
	}
	DEBUG01(CE_CONT, "DUMP done dmpstatus x%x scbstatus x%x\n",
		dmp->dmp_status, scb->scb_status);
	scb->scb_command = CMD_UNIT_ACK;
	SEND_586CA(dv);
	
out:
	/*
	* Restore the command buffer pointer to the transmit buffer list
	*/
	scb->scb_cbl_ofst = old_offset;
	splx(s);
	
	if (rcode == OK) {
		cmn_err(CE_WARN,"-------- DUMP OF 82586 REGISTERS --------");
		for (i = 0; i < sizeof(dump_buf) - 1; i += 2) {
			if (!(i % 16))
				cmn_err(CE_CONT,"\n%x: ", i);
				cmn_err(CE_CONT,"%b%b ", (*dumpbuf)[i + 1],
					(*dumpbuf)[i]);
		}
		cmn_err(CE_CONT,"\n");
	}
	return (rcode);
}
#endif


/*
 * e3Dsoft_config: Re-configure the boards PCR, RCR, and ICR registers.
 *
 * Returns: data to be written into EEPROM.
 * can't be static as e3Dli.c refers to it.
 */
u_long
e3Dsoft_config(e3Ddev_t *dp)
{
	register struct e3Drominfo	*pp;
	register struct e3Draminfo	*rp;
	u_long				eeprom;
	u_char				pcr;		/* ROM config reg */
	u_char				rcr;		/* RAM config reg */
	u_char				icr;		/* IRQ config reg */
	u_char				iobase;		/* I/O base, IO4..IO0 */

	/* ROM settings */
	CONFIG_ROMADDR(dp, pcr);
	if (dp->ex_bnc)
		pcr |= HROM_BNC;
	for (pp = e3Drominfo; pp < &e3Drominfo[ROMINFO_CNT]; pp++) {
		if (pp->rom_size == dp->ex_romsize)
			pcr |= pp->rom_setting;
	}
	SET_ROMREG(dp, pcr);

	/* RAM settings */
	for (rp = e3Draminfo; rp->ram_base; rp++) {
		/* bug: old way: if ((dp->ex_rambase == rp->ram_base) && - N */
		if ((dp->ex_base == rp->ram_base) && 
		   (dp->ex_memsize == rp->ram_size)) {
			rcr = rp->ram_setting;
			break;
		}
	}
	if (dp->ex_zerows)
		rcr |= HRAM_0WS;
	if (dp->ex_sad) {
		rcr |= HRAM_SAD;
	}
	SET_RAMREG(dp, rcr);

	/* Interrupt setting */
	SET_INTREG(dp, dp->ex_int);
	dp->ex_intr2 = dp->ex_int;

	/* I/O base address setting - already set on board in IOLOAD state */
	CONFIG_IOADDR(dp, iobase);

	eeprom = pcr << 24;
	eeprom |= rcr << 16;
	eeprom |= dp->ex_int << 8;
	eeprom |= iobase;
	return(eeprom);
}

