#ident "@(#)tokli.c	28.1"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 *
 * History:
 *	MDI003	7 Mar 1995
 *	Added warning for 8k RAM at 16mbps bus
 */

/*
 * 	MDI Driver for the IBM Token Ring Adapter
 */

#ifdef _KERNEL_HEADERS
# include <io/nd/mdi/tok/tok.h>
#else
# include "tok.h"
#endif

struct tokdevice *tokdevice;
struct tokhwconf *tokhwconf;

extern int tokSMtx (int unit, mblk_t *mp);

int tokopen(), tokclose(), tokuwput();
extern nulldev();

/* MIN PDU, MAX PDU, HI WATER MK, LO WATER MK */
static struct module_info tok_minfo = {
	0,      "tok",	 0,    MAX_PDU,    40000,	  0
};

static struct qinit tokurinit = {
	NULL,     NULL,   tokopen, tokclose, nulldev, &tok_minfo, NULL
};

static struct qinit tokuwinit = {
	tokuwput, NULL, tokopen, tokclose, nulldev, &tok_minfo, NULL
};

struct streamtab tokinfo = { &tokurinit, &tokuwinit, NULL, NULL };

void **tokcookie;
int tokdevflag = 0;
extern cm_num_t tokarch;
unsigned int tok_nunit;

int tok_fdx [4];
int tok_smode [4];
int tokframsz [4];

#define DRVNAME "tok - IBM Token-Ring Network Adapter Series MDI driver"
STATIC int tok_load(), tok_unload(), tok_verify();
MOD_ACDRV_WRAPPER(tok, tok_load, tok_unload, NULL, tok_verify, DRVNAME);

LKINFO_DECL(tok_lk, "MDI::tok mutex SV_WAIT lock", 0);

/* ndcfg/netcfg version string -- percent I percent gets expanded in SCO tree */
static char _ndversion[]="28.1";

STATIC int
tok_load()
{
	int ret = 0;

	if ((ret = tokinit()) != 0) {
		tokuninit();
	}
	return(ret);
}

STATIC int
tok_unload()
{
	tokuninit();
	return(0);
}

tokuninit()
{
	register int i;

	for (i = 0; i < tok_nunit && tokcookie; i++) {
		if ((caddr_t)tokcookie[i] != (caddr_t)NULL)
			cm_intr_detach(tokcookie[i]);
			if (tokdevice[i].timeout_id) {
				untimeout(tokdevice[i].timeout_id);	/* cancel timeout */
			}
			if (tokdevice[i].mmio)
				physmap_free((addr_t)tokdevice[i].mmio, 8192, 0);
			if (tokdevice[i].sram)
				physmap_free((addr_t)tokdevice[i].sram, tokhwconf[i].ramsz, 0);
	}
	if (tokcookie)
		kmem_free((void *)tokcookie, (sizeof(void **) * tok_nunit));
 	if (tokdevice)
		kmem_free((void *)tokdevice, (sizeof(struct tokdevice) * tok_nunit));
 	if (tokhwconf)
		kmem_free((void *)tokhwconf, (sizeof(struct tokhwconf) * tok_nunit));
}

