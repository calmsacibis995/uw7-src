#ident "@(#)tokmac.c	26.1"
#ident "$Header$"
/*
 *	  Copyright (C) The Santa Cruz Operation, 1993-1997.
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 *
 * History:
 *	MDI001  1 Mar 1995
 *		Defined-off non-fatal console message
 *	MDI002  2 Mar 1995
 *		Moved calculation of tx & rx buffer count from macro to
 *		functions.  Fixed allocations for card RAM larger than 16k.
 *		Added prototypes for private functions.  Added some casts
 *		and declarations to conform to ANSI semantics.
 *	MDI003	7 Mar 1995
 *		Removed support for > 32k RAM due to HW failure on some
 *		adapters.  Added additional info to some fatal errors.
 *		Added test for invalid RAM boundry.
 *		Reformatted some lines for readability.
 */

#ifdef _KERNEL_HEADERS
# include <io/nd/mdi/tok/tok.h>
#else
# include "tok.h"
#endif

struct tok_diag
{
	char i;
	char *s;
};

extern struct tokdevice *tokdevice;
extern struct tokhwconf *tokhwconf;
extern unchar tokhwaddr[][6];
extern ulong  tokframsz[];

extern int tok_fdx[];					  //thc-fdx
extern int tok_smode[];					  //thc-sm

/* assume ISA adapter */
cm_num_t tokarch = CM_BUS_UNK;
extern void **tokcookie;
extern int tokdevflag;

STATIC unchar tokirq_to_unit[20];		/* IRQ->Unit number map */
STATIC unchar tokfuncaddr[] = { 0xc0, 0x00, 0xff, 0xff, 0xff, 0xff };
STATIC ushort tokio_addrs[] = { 0xa20, 0xa24 };

STATIC int  tok_SMopen (int unit);
STATIC int  tokprocess_SMopen_cnf_ext (int unit);
STATIC int  tokprocess_SMopen_cnf_ram (int unit);
STATIC int  tok_normalopen (int unit);

int tokSMtx (int unit, mblk_t *mp);
STATIC mblk_t *tokfill_txbuffer (mblk_t *mp, sm_tx_buf *tx_buff);
STATIC void	tokSMtx_completion (int unit);

STATIC void tokSM_process_recv (int unit);
STATIC int tokSM_rxframe (struct tokdevice *device,
                          sm_rx_buf *frame_inic, sm_rx_buf *frame_last,
                          ushort n_bufs_fr);

STATIC int    tokhwreset(int unit);
STATIC void   toktimeout(int unit);
STATIC unchar tok_num_txbuf(int unit);
STATIC ushort tok_num_rxbuf(int unit);
STATIC int    tokprocess_open(int unit);
STATIC void   tok_issue_close_srb(struct tokdevice *device);
STATIC void   tokhwfailed(struct tokdevice *device);
STATIC void   tok_issue_send_srb(struct tokdevice *device);
STATIC void   tok_process_startio(struct tokdevice *device);
STATIC void   tok_issue_send_asb(struct tokdevice *device);
STATIC void   tok_process_modify_open(int unit);
STATIC void   tok_process_send_error(int unit);
STATIC void   tok_send_waiting_asb(struct tokdevice *device);
STATIC void   tok_send_waiting_srb(struct tokdevice *device);
STATIC void   tok_process_ring_status_change(int unit);
int           tokcheckaddr(struct tokdevice *device, unchar *l);
STATIC void   tok_process_recv(int unit);
STATIC void   tok_issue_recv_asb(struct tokdevice *device);
STATIC void   tok_issue_log_srb(struct tokdevice *device);
STATIC void   tok_issue_mod_srb(struct tokdevice *device);
STATIC void   tok_process_read_log(int unit);
STATIC char   *tok_check_t(struct tok_diag *table, unchar errno);
STATIC void   tokdiag_retc(int unit, char *desc, int errno);
STATIC void   tokdiag_openerr(int unit, unchar errno, ushort open_err);


STATIC int
tokhwreset(int unit)
{
	struct tokdevice		*device = &tokdevice[unit];
	struct tokhwconf		*hwconf = &tokhwconf[unit];
	struct srb_init_status	*srb;
	int				j;
	unchar			x;
	unchar			*y;

	/*
	 * To completely reset the adapter software must ensure at least
	 * 50ms between the RESET and RELEASE (see S.G.12.001.03 page 17)
	 */

	outb(TOKEN_RESET(unit), 0);		/* reset the adapter */
	drv_usecwait(50000);
	outb(TOKEN_RELEASE(unit), 0);	/* now release the adapter */

	/* enable interrupts */
	device->aca->isrp_even_set = 0xc0;

	/*
	 * wait for init to complete (bit 5 in ISRP odd set)
	 * wait for up to a second
	 */
	for (j=1000; j; --j) {
		if (device->aca->isrp_odd & SRB_RESP)
			break;
		drv_usecwait(10000);
	}

	if (!j)
	{
		cmn_err(CE_WARN, "tok%d: Timed out waiting for adapter to initialize",
				unit);
		return(0);
	}

	if (tokarch == CM_BUS_ISA)
	{
		unsigned char z;

		y = device->mmio + 0x1fa0;

#ifdef INIT_ACA
		z = hwconf->rambase >> 12;
		device->aca->rrr_even = z;
#endif
		if ((uint)((*y) & 0x0f) > 0x0c)	/* old adapter */
		{
			z = hwconf->rambase >> 12;
			device->aca->rrr_even = z;
			x = device->aca->rrr_even;
			if ( z != x)
			{
				cmn_err(CE_WARN,"tok%d: Failed to set RAM base address to 0x%x",
						unit, hwconf->rambase);
				return(0);
			}
		} else {
			z = device->aca->rrr_even;
			hwconf->rambase = (u_int)z << 12;	/* MDI002 */
			device->aca->rrr_even = z;
		}
	}

	/* Read the RAM size */
	j = 1;
	x = device->aca->rrr_odd & 0x0c;
	switch (x)
	{
	case 0x00:
		hwconf->ramsz = 8*1024;
		j = 0;
		break;
	case 0x04:
		hwconf->ramsz = 16*1024;
		j = device->aca->rrr_even & 0x03;			/* MDI003 */
		break;
	case 0x08:
		hwconf->ramsz = 32*1024;
		j = device->aca->rrr_even & 0x07;			/* MDI003 */
		break;
	case 0x0c:
		hwconf->ramsz = 64*1024;
		j = device->aca->rrr_even & 0x0f;			/* MDI003 */

		/*
		 * There is a hardware failure on some adapters if
		 * 64k RAM is selected.
		 */
		cmn_err(CE_WARN,					/* MDI003 */
		"tok%d: Greater than 32k RAM is not supported for this adapter",
									unit);
		return(0);
		break;
	}

	if (j)							/* MDI003 */
	{
		cmn_err(CE_WARN, "tok%d: Invalid RAM boundry for %dk: 0x%x",
				unit, hwconf->ramsz/1024, hwconf->rambase);
		return(0);
	}

	if (!device->sram)
	{
		device->sram =
			(unchar *)physmap(hwconf->rambase, hwconf->ramsz, KM_NOSLEEP);
		if (!device->sram)
		{
			cmn_err(CE_WARN, "tok%d: sptalloc(sram=0x%x) failed",
					unit, hwconf->rambase);
			return(0);
		}
	}

	/* Now look at the SRB just returned */
	device->srb = device->sram + mdi_ntohs(device->aca->wrb);
	srb = (struct srb_init_status *)device->srb;
	if (srb->command != 0x80)
	{
		cmn_err(CE_WARN,
			"tok%d: Reset response(%b) is not SRB_INIT_STATUS(0x80)",
			unit, srb->command);
		return(0);
	}

	if (srb->bringup_code)
	{
		cmn_err(CE_WARN, "tok%d: Initialization failed, code=%x",
				unit, srb->bringup_code);
		return(0);
	}

	/* verify if hw is fdx capable */
	device->fdx_enable = (srb->status2 & 0x08) && tok_fdx [unit];  //thc-fdx

	/* verify if hw is shallow mode capable */
	device->sm_enable = (srb->status & 0x80) && tok_smode [unit];  //thc-sm

	/* The IBM H/W Tech Ref states that bit 7 of status bit is the ring */
	/* speed indicator, it is in fact, bit 0 vvvvvv */
	device->ring_speed_save = (srb->status2 & 0x04) ? 1 : 0;
	device->ring_speed_detector = (srb->status & 0x40) ? 1 : 0;
	device->ring_speed = (srb->status & 0x01) ? 16000000L:4000000L;
	device->addr = (struct adapter_address_area *)(device->sram + mdi_ntohs(srb->adapter_address));
	device->parms = (struct adapter_parameters_area *)(device->sram + mdi_ntohs(srb->parms_address));

		/* Initialize the pre-allocation of tx buffer stuff */
	device->tokstartio_going = 0;
	device->txbufp = (unchar *)0;
	device->srb_busy = device->asb_busy = 0;

		/* Initialize the ASB buffers */
	bzero(&device->asb_xmt_resp, sizeof(struct asb_xmt_resp));
	bzero(&device->asb_rcv_resp, sizeof(struct asb_rcv_resp));
	device->asb_xmt_resp.command = 0x0a;		/* TRANSMIT.DIR.frame */
	device->asb_rcv_resp.command = 0x81;		/* RECEIVED.DATA */
	device->wait_send_xmt_asb = device->wait_send_rcv_asb = 0;

		/* Initialize the SRB buffers */
	bzero(&device->srb_send,	sizeof(struct srb_send));
	bzero(&device->srb_close,	sizeof(struct srb_close));
	bzero(&device->srb_read_log,	sizeof(struct srb_read_log));
	bzero(&device->srb_set_func_addr, sizeof(struct srb_set_func_addr));
	bzero(&device->srb_modify_open,	sizeof(struct srb_modify_open));
	device->wait_send_xmt_srb = device->wait_send_close_srb = 0;
	device->wait_send_log_srb = 0;
	device->wait_send_mod_srb = 0;
	device->srb_send.command = 0x0a;		/* TRANSMIT.DIR.frame */
	device->srb_send.retc = 0xfe;
	device->srb_send.station_id = 0;
	device->srb_set_func_addr.command = 0x07;	/* DIR.SET.FUNC.ADDR */
	device->srb_set_func_addr.retc = 0xfe;
	device->srb_set_func_addr.func_addr = 0xffffffff;
	device->srb_close.command = 0x04;		/* DIR.CLOSE.ADAPTER */
	device->srb_close.retc = 0xfe;
	device->srb_read_log.command = 0x08;		/* DIR.READ.LOG */
	device->srb_read_log.retc = 0xfe;


	/* get the factory MAC address, if we haven't already done so */
	for (j=0; j< 6; j++)
		if (device->hw_raddress[j] != 0x00)
			break;
	if ( j >= 6 ) {
		y = device->mmio+AIP;
		for (j=0; j< 6; j++) {
			device->hw_raddress[j] = (((*y) & 0x0f)<<4) |
						((*(y+2)) & 0x0f);
			y+=4;
		}
	}

	/* If tokhwaddr set, display IT */
	if (*(ulong *)(tokhwaddr[unit])) {
		bcopy(tokhwaddr[unit], device->hw_address, 6);
	} else {
		bcopy(device->hw_raddress, device->hw_address, 6);
	}

	device->aca->isrp_odd_rst = ~SRB_RESP;
	outb(TOKEN_INTR_REL(unit), 0);	 /* release TOKEN interrupt */
	return(1);
}

