#ident "@(#)e3Dli.c	28.2"
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
 */
	
#ifdef _KERNEL_HEADERS

#include <io/nd/mdi/e3D/e3D.h>

#else

#include "e3D.h"

#endif

/* broadcast address for routing to this interface */
macaddr_t e3D_broad = {0xff,0xff,0xff,0xff,0xff,0xff};

extern int e3Duwput(queue_t *, mblk_t *);
extern int e3Dopen(void *, channel_t *, int, cred_t *, queue_t *);
extern int e3Dclose(void *, channel_t , int , cred_t *, queue_t *);
extern int nulldev();
extern int e3Dconfig(cfg_func_t, void *, rm_key_t);
extern int e3Ddevinfo(void *, channel_t, di_parm_t, void *);
extern void e3Dbiostart(void *, channel_t , buf_t *);
extern int e3Dwrongioctl(void *, channel_t, int, void *, int , cred_t *, int *);
extern int e3Ddrvctl(void *, channel_t, int, void *);
extern ppid_t e3Dmmap(void *, channel_t, size_t, int);

/* XXX fix for ddi8 when stream.h structure declarations change */
#if 0
struct module_info e3D_minfo = {
	1, E3D_ETH_MAXPACK, 16*E3D_ETH_MAXPACK, 12*E3D_ETH_MAXPACK
};
struct qinit e3Durinit = {
	0, NULL, &e3D_minfo
};
struct qinit e3Duwinit = {
	e3Duwput, NULL, &e3D_minfo
};
#endif

/* old ddi7 style for now until struct qinit changes in stream.h */
struct module_info e3D_minfo = {
	0, "e3D", 1, E3D_ETH_MAXPACK, 16*E3D_ETH_MAXPACK, 12*E3D_ETH_MAXPACK
};
struct qinit e3Durinit = {
	0, NULL, e3Dopen, e3Dclose, nulldev, &e3D_minfo, 0
};
struct qinit e3Duwinit = {
	e3Duwput, NULL, e3Dopen, e3Dclose, nulldev, &e3D_minfo, 0
};

const struct streamtab e3Dinfo = {
	&e3Durinit, &e3Duwinit, 0, 0
};

/* ndcfg/netcfg version info string */
static char _ndversion[]="28.2";

static const char e3D_drv_name[]="e3D";

/* d_open and d_close can't be NULL */
STATIC drvops_t e3D_drvops = { 
	/* d_config	d_open		d_close		d_devinfo */
	e3Dconfig,	e3Dopen,	e3Dclose,	e3Ddevinfo, 

	/* d_biostart	d_ioctl		d_drvctl	d_mmap */ 
	e3Dbiostart,	e3Dwrongioctl,	e3Ddrvctl,	e3Dmmap 
};

/* - while this isn't a PCI device driver, we'll pretend that it supports 
 *   suspend and resume hotplug functionality by using D_HOT for demonstration
 *   purposes.
 * - We don't need to store any device-specific information in the minor 
 *   number as information provided with ADD_INSTANCE is sufficient for 
 *   all cases for e3D.  We also do not implement cloning. 
 *   Consequently, we set drv_maxchan to 0.
 */
STATIC drvinfo_t e3D_drvinfo = {
/* drv_ops	drv_name	drv_flags	drv_str		drv_maxchan */
&e3D_drvops,	e3D_drv_name,	D_STR|D_HOT,	&e3Dinfo,	0
};

STATIC void
e3Dbiostart(void *idata, channel_t channel, buf_t *bp)
{
	cmn_err(CE_NOTE,"e3D: got to e3Dbiostart");
	return;
}

STATIC int
e3Dwrongioctl(void *idata, channel_t channel, int cmd, 
		void *arg, int oflags, cred_t *crp, int *rvalp)
{
	cmn_err(CE_PANIC,"e3D: got to e3Dwrongioctl");
}

STATIC int
e3Ddrvctl(void *idata, channel_t channel, int cmd, void *arg)
{
	cmn_err(CE_PANIC,"e3D: got to e3Ddrvctl");
}

STATIC ppid_t
e3Dmmap(void *idata, channel_t channel, size_t offset, int prot)
{
	cmn_err(CE_PANIC,"e3D: got to e3Dmmap");
}

int
e3Ddevinfo(void *idata, channel_t channel, di_parm_t parm, void *valp)
{
	switch(parm) {
		case DI_MEDIA:
			*(char **)valp = "NIC"; /* short names here */
			return(0);
		case DI_SIZE:
		case DI_RBCBP:
		case DI_WBCBP:
		case DI_PHYS_HINT:
			return(EOPNOTSUPP);
		default:
			return(ENOSYS);
	}
	/* NOTREACHED */
}

/*
 * Function: _load(void)
 *
 * Purpose:
 * Called by DLM when the module is loaded
 * and calls our CFG_ADD code in a separate kernel thread
 */
int
_load(void)
{
	e3Drunstate();

	return(drv_attach(&e3D_drvinfo));
}

/*
 * Function: _unload
 *
 * Purpose:
 * Called by DLM when the module is unloading.  You should remember to:
 * - disable and remove device interrupts by calling the cm_intr_detach(D3) 
 * - deallocate memory acquired for private data 
 * - cancel any outstanding itimeout(D3), btimeout(D3), or bufcall(D3str) 
 *   requests made by the module 
 * calls our e3Dconfig CFG_REMOVE code in a separate thread
 */
int
_unload(void)
{
	drv_detach(&e3D_drvinfo);
	return(0);
}

/* Function:
 *    e3Dconfig
 * Remark:
 *    Here's where most of the action takes place
 */