STATIC int
tok_verify(key)
rm_key_t key;
{
	struct cm_args	cm_args;
	cm_range_t	range;
	cm_num_t	ConfIRQ;
	static unchar isa_irq[2][4] = { { 9, 3, 6, 7 },
					{ 9, 3, 10, 11 } };
	struct tokdevice device;
	struct tokhwconf hwconf;   
	unchar	x, irq;
	ulong	mmio;
	int     j;
	unchar *y, i;
	unsigned char z;
	struct srb_init_status *srb;

	cm_args.cm_key = key;
	cm_args.cm_n = 0;
	cm_args.cm_param = CM_IOADDR;
	cm_args.cm_val = &range;
	cm_args.cm_vallen = sizeof(struct cm_addr_rng);

	if (cm_getval(&cm_args) != 0) {
#ifdef DEBUG
		cmn_err(CE_WARN, "tok_verify: no IOADDR for key 0x%x", key);
#endif
		return(ENODEV);
	}
	hwconf.iobase = range.startaddr;



	/* Get board bus type */
	cm_args.cm_key = key; 
	cm_args.cm_n = 0;
	cm_args.cm_param = CM_BRDBUSTYPE;
	cm_args.cm_val = &tokarch;
	cm_args.cm_vallen = sizeof(cm_num_t);
	if (cm_getval(&cm_args) == 0) {
		switch (tokarch) {
		case CM_BUS_ISA:
			cm_args.cm_n = 0;
			cm_args.cm_param = CM_MEMADDR;
			cm_args.cm_val = &range;
			cm_args.cm_vallen = sizeof(struct cm_addr_rng);
			if (cm_getval(&cm_args) == 0) {
				hwconf.rambase = range.startaddr;
			} else {
#ifdef DEBUG
				cmn_err(CE_WARN, "tok_verify: no MEMADDR for key 0x%x", key);
#endif
			       	return(ENODEV);
			}
			break;
		default:
#ifdef DEBUG
			cmn_err(CE_WARN, "tok_verify: bus is not ISA");
#endif
			return(0);
		}
	}

	x = inb(hwconf.iobase);
	mmio = (x & 0xfc) << 11;
	mmio |= 1 << 19;


	device.mmio = (unchar *)physmap((paddr_t)mmio, 8192, KM_NOSLEEP); 
	if (!device.mmio)  
	{		 
#ifdef DEBUG
	   cmn_err(CE_WARN, "tokverify: no memory for physmap (mmio=0x%x)", mmio);
#endif
	   return(ENOMEM);
	}

	/* old ISA adp     ISA adp, Func ID=0xf     ISA adp, Func ID=0xe
	 * ===========     ====================     ====================
	 * IRQ 2(9)        IRQ 2(9)                 IRQ 2(9)
	 * IRQ 3           IRQ 3                    IRQ 3
	 * IRQ 6           IRQ 6                    IRQ 10
	 * IRQ 7           IRQ 7                    IRQ 11
	 */
		
	y = device.mmio + 0x1fba;
	i = ( ((*y) & 0x0f) == 0x0e ) ? 1 : 0;
	irq = isa_irq[i][x & 0x03];

	hwconf.irq = irq;

	/* get the configured IRQ */
	cm_args.cm_key = key;
	cm_args.cm_n = 0;
	cm_args.cm_param = CM_IRQ;
	cm_args.cm_val = &ConfIRQ;
	cm_args.cm_vallen = sizeof(cm_num_t);

	if (cm_getval(&cm_args) != 0) {
#ifdef DEBUG
		cmn_err(CE_WARN, "tok_verify: no IRQ for key 0x%x", key);
#endif
		return(ENODEV);
	}
	/* verify if the irqs are equal */
#ifdef DEBUG
	cmn_err(CE_CONT, "tok_verify: irq = %d , config. irq = %d ", irq, ConfIRQ);
#endif
	if (ConfIRQ != irq)
	{
#ifdef DEBUG
		cmn_err(CE_WARN, "tok_verify: irqs don't match ");
#endif
		return(ENODEV);
	}

	device.aca = (struct aca *)(device.mmio + ACA);
#ifdef DEBUG
	cmn_err(CE_CONT, "tok_verify: mmio = 0x%x  ", device.mmio);
#endif


	/*
	* To completely reset the adapter software must ensure at least
	* 50ms between the RESET and RELEASE (see S.G.12.001.03 page 17)
	*/

#ifdef DEBUG
	cmn_err(CE_CONT, "tok_verify: iobase = 0x%x  ", hwconf.iobase);
#endif

	outb(hwconf.iobase + 1, 0);		/* reset the adapter */
	drv_usecwait(50000);     
	outb(hwconf.iobase + 2 , 0);	/* now release the adapter */

	/* enable interrupts */
	device.aca->isrp_even_set = 0xc0;

#ifdef DEBUG
	cmn_err(CE_CONT, "tok_verify: vai iniciar o loop ");
#endif
	/*
	* wait for init to complete (bit 5 in ISRP odd set)
	* wait for up to a second
	*/
	for (j=1000; j; --j) {
		if (device.aca->isrp_odd & SRB_RESP)
			break;
		drv_usecwait(10000);  
	}

	if (!j)
	{
#ifdef DEBUG
		cmn_err(CE_WARN, " tok_verify: Timed out waiting");
#endif
		return(ENODEV);
	}


	y = device.mmio + 0x1fa0;

#ifdef INIT_ACA
	z = hwconf.rambase >> 12;
#ifdef DEBUG
	cmn_err(CE_CONT, "tok_verify: rrr_even antes = 0x%x", device.aca->rrr_even);
#endif
	device.aca->rrr_even = z;
#ifdef DEBUG
	cmn_err(CE_CONT, "tok_verify: rrr_even depois = 0x%x", device.aca->rrr_even);
#endif
#endif
	if ((uint)((*y) & 0x0f) > 0x0c)	/* old adapter */
	{
		z = hwconf.rambase >> 12;
		device.aca->rrr_even = z;
		x = device.aca->rrr_even;
#ifdef DEBUG
		cmn_err(CE_CONT, "tok_verify: old adapter: x = 0x%x   z = 0x%x", x, z);
#endif
		if ( z != x)
		{
#ifdef DEBUG
			cmn_err(CE_WARN,"tok_verify: Failed to set RAM base address to 0x%x", hwconf.rambase);
#endif
			return(ENODEV);
		}
	}

	/* Read the RAM size */
	j = 1;
	x = device.aca->rrr_odd & 0x0c;
	switch (x)
	{
	case 0x00:
		hwconf.ramsz = 8*1024;
		j = 0;
		break;

	case 0x04:
		hwconf.ramsz = 16*1024;
		j = device.aca->rrr_even & 0x03;		/* MDI003 */
		break;

	case 0x08:
		hwconf.ramsz = 32*1024;
		j = device.aca->rrr_even & 0x07;		/* MDI003 */
		break;

	case 0x0c:
		hwconf.ramsz = 64*1024;
		j = device.aca->rrr_even & 0x0f;		/* MDI003 */

		/*
		* There is a hardware failure on some adapters if
		* 64k RAM is selected.
		*/
#ifdef DEBUG
		cmn_err(CE_WARN,				/* MDI003 */
		"tok_verify: Greater than 32k RAM is not supported for this adapter");
#endif
		return(ENODEV);
		break;
	}

	if (j)							/* MDI003 */
	{
#ifdef DEBUG
		cmn_err(CE_WARN,"tok_verify: Invalid RAM boundry for %dk: 0x%x",
				hwconf.ramsz/1024, hwconf.rambase);
#endif
		return(ENODEV);
	}

	device.sram =
		(unchar *)physmap(hwconf.rambase, hwconf.ramsz, KM_NOSLEEP);
#ifdef DEBUG
	cmn_err(CE_CONT, "tok_verify: rambase = 0x%x , device.sram = 0x%x ", hwconf.rambase, device.sram);
#endif

	/* Now look at the SRB just returned */
	device.srb = device.sram + mdi_ntohs(device.aca->wrb);
	srb = (struct srb_init_status *)device.srb;
#ifdef DEBUG
	cmn_err(CE_CONT, "tok_verify: srb->command = 0x%x , srb->bringup_code = 0x%x ", srb->command, srb->bringup_code);
#endif
	if (srb->command != 0x80)
	{
#ifdef DEBUG
		cmn_err(CE_WARN,
		   "tok_verify:Reset response(%b) is not SRB_INIT_STATUS(0x80)",
			srb->command);
#endif
		return(ENODEV);
	}

	if (srb->bringup_code)
	{
#ifdef DEBUG
		cmn_err(CE_WARN, "tok_verify: Initialization failed, code=%x",
		    srb->bringup_code);
#endif
		return(ENODEV);
	}

	physmap_free((addr_t)device.mmio, 8192, 0);
	physmap_free((addr_t)device.sram, hwconf.ramsz, 0);

        /* end of the verification of the parameters. */
        /* at this point, we know that the board's configuration */
        /* is correctly done in the resource manager.            */
       
        /* now we must call mdi_AT_verify */ 
        {
          ulong_t sioa, eioa, scma, ecma;
          int     getset, vector, err;
 
          getset = MDI_ISAVERIFY_UNKNOWN;
          err = mdi_AT_verify (key, &getset, &sioa, &eioa, &vector,
                               &scma, &ecma, NULL); 
          if (err) {
            cmn_err (CE_WARN, "tok_verify : mdi_AT_verify key=%d failed\n",key);
            return (ENODEV);
          }
 
          if (getset == MDI_ISAVERIFY_TRADITIONAL) {
            return (0); /* DCU calling : don't do nothing */
          } else if (getset == MDI_ISAVERIFY_GET) {
            /* set custom parameters in resmgr and call */
            /* mdi_AT_verify to update information      */   
           
            sioa = hwconf.iobase;
            eioa = sioa + 3;
            vector = irq;
            scma = hwconf.rambase;
            ecma = scma + hwconf.ramsz - 1;
            getset = MDI_ISAVERIFY_GET_REPLY; 
            err = mdi_AT_verify(key, &getset, &sioa, &eioa, &vector,
                                &scma, &ecma, NULL);
   
            if (err != 0) {
               cmn_err(CE_CONT,"tok_verify: mdi_AT_verify returned %d",err);
               return(ENODEV);
            }
          } else if (getset == MDI_ISAVERIFY_SET) {
             cmn_err(CE_CONT,"tok_verify: MDI_ISAVERIFY_SET not supported");
             return(ENODEV);
          } else {
             cmn_err(CE_WARN, "tok_verify: unknown mode %d\n",getset);
             return(ENODEV);
          }
            
        }    

	return(0);
}