/*
 * Presence test for IBM Token Ring Network Adapter
 */
int
tokpresent(int ioaddr)
{
	static unchar mca_irq[] = { 9, 3,10,11 };
	static unchar isa_irq[2][4] = { { 9, 3,  6,  7 },
	                                { 9, 3, 10, 11 } };
	struct tokdevice *device;
	struct tokhwconf *hwconf;
	void tokintr(int);
	unchar	x, irq;
	ulong	mmio;
	int dei;
	register int unit;
	unsigned int numio;
	struct cm_args cm_args;
	cm_range_t range;
	cm_num_t slot;

	/* determine if card is present */
	numio = sizeof(tokio_addrs)/sizeof(ushort);
	for (unit = 0; unit < numio; unit++) {
		if (ioaddr == tokhwconf[unit].iobase)
			break;
	}
	if (unit >= numio)
		return (0);

	device = &tokdevice[unit];
	hwconf = &tokhwconf[unit];

	/* Get board bus type */
	cm_args.cm_key = cm_getbrdkey("tok", unit);
	cm_args.cm_n = 0;
	cm_args.cm_param = CM_BRDBUSTYPE;
	cm_args.cm_val = &tokarch;
	cm_args.cm_vallen = sizeof(cm_num_t);
	if (cm_getval(&cm_args) == 0) {
		switch (tokarch) {
		case CM_BUS_MCA:
			cm_args.cm_param = CM_SLOT;
			cm_args.cm_val = (cm_num_t *)&slot;
			cm_args.cm_vallen = sizeof(cm_num_t);
			if (cm_getval(&cm_args) == 0) {
				hwconf->slot = (u_int)slot;
			} else {
				cmn_err(CE_CONT, "tokpresent: cm_getval(SLOT) failed");
			}
			break;
		case CM_BUS_UNK:
			tokarch = CM_BUS_ISA;		/* fall thru */
		case CM_BUS_ISA:
			cm_args.cm_n = 0;
			cm_args.cm_param = CM_MEMADDR;
			cm_args.cm_val = &range;
			cm_args.cm_vallen = sizeof(struct cm_addr_rng);
			if (cm_getval(&cm_args) == 0) {
				hwconf->rambase = range.startaddr;
			} else {
				cmn_err(CE_CONT, "tokpresent: cm_getval(MEMADDR) failed");
			}
			break;
		default:
			cmn_err(CE_WARN, "tokpresent() - bus %x", tokarch);
		}
	}

	/* get adapters I/O base address from Slot and POS regs if MCA */
	if (tokarch == CM_BUS_MCA)
	{
		outb(ADENABLE, CH_SETUP + hwconf->slot - 1);
		hwconf->iobase = TOKEN_MCA_BASE(inb(POS3));
		outb(ADENABLE, 0);
	}

	x = inb(TOKEN_SWTCH_RD(unit));
	mmio = (x & 0xfc) << 11;
	if (tokarch == CM_BUS_MCA)
	{
		unchar	setup2;

		setup2 = inb(TOKEN_SWTCH2_RD(unit));
		mmio |= (setup2 & 0x01) << 19;
		hwconf->rambase = (setup2 & 0xfe) << 12;
	} else {
		mmio |= 1 << 19;
	}

	device->mmio = (unchar *)physmap((paddr_t)mmio, 8192, KM_NOSLEEP);
	if (!device->mmio) {
	   cmn_err(CE_WARN, "tokinit: no memory for physmap (mmio=0x%x)", mmio);
	   return(0);
	}

	if (tokarch == CM_BUS_MCA) {
		irq = mca_irq[x & 0x03];
	} else {
		unchar *y, i;

		/* old ISA adp   ISA adp, Func ID=0xf   ISA adp, Func ID=0xe
		 * ===========   ====================   ====================
		 * IRQ 2(9)      IRQ 2(9)               IRQ 2(9)
		 * IRQ 3         IRQ 3                  IRQ 3
		 * IRQ 6         IRQ 6                  IRQ 10
		 * IRQ 7         IRQ 7                  IRQ 11
		 */
		
		y = device->mmio + 0x1fba;
		i = ( ((*y) & 0x0f) == 0x0e ) ? 1 : 0;
		irq = isa_irq[i][x & 0x03];
	}

	/* TODO - verify resmgr IRQ = board IRQ before cm_intr_attach() */ 
	if (cm_intr_attach(cm_args.cm_key, tokintr, &tokdevflag, &tokcookie[unit]) == 0) {
		cmn_err(CE_WARN, "tokinit: cm_intr_attach failed (key %d)", cm_args.cm_key);
	}

	hwconf->irq = irq;
	device->aca = (struct aca *)(device->mmio + ACA);

	tokirq_to_unit[hwconf->irq] = unit;
	device->max_pdu = tokframsz[unit];

	/* If reset fails then the hardware must not exist */
	return (tokhwreset(unit));
}

STATIC void
toktimeout(int unit)
{
	struct tokdevice *device = &tokdevice[unit];

	device->wait_srb_complete=0;
	device->device_timeout=1;
	SV_SIGNAL(device->sv, 0);
}

/* modifies open_options */
void
tokmodopen(int unit, int options)
{
	struct tokdevice *device = &tokdevice[unit];
	struct srb_modify_open *srb_modify_open;

	srb_modify_open = &device->srb_modify_open;
	srb_modify_open->open_options = options;
	srb_modify_open->command = 0x01;
	if (device->srb_busy)
		device->wait_send_mod_srb = 1;
	else
		tok_issue_mod_srb(device);
}