int
e3Dconfig(cfg_func_t func, void *idata, rm_key_t rmkey)
{
	int ret;

	switch(func) {
	case CFG_ADD: 
	{
	void **idatap = idata;
	int unit;
	struct cm_args cm_args;
	cm_range_t range;
	cm_num_t cm_num;
	char macaddr[18];  /* 22:33:44:55:66:77<NUL> */
	e3Ddev_t	*dev;
	extern char *e3Dpartnostr(unsigned char *);

	/* determine if this instance has been configured with netcfg.  While
	 * just a good thing for ISA drivers, calling mdi_get_unit is essential for 
	 * ALL smart-bus (PCI, EISA, MCA) drivers - your CFG_ADD code will be
	 * invoked for each instance in the resmgr that has MODNAME set to your
	 * driver(e3D in this example driver).
	 */
	if (mdi_get_unit(rmkey, NULL) == B_FALSE) {
		return(ENXIO);
	}

	if ((dev = (e3Ddev_t *)kmem_zalloc((sizeof(e3Ddev_t)), KM_NOSLEEP))
			== NULL) {
		cmn_err(CE_WARN, "e3Dconfig: no memory for e3Ddev_t");
		return(ENOMEM);
	}

	dev->ex_rambase = 0;
	dev->rmkey = rmkey;

	cm_args.cm_key = rmkey;
	cm_args.cm_n = 0;
	cm_args.cm_param = CM_IOADDR;
	cm_args.cm_val = &range;
	cm_args.cm_vallen = sizeof(cm_range_t);

	cm_begin_trans(cm_args.cm_key, RM_READ);
	if (ret = cm_getval(&cm_args)) {
		cm_end_trans(cm_args.cm_key);
		cmn_err(CE_WARN, "e3Dconfig: cm_getval(IOADDR) at key %d "
			"failed(error=%d)\n",cm_args.cm_key, ret);
		kmem_free(dev, sizeof(e3Ddev_t));
		return(ret);
	} else {
		cm_end_trans(cm_args.cm_key);
		dev->ex_ioaddr = (short) range.startaddr;
	}
	if (e3Dpresent(dev, 0) != TRUE) {
		mdi_printcfg("e3D", dev->ex_ioaddr, 0xf, -1, -1,
			"ADAPTER NOT FOUND");
		kmem_free(dev, sizeof(e3Ddev_t));
		return(ENXIO);
	}
	e3Dputconf(cm_args.cm_key, dev);
	/* save factory address */
	bcopy(dev->ex_eaddr, dev->ex_hwaddr, sizeof(dev->ex_hwaddr));

	/*
	 * if MACADDR parameter (overrides factory MAC address)
	 * is set in resmgr, use it.  If set to __STRING__ then user didn't
	 * want to change it so leave it the way it was
	 */
	cm_args.cm_key = rmkey;
	cm_args.cm_n = 0;
	cm_args.cm_param = "MACADDR";
	cm_args.cm_val = macaddr;
	cm_args.cm_vallen = sizeof(macaddr);

	/* MACADDR is a colon delimited MAC address override.  For example,
	 * "11:22:cc:dd:33:44"
	 */
	cm_begin_trans(cm_args.cm_key, RM_READ);
	if (cm_getval(&cm_args) == 0) {   /* may not be in resmgr */
		int loop;

		if (strncmp(macaddr, "__STRING__", 10) != 0) {
			for (loop=0; loop<16; loop+=3) {
				dev->ex_eaddr[loop / 3] = strtoul(macaddr+loop,
								 NULL, 16);
			}
		}
	}
	cm_end_trans(cm_args.cm_key);

	/* old way was to pull MAC override from Space.c: 
	 * if (e3Deth_addr[unit][0] != '\0') {
	 * bcopy(e3Deth_addr[unit],
	 *    dev->ex_eaddr,
	 *    sizeof(dev->ex_eaddr));
	 * }
	 */

	/* set idata which is automatically used as argument to our other 
	 * (D2) routines(in particular other CFG_* functions in e3Dconfig).
	 * only exception: the idata argument to interrupt routine is set 
	 * in call to cm_intr_attach(3rd argument)
	 */
	*idatap = (void *)dev;

	/* we fill in cookie with a non-NULL value because we want to call
	 * cm_intr_detach later on.  If you pass in NULL for cookie to 
	 * cm_intr_attach that means that the interrupts will never be
	 * detached.  cm_intr_attach reads IRQ, ITYPE, IPL, and BINDCPU from 
	 * the resmgr for us.   Note that the earlier call to e3Dputconf
	 * may have changed IRQ and ITYPE from its initial values upon
	 * entering e3Dconfig since that will change IRQ in the resmgr to the 
	 * EEPROM/NVRAM's setting.  This may cause confusion...
	 */
	if (cm_intr_attach(rmkey, e3Dintr, (void *)dev, &e3D_drvinfo, 
		&dev->intr_cookie) == 0) {
		cmn_err(CE_WARN, "e3Dconfig: cm_intr_attach failed (key %d)\n", 
			rmkey);
		kmem_free(dev, sizeof(e3Ddev_t));
		return(ENXIO);
	}
	dev->ex_present = TRUE;

	/* announce ourselves to the world */
	mdi_printcfg("e3D",        /* name */
		dev->ex_ioaddr,         /* base */
		0xf,                    /* offset */
		dev->ex_int,            /* vec */
		-1,                     /* dma */
		"type=3C507 addr=%s",   /* fmt */
		e3Daddrstr(dev->ex_hwaddr));

	/* and provide extended information as well */
	cmn_err(CE_CONT,"\t\t\t\t\tRev %s",
		e3Dpartnostr(dev->ex_partno));

	if (dev->ex_partno[5] != 0xff) {
		cmn_err(CE_CONT," Manufacture Date:%02d-%02d-%02d",
			dev->ex_partno[5] >> 4,
			dev->ex_partno[4],
			(dev->ex_partno[5] & 0x0f) + 90);
	}
	cmn_err(CE_CONT,"\n");
	}  /* end of CFG_ADD code */
	break;

	case CFG_SUSPEND:    
	{
		/* normally called by mod_notifyd/mod_notify_daemon 
		 * but can be called by our own driver for testing purposes 
		 */
		e3Ddev_t	*dev=idata;
		int ret;

		if (dev == NULL) {
			/* shouldn't happen */
			cmn_err(CE_NOTE,"e3Dconfig: CFG_SUSPEND with no idata");
			return(ENXIO);
		}

		/* tell whatever's above us (dlpi module, stream head) that 
		 * we're going to start dropping packets.  config(D2) says we 
		 * should queue frames but that's not reasonable for STREAMS --
		 * we must drop.
		 */
		ASSERT(dev->rmkey == rmkey);
		/* if we've issued a MAC_BIND_REQ then send a suspend notice
		 * up stack.  We actually don't save the queue information in 
		 * our open routine so we don't have much of a choice here.  
		 * It also means that if we've opened up the card but not yet 
		 * issued the BIND_REQ then we'll fail any attempt at the 
		 * point we try to issue the BIND_REQ.
		 */
		if (dev->ex_up_queue) {
			if ((ret=mdi_hw_suspended(dev->ex_up_queue, rmkey)) != 0) {
				return(ret);
			}
		} else {
			/* suspending a driver that has been opened but 
			 * no BIND_REQ seen yet.  don't call mdi_hw_suspended.   
			 */
		}

		/* we need to both pacify the kernel and hardware:
		 * - call cm_intr_detach and devmem_mapout.
		 * - disable interrupts on board
		 */
		e3Dhwclose(dev);
		cm_intr_detach(dev->intr_cookie);
		if (dev->ex_rambase) {
			devmem_mapout(dev->ex_rambase, dev->ex_memsize);
		}
	
		/* tell e3D driver that we're suspended; any further attempts 
		 * to talk to the hardware won't happen and we will silently 
		 * drop frames until a CFG_RESUME is sent
		 */
		dev->issuspended = 1;
		dev->ex_present = FALSE;

	}  /* end of CFG_SUSPEND code */
	break;

	case CFG_RESUME:
	{
		e3Ddev_t	*dev=idata;

		/* we don't know what state the new board is in.  force it (and
		 *  everyone else to runstate.  will cause problems with D_MP 
		 * version of driver as someone else could be in verify routine
		 * right now where we need to be in load state
		 */
		e3Drunstate();

		/* call devmem_mapin again by calling e3Dpresent */
		if (e3Dpresent(dev, 0) != TRUE) {
			mdi_printcfg("e3D", dev->ex_ioaddr, 0xf, -1, -1,
				"ADAPTER NOT FOUND");
			/* don't kmem_free dev yet */
			return(ENXIO);
		}
		e3Dputconf(rmkey, dev);
		/* re-enable interrupts on board */
		if (e3Dhwinit(dev) == FALSE) {
			return (ENXIO);
		}

		if (cm_intr_attach(rmkey, e3Dintr, (void *)dev, &e3D_drvinfo, 
			&dev->intr_cookie) == 0) {
			cmn_err(CE_WARN, "e3Dconfig: cm_intr_attach failed "
					"(key %d)\n", rmkey);
			/* don't kmem_free dev yet */
			return(ENXIO);
		}

		/* Always call mdi_hw_resumed to tell the stack to resume 
		 * frames, as we don't know if user a) never issued the 
		 * BIND_REQ before the suspend or b) issued the BIND_REQ 
		 * successfully, got the suspend, then closed the device 
		 * while we were suspended.
		 */
		if ((ret=mdi_hw_resumed(dev->ex_up_queue, rmkey)) != 0) {
			return(ret);
		}

		dev->issuspended = 0;
		dev->ex_present = TRUE;

		/* maybe the user process using the device terminated while 
		 * we were suspended.  if so then emulate close routine and 
		 * disable interrupts.
		 */
		if (dev->ex_open == FALSE) {
			e3Dhwclose(dev);
		}

		/* XXX TODO: restore multicast list */

	}  /* end of CFG_RESUME code */
	break;

	case CFG_REMOVE:
	{
	/* NOTES: 
	 * - we can't manipulate remsgr key rmkey at all here 
	 * - device may or may not be currently suspended
	 */
	e3Ddev_t	*dev=idata;

	if (dev == NULL) {
		/* previously freed */
		break;
	}

	if (! dev->issuspended) {
		/* detach our interrupt */
		cm_intr_detach(dev->intr_cookie);
	}

	if (dev->ex_rambase) {
		devmem_mapout(dev->ex_rambase, dev->ex_memsize);
	}
	kmem_free((void *)dev, sizeof(e3Ddev_t));
	}  /* end of CFG_REMOVE code */
	break;

	case CFG_MODIFY:
	/* Tell the driver to use new hardware parameters for device instance 
	 * idata.  The driver must no longer access the resource manager key 
	 * that was previously attached to the device instance or the 
	 * associated actual device. Instead, it must obtain new parameters 
	 * from the new resource manager key, key, which may or may not be the 
	 * same key that was previously associated with the device instance. 
	 * All subsequent accesses to resource manager information for this 
	 * device instance must be through the new key. CFG_MODIFY may be used 
	 * either to replace one piece of hardware with another or to correct a
	 * hardware parameter value. The driver is responsible for resetting 
	 * the new hardware to a known state and programming it with any 
	 * options or parameters (such as MAC address for a network adapter 
	 * card) that were previously programmed into the original device 
	 * instance. CFG_MODIFY is called only for a device instance that is 
	 * currently suspended with the CFG_SUSPEND subfunction.  The new key 
	 * and its associated device are guaranteed not to be currently 
	 * associated with any other device instance. 
	 *
	 * Note that CFG_MODIFY should not itself touch the hardware, but
	 * prepare itself for doing to in an upcoming CFG_RESUME call.  
	 * CFG_MODIFY is called when hpci says to resume operations on 
	 * suspended instance since anything in the resmgr may have changed, 
	 * this means that we should - re-read ISA parameters (IOADDR, MEMADDR,
	 * IRQ, DMA, etc.) - re-read custom parameters(CABLETYPE, ZEROWAIT, 
	 * DATAMODE, MACADDR) and re-load those into dev.  When we get the 
	 * CFG_RESUME we'll use those new values.  Anyway, that's the idea.
	 *
	 * We cheat a bit here: since e3Dpresent already reads these parameters 
	 * and all of the custom parameters we don't do any of this work here
	 * but let it happen when we do the CFG_RESUME since it isn't 
	 * specifically illegal to read the resmgr in CFG_RESUME while you 
	 * re-poke the hardware.  This means that there's nothing to do for 
	 * us in CFG_MODIFY.
	 */
	break;

	case CFG_VERIFY:
		/* idata argument is NULL for CFG_VERIFY */
		return(e3Dverify(rmkey));
	break;

	default:
		return(EOPNOTSUPP);
	}
	return(0);
}