/*
 * The following six routines are the normal streams driver
 * access routines
 */
tokopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *crp)
{
	int i;
	struct tokdevice	*device;

	minor_t unit = getminor(*dev);
	if (unit == -1) {
		cmn_err(CE_NOTE,"tok: invalid minor -1");
		return(ENXIO);
	}
	for (i = 0; i < tok_nunit; i++) {
		if (unit == tokdevice[i].unit) {
			break;
		}
	}
	if (i == tok_nunit) {
		/* this minor device doesn't match any configured adapters */
		return(ENXIO);
	}
	device = &(tokdevice[i]);

	if (sflag==CLONEOPEN) {
		return(EINVAL);
	}
	if (device->mdi_state != MDI_CLOSED) {
		return(EBUSY);
	}
	bzero(&device->stats, sizeof(mac_stats_tr_t));

	q->q_ptr = WR(q)->q_ptr = (char *)i;
	device->lock = LOCK_ALLOC(0, getpl(), &tok_lk, KM_SLEEP);
	device->sv = SV_ALLOC(KM_SLEEP);
	if (!tokhwinit(i)) {
		LOCK_DEALLOC(device->lock);
		SV_DEALLOC(device->sv);
		return(ENXIO);
	}
	device->mdi_state = MDI_OPEN;

	return(0);
}