/*
 * The original Token-Ring Network PC Adapter has 8k of shared RAM
 * The Token-Ring Network PC Adapter II has 16k of shared RAM
 * The Token-Ring Network Adapter/A has 16k of shared RAM
 * The Token-Ring Network 16/4 Adapter/A has 64k of shared RAM
 * The Token-Ring Network 16/4 Adapter has 64k of shared RAM
 *
 * Adapter Shared Memory is broken down like this:
 *
 * TXBUF_LEN must be 96..2048 % 8
 * RXBUF_LEN must be 96..2048 % 8
 *
 *	Private Variables: 	1496 bytes (or 1416 bytes for newer cards)
 *	1 x SSB:		  20 bytes
 *	1 x ARB:		  28 bytes
 *	1 x SRB:		  28 bytes
 *	1 x ASB:		  12 bytes
 *	SUBTOTAL:		1584 bytes
 *
 *				  8K RAM	16, 32, 64
 *				----------	----------
 *	num_txbuf x TXBUF_LEN	2048 bytes	4096 bytes
 *				==========	==========
 *	TOTAL:			3632 bytes	5680 bytes (6k-464)
 *				==========	==========
 *	Rest for RXBUF's
 *		 8k RAM	2x2048		  7728
 *		16k RAM	5x2048				 15920
 *		32k RAM	13x2048				 32304
 *		64k RAM	29x2048				 65072
 *
 * On 64k cards, 512 kytes are reserved at the top.
 *
 * HOWEVER: Testing shows that:
 *	for 32k cards, above 12 buffers fails
 *	for 64k cards, above 26 buffers fails
 *
 * Shared RAM paging is not supported by this driver.
 * These routines are written in anticipation that > 64k may someday be used.
 */

/*
 * For RAM sizes below 16k (like 8k) we only use one transmit buffer
 * otherwise we use two.  The value must be either one or two.
 */

STATIC unchar
tok_num_txbuf(int unit)		/* MDI002 */
{
	return(tokhwconf[unit].ramsz < (16*1024) ? 1 : 2);
}

STATIC ushort
tok_num_rxbuf(int unit)		/* MDI002 */
{
#ifdef RAM_CALCULATED
	u_int	Used;

	Used = 1584 + (tok_num_txbuf(unit) * TXBUF_LEN);

	if (tokhwconf[unit].ramsz > (32*1024))
	Used += 512;	/* Adapter reserved area on 64k cards */

	return((tokhwconf[unit].ramsz - Used) / RXBUF_LEN);
#else
	register u_int	ramsz = tokhwconf[unit].ramsz;

	if (ramsz >= (64*1024))
	return 26;		/* 64k */

	if (ramsz >= (32*1024))
	return 12;		/* 32k */

	if (ramsz >= (16*1024))
	return 5;		/* 16k */

	return 2;			/* 8k */
#endif
}

/*
 * Initiate the opening of the token ring adapter
 */
int
tokhwinit(unit)
{
	struct tokdevice *device = &tokdevice[unit];

	if (!tokhwreset(unit))
		return(0);

	if (device->sm_enable)
		return (tok_SMopen (unit));					//thc-sm
	else
		return (tok_normalopen (unit));				//thc-sm
}


STATIC int
tok_SMopen (unit)					  //thc-sm
{
	struct tokdevice		*device;
	srb_cnf_ext				*srb_cnfext;
	srb_cnf_fp_ram			*srb_cnffp;
	struct srb_open			*srb_open;
	volatile fp_tx_c_area	*fp_tx_ca;
	volatile fp_rx_c_area	*fp_rx_ca;
	ushort					tx_ram_size;

	cmn_err (CE_CONT, "tok%d: Opening in shallow mode\n", unit);

	device = &tokdevice[unit];

	/*
	* Config Extension
	*/

	/* build Config Extension SRB */
	srb_cnfext = (struct srb_cnf_ext *)device->srb;
	bzero(srb_cnfext, sizeof(struct srb_cnf_ext));

	srb_cnfext->command = 0x20;

	srb_cnfext->retc = 0x0;
	srb_cnfext->options_flag = SM_CNF_EXT_OPTIONS;

	device->wait_srb_complete = 1;
	device->device_timeout=0;
	device->timeout_id = itimeout(toktimeout, (void *)unit, 30*HZ, plstr);
	device->aca->isra_odd_set = SRB_CMD;	/* tell card that cmd is avail*/

	LOCK(device->lock, getpl());
	while (device->wait_srb_complete)
		SV_WAIT(device->sv, prinet, device->lock);
	
	if (device->device_timeout)
	{
		cmn_err (CE_WARN, 
			   "tokhwinit(unit=%d) - Config Extensions: Device timed out",
				unit);
		return(0);
	}

	if (device->sm_retc != 0)
	{
		cmn_err (CE_WARN, 
			   "tokhwinit(unit=%d) - SMopen: Config Extensions failed", unit);
		return(0);
	}

	/*
	* Config Extension Fast Path Ram
	*/
	tx_ram_size = (tokframsz [unit]/SM_TX_BUFDATA_SZ + 2) * SM_TX_BUFFER_SIZE;
	if (tx_ram_size < tokhwconf [unit].ramsz/2)
		tx_ram_size = tokhwconf [unit].ramsz/2;
	tx_ram_size = (tx_ram_size+7)/8;

	/* build Config Fast Path RAM SRB */
	srb_cnffp = (struct srb_cnf_fp_ram *)device->srb;
	bzero(srb_cnffp, sizeof(struct srb_cnf_fp_ram));

	srb_cnffp->command = 0x12;

	srb_cnffp->retc = 0x00;
	srb_cnffp->tx_ram_size = mdi_htons (tx_ram_size);
	srb_cnffp->tx_buffer_size = mdi_htons (SM_TX_BUFFER_SIZE);

	device->wait_srb_complete = 1;
	device->device_timeout=0;
	device->timeout_id = itimeout(toktimeout, (void *)unit, 5*HZ, plstr);
	device->aca->isra_odd_set = SRB_CMD;

	LOCK(device->lock, getpl());
	while (device->wait_srb_complete)
		SV_WAIT(device->sv, prinet, device->lock);

	if (device->device_timeout)
	{
		cmn_err (CE_WARN, 
			   "tokhwinit(unit=%d) - Config RAM Extensions: Device timed out",
				unit);
		return(0);
	}

	if (device->sm_retc != 0)
	{
		cmn_err (CE_WARN, 
			   "tokhwinit(unit=%d) - SMopen: Config RAM failed", unit);
		return(0);
	}

	/*
	* Open the adapter
	*/

	/* now build SRB Open				*/
	srb_open = (struct srb_open *)device->srb;
	bzero(srb_open, sizeof(struct srb_open));

	srb_open->command = 0x03;

	srb_open->num_rcv_buff = mdi_htons (tokframsz [unit]/SM_RX_BUFDATA_SZ+1);
	srb_open->rcv_buf_len = mdi_htons (SM_RX_BUFFER_SIZE);
	srb_open->dhb_buf_len = mdi_htons (tokframsz [unit]);

	bcopy(tokhwaddr[unit], srb_open->node_address, 6);
	if (device->fdx_enable)
		srb_open->open_options |= mdi_htons (0x0040);  /* enables FDX */

	device->wait_srb_complete = 1;
	device->device_timeout=0;
	device->timeout_id = itimeout(toktimeout, (void *)unit, 30*HZ, plstr);
	device->aca->isra_odd_set = SRB_CMD;	/* tell card that cmd is avail*/

	LOCK(device->lock, getpl());
	while (device->wait_srb_complete)
		SV_WAIT(device->sv, prinet, device->lock);

	if (device->device_timeout)
	{
		cmn_err (CE_WARN, 
			   "tokhwinit(unit=%d) - Device timed out while opening",
				unit);
		return(0);
	}

	/* update shallow mode fields */
	fp_tx_ca = device->fp_tx_ca;
	device->tx_completion_q_head = mdi_ntohs (fp_tx_ca->completion_q_tail);
	device->tx_buffer_count = mdi_ntohs (fp_tx_ca->buffer_count) - 1;

	fp_rx_ca = device->fp_rx_ca;
	device->rx_queue_pointer = mdi_ntohs (fp_rx_ca->posted_q_head);
	

	/*
	* Set Functional Address
	*/

	device->wait_srb_complete = 1;
	bcopy (&device->srb_set_func_addr, device->srb, 
			sizeof(struct srb_set_func_addr));
	device->device_timeout=0;
	device->timeout_id = itimeout(toktimeout, (void *)unit, 5*HZ, plstr);
	device->aca->isra_odd_set = SRB_CMD;

	LOCK(device->lock, getpl());
	while (device->wait_srb_complete)
		SV_WAIT(device->sv, prinet, device->lock);

	if (device->device_timeout)
	{
		cmn_err (CE_WARN,
			"tokhwinit(unit=%d) - Device timed out while setting func addr",
			unit);
		return(0);
	}


	bcopy(tokfuncaddr, device->stats.mac_funcaddr, 6);
	return(device->arb != 0);
}