/*
 * Function: e3Dverify
 *
 * Purpose:
 * verify that the driver can talk to the NIC 
 * called from e3Dconfig
 * DDI8 NOTES: 
 *   - there may or may not be a corresponding attached device instance 
 *     and it may or may not currently be suspended
 *   - unlike ddi7 _verify routine, CFG_VERIFY is called after the driver 
 *     is loaded with the _load(D2) entry point routine, like all other
 *     config( ) subfunctions
 *   - call _load first, that will do a drv_attach... we then
 *     will look up the drv and call 'config' with CFG_VERIFY
 *     if the driver wasn't loaded, we'll then call _unload.
 *   - the _load runs on any cpu... since a) _load can't do anything to 
 *     hardware and b) the info we need to make this initial attempt at
 *     binding (D_MP flag) can't be obtained till we call  _load
 *   - If, after calling _load, we find that driver doesn't have D_MP,
 *     we must bind it to CPU 0.  Otherwise driver can float to any CPU
 *   - after calling _load, kernel calls e3Dconfig with arguments 
 *     (CFG_VERIFY, NULL, resmgr_key).  Note that since we never call
 *     ADD_INSTANCE the idata pointer (second argument) can only be NULL.
 */
STATIC int
e3Dverify(rm_key_t key)
{
	rm_key_t		putconfkey;
	e3Ddev_t		e3Dtmpdevice;
	struct cm_args		cm_args;
	cm_range_t		range;
	cm_num_t		bustype = 0;
	register e3Ddev_t	*dv = &e3Dtmpdevice;
	char			answer[2];	/* single digit plus null */
	int			getset, err, vector;
	ulong_t			scma, ecma, sioa, eioa;
	
	answer[0]='1';
	answer[1]='\0';
	bzero((char *)dv, sizeof(e3Ddev_t));
	e3Dtmpdevice.ex_rambase = 0;
	e3Dtmpdevice.rmkey = key;
	e3Dtmpdevice.issuspended = 0;

	/* check BRDBUSTYPE */
	cm_args.cm_key = key;
	cm_args.cm_n = 0;
	cm_args.cm_param = CM_BRDBUSTYPE;
	cm_args.cm_val = &bustype;
	cm_args.cm_vallen = sizeof(bustype);
	cm_begin_trans(cm_args.cm_key, RM_READ);
	if (cm_getval(&cm_args) != 0) {
		cm_end_trans(cm_args.cm_key);
		cmn_err(CE_WARN, "e3Dverify: no BRDBUSTYPE for key 0x%x\n",key);
		return(ENODEV);
	}
	cm_end_trans(cm_args.cm_key);

	/* bustype is really a bitmask, but we treat it as a 
	 * single value here 
	 */
	if (bustype != CM_BUS_ISA) {
		cmn_err(CE_CONT, "!e3Dverify: not ISA board\n");
		return(ENODEV);
	}
	
	cm_args.cm_key = key;
	cm_args.cm_n = 0;
	cm_args.cm_param = CM_IOADDR;
	cm_args.cm_val = &range;
	cm_args.cm_vallen = sizeof(struct cm_addr_rng);
	
	cm_begin_trans(cm_args.cm_key, RM_READ);
	if (cm_getval(&cm_args) != 0) {
		cm_end_trans(cm_args.cm_key);
		cmn_err(CE_NOTE,"e3Dverify: IOADDR not found in resmgr\n");
		return(ENODEV);
	}
	cm_end_trans(cm_args.cm_key);

	e3Drunstate();
	e3Dtmpdevice.ex_ioaddr = (short) range.startaddr;

	if ( e3Dpresent(&e3Dtmpdevice, 1) == TRUE ) {
		/* we don't need the memory mapping any more */
		if (e3Dtmpdevice.ex_rambase) {
			devmem_mapout(e3Dtmpdevice.ex_rambase, 
				e3Dtmpdevice.ex_memsize);
			e3Dtmpdevice.ex_rambase = (caddr_t) 0;
		}

		/*
		 * We have e3D hardware at this I/O address.
		 * Now load the EEPROM values
		 */

		/* getset must be MDI_ISA_VERIFY_UNKNOWN initially */
		getset = MDI_ISAVERIFY_UNKNOWN;

		/* we don't use DMA so dma argument is NULL */
		err = mdi_AT_verify(key, &getset, &sioa, &eioa,
					&vector, &scma, &ecma, NULL);
		if (err) {
			cmn_err(CE_WARN, "!e3Dverify: mdi_AT_verify key=%d failed\n",key);
			return(ENODEV);
		}

		if (getset == MDI_ISAVERIFY_TRADITIONAL) {
			/*
			 * Probably called from the dcu.
			 * We have already confirmed that the board is
			 * present at the io address given by CM_IOADDR
			 * in the resmgr in e3Dpresent so we know there
			 * is a board at this address.  Return success
			 * as this is all the traditional verify
			 * routine does.
			 */
			return(0);
		} else if (getset == MDI_ISAVERIFY_GET) {
			/*
			 * Retrieve parameters from firmware, set
			 * custom parameters in resmgr, call
			 * mdi_AT_verify again with updated information,
			 * and return 0.
			 * We've already read EEPROM information by
			 * calling e3Dpresent
			 */

			cm_args.cm_key = key;
			cm_args.cm_n = 0;
			cm_args.cm_val = &answer;
			cm_args.cm_vallen = strlen(answer)+1;
 
			/*
			 * ex_bnc is 0 for AUI or HROM_BNC(0x80) for
			 * other.  The bcfg file says '0' is
			 * BNC/TP and '1' is AUI.
			 */

			/* cm_param must be less than 11 chars */
			cm_args.cm_param = "CABLETYPE";
			answer[0] = (e3Dtmpdevice.ex_bnc == 0 ? '1' : '0');

			cm_begin_trans(cm_args.cm_key, RM_RDWR);
			(void) cm_delval(&cm_args);
			if (cm_addval(&cm_args)) {
				cm_abort_trans(cm_args.cm_key);
				cmn_err(CE_WARN,"e3Dverify: cm_addval for CABLETYPE at key %d failed", key);
				return(ENODEV);
			}
			cm_end_trans(cm_args.cm_key);

			/*
			 * ex_zerows is 0 for disabled or HRAM_0WS for
			 * enabled.  The bcfg file says '0' is
			 * disabled, '1' is enabled
			 */
			cm_args.cm_param = "ZEROWAIT";
			answer[0] = (e3Dtmpdevice.ex_zerows == 0 ? '0' : '1');

			cm_begin_trans(cm_args.cm_key, RM_RDWR);
			(void) cm_delval(&cm_args);
			if (cm_addval(&cm_args)) {
				cm_abort_trans(cm_args.cm_key);
				cmn_err(CE_WARN,"e3Dverify: cm_addval for ZEROWAIT at key %d failed", key);
				return(ENODEV);
			}
			cm_end_trans(cm_args.cm_key);

			/*
			 * ex_sad is 0 for Turbo and HRAM_SAD for
			 * Standard.  The bcfg file says '0' is
			 * Turbo and '1' is Standard
			 */
			cm_args.cm_param = "DATAMODE";
			answer[0] = (e3Dtmpdevice.ex_sad == 0 ? '0' : '1');

			cm_begin_trans(cm_args.cm_key, RM_RDWR);
			(void) cm_delval(&cm_args);
			if (cm_addval(&cm_args)) {
				cm_abort_trans(cm_args.cm_key);
				cmn_err(CE_WARN,"e3Dverify: cm_addval for DATAMODE at key %d failed", key);
				return(ENODEV);
			}
			cm_end_trans(cm_args.cm_key);

			sioa = e3Dtmpdevice.ex_ioaddr;
			eioa = e3Dtmpdevice.ex_ioaddr + 0xf;
			/*
			 * if your board/bcfg uses 2 for the irq you
			 * should translate it to 9 here
			 */
			vector = e3Dtmpdevice.ex_int;
			scma = (ulong_t) e3Dtmpdevice.ex_base;
			ecma = (ulong_t) e3Dtmpdevice.ex_base + e3Dtmpdevice.ex_memsize - 1;

			/*
			 * We have filled in the custom parameters and
			 * other ISA parameters.  Tell ndcfg about them.
			 * We set DMA to null as we don't need it.
			 * Only call mdi_AT_verify after all custom
			 * parameters have been added to the resmgr.
			 */
			getset = MDI_ISAVERIFY_GET_REPLY;
			err = mdi_AT_verify(key, &getset, &sioa, &eioa,
					&vector, &scma, &ecma, NULL);
	
			if (err != 0) {
				cmn_err(CE_CONT,"e3Dverify: mdi_AT_verify returned %d",err);
				return(ENODEV);
			}
			
			return(0);

		} else if (getset == MDI_ISAVERIFY_SET) {
			extern void e3Dconfigure(e3Ddev_t *);

			/*
			 * set the eeprom/nvram to parameters indicated by 
			 * those returned from mdi_AT_verify.  If the parameter
			 * is -1 then it shouldn't change from its current
			 * value.  Note we must read custom parameters from
			 * the resmgr by calling cm_getval explicitly
			 */

			if (sioa != -1) {
				e3Dtmpdevice.ex_ioaddr = sioa;
			}

			/* irq 2 and 9 are the same on isa machines */
			if (vector != -1) {
				e3Dtmpdevice.ex_int = vector;
			}
			if (scma != -1 && ecma != -1) {
				e3Dtmpdevice.ex_base = (caddr_t) scma;
				e3Dtmpdevice.ex_memsize = ecma+1-scma;
			}

			/* now get custom parameters */
			cm_args.cm_key = key;
			cm_args.cm_n = 0;
			cm_args.cm_val = &answer;
			cm_args.cm_vallen = sizeof(answer);
			cm_args.cm_param = "CABLETYPE"; /* less than 11 chars */

			/*
			 * If the custom parameter shouldn't change, it may not
			 * be set in the resmgr.  Don't error, continue on to
			 * next parameter.
			 */

			/*
			 * ex_bnc is 0 for AUI or HROM_BNC(0x80) for other.
			 * The bcfg file says '0' is BNC/TP, '1' is AUI.
			 */
			cm_begin_trans(cm_args.cm_key, RM_READ);
			if (cm_getval(&cm_args) == 0) {
				e3Dtmpdevice.ex_bnc = (answer[0] == '0' ? HROM_BNC : 0);
			}
			cm_end_trans(cm_args.cm_key);

			cm_args.cm_key = key;
			cm_args.cm_n = 0;
			cm_args.cm_val = &answer;
			cm_args.cm_vallen = sizeof(answer);
			cm_args.cm_param = "ZEROWAIT";

			/*
			 * ex_zerows is 0 for disabled or HRAM_0WS for enabled.
			 * .bcfg file says '0' is disabled, '1' is enabled
			 * and puts diabled first so it's the default
			 */
			cm_begin_trans(cm_args.cm_key, RM_READ);
			if (cm_getval(&cm_args) == 0) {
				e3Dtmpdevice.ex_zerows = (answer[0] == '0' ? 0 : HRAM_0WS);
			}
			cm_end_trans(cm_args.cm_key);

			cm_args.cm_key = key;
			cm_args.cm_n = 0;
			cm_args.cm_val = &answer;
			cm_args.cm_vallen = sizeof(answer);
			cm_args.cm_param = "DATAMODE";

			/*
			 * ex_sad is 0 for Turbo and HRAM_SAD for Standard
			 * .bcfg file says '0' is Turbo and '1' is Standard
			 * Turbo is the default
			 */
			cm_begin_trans(cm_args.cm_key, RM_READ);
			if (cm_getval(&cm_args) == 0) {
				e3Dtmpdevice.ex_sad = (answer[0] == '0' ? 0 : HRAM_SAD);
			}
			cm_end_trans(cm_args.cm_key);

			/*
			 * There is no way to change the ROM base address 
			 * or ROM size.  We'll set its size to 0 so as not
			 * to interfere with anything else on the system.
			 */
			e3Dtmpdevice.ex_romsize = 0;

			/*
			 * We must call cm_AT_putconf again to indicate the new
			 * settings at the key.	if we are modifying an existing
			 * board this is not an issue as the specific parameter
			 * will already have been changed by ndcfg but if we
			 * are adding a new board this is necessary.  Why?
			 * The order is as follows:
			 * 1) netcfg issues the 'idinstall' command to ndcfg
			 * 2) ndcfg creates new key(we're ISA), populates with
			 *	all information necessary
			 * 3) ndcfg idinstalls the driver
			 * 4) ndcfg loads the driver.  This calls our init
			 *    routine which calls cm_AT_putconf giving it
			 *    the firmware's current arguments, not necessarily
			 *    what the user wanted to use.
			 * 5) ndcfg calls our verify routine with
			 *    MDI_ISAVERIFY_SET telling us to reprogram
			 *    firmware.  Ok, we will do that, but the resmgr
			 *    still has old values, which are wrong.
			 *    Of course, this will be taken care of at next boot
			 *    since we will call cm_AT_putconf again, but why
			 *    prolong the agony?
			 * 6) We reprogram firmware and call cm_AT_putconf
			 *    again here, updating the resmgr
			 * 7) ndcfg calls idconfupdate after we return from the
			 *    verify routine (in both idinstall and idmodify
			 *    cases)  If this driver never called cm_AT_putconf
			 *    in the init routine then we wouldn't have to call
			 *    it again here.  Also, since many of these values
			 *    may not be set from mdi_AT_verify, we must use
			 *    the later of:
			 *        a) current firmware
			 *        b) modified setting from mdi_AT_verify
			 *    in our call to cm_AT_putconf below.
			 * Finally, the key passed to our verify routine is not
			 * the key we need to modify with cm_AT_putconf.
			 *  The key to pass to cm_AT_putconf is stored in the
			 * parameter "PUTCONFKEY".  This parameter only exists
			 * for MDI_ISAVERIFY_SET mode.
			 */

			cm_args.cm_key = key;
			cm_args.cm_n = 0;
			cm_args.cm_param = "PUTCONFKEY";
			cm_args.cm_val = &putconfkey;
			cm_args.cm_vallen = sizeof(rm_key_t);

			cm_begin_trans(cm_args.cm_key, RM_READ);
			if (cm_getval(&cm_args) != 0) {
				cm_end_trans(cm_args.cm_key);
				cmn_err(CE_WARN, "e3Dverify: no PUTCONFKEY at key 0x%x\n",
					key);
				return(ENODEV);
			}
			cm_end_trans(cm_args.cm_key);

			/* update resmgr */
			e3Dputconf(putconfkey, &e3Dtmpdevice);

			e3Dconfigure(&e3Dtmpdevice);	/* update firmware */

			return(0);
		} else {
			cmn_err(CE_WARN, "!e3Dverify: unknown mode %d\n",getset);
			return(ENODEV);
		}
		/* NOTREACHED */
	}
	/*
	 * to get here e3Dpresent returned false.  Not a problem unless 
	 * we're in set mode and the user wishes to change the io address
	 * of the board from its current value and the user forgot to
	 * supply the OLDIOADDR= parameter to the idinstall cmd in ndcfg.
	 * netcfg takes care of this for us.
	 * Also possible that we're trying to install (and set) parameters
	 * on the board but the user hasn't installed the board in the
	 * machine yet.	In this case there's not much we can do, although
	 * the next time we reboot the user will notice something went wrong
	 * as the resmgr parameters will be different (since we call
	 * cm_AT_putconf from load/init).  Since we can also legimately get here
	 * from ISA "search" mode where ndcfg tries each of the possible I/O
	 * addresses we don't call cmn_err.
	 */
	
	if (e3Dtmpdevice.ex_rambase) {
		/* e3Dpresent returned false.	free devmem_mapin'd memory */
		devmem_mapout(e3Dtmpdevice.ex_rambase, e3Dtmpdevice.ex_memsize);
		e3Dtmpdevice.ex_rambase = (caddr_t) 0;
	}
	
	return(ENODEV);
}