tokclose(queue_t *q)
{
	int bd = (int)q->q_ptr;
	struct tokdevice *device = &tokdevice[bd];
	int x;

	x = splstr();
	flushq(WR(q),1);
	q->q_ptr = (char *)0;
	splx(x);

	if (device->mdi_state == MDI_OPEN || device->mdi_state == MDI_BOUND)
		tokhwhalt(bd);
	device->up_queue = 0;
	device->mdi_state = MDI_CLOSED;
	if (device->lock)
		LOCK_DEALLOC(device->lock);
	if (device->sv)
		SV_DEALLOC(device->sv);
	return(0);
}

tokuwput(queue_t *q, mblk_t *mp)
{
	switch (mp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		tokmacproto(q, mp);
		return;

	case M_DATA:
		tokdata(q,mp);
		return;

	case M_IOCTL:
		tokioctl(q, mp);
		return;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHALL);
			*mp->b_rptr &= ~FLUSHW;
		}
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHALL);
			qreply(q, mp);
		}
		else
			freemsg(mp);
		return;
	default:
		cmn_err(CE_WARN, "tokuwput: unknown STR msg type %x received mp = %x" , mp->b_datap->db_type, mp);
		freemsg(mp);
		return;
	}
}

STATIC
tokdata(q,mp)
queue_t *q;
mblk_t *mp;
{
	int              bd;
	struct tokdevice *device;
	int              s;

	mac_hdr_tr       *hdr;	     //thc-sm thc-fdx


	bd = (int)q->q_ptr;
	device = &tokdevice[bd];

	if (device->mdi_state != MDI_BOUND) {
		mdi_macerrorack(RD(q), M_DATA, MAC_OUTSTATE);
		freemsg(mp);
		return;
	}

	if (mp == 0)
	{
		return (TRUE);
	}

	/* //thc-sm thc-fdx
	 *
	 * The hardware can't do loopback in FDX mode:
	 *       Do it here
	 */
// the hardware DO loopvack in shallow mode        
//	if (device->sm_enable || device->fdx_opened)
	if (device->fdx_opened)
	{
		hdr = (struct mac_hdr_tr *) mp->b_rptr;
		if ((ISMCATR(hdr->mh_dst) &&
			tokcheckaddr(device, (unchar *)(hdr->mh_dst))) ||
			ISBROADCAST(hdr->mh_dst))
		{
			mdi_do_loopback (q, mp, 14);
		} else {
			if (mdi_addrs_equal (device->hw_address, hdr->mh_dst))
			{
			        mdi_do_loopback (q, mp, 14);
//moah-LBACK				putnext (RD(q), mp);
				return;
			}
		}
	}

	s = splstr();

	if (device->sm_enable) {	 /* shallow mode operation */
		if (q->q_first || !tokSMtx (bd, mp))
			putq(q, mp);
	} else {			 /* normal operation */
		if (!device->tokstartio_going) {
			device->tokstartio_going = 1;
			tokstartio(device);
			putq(q,mp);
		} else {
			if (q->q_first || !device->txbufp) {
				putq(q, mp);
			} else {
				tokhwput(bd,mp);
			}
		}
	}

	splx(s);
}