STATIC int
tokprocess_SMopen_cnf_ext (int unit)				  //thc-sm
{
	struct tokdevice		 *device = &tokdevice[unit];
	struct srb_cnf_ext_resp  *srb_resp;

	srb_resp = (struct srb_cnf_ext_resp *)device->srb;
	device->sm_retc = srb_resp->retc;

	return;
}


STATIC int
tokprocess_SMopen_cnf_ram (int unit)				  //thc-sm
{
	struct tokdevice			*device = &tokdevice[unit];
	srb_cnf_fp_ram_resp		 *srb_resp;

	srb_resp = (struct srb_cnf_fp_ram_resp *)device->srb;
	device->sm_retc = srb_resp->retc;

	if (srb_resp->retc == 0)
	{
		/* update the fast path transmit and receive controls areas */
		device->fp_tx_ca = (struct fp_tx_c_area *)(device->sram + 
								 mdi_ntohs (srb_resp->fp_tx_sram));
		device->fp_rx_ca = (struct fp_rx_c_area *)(device->sram + 
								 mdi_ntohs (srb_resp->fp_rx_sram));

		/* update the srb with the next one */
		device->srb = device->sram + mdi_ntohs (srb_resp->srb_address);
	}

	return;
}


STATIC int
tok_normalopen (unit)					  //thc-sm
{
	struct tokdevice *device = &tokdevice[unit];
	struct tokhwconf *hwconf = &tokhwconf[unit];
	struct srb_open			*srb_open;
	struct srb_open_response	*srb_resp;

	/* now build SRB Open				*/
	srb_open = (struct srb_open *)device->srb;
	bzero(srb_open, sizeof(struct srb_open));
	srb_open->command = 0x03;
	srb_open->num_rcv_buff = mdi_htons((ushort)tok_num_rxbuf(unit));
	srb_open->rcv_buf_len = mdi_htons(RXBUF_LEN);
	srb_open->dhb_buf_len = mdi_htons(TXBUF_LEN);
	srb_open->num_dhb = tok_num_txbuf(unit);
	bcopy(tokhwaddr[unit], srb_open->node_address, 6);
	if (device->fdx_enable)									//thc-fdx
	 srb_open->open_options |= mdi_htons (0x0040);  /* enables FDX */

	device->wait_srb_complete = 1;
	device->device_timeout=0;
	device->timeout_id = itimeout(toktimeout, (void *)unit, 30*HZ, plstr);
	device->aca->isra_odd_set = SRB_CMD;	/* tell card that cmd is avail*/

	LOCK(device->lock, getpl());
	while (device->wait_srb_complete)
		SV_WAIT(device->sv, prinet, device->lock);

	if (device->device_timeout) {
		cmn_err (CE_WARN, 
			"tokhwinit(unit=%d) - Device timed out while attempting to open",
			unit);
		return(0);
	}

	/* now set functional adress */
	device->wait_srb_complete = 1;
	bcopy (&device->srb_set_func_addr, 
			device->srb, sizeof(struct srb_set_func_addr));
	device->device_timeout=0;
	device->timeout_id = itimeout(toktimeout, (void *)unit, 5*HZ, plstr);
	device->aca->isra_odd_set = SRB_CMD;

	LOCK(device->lock, getpl());
	while (device->wait_srb_complete)
		SV_WAIT(device->sv, prinet, device->lock);

	if (device->device_timeout) {
		cmn_err (CE_WARN,
			"tokhwinit(unit=%d) - Device timed out while attempting to set func addr",
			unit);
		return(0);
	}

	bcopy(tokfuncaddr, device->stats.mac_funcaddr, 6);
	return(device->arb != 0);
}

STATIC int
tokprocess_open(int unit)	//thc-sm
{
	struct tokdevice *device = &tokdevice[unit];
	struct srb_open_response *srb_resp;
	static unchar retry_open = 0;


	srb_resp = (struct srb_open_response *)device->srb;

#ifdef DEBUG
	cmn_err (CE_CONT,"tok%d: tokprocess_open retc = [x%x]\n", unit, srb_resp->retc);
#endif

	if (srb_resp->retc == 0) {
		/* operation completed successfully */
		/* set all addresses */
		if (!device->sm_enable)
		{
			device->asb = device->sram + mdi_ntohs(srb_resp->asb_address);
		}

		if (device->fdx_enable &&
			((srb_resp->open_status & OPEN_STATUS_FDX) != 0))
		{
			device->fdx_opened = TRUE;							//thc-fdx
		} else {
			device->fdx_opened = FALSE;						   //thc-fdx
		}

		device->srb = device->sram + mdi_ntohs(srb_resp->srb_address);
		device->arb = device->sram + mdi_ntohs(srb_resp->arb_address);
		device->ssb = device->sram + mdi_ntohs(srb_resp->ssb_address);
		retry_open = 0;
		return(1);
	}
	/* an error has occurred */
	if ( (srb_resp->retc == 0x7) &&
		(mdi_ntohs(srb_resp->open_error_code) == 0x24) &&
		(device->ring_speed_detector) &&
		(device->ring_speed_save) &&
		(++retry_open == 1) )
		/* First open fails, because of frequency error (0x24).
		 * If adapter can detect adapter and ring speed mismatch
		 * and can automatically update the default ring speed,
		 * reset ring speed and retry open.
		 * But only retry once.
		 */
	{
		struct srb_open *srb_open;

#ifdef DEBUG
		cmn_err (CE_CONT, "tok%d: tokprocess_open recovering freq. error\n", unit);
#endif

		device->ring_speed =
			(device->ring_speed==16000000L) ? 4000000L : 16000000L;

		srb_open = (struct srb_open *)device->srb;
		bzero(srb_open, sizeof(struct srb_open));
		srb_open->command = 0x03;

		if (device->sm_enable)									 //thc-sm
		{
			srb_open->num_rcv_buff = 
					mdi_htons (tokframsz [unit]/SM_RX_BUFDATA_SZ+1);
			srb_open->rcv_buf_len = mdi_htons (SM_RX_BUFFER_SIZE);
			srb_open->dhb_buf_len = mdi_htons (tokframsz [unit]);
		} else {
			srb_open->num_rcv_buff = mdi_htons((ushort)tok_num_rxbuf(unit));
			srb_open->rcv_buf_len = mdi_htons(RXBUF_LEN);
			srb_open->dhb_buf_len = mdi_htons(TXBUF_LEN);
			srb_open->num_dhb = tok_num_txbuf(unit);
		}
		bcopy(tokhwaddr[unit], srb_open->node_address, 6);
		if (device->fdx_enable)									//thc-fdx
			srb_open->open_options |= mdi_htons (0x0040);  /* enables FDX */

		device->aca->isra_odd_set = SRB_CMD;
		return(0);
	}

	if (retry_open > 1)
		cmn_err(CE_WARN, "tok(unit=%d) - retry open failed", unit);

#ifdef DEBUG
	cmn_err (CE_CONT, "tok%d: tokprocess_open ERROR = [x%x]\n",
			unit, mdi_ntohs(srb_resp->open_error_code));
#endif

	retry_open = 0;
	tokdiag_openerr(unit, srb_resp->retc, mdi_ntohs(srb_resp->open_error_code));
	device->asb = device->arb = device->ssb = 0;
	return(1);
}

/* This function will close adapter */
void
tokhwhalt(int unit)
{
	struct tokdevice *device = &tokdevice[unit];
	int x;
	
	device->wait_srb_complete = 1;
	device->device_timeout=0;
	device->timeout_id = itimeout(toktimeout, (void *)unit, 5*HZ, plstr);

	x = splstr();
	if (device->srb_busy)
		device->wait_send_close_srb = 1;
	else
		tok_issue_close_srb(device);
	splx(x);

	LOCK(device->lock, getpl());
	while (device->wait_srb_complete)
		SV_WAIT(device->sv, prinet, device->lock);

	if (device->device_timeout)
		cmn_err(CE_NOTE, "tokhwhalt(unit=%d) - Device timed out while attempting to close", unit);
}

STATIC void
tok_issue_close_srb(struct tokdevice *device)
{
	if (device->srb_busy)
		cmn_err(CE_WARN, "tok(issue_close_srb) - SRB is busy");

	device->srb_busy = 1;
	bcopy(&device->srb_close, device->srb, sizeof(struct srb_close));
					/* tell card that cmd is avail*/
	device->aca->isra_odd_set = SRB_CMD|SRB_FREE_REQ;
}

/*
 * This function should be called to signal that the adapter has failed
 * and is being closed
 */