/*
 * Function: e3Dputconf
 *
 * Purpose:
 * Store 3C507 hardware parameters in the resource manager database
 * called by e3Dconfig and e3Dverify routine.
 */
STATIC int 
e3Dputconf(rm_key_t key, e3Ddev_t *dev )
{
	struct cm_args	cm_args;
	cm_range_t	range;
	cm_num_t	num;
	int		returnval;
	
	/* sets CM_IRQ, CM_ITYPE, CM_IOADDR, CM_MEMADDR */
	/* cm_AT_putconf calls cm_begin_trans/cm_end_trans for us */
	returnval = cm_AT_putconf(key, dev->ex_int, CM_ITYPE_EDGE, 
			dev->ex_ioaddr, dev->ex_ioaddr+0x0F,
			(ulong_t)dev->ex_base, 
			(ulong_t)(dev->ex_base+dev->ex_memsize-1), -1,
			CM_SET_IRQ|CM_SET_ITYPE|CM_SET_IOADDR|CM_SET_MEMADDR,
			0);

	if (returnval) {
		cmn_err(CE_NOTE,"e3Dputconf: cm_AT_putconf returned %d\n",returnval);
		return(returnval);
	}
	
	return(0);
}


/*
 * Function: e3Daddrstr
 *
 * Purpose:
 *	Generate a printable version of the hw mac address
 *	ie. turn 02 60 8c f0 12 3a into "02:60:8c:f0:12:3a"
 */