/*
 * Messages are placed on the write queue in tokdata.  This function is called
 * from a transmit complete interrupt to get a message from the queue and
 * send it to the adapter using tokhwput.  Assumes splstr()
 */
toksend_queued(q)
register queue_t *q;
{
	mblk_t *mp;
	int unit = (int)q->q_ptr;
	struct tokdevice *device = &tokdevice[unit];

	if (mp=getq(q)) {
		if (device->txbufp)
			tokhwput(unit, mp);
		else
			putbq(q,mp);
	}
}

/*
 * boot init
 */
tokinit()
{
	struct tokdevice *device;
	struct tokhwconf *hwconf;
	int i, t;
	char *tp;
	unsigned char *x;
	struct cm_args	cm_args;
	cm_range_t	range;
	int ret;
	char str [64];
	uint_t unit;		/* filled in by mdi_get_unit */

#ifdef DEBUG
//Using "#interface mdi.2" this function could not be found. Is there another?
//	(*cdebugger) (DR_OTHER, NO_FRAME);
#endif /* DEBUG */


	tok_nunit = cm_getnbrd("tok");

	if (tok_nunit == 0) {
		cmn_err(CE_WARN, "tokinit: can't find tok in resmgr");
		return(ENODEV);
	}

	if ((tokcookie = (void **)kmem_zalloc
		((sizeof(void **) * tok_nunit), KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN, "tokinit: no memory for tokcookie");
		return(ENOMEM);
	}

	if ((tokdevice = (struct tokdevice *)kmem_zalloc
		((sizeof(struct tokdevice) * tok_nunit), KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN, "tokinit: no memory for tokdevice");
		kmem_free((void *)tokcookie, (sizeof(void **) * tok_nunit));
		return(ENOMEM);
	}

	if ((tokhwconf = (struct tokhwconf *)kmem_zalloc
		((sizeof(struct tokhwconf) * tok_nunit), KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN, "tokinit: no memory for tokhwconf");
		kmem_free((void *)tokcookie, (sizeof(void **) * tok_nunit));
		kmem_free((void *)tokdevice, (sizeof(struct tokdevice) * tok_nunit));
		return(ENOMEM);
	}

	for (i = 0; i < tok_nunit; i++) {
		cm_args.cm_key = cm_getbrdkey("tok", i);
		if (mdi_get_unit(cm_args.cm_key, &unit) == B_FALSE) {
			tokdevice[i].unit = (uint_t) -1;
			tokdevice[i].mdi_state = MDI_NOT_PRESENT;
			continue;
		}
		tokdevice[i].unit = unit;

		device = &tokdevice[i];
		hwconf = &tokhwconf[i];
		device->mdi_state = MDI_NOT_PRESENT;

		/* initialize the all-multicast-address variable to disable */
		device->all_mca = 0;

		/* assume it's an ISA adapter */
		cm_args.cm_n = 0;
		cm_args.cm_param = CM_IOADDR;
		cm_args.cm_val = &range;
		cm_args.cm_vallen = sizeof(struct cm_addr_rng);
		if (ret = cm_getval(&cm_args)) {
			if (ret != ENOENT)
				cmn_err(CE_CONT, "tokinit: cm_getval(IOADDR) failed");
			continue;
		} else {
			hwconf->iobase = range.startaddr;
		}

		/* get the frame size configured */
		cm_args.cm_key = cm_getbrdkey("tok", i);
		cm_args.cm_n = 0;
		cm_args.cm_param = "FRAMESZ";
		cm_args.cm_val = str;
		cm_args.cm_vallen = sizeof(str);
		if (ret = cm_getval(&cm_args)) {
			if (ret != ENOENT) {
				cmn_err(CE_CONT, "tokinit: cm_getval(FRAMESZ) failed");
				continue;
			} else {
				cmn_err(CE_CONT, "tokinit: Cannot find parameter FRAMESZ. Assuming default of 2000\n");
				tokframsz [i] = 2000; 
			}
		} else {
			switch (str [0]) {
			case '9':
				tokframsz [i] = 988; 
				break;  
			case '1':
				tokframsz [i] = 1500; 
				break;  
			case '2':
				tokframsz [i] =2000; 
				break;  
			default:
				cmn_err(CE_CONT, "tokinit: board %d has unknown FRAMESZ %s in resmgr\n", i, str);
				continue;  
			}
		}

		/* is full-duplex enabled? */
		cm_args.cm_key = cm_getbrdkey("tok", i);
		cm_args.cm_n = 0;
		cm_args.cm_param = "FDX";
		cm_args.cm_val = str;
		cm_args.cm_vallen = sizeof(str);
		if (ret = cm_getval(&cm_args)) {
			if (ret != ENOENT) {
				cmn_err(CE_CONT, "tokinit: cm_getval(FDX) failed");
				continue;
			} else {
//				cmn_err(CE_CONT, "tokinit: Cannot find parameter FDX. Assuming default of disabled\n");
				tok_fdx [i] = 0; 
			}
		} else {
			switch (str [0]) {
			case '0':
				tok_fdx [i] = 0; 
				break;  
			case '1':
				tok_fdx [i] = 1; 
				break;  
			default:
				cmn_err(CE_CONT, "tokinit: board %d has unknown FDX %s in resmgr\n", i, str);
				continue;  
			}
		}

		/* is shallow mode enabled? */
		cm_args.cm_key = cm_getbrdkey("tok", i);
		cm_args.cm_n = 0;
		cm_args.cm_param = "SHMODE";
		cm_args.cm_val = str;
		cm_args.cm_vallen = sizeof(str);
		if (ret = cm_getval(&cm_args)) {
			if (ret != ENOENT) {
				cmn_err(CE_CONT, "tokinit: cm_getval(SHMODE) failed");
				continue;
			} else {
//				cmn_err(CE_CONT, "!tokinit: Cannot find parameter SHMODE. Assuming default of disabled\n");
				tok_smode [i] = 0; 
			}
		} else {
			switch (str [0]) {
			case '0':
				tok_smode [i] = 0; 
				break;  
			case '1':
				tok_smode [i] = 1; 
				break;  
			default:
				mdi_printcfg("tok", hwconf->iobase, 3, -1, -1,
					"board %d has unknown SHMODE %s in resmgr", i, str);
				continue;  
			}
		}

		if ( (t = tokpresent(hwconf->iobase)) ) {
			x = device->hw_address;

			mdi_printcfg("tok", hwconf->iobase, 3, hwconf->irq, -1,
				"RAMsz=%d addr=%b:%b:%b:%b:%b:%b", hwconf->ramsz,
				*(x+0),*(x+1),*(x+2),*(x+3),*(x+4),*(x+5) );

			if (tokhwconf[i].ramsz < (16*1024))	/* MDI003 */
				switch (device->mmio[0x1fa2]) {
				/*	case 0x0f:	/* 4 mbps bus */
				/*		break;	*/
				case 0x0e:	/* 16 mbps bus */
				case 0x0d:	/* 4 and 16 mbps bus */
					cmn_err(CE_WARN,
						"At least 16K Token Ring adapter RAM suggested");
				}

			device->mdi_state = MDI_CLOSED;
		} else {
			mdi_printcfg("tok", hwconf->iobase, 3, hwconf->irq, -1,
				"NOT PRESENT");
		}
	}
	return(0);
}

/*
 * The following three routines implement MDI interface
 */
STATIC
tokmacproto(q,mp)
queue_t *q;
mblk_t *mp;
{
	union MAC_primitives *prim;
	mac_bind_req_t       *req;
	mblk_t *mp1;
	mac_info_ack_t *info_ack;
	mac_ok_ack_t *ok_ack;

	int bd = (int)q->q_ptr;
	struct tokdevice *device = &tokdevice[bd];

	prim = (union MAC_primitives *) mp->b_rptr;
	req = (struct mac_bind_req *) mp->b_rptr;
	switch(prim->mac_primitive) {
	case MAC_INFO_REQ:
		if (!(mp1 = allocb(sizeof(mac_info_ack_t), BPRI_HI))) {
			cmn_err(CE_WARN, "tokmacproto - Out of STREAMs");
			freemsg(mp);
			return;
		}
		info_ack = (mac_info_ack_t *) mp1->b_rptr;
		mp1->b_wptr = mp1->b_rptr + sizeof(mac_info_ack_t);
		mp1->b_datap->db_type = M_PCPROTO;
		info_ack->mac_primitive = MAC_INFO_ACK;
		info_ack->mac_max_sdu = device->max_pdu;
		info_ack->mac_min_sdu = 14;
		info_ack->mac_mac_type = MAC_TPR;
		info_ack->mac_driver_version = MDI_VERSION;
		info_ack->mac_if_speed = device->ring_speed;

		freemsg(mp);
		qreply(q, mp1);
		break;
	case MAC_BIND_REQ:
		if (device->mdi_state != MDI_OPEN) {
			mdi_macerrorack(RD(q),prim->mac_primitive,MAC_OUTSTATE);
			freemsg(mp);
			return;
		}
		device->mdi_state = MDI_BOUND;
		device->up_queue = RD(q);
		device->mac_bind_cookie = req->mac_cookie; /* smy */
		mdi_macokack(RD(q),prim->mac_primitive);
		freemsg(mp);
		break;

	default:
		mdi_macerrorack(RD(q), prim->mac_primitive, MAC_BADPRIM);
		freemsg(mp);
		break;
	}
}

/*
 * Handle an ioctl message from the stream.
 */
STATIC
tokioctl(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocp;
	int bd = (int)q->q_ptr;
	struct tokdevice *device = &tokdevice[bd];
	int i, j;
	unsigned char *datap;
	ulong l;
	extern unchar tokhwaddr[][6];

	iocp = (struct iocblk *)mp->b_rptr;
	if (mp->b_cont)
		datap = mp->b_cont->b_rptr;
	else
		datap = (unsigned char *)0;

	switch (iocp->ioc_cmd) {
	case MACIOC_GETRADDR:	/* get factory mac address */
		if (!datap || iocp->ioc_count != 6) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		}
		bcopy(device->hw_raddress, datap, 6);
		mp->b_cont->b_wptr = mp->b_cont->b_rptr + 6;
		goto ioctl_ack;
	case MACIOC_GETADDR:	/* Return hardware address of the card */
		if (!datap || iocp->ioc_count != 6) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		}
		bcopy(device->hw_address, datap, 6);
		mp->b_cont->b_wptr = mp->b_cont->b_rptr + 6;
		goto ioctl_ack;
	case MACIOC_SETADDR:	/* Set Multicast address */
		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			goto ioctl_nak;
		}
		if (!datap || iocp->ioc_count != 6) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		}
		/* Don't change the current address until next open */
		/*bcopy(datap, device->hw_address, 6);*/
		bcopy(datap, tokhwaddr[bd], 6);
		goto ioctl_ack;
	case MACIOC_SETALLMCA:	/* Enable All Multicast address */
                device->all_mca = 1;
		goto ioctl_ack;
	case MACIOC_DELALLMCA:	/* Disable All Multicast address */
                device->all_mca = 0;
		goto ioctl_ack;
	case MACIOC_SETMCA:	/* Set Multicast address */
		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			goto ioctl_nak;
		}
		if (!datap || iocp->ioc_count != 6) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		}
		if (*datap != 0xc0 || *(datap+1) != 00) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		}
		goto ioctl_ack;
	case MACIOC_DELMCA: /* Del Multicast address */
		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			goto ioctl_nak;
		}
		if (!datap || iocp->ioc_count != 6) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		}
		if (*datap != 0xc0 || *(datap+1) != 00) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		}
		goto ioctl_ack;

	case MACIOC_GETSTAT:
		if (!datap || iocp->ioc_count!=sizeof(mac_stats_tr_t)) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		}
		tokupdate_stats(bd, mp);
		return;

	case MACIOC_CLRSTAT:		/* clear statistics */
		if (iocp->ioc_uid != 0) {
			iocp->ioc_error=EPERM;
			goto ioctl_nak;
		}
		bzero(&device->stats, sizeof(mac_stats_tr_t));
		goto ioctl_ack;

	case MACIOC_SETSTAT:	/* set MAC statistics */
		if (!datap || (iocp->ioc_count != sizeof(mac_setstat_t))) {
			iocp->ioc_error = EINVAL;
			goto ioctl_nak;
		} else {
			i = 0;
			/*
			 * loop starts at highest bit to verify valid flag(s)
			 * are set before updating statistic
			 */
			for (j = ((10*NFLBITS) - 1); j >= 0; --j) {
				if (MACFL_ISSET(j, (mac_setstat_t *) datap)) {
					switch (j) {
					case MACSTAT_ACTMONPARTICIPATE:
						if (((mac_setstat_t *)datap)->mac_stats.tr.mac_actmonparticipate) {
							i |= MON_CONTENDER;
							device->stats.mac_actmonparticipate = 1;
						} else {
							i &= ~MON_CONTENDER;
							device->stats.mac_actmonparticipate = 0;
						}
						break;
					default:
						/* invalid setstat flag set */
						iocp->ioc_error = EINVAL;
						goto ioctl_nak;
					}
				}
			}
			tokmodopen(bd, i);
			iocp->ioc_count = 0;
			iocp->ioc_rval = 0;
			if (mp->b_cont) {
				freemsg(mp->b_cont);
				mp->b_cont = (mblk_t *)0;
			}
		}
		goto ioctl_ack;

	default:
ioctl_nak:
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_count = 0;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = (mblk_t *)0;
		}
		qreply(q, mp);
		return;
	}
ioctl_ack:
	mp->b_datap->db_type = M_IOCACK;
	qreply(q, mp);
}