STATIC void
tokhwfailed(struct tokdevice *device)
{
	mblk_t *mp;
	struct mac_hwfail_ind *hp;
	
	mp = allocb(sizeof(struct mac_hwfail_ind), BPRI_HI);
	if (mp) {
		hp = (struct mac_hwfail_ind *)(mp->b_rptr);
		mp->b_datap->db_type = M_PCPROTO;
//		mp->b_wptr += sizeof(struct mac_hwfail_ind);
		mp->b_wptr = mp->b_rptr + sizeof(struct mac_hwfail_ind);

		hp->mac_primitive = MAC_HWFAIL_IND;
//gem		hp->mac_mcast_length = 0;
//gem		hp->mac_mcast_offset = sizeof(struct mac_hwfail_ind);
		
		putnext(device->up_queue, mp);
	}
	device->mdi_state = MDI_CLOSED;
}

/* thc-sm
 * Send a frame to the ring in shallow mode.
 * it will return  FALSE if there isn't enough buffer available.
 */
int
tokSMtx (int unit, mblk_t *mp)					   //thc-sm
{
	struct tokdevice      *device;
	volatile fp_tx_c_area *fp_tx_ca;
	ushort                len;
	mblk_t                *m, *mp1;
	ushort                frame_len;
	ushort                frag_len;
	ushort                n_buffers_neded;
	ushort                n_buffers_used;
	ushort                free_q_head_offset;
	sm_tx_buf             *free_q_head;
	ushort                next_buff_offset_n;
	ushort                last_buff_offset;
	sm_tx_buf             *tx_buff;
	mblk_t                *mp_aux;

	device = &tokdevice[unit];

	/*
	* check packet length and if there is enough buffers available.
	*/
	frame_len = 0;
	for (mp_aux = mp; mp_aux; mp_aux = mp_aux->b_cont)
	{
		frame_len += (mp_aux->b_wptr - mp_aux->b_rptr);
	}

	n_buffers_neded = (frame_len + SM_TX_BUFDATA_SZ - 1)/(SM_TX_BUFDATA_SZ);
	if (n_buffers_neded > device->tx_buffer_count)
	{
		return (FALSE);
	}

	fp_tx_ca = device->fp_tx_ca;
	free_q_head_offset = mdi_ntohs (fp_tx_ca->free_q_head);
	free_q_head = (struct sm_tx_buf *) (device->sram + 
									   free_q_head_offset - SM_OFFSET_TXNEXT);

	next_buff_offset_n = fp_tx_ca->free_q_head;
	mp_aux = mp;
	n_buffers_used = 0;
	while (mp_aux)
	{
		if (fp_tx_ca->free_q_tail == next_buff_offset_n)
		{
//			cmn_err(CE_WARN, "tokSMtx(unit=%d) - adapter buffers fail", unit);
//			freemsg(mp);
//			return (TRUE);
			return (FALSE);
		}

		last_buff_offset = mdi_ntohs (next_buff_offset_n);
		tx_buff = (struct sm_tx_buf *) (device->sram +
					last_buff_offset - SM_OFFSET_TXNEXT);
		next_buff_offset_n = tx_buff->next_buffer;
		n_buffers_used++;

		mp_aux = tokfill_txbuffer (mp_aux, tx_buff);
	}
	freemsg(mp);
	device->tx_buffer_count += n_buffers_used;

	tx_buff = free_q_head;

	free_q_head->retcode = 0xFE;
	free_q_head->station_id = 0;
	free_q_head->frame_len = mdi_htons (frame_len);
	free_q_head->last_buffer = mdi_htons (last_buff_offset);

	fp_tx_ca->free_q_head = next_buff_offset_n;

	/* issue the transmit request */
	device->aca->isra_odd_set = SM_TX_REQUEST;

	return (TRUE);
}


STATIC mblk_t *
tokfill_txbuffer (mblk_t *mp, sm_tx_buf *tx_buff)
{
	mblk_t  *mp_aux;
	unchar  *p_data;
	ushort  data_len;
	unchar  *p_buff;
	ushort  buff_space;
	ushort  buff_len;

	p_buff = tx_buff->frame_data;
	buff_space = SM_TX_BUFDATA_SZ;
	buff_len =0;
	for (mp_aux = mp; mp_aux && buff_space > 0; mp_aux = mp_aux->b_cont)
	{
		p_data = mp_aux->b_rptr;
		data_len = mp_aux->b_wptr - p_data;
		if (data_len > buff_space) {
			bcopy (p_data, p_buff, buff_space);
			buff_len += buff_space;
			mp_aux->b_rptr += buff_space;
			buff_space = 0;
			break;
		}

		bcopy(p_data, p_buff, data_len);
		buff_len += data_len;
		p_buff += data_len;
		buff_space -= data_len;
	}
	tx_buff->buffer_len = mdi_htons (buff_len);

	return (mp_aux);
}


STATIC void
tokSMtx_completion (int unit)		 //thc-sm
{
	struct tokdevice      *device;
	volatile fp_tx_c_area *fp_tx_ca;
	sm_tx_buf             *frame_ptr;
	sm_tx_buf             *frame;
	ushort                frame_len;
	ushort                n_buffers_used;
	queue_t               *q;
	mblk_t                *mp;

	device = &tokdevice[unit];
	fp_tx_ca = device->fp_tx_ca;

	while (device->tx_completion_q_head !=
		mdi_ntohs (fp_tx_ca->completion_q_tail))
	{
		frame_ptr = (struct sm_tx_buf *) (device->sram +
				device->tx_completion_q_head - SM_OFFSET_TXNEXT);

		frame = (struct sm_tx_buf *) (device->sram +
				mdi_ntohs (frame_ptr->next_buffer) - SM_OFFSET_TXNEXT);

		/*
		 * Calculate the number of buffers used by this frame and release them
		 *
		 * We assume that for all buffers, except the last, the Buffer.length
		 * is SM_TX_BUFDATA_SZ.
		 */
		frame_len = mdi_ntohs (frame->frame_len);
		n_buffers_used = (frame_len + SM_TX_BUFDATA_SZ - 1)/(SM_TX_BUFDATA_SZ);
		device->tx_buffer_count += n_buffers_used;

		device->tx_completion_q_head = mdi_ntohs (frame->last_buffer);
		fp_tx_ca->free_q_tail = frame->last_buffer;
	}

	/*
	* Verify if there are a message in in the trasnmit queue.
	* If so send-it using tokSMtx.
	*/
	q = WR(device->up_queue);
	if (mp=getq(q))
	{
		if (!tokSMtx (unit, mp))
			putbq (q,mp);
	}
}


STATIC void
tokSM_process_recv (int unit)		 //thc-sm
{
	struct   tokdevice	  *device;
	volatile fp_rx_c_area   *fp_rx_ca;

	sm_rx_buf		   *frame;
	sm_rx_buf		   *frame_inic;
	sm_rx_buf		   *last_frame;
	ushort			  last_frame_offset;

	ushort			  nbufs_posted;
	ushort			  nbufs_fr;



	device = &tokdevice [unit];

	fp_rx_ca = device->fp_rx_ca;

	nbufs_posted = mdi_ntohs (fp_rx_ca->posted_count) - 
				  mdi_ntohs (fp_rx_ca->completion_count);
	while (nbufs_posted > 0)
	{
		frame = (struct sm_rx_buf *) (device->sram +
				device->rx_queue_pointer - SM_OFFSET_RXNEXT);

		frame_inic = (struct sm_rx_buf *) (device->sram +
				mdi_ntohs (frame->next_buffer) - SM_OFFSET_RXNEXT);

		/* find the end of frame */
		last_frame = 0;
		for (nbufs_fr = 1;
			(nbufs_fr <= nbufs_posted) && (frame->next_buffer != 0);
			nbufs_fr++)
		{
			last_frame_offset = mdi_ntohs (frame->next_buffer);
			frame = (struct sm_rx_buf *) (device->sram +
				   last_frame_offset - SM_OFFSET_RXNEXT);

			if (frame->receive_status & SM_RX_STATUS_EOF) {
				last_frame = frame;
				break;
			}
		}

		if (last_frame == 0) {
#ifdef DEBUG
			cmn_err (CE_CONT, "tok%d: ERROR: end of rx not found\n", unit);
			cmn_err (CE_CONT, "fp_rx_ca: [x%x] - next_buffer: [x%x] - nbufs_fr: [%d]\n",
			   fp_rx_ca, mdi_ntohs (frame->next_buffer), nbufs_fr);
			cmn_err (CE_CONT, "Posted.Count: [x%x] Completion.Count: [x%x] nbufs_posted: [%d]\n",
				mdi_ntohs (fp_rx_ca->posted_count),
				mdi_ntohs (fp_rx_ca->completion_count),
				nbufs_posted);
#endif

			break;
		}

		tokSM_rxframe (device, frame_inic, last_frame, nbufs_fr);

		device->rx_queue_pointer = last_frame_offset;
		fp_rx_ca->completion_count  =
			  mdi_htons (mdi_ntohs (fp_rx_ca->completion_count) + nbufs_fr);

		/* issue the rx buffer complete request */
		device->aca->isra_odd_set = SM_RXBUFF_CPLT;

		/* verify if there more buffers posted. */
		nbufs_posted = mdi_ntohs (fp_rx_ca->posted_count) - 
				 mdi_ntohs (fp_rx_ca->completion_count);
	}
}