#define	EASTRINGSIZE	18		/* size of string with ether address */ 
char *
e3Daddrstr(macaddr_t ea)
{
	static char 	buf[EASTRINGSIZE];
	int 		i;
	char 		*s = "0123456789abcdef";
	
	for (i = 0; i < sizeof(macaddr_t); ++i) {
		buf[i*3] = s[(ea[i] >> 4) & 0xf];
		buf[i*3 + 1] = s[ea[i] & 0xf];
		buf[i*3 + 2] = ':';
	}
	buf[EASTRINGSIZE - 1] = '\0';
	return(buf);
}

STATIC char *
e3Dpartnostr(unsigned char *ea)
{
	static char buf[10];
	char *s="0123456789abcdef";

	buf[0] = s[(ea[0] >> 4) & 0xf];
	buf[1] = s[ea[0] & 0xf];
	buf[2] = s[(ea[1] >> 4) & 0xf];
	buf[3] = s[ea[1] & 0xf];
	buf[4] = s[(ea[2] >> 4) & 0xf];
	buf[5] = s[ea[2] & 0xf];
	buf[6] = '-';
	buf[7] = s[(ea[3] >> 4) & 0xf];
	buf[8] = s[ea[3] & 0xf];
	buf[9] = '\0';
	return(buf);
}

/*
 * Function: e3Ddata
 * 
 * Purpose:
 *	Handle a data message.
 * DDI8 SUSPEND note:  we silently throw away data if suspended...
 */
STATIC void
e3Ddata(queue_t *q, mblk_t *mp)
{
	e3Ddev_t	*dev;
	int		do_loop;
	macaddr_t	*ea;
	int 		s;
	
	dev = (e3Ddev_t *)q->q_ptr;
	ASSERT(dev != NULL);
	
	if (!dev->ex_up_queue) {
		mdi_macerrorack(RD(q), M_DATA, MAC_OUTSTATE);
		freemsg(mp);
		return;
	}

	/* we silently throw away any data to send on wire if received while 
	 * suspended rather than clog streams, exceeding our high water mark
	 * if upstream is dlpi (if upstream is stream head it pays attention
	 * to our high water mark but still clogs streams)
	 */
	if (dev->issuspended) {
		dev->dropsuspended++;
		freemsg(mp);
		return;
	}
	
	ea = &(((e_frame_t *) mp->b_rptr)->eh_daddr);
	
	if ((ISMULTICAST(*ea) && mdi_valid_mca(dev->dlpi_cookie, *ea) != -1)
	   || ISBROADCAST(*ea)
	   || mdi_addrs_equal(dev->ex_eaddr, *ea))
		mdi_do_loopback(q, mp, E3D_ETH_MINPACK);
	
	s = splstr();
	if (q->q_first || !e3Doktoput(dev)) {
		putq(q, mp);
	} else {
		e3Dhwput(dev, mp);
		freemsg(mp);
	}
	splx(s);
}

/* 
 * Function: e3Dioctl
 * 
 * Purpose:
 *	Process an ioctl streams message.
 * DDI8 SUSPEND note:  if ioctl cmd doesn't talk to the hardware we allow it
 *                     but if it does and if we're suspended then we fail 
 *                     ioctl with EAGAIN.
 */