STATIC int
tokSM_rxframe (struct tokdevice *device,			  //thc-sm
			  sm_rx_buf *frame_inic, sm_rx_buf *frame_last,
			  ushort nbufs_fr)
{
	mblk_t	   *mp;

	sm_rx_buf	*frame;
	int		  frame_len;

	int		  buff_len;
	int		  dif_last;
	int		  i;

	frame_len = mdi_ntohs (frame_last->frame_len) - 4;

	if (frame_len > MAX_PDU)
	{

#ifdef DEBUG
		cmn_err (CE_CONT, "tokSM_rxframe - frame too large\n");
#endif

		device->stats.mac_badlen++;
		return (FALSE);
	}

	if (frame_len <= 4)  //thc??
	{
#ifdef DEBUG
		cmn_err (CE_CONT, "tokSM_rxframe - frame too small\n");
#endif

		device->stats.mac_badlen++;
		return (FALSE);
	}

	if (!(mp = allocb(frame_len,BPRI_MED)))
	{

#ifdef DEBUG
		cmn_err (CE_CONT, "tokSM_rxframe - no message block available\n");
#endif

		device->stats.mac_frame_nosr++;
		return (FALSE);
	}

	buff_len = mdi_ntohs (frame_last->buffer_len);
	if (buff_len > 4)
		dif_last = 4;
	else {
		nbufs_fr -= 1;
		dif_last = 4 - buff_len;
	}

        mp->b_wptr = mp->b_rptr;   //th
	frame = frame_inic;
	for (i = 1; i <= nbufs_fr; i++) {
		buff_len = mdi_ntohs (frame->buffer_len);

		if (i == nbufs_fr)	//thc??
			buff_len -= dif_last;

		bcopy (frame->frame_data, mp->b_wptr, buff_len);
		mp->b_wptr += buff_len;

		frame = (struct sm_rx_buf *) (device->sram +
			   mdi_ntohs (frame->next_buffer) - SM_OFFSET_RXNEXT);
	}

	/*
	* We have a packet from the ring, check the first two bytes of the
	* destination address to see if it starts with 'C000', if it does
	* then its possibly a Multi-cast address, perform filtering.
	*/
	if (*(mp->b_rptr+2)==0xc0 && *(mp->b_rptr+3)==0x00) {
     		if (!tokcheckaddr(device, (unchar *)(mp->b_rptr+2))) {
#ifdef DEBUG
			cmn_err (CE_CONT, "tokSM_rxframe - discarding Multicast address\n");
#endif

			freemsg(mp);
			return (FALSE);
		}
	}
	if (device->up_queue)
		putnext(device->up_queue, mp);
	else {
#ifdef DEBUG
		cmn_err (CE_CONT, "tokSM_rxframe - no upperstream queue\n");
#endif

		freemsg(mp);
	}

	return (TRUE);
}


/*
 * The driver has a frame to send, so tell the adapter.  When the adapter
 * is ready to send, enable the WR(q) so that toksend_queued will run and that
 * will call tokhwput to send the frame to the network
 */
void
tokstartio(struct tokdevice *device)
{
	/* Tell the adapter we wish to send something */
	if (device->srb_busy) {
		device->wait_send_xmt_srb = 1;
	} else {
		tok_issue_send_srb(device);
	}
}

STATIC void
tok_issue_send_srb(struct tokdevice *device)
{
	if (device->srb_busy)
		cmn_err(CE_WARN, "tok(issue_send_srb) - SRB is busy");

	device->srb_busy = 1;
	bcopy(&device->srb_send, device->srb, sizeof(struct srb_send));
	/* tell card that cmd is avail*/
	device->aca->isra_odd_set = SRB_CMD|SRB_FREE_REQ;
}

/* Called when the SRB TRANSMIT.DIR.frame command has completed */
STATIC void
tok_process_startio(struct tokdevice *device)
{
	struct arb_xmt_cmd  *arbp = (struct arb_xmt_cmd *)device->arb;

	device->txbufp = device->sram + mdi_ntohs(arbp->dhb_address);
	device->asb_xmt_resp.correlate = arbp->correlate;
	toksend_queued(WR(device->up_queue));
}

/*
 * Send a frame to the ring.  This function actually only queues the transfer
 * it will return an error (-1) if there is an immediate error.
 */
void
tokhwput(int unit, mblk_t *mp)
{
	struct tokdevice *device = &tokdevice[unit];
	unchar	*tx_buf;
	ushort	len, tlen;
	mblk_t *mp1;

	if (!device->txbufp) {
		cmn_err(CE_WARN, "tokhwput(unit=%d) - Called with no send buffer", unit);
		freemsg(mp);
		return;
	}
	tx_buf = device->txbufp;
	device->txbufp = (unchar *)0;

	/* We have previously told the adapter we wish to send */
	/* something, now we should actually send the data */

	tlen = 0;
	mp1=mp;
	while (mp1) {
		len = mp1->b_wptr - mp1->b_rptr;
		if ((uint)(tlen + len) > (uint)TXBUF_LEN) {
			device->stats.mac_badlen++;
			break;
		}
		tlen += len;
		bcopy(mp1->b_rptr, tx_buf, len);
		tx_buf += len;
		mp1 = mp1->b_cont;
	}
	freemsg(mp);
	/* Build ASB Command Block	  */
	device->asb_xmt_resp.frame_len = mdi_ntohs(tlen);

	if (device->asb_busy) {
		device->wait_send_xmt_asb = 1;
	} else {
		tok_issue_send_asb(device);		/* Send the ASB now */
	}
}

STATIC void
tok_issue_send_asb(struct tokdevice *device)
{
	if (device->asb_busy)
		cmn_err(CE_WARN, "tok(issue_send_asb) - ASB is busy");
	
	device->asb_busy = 1;
	bcopy(&device->asb_xmt_resp, device->asb, sizeof(struct asb_xmt_resp));
	device->aca->isra_odd_set = NM_ASB_RESP|NM_ASB_FREE_REQ; /* send ASB */

	tokstartio(device);			/* Preallocate next tx buf */
}

STATIC void
tok_process_modify_open(int unit)
{
	struct tokdevice *device = &tokdevice[unit];
	struct srb_modify_open *srbp = (struct srb_modify_open *)device->srb;

	if (srbp->retc)
		tokdiag_retc(unit, "DIR.MODIFY.OPEN.PARMS", srbp->retc);
}

STATIC void
tok_process_send_error(int unit)
{
	struct tokdevice *device = &tokdevice[unit];
	struct srb_send *srbp = (struct srb_send *)device->srb;

	if (srbp->retc = 0xff)
		return;
	cmn_err(CE_WARN, "tok(unit=%d) - Send failed, retc=%b",unit,srbp->retc);
	tokhwfailed(device);
}

STATIC void
tok_send_waiting_asb(struct tokdevice *device)
{
	device->asb_busy = 0;

	if (device->wait_send_xmt_asb) {
		device->wait_send_xmt_asb = 0;
		tok_issue_send_asb(device);
	} else if (device->wait_send_rcv_asb) {
		device->wait_send_rcv_asb = 0;
		tok_issue_recv_asb(device);
	}
}

STATIC void
tok_send_waiting_srb(struct tokdevice *device)
{
	device->srb_busy = 0;
	if (device->wait_send_close_srb) {
		device->wait_send_close_srb = 0;
		tok_issue_close_srb(device);
	} else if (device->wait_send_xmt_srb) {
		device->wait_send_xmt_srb = 0;
		tok_issue_send_srb(device);
	} else if (device->wait_send_log_srb) {
		device->wait_send_log_srb = 0;
		tok_issue_log_srb(device);
	} else if (device->wait_send_mod_srb) {
		device->wait_send_mod_srb = 0;
		tok_issue_mod_srb(device);
	}
}

void
tokintr(int lev)
{
	int			unit = tokirq_to_unit[lev];
	struct tokdevice	*device = &tokdevice[unit];
	unchar		isrp_odd;
	unchar		count;
	unchar		x;

	if (device->mdi_state == MDI_NOT_PRESENT) {
		device->stats.mac_spur_intr++;
		return;
	}

	count = 50;
	isrp_odd = device->aca->isrp_odd;

	while (--count && isrp_odd)
	{
		if (device->sm_enable)						 //thc-sm
		{
			/* shallow mode operation */
			if (isrp_odd & SM_RXBUFF_POST) {
				device->aca->isrp_odd_rst= ~SM_RXBUFF_POST;
				tokSM_process_recv (unit);
			}

			if (isrp_odd & SM_TXCPLT) {
				device->aca->isrp_odd_rst= ~SM_TXCPLT;
				tokSMtx_completion (unit);
			}
		} else {
			/* normal mode operation */
			if (isrp_odd & NM_ASB_FREE) {
				device->aca->isrp_odd_rst= ~NM_ASB_FREE;
				tok_send_waiting_asb(device);
			}

			if (isrp_odd & NM_SSB_RESP) {
				struct ssb_xmt_resp *ssbp;

				device->aca->isrp_odd_rst= ~NM_SSB_RESP;

				ssbp = (struct ssb_xmt_resp *)device->ssb;
				switch (ssbp->retc) {
				case 0x00:	/* Completed sucessfully - fall through */
				case 0x22:	/* Status from remote MAC */
					break;
				default:
					cmn_err(CE_NOTE, "tokintr(unit=%d) - Send error, code=%b",
						unit, ssbp->retc);
				}

				device->aca->isra_odd_set = NM_SSB_FREE;
			}
		}

		if (isrp_odd & SRB_RESP)	/* SRB response		 */
		{
			device->aca->isrp_odd_rst= ~SRB_RESP;

			switch (*(device->srb)) {
			case 0x01:		/* DIR.MODIFY.OPEN.PARMS */
				tok_process_modify_open(unit);
				break;
			case 0x03:		/* DIR.OPEN.ADAPTER */
				if (tokprocess_open(unit) == 0 )
					break;
			case 0x04:		/* DIR.CLOSE.ADAPTER - fall through */
			case 0x07:		/* DIR.SET.FUNCTIONAL.ADDR */
				if (device->wait_srb_complete) {
					device->wait_srb_complete = 0;
					untimeout(device->timeout_id);
					SV_SIGNAL(device->sv, 0);
				}
				break;
			case 0x08:
				tok_process_read_log(unit);
				break;
			case 0x0a:		/* TRANSMIT.DATA.REQ */
				tok_process_send_error(unit);
				break;
			case 0x12:		/* DIR.CONFIG.FAST.PATH.RAM */ //thc-sm
				tokprocess_SMopen_cnf_ram (unit);
				if (device->wait_srb_complete) {
					device->wait_srb_complete = 0;
					untimeout(device->timeout_id);
					SV_SIGNAL(device->sv, 0);
				}
				break;
			case 0x20:		/* DIR.CONFIG.EXTENSIONS */   //thc-sm
				tokprocess_SMopen_cnf_ext (unit);
				if (device->wait_srb_complete) {
					device->wait_srb_complete = 0;
					untimeout(device->timeout_id);
					SV_SIGNAL(device->sv, 0);
				}
				break;
			default:
				cmn_err(CE_WARN,
					"tokintr(unit=%d) - Unknown SRB response %b",
					unit, *(device->srb));
			}

			device->srb_busy = 0;
			tok_send_waiting_srb(device);
		}

		if (isrp_odd & ARB_CMD)		/* ARB command		  */
		{
			device->aca->isrp_odd_rst= ~ARB_CMD;

			switch (*(device->arb)) {
			case 0x82:		/* TRANSMIT.DATA.REQ */
				tok_process_startio(device);
				break;
			case 0x85:	/* Global broadcast is passed as bridge data */
			case 0x81:	/* Received Data	*/
				tok_process_recv(unit);
				break;
			case 0x84: 		/* RING.STATUS.CHANGE */
				tok_process_ring_status_change(unit);
				break;
			default:
				cmn_err(CE_WARN,
					"tokintr(unit=%d) - Unknown ARB command 0x%b",
					unit, *(device->arb));
			}

			device->aca->isra_odd_set = ARB_FREE;
		}

		if (!device->sm_enable)									 //thc-sm
		{
			/* normal mode operation */
			if (isrp_odd & NM_IMPL_RCV)	/* Force IMPL MAC frame */
				device->aca->isrp_odd_rst= ~NM_IMPL_RCV;
		}

		if (isrp_odd & ADAP_CHK)	/* Adapter check		*/
		{
			cmn_err(CE_WARN, "tokintr(unit=%d) - Adapter Check", unit);
			tokhwfailed(device);
			device->aca->isrp_odd_rst= ~ADAP_CHK;
			return;						/* MDI003 */
		}

		/* Adapter error */
		if (device->aca->isrp_even & (TIMER_INT|ERROR_INT|ACCESS_INT)) {
			cmn_err(CE_WARN, "tokintr(unit=%d) - Adapter Failure: 0x%b",
				unit, device->aca->isrp_even);
			tokhwfailed(device);
			device->aca->isrp_even_rst = ~(TIMER_INT|ERROR_INT|ACCESS_INT);
			return;						/* MDI003 */
		}

		isrp_odd = device->aca->isrp_odd;
	} /* while */

	outb(TOKEN_INTR_REL(unit), 0);		/* release interrupt */
	if (!count) {
		cmn_err(CE_WARN, "tokintr(unit=%d) - Fatal interrupt flood", unit);
		tokhwfailed(device);
	}
}

STATIC void
tok_process_ring_status_change(int unit)
{
	struct tokdevice	*device = &tokdevice[unit];
	struct arb_ring_status *arbp = (struct arb_ring_status *)device->arb;
	ushort old_s = device->stats.mac_ringstatus;
	ushort new_s = mdi_ntohs(arbp->ring_status);
	ushort changed = old_s ^ new_s;			/* X-OR */

	device->stats.mac_ringstatus = new_s;

	if (changed & RST_RING_RECOVERY) {
		if (new_s & RST_RING_RECOVERY)
			device->stats.mac_recoverys++;
	}
	if (changed & RST_SINGLE_STATION) {
		if (new_s & RST_SINGLE_STATION)
			device->stats.mac_statssingles++;
	}

#ifdef NON_FATAL_MESSAGE	/* MDI001 */
	if (new_s & RST_COUNTER_OVERFLOW) {
		cmn_err(CE_WARN, "tok(unit=%d) - Status: Counter overflow", unit);
	}
#endif

	if (new_s & RST_FDX_ERROR) {	//thc-fdx
		cmn_err(CE_WARN, "tok(unit=%d) - Status: FDX protocol error, adapter closing",unit);						//thc-fdx
		tokhwfailed(device);		//thc-fdx
	}

	if (new_s & RST_REMOVE_RECEIVED) {
		cmn_err(CE_WARN, "tok(unit=%d) - Status: Remove received, adapter closing",unit);
		tokhwfailed(device);
	}
	if (new_s & RST_AUTO_RMV_ERROR) {
		cmn_err(CE_WARN, "tok(unit=%d) - Status: H/W Error detected", unit);
		tokhwfailed(device);
	}
	if (new_s & RST_LOBE_WIRE_FAULT) {
		cmn_err(CE_WARN,"tok(unit=%d) - Status: Open/Short circuit in data path",unit);
		tokhwfailed(device);
	}
	if (changed & RST_TRANSMIT_BEACON) {
		if (new_s & RST_TRANSMIT_BEACON)
			device->stats.mac_transmitbeacons++;
	}
	if (changed & RST_SOFT_ERROR) {
		if (new_s & RST_SOFT_ERROR)
			device->stats.mac_softerrors++;
	}
	if (changed & RST_HARD_ERROR) {
		if (new_s & RST_HARD_ERROR)
			device->stats.mac_harderrors++;
	}
	if (changed & RST_SIGNAL_LOSS) {
		if (new_s & RST_SIGNAL_LOSS)
			device->stats.mac_signalloss++;
	}
}

int
tokcheckaddr( struct tokdevice *device, unchar *l )
{
	int i;
        macaddr_t MCA;

	bcopy (l, (unchar *)MCA, MDI_MACADDRSIZE);

	if ((ADDR_TO_LONG(l)) == 0xffffffff)
		return(1);
	if (device->all_mca)
		return(1);
        if (mdi_valid_mca (device->mac_bind_cookie, MCA))
                return(1);

	return(0);
}