STATIC void
e3Dioctl(queue_t *q, mblk_t *mp)
{
	unsigned char	*cp;
	unsigned char	*data;
	e3Ddev_t	*dev;
	struct iocblk	*iocp = (struct iocblk *) mp->b_rptr;
	int 		j;
	int 		size;
	unsigned int	x;
	
	dev = (e3Ddev_t *)q->q_ptr;
	ASSERT(dev != NULL);
	
	data = (mp->b_cont) ? mp->b_cont->b_rptr : NULL;
	iocp->ioc_error = 0;
	iocp->ioc_rval = 0;
	
	switch (iocp->ioc_cmd) {
	case MACIOC_GETSTAT:	/* dump mac or dod statistics */
		if (data == NULL || iocp->ioc_count < sizeof(mac_stats_eth_t)) {
			iocp->ioc_error = EINVAL;
		} else {
			bcopy(&dev->ex_macstats, data, sizeof(mac_stats_eth_t));
			mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof(mac_stats_eth_t);
			iocp->ioc_count = sizeof(mac_stats_eth_t);
		}
		break;
		
	case MACIOC_CLRSTAT:	/* clear mac statistics */
		if (drv_priv(iocp->ioc_cr) == EPERM) {
			iocp->ioc_error = EPERM;
		} else {
			x = splstr();
			bzero(&dev->ex_macstats, sizeof(mac_stats_eth_t));
			splx(x);
			iocp->ioc_count = 0;
			if (mp->b_cont) {
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}
		}
		break;
		
	case MACIOC_GETRADDR:	/* get factory mac address */
		if (data == NULL || iocp->ioc_count < sizeof(macaddr_t)) {
			iocp->ioc_error = EINVAL;
		} else {
			x = splstr();
			bcopy(&dev->ex_hwaddr, data, sizeof(macaddr_t));
			splx(x);
			mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof(macaddr_t);
			iocp->ioc_count = sizeof(macaddr_t);
		}
		break;
	case MACIOC_GETADDR:	/* get mac address */
		if (data == NULL || iocp->ioc_count < sizeof(macaddr_t)) {
			iocp->ioc_error = EINVAL;
		} else {
			x = splstr();
			bcopy(&dev->ex_eaddr, data, sizeof(macaddr_t));
			splx(x);
			mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof(macaddr_t);
			iocp->ioc_count = sizeof(macaddr_t);
		}
		break;

	case MACIOC_SETADDR:	/* set mac address */
		/* too bad, we talk to hardware, please try again later */
		if (dev->issuspended) {
			iocp->ioc_error = EAGAIN;
		} else if (data == NULL || iocp->ioc_count < sizeof(macaddr_t)) {
			iocp->ioc_error = EINVAL;
		} else if (drv_priv(iocp->ioc_cr) == EPERM) {
			iocp->ioc_error = EPERM;
		} else {
			x = splstr();
			if (e3Dmacaddrset(dev, q, mp) == TRUE) {
				splx(x);
			
				/* h/w has to respond before we ACK the IOCTL */
				return;
			}
			/* can't handle IOCTL until previous one completes */
			iocp->ioc_error = EBUSY;
			splx(x);
		}
		break;
		
	case MACIOC_SETMCA:	/* multicast setup */
		if (data == NULL)
			iocp->ioc_error = EINVAL;
		else if (dev->issuspended)
			/* too bad, we can't talk to hardware, please try again later */
			iocp->ioc_error = EAGAIN;
		else if (dev->mccnt >= E3D_NMCADDR)
			iocp->ioc_error = ENOSPC;
		else if (iocp->ioc_count != sizeof(macaddr_t))
			iocp->ioc_error = EINVAL;
		else if (drv_priv(iocp->ioc_cr) == EPERM)
			iocp->ioc_error = EPERM;
		else if (!ISMULTICAST( *((macaddr_t *) data)))
			iocp->ioc_error = EINVAL;
		else {
			x = splstr();
			if (e3Dmcaddrset(dev, q, mp) == TRUE) {
				splx(x);
			
				/* h/w responds before ACK */
				return;
			}
			/* can't handle IOCTL until previous one completes */
			iocp->ioc_error = EBUSY;
			splx(x);
		}
		break;
		
	case MACIOC_DELMCA:	/* multicast delete */
		if (data == NULL)
			iocp->ioc_error = EINVAL;
		else if (dev->issuspended)
			/* too bad, we talk to hardware, please try again later */
			iocp->ioc_error = EAGAIN;
		else if (iocp->ioc_count != sizeof(macaddr_t))
			iocp->ioc_error = EINVAL;
		else if (drv_priv(iocp->ioc_cr) == EPERM)
			iocp->ioc_error = EPERM;
		else {
			x = splstr();
			if (e3Dmcaddrset(dev, q, mp) == TRUE) {
				splx(x);
				/* h/w responds before ACK */
				return;
			}
			/* can't handle IOCTL until previous one completes */
			iocp->ioc_error = EBUSY;
			splx(x);
		}
		break;

		/* see demo code at the end of this file */
	case 0xdeadbeef:   /* fake a CFG_SUSPEND */
		if (drv_priv(iocp->ioc_cr) == EPERM) {
			iocp->ioc_error = EPERM;
		} else {
			/* normally hpsl/hpci will invoke e3Dconfig for us automatically */
			if ((x=e3Dconfig(CFG_SUSPEND, dev, dev->rmkey))) {
				cmn_err(CE_NOTE,"Problem invoking CFG_SUSPEND (ret=%d)",x);
			}
		}
		break;

		/* see demo code at the end of this file */
	case 0xfeedface:   /* fake a CFG_RESUME */
		if (drv_priv(iocp->ioc_cr) == EPERM) {
			iocp->ioc_error = EPERM;
		} else {
			/* normally hpsl/hpci will invoke e3Dconfig for us automatically */
			if ((x=e3Dconfig(CFG_RESUME, dev, dev->rmkey))) {
				cmn_err(CE_NOTE,"Problem invoking CFG_RESUME (ret=%d)",x);
			}
		}
		break;
		
	default:
		cmn_err(CE_NOTE,"!e3Dioctl: unknown cmd 0x%x\n",iocp->ioc_cmd);
		iocp->ioc_error = EINVAL;
		break;
	}
	
	if (iocp->ioc_error) {
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_count = 0;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
	} else {
		mp->b_datap->db_type = M_IOCACK;
	}
	qreply(q, mp);
	return;
}

/*
 * Function: e3Dmacproto
 *
 * Purpose:
 *	Handle MAC protocol messages.
 */
STATIC void
e3Dmacproto(queue_t *q, mblk_t *mp)
{
	e3Ddev_t		*dev;
	mac_stats_eth_t		*e3Dmacstats;
	mac_info_ack_t		*info_ack;
	union MAC_primitives	*prim;
	mblk_t			*reply_mp;
	int 			s;

	dev = (e3Ddev_t *)q->q_ptr;
	ASSERT(dev != NULL);
	prim = (union MAC_primitives *) mp->b_rptr;
	e3Dmacstats = &dev->ex_macstats;
	
	DEBUG12(CE_CONT,"e3Dmacproto: prim=x%x\n", prim->mac_primitive);
	
	switch(prim->mac_primitive) {
	case MAC_INFO_REQ:
		/* it's legal to issue this if hardware is suspended from both
		 * ddi8 and DLPI point of view.
		 */
		if ((reply_mp=allocb(sizeof(mac_info_ack_t), BPRI_HI)) == NULL)
			cmn_err(CE_WARN, "e3D:e3Dmacproto - Out of STREAMs");
		else {
			info_ack = (mac_info_ack_t *) reply_mp->b_rptr;
			reply_mp->b_wptr += sizeof(mac_info_ack_t);
			reply_mp->b_datap->db_type = M_PCPROTO;
			info_ack->mac_primitive = MAC_INFO_ACK;
			info_ack->mac_max_sdu = E3D_ETH_MAXPACK;
			info_ack->mac_min_sdu = 1;
			info_ack->mac_mac_type = MAC_CSMACD;
			info_ack->mac_driver_version = MDI_VERSION;
			info_ack->mac_if_speed = 10000000;
			qreply(q, reply_mp);
		}
		break;
		
	case MAC_BIND_REQ:
		/* DLPI says that hardware must be ready *after* the BIND succeeds
		 * we fail the bind if hardware is currently suspended 
		 * we emulate this for MDI.
		 */
		if (dev->issuspended)
			mdi_macerrorack(RD(q), prim->mac_primitive, MAC_HWNOTAVAIL);
		if (dev->ex_up_queue)
			mdi_macerrorack(RD(q), prim->mac_primitive, MAC_OUTSTATE);
		else {
			dev->ex_up_queue = RD(q);
			dev->dlpi_cookie = prim->bind_req.mac_cookie;
			mdi_macokack(RD(q), prim->mac_primitive);
		}
		break;
		
	default:
		mdi_macerrorack(RD(q), prim->mac_primitive, MAC_BADPRIM);
		break;
	}
	
	freemsg(mp);
	return;
}

/*
 * Function: e3Dioctlack
 *
 * Purpose:
 *	Acknowledge an ioctl request.
 */
void
e3Dioctlack(e3Ddev_t *dv, queue_t *q, mblk_t *mp, int status, 
		int release_b_cont)
{
	struct iocblk *iocp = (struct iocblk *) mp->b_rptr;
	
	ASSERT(q && mp);
	ASSERT(mp->b_datap->db_type == M_IOCTL);
	
	if (status == OK) {
		iocp->ioc_rval = 0;
		iocp->ioc_count = 0;
		iocp->ioc_error = 0;
		mp->b_datap->db_type = M_IOCACK;
	} else {
		iocp->ioc_rval = -1;
		iocp->ioc_count = 0;
		iocp->ioc_error = EIO;
		mp->b_datap->db_type = M_IOCNAK;
	}
	if (mp->b_cont && release_b_cont) {
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
	}
	qreply(q, mp);
	return;
}

/*
 * Function: e3Dopen
 *
 * Purpose:
 *	Usual driver open routine.  Note new DDI8 arguments that are same for both
 * streams and non streams devices
 * Generally called by ddi_bind_open which uses the routine e3Dopen from
 * the e3D_drvops structure and not from the qinit structures.
 */
int
e3Dopen(void *idata, channel_t *channelp, int oflags, 
        cred_t *crp, queue_t *q)
{
	e3Ddev_t	*dv;
	int 		x;
	
#ifdef DEMO_FILEOPS
	/* your driver may wish to download code to the card(s)
	 * or pull in some message catalogs etc.  The e3D driver doesn't
	 * need to do any of this, but we'll show how your driver can read
	 * any file on the system to do this.
	 */
	e3Dcat_file("/etc/inst/nd/dlpimdi");
#endif

	if (idata == NULL) {
		/* e3Dconfig with CFG_ADD failed previously */
		return(ENXIO);
	}

	dv = (e3Ddev_t *)idata;
	if (dv->ex_present != TRUE) {
		DEBUG11(CE_CONT,"e3Dopen: board not present\n");
		return (ENXIO);
	}
	
	if (dv->ex_open == TRUE) {
		return (EBUSY);
	}

	if (dv->issuspended) {
		/* hardware currently suspended, can't call hwinit, fail open.
		 * XXX should block here unless O_NONBLOCK/O_NDELAY set.
		 * note that opens on /dev/netX are not impacted by this return(EAGAIN)
		 * but by the earlier call to mdi_hw_suspended
		 * this return will impact raw MDI consumers: dlpid, tcpdump, test suites
		 */
		return(EAGAIN);
	}
	
	x = splstr();
	if (e3Dhwinit(dv) == FALSE) {
		splx(x);
		return (ENXIO);
	}
	
	q->q_ptr = WR(q)->q_ptr = idata;
	dv->ex_open = TRUE;
	dv->ex_up_queue = 0;
	dv->mccnt = 0;
	dv->ex_hwinited = 1;
	splx(x);
	return(0);
}

/* note new DDI8 arguments that are now the same for both streams and 
 * non-streams devices
 * DDI8 note:
 *  - one close call is issued for each open call for all DDI8 drivers.
 */
int
e3Dclose(void *idata, channel_t channel, int oflags, cred_t *crp, queue_t *q)
{
	e3Ddev_t	*dev;
	int		x;
	
	dev = (e3Ddev_t *)q->q_ptr;
	ASSERT(dev != NULL);
	ASSERT(dev == idata);
	
	DEBUG11(CE_CONT,"e3Dclose idata=0x%x\n", idata);

	x = splstr();
	
	flushq(WR(q), 1);
	q->q_ptr = NULL;
	
	dev->ex_up_queue = 0;
	
	if (!dev->issuspended) {
		/* e3Dhwclose was called at suspend time to prevent board from generating
		 * more interrupts.  We also check ex_open again at
		 * CFG_RESUME time to see if we should call e3Dhwclose again
		 * i.e. user process terminated while we were suspended
		 */
		e3Dhwclose(dev);
	}
	dev->mccnt = 0;
	dev->ex_open = FALSE;
	dev->ex_hwinited = FALSE;
	
	splx(x);
	
	return (0);
}

/*
 * Function: e3Duwput
 *
 * Purpose:
 *	Write put routine.
 */
int
e3Duwput(queue_t *q, mblk_t *mp)
{
	DEBUG12(CE_CONT,"e3Duwput: type=x%x\n",mp->b_datap->db_type);
	
	switch (mp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		e3Dmacproto(q, mp);
		break;
		
	case M_DATA:
		e3Ddata(q, mp);
		break;
		
	case M_IOCTL:
		e3Dioctl(q, mp);
		break;
		
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHALL);
			*mp->b_rptr &= ~FLUSHW;
		}
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHALL);
			qreply(q, mp);
		} else {
			freemsg(mp);
		}
		break;
		
	default:
		DEBUG12(CE_CONT,"e3Duwput:unknown STR msg type=x%x mp=x%x\n",
			mp->b_datap->db_type, mp);
		freemsg(mp);
		break;
	}
	
	return;
}

/* DEMO_FILEOPS isn't defined, but feel free to do so if you want to 
 * see how this works 
 */

#ifdef DEMO_FILEOPS

/*
 * Function: e3Dcat_file
 *
 * Purpose:
 *      Demonstrate how a driver can open up a file residing in the
 *      filesystem.   idinstall(1M) will handle firmware.o and msg.o
 *      files automatically as part of your DSP, and they will will wind 
 *      up in the /etc/conf/pack.d/<your_drivername> directory.
 *      You can use this pathname (ignoring idbuild's $ROOT)
 *      or hard wire your own, as long as it's an absolute pathname.
 *      The e3D driver doesn't need to do any of this, but this example
 *      will show how it's done.
 *
 *      This technique is useful for obtaining download firmware or a 
 *      driver's message catalog from user space and avoids recompiling
 *      the driver for different locales.
 *      Indeed, this can eliminate PRE_SCRIPT and POST_SCRIPT (although
 *      you must still list the extra files in EXTRA_FILES)
 */
void
e3Dcat_file(char *pathname)
{
	unsigned char *filebuffer;
	int ret,loop,bufferlength;

	ret=mdi_open_file(pathname, &filebuffer, &bufferlength);
	if (ret) {
		/* some sort of problem encountered:
		 * ENOENT: null path, not absolute, lookupname failed,
		 *         not regular file, 
		 * EACCES: couldn't open file, couldn't stat file, 
		 * EFBIG:  file is 0 bytes or too big (> 64K)
		 * ENOMEM: no memory available to hold buffer to file
		 * others are possible, these are the main ones
		 * cmn_err will have already been called by mdi_open_file.
		 */
		cmn_err(CE_NOTE,"e3Dcat_file: couldn't open %s\n",pathname);
		return;
	}
	/* filebuffer now points to the contents of the file
	 * bufferlength has the size of the file
	 * you would pump the contents of the file to the firmware here.
	 * --->Note that if something goes wrong past this point we must still
	 * call mdi_close_file to close the file and release the memory!<----
	 * for demonstration purposes we'll just display the file on the 
	 * console.
	 */
	cmn_err(CE_NOTE,
		"%s is %d bytes long and contains the following text:\n",
		pathname, bufferlength);
	for (ret=0; ret<bufferlength; ret++) {
		cmn_err(CE_CONT,"%c",filebuffer[ret]);
	}
	cmn_err(CE_CONT,"\n");

	/* close the file we previously opened */
	mdi_close_file(filebuffer);

	return;
}
#endif   /* DEMO_FILEOPS */

/* new routines added to write to eeprom to accompany e3Dverify */

/*
 * e3Dstart_eeprom_cmd():	Set up EEPROM for a command.
 */