STATIC void
tok_process_recv(int unit)
{
	struct tokdevice	*device = &tokdevice[unit];
	struct arb_rcv_data	*arbp = (struct arb_rcv_data *)device->arb;

	struct rcv_buf		*rcv_buf;		/* Receive Buffer pointer */
	mblk_t *mp;

	ushort x;

	int i, frame_len;
	long l;

	device->asb_rcv_resp.rcv_buf_ptr = arbp->rcv_buf_ptr;
	x = mdi_ntohs(arbp->rcv_buf_ptr);
	frame_len = mdi_ntohs(arbp->frame_len);

	if (frame_len > MAX_PDU) {
		device->stats.mac_badlen++;
		if (device->asb_busy)
			device->wait_send_rcv_asb = 1;
		else
			tok_issue_recv_asb(device);
		return;
	}

	if (!(mp = allocb(frame_len,BPRI_MED))) {
		device->stats.mac_frame_nosr++;
		if (device->asb_busy)
			device->wait_send_rcv_asb = 1;
		else
			tok_issue_recv_asb(device);
		return;
	}

        mp->b_wptr = mp->b_rptr;
	while (1) {
		int j;

		rcv_buf = (struct rcv_buf *)(device->sram + x);

		j = mdi_ntohs(rcv_buf->frame_len);
		bcopy(rcv_buf->data, mp->b_wptr, j);
		mp->b_wptr += j;
		if (rcv_buf->rcv_buf_ptr == 0)
			break;
		x = mdi_ntohs(rcv_buf->rcv_buf_ptr)-2;
	}
	/* We have copied the data from the ARB, now issue the ASB response */
	if (device->asb_busy) {
		device->wait_send_rcv_asb = 1;
	} else {
		tok_issue_recv_asb(device);
	}

	/*
	 * We have a packet from the ring, check the first two bytes of the
	 * destination address to see if it starts with 'C000', if it does
	 * then its possibly a Multi-cast address, perform filtering.
	 */
	if (*(mp->b_rptr+2)==0xc0 && *(mp->b_rptr+3)==0x00) {
      		if (!tokcheckaddr(device, (unchar *)(mp->b_rptr+2))) {
			freemsg(mp);
			return;
		}
	}
	if (device->up_queue)
		putnext(device->up_queue, mp);
	else
		freemsg(mp);
	return;
}

STATIC void
tok_issue_recv_asb(struct tokdevice *device)
{
	if (device->asb_busy)
		cmn_err(CE_WARN, "tok(issue_recv_asb) - ASB is busy");
	device->asb_busy = 1;
	bcopy(&device->asb_rcv_resp, device->asb, sizeof(struct asb_rcv_resp));
	device->aca->isra_odd_set = NM_ASB_RESP|NM_ASB_FREE_REQ;  /* send ASB */
}

STATIC void
tok_issue_log_srb(struct tokdevice *device)
{
	if (device->srb_busy)
		cmn_err(CE_WARN, "tok(issue_log_srb) SRB is busy");

	device->srb_busy = 1;
	bcopy(&device->srb_read_log, device->srb, sizeof(struct srb_read_log));
	device->aca->isra_odd_set = SRB_CMD|SRB_FREE_REQ;
}

STATIC void
tok_issue_mod_srb(struct tokdevice *device)
{
	if (device->srb_busy)
		cmn_err(CE_WARN, "tok(issue_mod_srb) SRB is busy");

	device->srb_busy = 1;
	bcopy(&device->srb_modify_open, device->srb, sizeof(struct srb_modify_open));
	device->aca->isra_odd_set = SRB_CMD|SRB_FREE_REQ;
}

STATIC void
tok_process_read_log(int unit)
{
	struct tokdevice *device = &tokdevice[unit];
	struct srb_read_log *srbp = (struct srb_read_log *)device->srb;
	mblk_t		*mp = device->iocmp;
	struct iocblk	*iocp;

	if (mp)
		iocp = (struct iocblk *)mp->b_rptr;
	if (srbp->retc) {
		if (mp) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_count = 0;
		}
	} else {
		device->stats.mac_lineerrors += srbp->line;
		device->stats.mac_internalerrors += srbp->internal;
		device->stats.mac_bursterrors += srbp->burst;
		device->stats.mac_acerrors += srbp->ac;
		device->stats.mac_aborttranserrors += srbp->abort;
		device->stats.mac_lostframeerrors += srbp->lost_frames;
		device->stats.mac_receivecongestions += srbp->congestion;
		device->stats.mac_framecopiederrors += srbp->copied;
		device->stats.mac_frequencyerrors += srbp->freq;
		device->stats.mac_tokenerrors += srbp->token;
		if (mp) {
			mp->b_datap->db_type = M_IOCACK;
			bcopy(&device->stats, mp->b_cont->b_rptr, sizeof(mac_stats_tr_t));
		}
	}
	if (mp) {
		device->iocmp = (mblk_t *)0;
		putnext(device->up_queue, mp);
	}
}

/*
 * This function issues a DIR.READ.LOG to obtain statistics, and then
 * returns them in 'mp'
 */
void
tokupdate_stats(int unit, mblk_t *mp)
{
	struct tokdevice	*device = &tokdevice[unit];

	device->iocmp = mp;
	
	ADDR_TO_LONG(device->stats.mac_funcaddr) = device->addr->func_addr; 
	bcopy(device->parms->up_node_addr, device->stats.mac_upstream, 6);
	if (device->srb_busy)
		device->wait_send_log_srb = 1;
	else
		tok_issue_log_srb(device);
}

static struct tok_diag retc_t[] = {
	{ 0x00, "Operation completed OK (no error)" },
	{ 0x01, "Invalid command code" },
	{ 0x03, "Adapter should be closed, but isn't" },
	{ 0x04, "Adapter should be open, but isn't" },
	{ 0x07, "Command canceled, unrecoverable failure" },
	{ 0x09, "Adapter not initialized" },
	{ 0x0B, "Adapter closed while in use" },
	{ 0x0C, "Command completed, adapter is now closed" },
	{ 0x11, "Lobe failure, check connection between host and ring" },
	{ 0x19, "Insufficient buffers for request" },
	{ 0x20, "Dropped data on receipt, no buffers" },
	{ 0x21, "Dropped data on receipt, no buffer space" },
	{ 0x22, "Error detected during a receive" },
	{ 0x25, "Maximum commands exceeded" },
	{ 0x26, "Unrecognised command correlator" },
	{ 0x27, "Link not transmitting I-frames" },
	{ 0x28, "Invalid transmit frame length" },
	{ 0x30, "Not enough receive buffers for adapter to open" },
	{ 0x32, "Invalid NODE address" },
	{ 0x33, "Invalid adapter receive buffer length defined" },
	{ 0x34, "Invalid adapter transmit buffer length defined" },
	{ 0x40, "Invalid station ID" },
	{ 0x43, "Invalid SAP / SAP in use" },
	{ 0x44, "Invalid routing field length" },
	{ 0x46, "Inadequate link stations" },
	{ 0x47, "SAP cannot close until all SAP's stations are closed" },
	{ 0x5A, "Cannot initialize adapter" },
	{ -1, 0 }
};

static struct tok_diag open_t[] = {
	{ 0x0f, "FDX Protocol Error" },								//thc-fdx
	{ 0x24, "Frequency error, adapter does not match ring speed" },
	{ 0x26, "Ring failure, try again or contact system admin" },
	{ 0x27, "Wrong data rate / claim token failure" },
	{ 0x2A, "Ring connect timeout - try again in about 10 minutes" },
	{ 0x2D, "No ring monitor detected" },
	{ 0x38, "Duplicate node address" },
	{ 0x42, "Signal loss, ring failure" },
	{ -1, 0 }
};

STATIC char *
tok_check_t(struct tok_diag *table, unchar errno)
{
	static char unknown[]="Error 0xXX is unknown";
	static char atox[] = "0123456789abcdef";

	for (; table->i != -1; table++)
		if (table->i == errno)
			return (table->s);
	table = retc_t;
	for (; table->i != -1; table++)
		if (table->i == errno)
			return (table->s);
	unknown[8] = atox[errno >> 4];
	unknown[9] = atox[errno & 0xf];
	return (unknown);
}

STATIC void
tokdiag_retc(int unit, char *desc, int errno)
{
	cmn_err(CE_WARN, "tok(unit=%d) - %s failed:", unit, desc);
	cmn_err(CE_WARN, "\t\"%s\"", tok_check_t(retc_t, (unchar)errno));
}

STATIC void
tokdiag_openerr(int unit, unchar errno, ushort open_err)	 //thc-fdx
{
	cmn_err(CE_WARN, "tok(unit=%d) - Adapter open failed:", unit);

	if (errno == 0x07) {
		if (((unchar)open_err & 0x0f) == 0x0f)		 //thc-fdx
			cmn_err(CE_WARN, "\t\"%s\"",
                                 tok_check_t(open_t, 0x0f)); //thc-fdx
		else						 //thc-fdx
			cmn_err(CE_WARN, "\t\"%s\"",
                                tok_check_t(open_t, (unchar)open_err));
	} else
		cmn_err(CE_WARN, "\t\"%s\"",
                          tok_check_t(retc_t, (unchar)errno));
}