STATIC
e3Dstart_eeprom_cmd(e3Ddev_t *dp, int cmd)
{
	register int i;

	dp->ex_intr2 &= 0x0f;	/* ECS=0, ESK=0, EDI=0, RST=0 */
	/* clear chip select and do one clock cycle */
	SET_ECS_EEPROM(dp)	/* ECS=1 */
	drv_usecwait(2);

	/* instruction start bits 01 */
	SET_ESK_EEPROM(dp)	/* start bit 0 clocked in */
	drv_usecwait(2);
	CLR_ESK_EEPROM(dp)
	drv_usecwait(2);
	SET_EDI(dp)
	SET_ESK_EEPROM(dp)	/* start bit 1 clocked in */
	drv_usecwait(2);

	/* output command opcode */
	for (i = (NEEPROM_CMD - 1); i >= 0; i--) {
		CLR_ESK_EEPROM(dp)
		if ((cmd >> i) & 1)
			SET_EDI(dp)
		else
			CLR_EDI(dp);
		drv_usecwait(2);
		SET_ESK_EEPROM(dp)
		drv_usecwait(2);
	}
}

/*
 * e3Ddata_eeprom():	Writes 2 bytes of data to the EEPROM.
 */
STATIC
e3Ddata_eeprom(e3Ddev_t *dp, int data)
{
	register int i;

	/* output data */
	for (i = (NEEPROM_DATA_BITS - 1); i >= 0; i--) {
		CLR_ESK_EEPROM(dp)
		if ((data >> i) & 1)
			SET_EDI(dp)
		else
			CLR_EDI(dp);
		drv_usecwait(2);
		SET_ESK_EEPROM(dp)
		drv_usecwait(2);
	}
}


/*
 * e3Dend_eeeprom_cmd():	complete EEPROM command.
 */
STATIC
e3Dend_eeprom_cmd(e3Ddev_t *dp)
{
	CLR_ECS_EEPROM(dp)	/* clear chip select */
	SET_EDI(dp)
	SET_ESK_EEPROM(dp)	/* wait for one ESK clock cycle */
	drv_usecwait(2);
	CLR_ESK_EEPROM(dp)
	drv_usecwait(2);
}

/*
 * e3Dread_eeprom():	reads data from the EEPROM.
 */
STATIC u_short
e3Dread_eeprom(e3Ddev_t *dp, int addr)
{
	register int		i;
	register u_short	eeword = 0;

	if (addr)
		e3Dstart_eeprom_cmd(dp, EEPROM_READ1);
	else
		e3Dstart_eeprom_cmd(dp, EEPROM_READ0);

	/* first data bit is always 0 - toss it */
	CLR_ESK_EEPROM(dp)
	drv_usecwait(2);
	SET_ESK_EEPROM(dp)
	drv_usecwait(2);

	/* read 16 bits */
	for (i = 15; i >= 0; i--) {
		eeword |= (GET_EDO(dp) << i);
		CLR_ESK_EEPROM(dp)
		drv_usecwait(2);
		SET_ESK_EEPROM(dp)
		drv_usecwait(2);
	}
	e3Dend_eeprom_cmd(dp);
	return(eeword);
}

/*
 * e3Dwrite_eeprom():	Writes 2 bytes of data to the EEPROM.
 */
STATIC
e3Dwrite_eeprom(e3Ddev_t *dp, u_long eeprom)
{
	register int		i;
	register u_short	eeprom_word;
	u_long			new_eeprom;

	e3Dstart_eeprom_cmd(dp, EEPROM_EWEN);	 /* enable erase/write */
	e3Ddata_eeprom(dp, 0);
	e3Dend_eeprom_cmd(dp);

	e3Dstart_eeprom_cmd(dp, EEPROM_ERASE0);	/* erase register 0 */
	e3Ddata_eeprom(dp, 0);
	e3Dend_eeprom_cmd(dp);
	drv_usecwait(10000);			/* Te/w is 10-30 ms */

	/* write data word register 0 */
	eeprom_word = (u_short) eeprom & WORD_MASK;
	e3Dstart_eeprom_cmd(dp, EEPROM_WRITE0);
	e3Ddata_eeprom(dp, eeprom_word);
	e3Dend_eeprom_cmd(dp);
	drv_usecwait(10000);			/* Te/w is 10-30 ms */

	e3Dstart_eeprom_cmd(dp, EEPROM_ERASE1);	/* erase register 1 */
	e3Ddata_eeprom(dp, 0);
	e3Dend_eeprom_cmd(dp);
	drv_usecwait(10000);			/* Te/w is 10-30 ms */

	/* write data word register 1 */
	eeprom_word = (u_short) (eeprom >> 16) & WORD_MASK;
	e3Dstart_eeprom_cmd(dp, EEPROM_WRITE1);
	e3Ddata_eeprom(dp, eeprom_word);
	e3Dend_eeprom_cmd(dp);
	drv_usecwait(10000);			/* Te/w is 10-30 ms */

	e3Dstart_eeprom_cmd(dp, EEPROM_EWDS);	/* disable erase/write */
	e3Ddata_eeprom(dp, 0);
	e3Dend_eeprom_cmd(dp);

	/* RESET EEPROM state machine logic by writing to offset 0x0c */
	dp->ex_intr2 &= 0x1f;	/* ECS=0, ESK=0, EDI=0, RST=0 */
	SET_INTREG(dp, dp->ex_intr2);
	drv_usecwait(20000);	/* Takes 20ms to read out config registers */

	new_eeprom = e3Dread_eeprom(dp, 0);
	new_eeprom |= (u_long)(e3Dread_eeprom(dp, 1) << 16);

	e3Drunstate();				/* run state */

	if (eeprom != new_eeprom)
		return(NOT_OK);
	return(OK);
}

/*
 * e3Dconfigure: Re-configure the board.
 *			Assumes e3dset[1] has the new configuration.
 *
 * The adapter is placed in the IOLOAD state; writing the (new) I/O address
 * to the ID port puts the adapter into the CONFIG state; then other parameters
 * can be written.
 *
 * MP NOTE:	we should have a lock in e3Ddev_t struct so that we won't have
 * multiple people in this routine, since it changes the state of the
 * board(s) installed in the system and we don't want anyone else doing
 * the same thing. 
 * Even better is to lock down the i/o address directly as the pointer
 * we get here is from e3Dverify and not the global e3Ddev_t structure
 * since the verify routine can be called when board is already in use!
 */
STATIC void
e3Dconfigure(e3Ddev_t *dp)
{
	register int i;
	u_long eeprom = 0;
	short ioaddr=dp->ex_ioaddr;

	e3Dloadstate();

	LOAD_IOADDR(dp->ex_ioaddr);	 /* config state */
	eeprom = e3Dsoft_config(dp);
	if (!e3Dwrite_eeprom(dp, eeprom)) {
		cmn_err(CE_WARN,"e3D: possible conflict detected at "
			"base I/O address 0x%x\n",ioaddr);
		cmn_err(CE_CONT,"when writing to EEPROM.	The new "
			"configurations are not saved and will be "
			"lost when the machine is powered off\n");
	}
}

#ifdef NEVER

Since this is an ISA hot plug driver, we need some way to fake 
a CFG_SUSPEND/CFG_RESUME to test our e3Dconfig routine.  hpsl/hpci 
currently only works for PCI devices.   While not exactly elegant,
it does work...

******************************************************************************
/* code to demonstrate CFG_SUSPEND */
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stream.h>
#include <sys/stropts.h>

main()
{
int fd;
unsigned int beef=0xdeadbeef;
struct strioctl si;

si.ic_cmd = 0xdeadbeef;
si.ic_timout = 0;
si.ic_len = sizeof(beef);
si.ic_dp = (char *)&beef;
fd=open("/dev/net0", O_RDWR);
ioctl(fd, I_STR, &si);
close(fd);
}


******************************************************************************

/* code to demonstrate CFG_RESUME */

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stream.h>
#include <sys/stropts.h>

main()
{
int fd;
unsigned int feed=0xfeedface;
struct strioctl si;

si.ic_cmd = 0xfeedface;
si.ic_timout = 0;
si.ic_len = sizeof(feed);
si.ic_dp = (char *)&feed;
fd=open("/dev/net0", O_RDWR);
ioctl(fd, I_STR, &si);
close(fd);
}

#endif  /* NEVER */
