#ident "@(#)e3Bli.c	28.1"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 *	System V STREAMS TCP - Release 4.0
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
/*      SCCS IDENTIFICATION        */

#ifdef _KERNEL_HEADERS

#include <io/nd/mdi/e3B/e3B.h>

#else

#include "e3B.h"

#endif

int e3Bopen(), e3Bclose(), e3Buwput();
extern int nulldev();

struct module_info e3B_minfo = {
	0, "e3B", 1, E3BETHERMTU, 16*E3BETHERMTU, 12*E3BETHERMTU
};

struct qinit e3Burinit = {
	0,  0, e3Bopen, e3Bclose, nulldev, &e3B_minfo, 0
};

struct qinit e3Buwinit = {
	e3Buwput, 0, e3Bopen, e3Bclose, nulldev, &e3B_minfo, 0
};

struct streamtab e3Binfo = { &e3Burinit, &e3Buwinit, 0, 0 };
unsigned char e3B_broad[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int e3Bstrncmp();

extern e3Bhwput(), e3Bhwinit();

unsigned int e3B_nunit;
struct e3Bdevice *e3Bdev;

#ifdef UNIXWARE
void **e3Bcookie;
int e3Bdevflag = 0;

/* ndcfg/netcfg version string */
static char _ndversion[]="28.2";

#define DRVNAME "e3B - 3Com EtherLink II series MDI driver"
STATIC int e3B_load(), e3B_unload(), e3B_verify();
MOD_ACDRV_WRAPPER(e3B, e3B_load, e3B_unload, NULL, e3B_verify, DRVNAME);

STATIC int
e3B_load()
{
	int ret = 0;

	if ((ret = e3Binit()) != 0) {
		e3Buninit();
	}
	return(ret);
}

STATIC int
e3B_unload()
{
	e3Buninit();
	return(0);
}

e3Buninit()
{
	register int i;

	for (i = 0; i < e3B_nunit && e3Bcookie; i++) {
		if ((caddr_t)e3Bcookie[i] != (caddr_t)NULL)
			cm_intr_detach(e3Bcookie[i]);
			if (e3Bdev[i].tid) {
				untimeout(e3Bdev[i].tid);		/* cancel watchdog routine */
			}
	}
	if (e3Bcookie)
		kmem_free((void *)e3Bcookie, (sizeof(void **) * e3B_nunit));
 	if (e3Bdev)
		kmem_free((void *)e3Bdev, (sizeof(struct e3Bdevice) * e3B_nunit));
}

STATIC int
e3B_verify(key)
rm_key_t key;
{
	struct cm_args	cm_args;
	cm_range_t	range;
	unsigned char e[6];

	cm_args.cm_key = key;
	cm_args.cm_n = 0;
	cm_args.cm_param = CM_IOADDR;
	cm_args.cm_val = &range;
	cm_args.cm_vallen = sizeof(struct cm_addr_rng);

	if (cm_getval(&cm_args) != 0) {
		cmn_err(CE_WARN, "e3B_verify: no IOADDR for key 0x%x\n", key);
		return(ENODEV);
	}
	if (e3Bpresent(range.startaddr, e) == 0) {
		return(ENODEV);
	}
	return(0);
}
#endif /* UNIXWARE */

/*
 * The following six routines are the normal streams driver
 * access routines
 */

#ifdef UNIXWARE

e3Bopen(q, dev, flag, sflag, crp)
queue_t *q;
dev_t *dev;
cred_t *crp;

#else

e3Bopen(q, dev, flag, sflag)
register queue_t *q; 

#endif
{
	struct e3Bdevice *device;
	int x;
	int i;

#ifdef UNIXWARE
	minor_t unit = getminor(*dev);
	if (unit == -1) {
		cmn_err(CE_NOTE,"e3B: invalid minor -1");
		return(ENXIO);
	}
	for (i = 0; i < e3B_nunit; i++) {
		if (unit == e3Bdev[i].unit) {
			break;
		}
	}
	if (i == e3B_nunit) {
		/* walked through all of e3Bdev[] without finding a match */
		return(ENXIO);
	}
	device = &e3Bdev[i];
	unit = i;		/* avoid ifdefs where unit is used below */
#else
	int unit = minor(dev);
	if (unit >= e3B_nunit) {
	    u.u_error = ENXIO;
	    return(OPENFAIL);
	}
#endif
	device = &e3Bdev[unit];

	if (sflag == CLONEOPEN) {
#ifdef UNIXWARE
		return(EINVAL);
#else
		u.u_error = EINVAL;
		return(OPENFAIL);
#endif
	}

	if (! device->present) {
#ifdef UNIXWARE
		return(ENXIO);
#else
		u.u_error = ENXIO;
		return(OPENFAIL);
#endif
	}

	if (device->open == TRUE) {
#ifdef UNIXWARE
		return(EBUSY);
#else
	    u.u_error = EBUSY;
	    return (OPENFAIL);
#endif
	}
	if (e3BINIT(unit)) {
#ifdef UNIXWARE
		return(ENXIO);
#else
	    u.u_error = ENXIO;
	    return(OPENFAIL);
#endif
	}

	device->up_queue = (queue_t *)0;
	device->open = TRUE;
	x = splstr();
	q->q_ptr = WR(q)->q_ptr = (char *)unit;
	noenable(WR(q));	/* enabled after tx completion */
	splx(x);

#ifdef UNIXWARE
	return 0;
#else
	return(unit);
#endif
}

e3Bclose(q)
register queue_t *q; 
{
	int bd = (int)q->q_ptr;
	struct e3Bdevice *device = &e3Bdev[bd];
	int x;

	x = splstr();

	flushq(WR(q), 1);
	q->q_ptr = NULL;

	device->up_queue = 0;
	device->open = FALSE;

	e3Bhwclose(bd);
	e3Bmcset(bd, MCOFF);
	device->mccnt = 0;

	splx(x);
}

e3Buwput(q, mp)
register queue_t *q;
register mblk_t *mp; 
{
	STRLOG(ENETM_ID, 0, 9, SL_TRACE, "e3Buwput: type=0x%x", mp->b_datap->db_type);
	switch (mp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		e3Bmacproto(q, mp);
	return;

	case M_DATA:
		e3Bdata(q, mp);
	return;

	case M_IOCTL:
		e3Bioctl(q, mp);
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
		cmn_err(CE_WARN, "e3Buwput: unknown STR msg type %x received mp = %x\n",
			mp->b_datap->db_type, mp);
		freemsg(mp);
		return;
	}
}

e3Bdequeue(q)
register queue_t *q; 
{
	register unit = (int)q->q_ptr;

	while (q->q_first) {
		if (!e3Boktoput(unit))
			return;
		e3Bhwput(unit, getq(q));
	}
}

e3Binit()
{
	extern char *eaddrstr();
	int i;
	int boottmp;
	char *tp;
	extern void e3Bintr();
#ifdef UNIXWARE
	struct cm_args cm_args;
	cm_range_t range;
	cm_num_t cm_num;
	int ret;
	char cable_str[64];
	uint_t unit;

	e3B_nunit = cm_getnbrd("e3B");

	if (e3B_nunit == 0) {
		cmn_err(CE_WARN, "e3Binit: can't find e3B in resmgr");
		return(ENODEV);
	}

	if ((e3Bcookie = (void **)kmem_zalloc
		((sizeof(void **) * e3B_nunit), KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN, "e3Binit: no memory for e3Bcookie");
		return(ENOMEM);
	}
#endif

	if ((e3Bdev = (struct e3Bdevice *)kmem_zalloc
		((sizeof(struct e3Bdevice) * e3B_nunit), KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN, "e3Binit: no memory for e3Bdev");
		kmem_free((void *)e3Bcookie, (sizeof(void **) * e3B_nunit));
		return(ENOMEM);
	}

	for (i = 0; i < e3B_nunit; i++) {
		e3Bdev[i].present = 0;
#ifdef UNIXWARE
		cm_args.cm_key = cm_getbrdkey("e3B", i);

		if (mdi_get_unit(cm_args.cm_key, &unit) == B_FALSE) {
			e3Bdev[i].unit = (uint_t) -1;
			continue;
		}
		e3Bdev[i].unit = unit;

		cm_args.cm_n = 0;
		cm_args.cm_param = CM_IOADDR;
		cm_args.cm_val = &range;
		cm_args.cm_vallen = sizeof(struct cm_addr_rng);
		if (ret = cm_getval(&cm_args)) {
			if (ret != ENOENT)
				cmn_err(CE_CONT, "e3Binit: cm_getval(IOADDR) failed\n");
			continue;
		} else {
			e3Bdev[i].base = range.startaddr;
		}

		cm_args.cm_n = 0;
		cm_args.cm_param = CM_IRQ;
		cm_args.cm_val = &cm_num;
		cm_args.cm_vallen = sizeof(cm_num_t);
		if (ret = cm_getval(&cm_args)) {
			if (ret != ENOENT)
				cmn_err(CE_CONT, "e3Binit: cm_getval(IRQ) failed\n");
			continue;
		} else {
			e3Bdev[i].irq = (cm_num == 2) ? 9 : cm_num;
		}

		cm_args.cm_n = 0;
		cm_args.cm_param = "CABLE_TYPE";
		cm_args.cm_val = cable_str;
		cm_args.cm_vallen = sizeof(cable_str);
		if (ret = cm_getval(&cm_args)) {
			if (ret != ENOENT)
				cmn_err(CE_CONT, "e3Binit: cm_getval(CABLE_TYPE) failed\n");
			continue;
		} else {
			switch(cable_str[0]) {
				case '0':
					e3Bdev[i].xcvr = 0;
					break;
				case '1':
					e3Bdev[i].xcvr = 1;
					break;
				default:
					cmn_err (CE_CONT,
						"e3Binit: board %d has unknown CABLE_TYPE %s in resmgr\n",
						i, cable_str);
					continue;
			}
		}
#else
		/* set defaults from boot line if they're available */
		if ((boottmp = mdi_netboot_info("e3B", i, "io")) > 0) {
		    e3Bdev[i].base = (unsigned int)boottmp;
			if ((boottmp = mdi_netboot_info("e3B", i, "irq")) > 0)
				e3Bdev[i].irq = (unsigned int)boottmp;
			if ((boottmp = mdi_netboot_info("e3B", i, "phy")) > 0) {
				if (boottmp == MDI_MEDIA_AUI)
					e3Bdev[i].xcvr = 1L;
				else
					e3Bdev[i].xcvr = 0L;
			}
		}
#endif /* UNIXWARE */

		if (e3Bpresent(e3Bdev[i].base, e3Bdev[i].eaddr)) {
#ifdef UNIXWARE
			if (cm_intr_attach(cm_args.cm_key, e3Bintr, &e3Bdevflag, &e3Bcookie[i]) == 0) {
				mdi_printcfg("e3B", e3Bdev[i].base, EDEVSIZ - 1,
					-1, -1, "UNABLE TO ADD INTR HANDLER AT RESMGR KEY %d", 
					cm_args.cm_key);
				continue;
			}
			mdi_printcfg("e3B", e3Bdev[i].base, EDEVSIZ-1, e3Bdev[i].irq,
				-1, "type=3c503 addr=%s", eaddrstr(e3Bdev[i].eaddr));
		} else {
			mdi_printcfg("e3B", e3Bdev[i].base, EDEVSIZ-1, -1, -1, "ADAPTER NOT FOUND");
			continue;
		}
#else
			if (add_intr_handler(1, e3Bdev[i].irq, e3Bintr, 5) != 0) {
				printcfg("e3B", e3Bdev[i].base, EDEVSIZ - 1,
					-1, -1, "UNABLE TO ADD INTR HANDLER (%d)", e3Bdev[i].eaddr);
				continue;
			}
			printcfg("e3B", e3Bdev[i].base, EDEVSIZ-1, e3Bintl[i],
				-1, "type=%s addr=%s", "3c503", eaddrstr(e3Bdev[i].eaddr));
		} else if (!mdi_netboot_quiet("e3B", i))
			printcfg("e3B", e3Bdev[i].base, EDEVSIZ-1, -1, -1, "ADAPTER NOT FOUND");
#endif
		e3Bdev[i].present = 1;
	}
	return(0);
}

/*
 * real init
 */
STATIC
e3BINIT(unit)
register unit;
{
	int i;
	int x;
	mac_stats_eth_t *macstats =	&(e3Bdev[unit].macstats);
	int t;

	x = splstr();

	if ((t = e3Bhwinit(unit)) == 0) {
		cmn_err(CE_WARN, "No board for unit %d\n", unit);
		splx(x);
		return(1);
	}
	splx(x);
	return(0);
}

/*
 * The following four routines implement MDI interface 
 */
e3Bmacproto(q, mp)
queue_t *q;
mblk_t *mp; 
{
	int			bd	= (int)q->q_ptr;
	struct e3Bdevice	*device	= &e3Bdev[bd];
	union MAC_primitives	*prim = (union MAC_primitives *) mp->b_rptr;
	mblk_t			*mp1, *mp2;
	mac_info_ack_t		*info_ack;
	mac_ok_ack_t		*ok_ack;
	int 			s;
	mac_stats_eth_t		*e3Bmacstats = &(e3Bdev[bd].macstats);

	STRLOG(ENETM_ID, 0, 9, SL_TRACE, "e3Bmacproto: prim=0x%x", prim->mac_primitive);

	switch(prim->mac_primitive) {
	case MAC_INFO_REQ:
		if ((mp1 = allocb(sizeof(mac_info_ack_t), BPRI_HI)) == NULL) {
			cmn_err(CE_WARN, "e3Bmacproto - Out of STREAMs");
			freemsg(mp);
			return;
		}
		info_ack = (mac_info_ack_t *) mp1->b_rptr;
		mp1->b_wptr += sizeof(mac_info_ack_t);
		mp1->b_datap->db_type = M_PCPROTO;
		info_ack->mac_primitive = MAC_INFO_ACK;
		info_ack->mac_max_sdu = E3BETHERMTU;
		info_ack->mac_min_sdu = 14;
		info_ack->mac_mac_type = MAC_CSMACD;
		info_ack->mac_driver_version = MDI_VERSION;
		info_ack->mac_if_speed = 10000000;
		freemsg(mp);
		qreply(q, mp1);
		break;

	case MAC_BIND_REQ:
		if (device->up_queue) {
			mdi_macerrorack(RD(q),prim->mac_primitive,MAC_OUTSTATE);
			freemsg(mp);
			return;
		}
		device->up_queue = RD(q);
		device->dlpi_cookie = prim->bind_req.mac_cookie;
		mdi_macokack(RD(q), prim->mac_primitive);
		freemsg(mp);
		break;
	default:
		freemsg(mp);
		mdi_macerrorack(RD(q), prim->mac_primitive, MAC_BADPRIM);
		break;
	}
}

STATIC
e3Bdata(q,mp)
queue_t *q;
mblk_t *mp;
{
	int			bd	= (int)q->q_ptr;
	struct e3Bdevice	*device	= &e3Bdev[bd];
	int 			s;
	mac_stats_eth_t		*e3Bmacstats = &(e3Bdev[bd].macstats);
	int			do_loop;

	if (!device->up_queue) {
		mdi_macerrorack(RD(q), M_DATA, MAC_OUTSTATE);
		freemsg(mp);
		return;
	}


	if (*mp->b_rptr & 0x01) {		/* Multicast/B'cast */
		do_loop = !e3Bchktbl(mp->b_rptr, bd);
	} else {
		do_loop = !e3Bstrncmp(device->eaddr, mp->b_rptr, E3COM_ADDR);
	}

	if (do_loop)
	    mdi_do_loopback(q, mp, E3COM_MINPACK);

	s = splstr();
	if (q->q_first || !e3Boktoput(bd))
		putq(q, mp);
	else 
		e3Bhwput(bd, mp);
	splx(s);
}

static
e3Bioctl(q, mp)
queue_t *q;
mblk_t *mp; 
{
	int			unit = (int) q->q_ptr;
	struct e3Bdevice	*device = &e3Bdev[unit];
	struct iocblk		*iocp = (struct iocblk *) mp->b_rptr;
	unsigned char *data;
	int i;
	int j;
	unsigned char *cp;
	unsigned int x;

	data = (mp->b_cont) ? mp->b_cont->b_rptr : NULL;

	switch (iocp->ioc_cmd) {
	case MACIOC_GETSTAT:	/* dump mac or dod statistics */
		if (data == NULL) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}
		if (iocp->ioc_count != sizeof(mac_stats_eth_t)) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}
		bcopy(&device->macstats, data, sizeof(mac_stats_eth_t));
		mp->b_cont->b_wptr = mp->b_cont->b_rptr
			+ sizeof(mac_stats_eth_t);
		goto e3Bioctl_ack;

	case MACIOC_CLRSTAT:	/* clear mac statistics */
		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			goto e3Bioctl_nak;
		}

		x = splstr();

		bzero(&device->macstats, sizeof(mac_stats_eth_t));

		iocp->ioc_count = 0;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
		splx(x);
		goto e3Bioctl_ack;

	case MACIOC_GETRADDR:	/* get factory mac address - same for now */
	case MACIOC_GETADDR:	/* get mac address */
		if (data == NULL || iocp->ioc_count != E3COM_ADDR) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}
		bcopy(device->eaddr, data, E3COM_ADDR);
		mp->b_cont->b_wptr = mp->b_cont->b_rptr + E3COM_ADDR;
		goto e3Bioctl_ack;

	case MACIOC_SETALLMCA:	/* enable all multicast addresses */
		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			goto e3Bioctl_nak;
		}

		if (device->flags & E3BALLMCA || device->mccnt != 0) {
			goto e3Bioctl_ack;
		}

		x = splstr();
		device->flags |= E3BALLMCA;
		e3Bmcset(unit, MCON);
		splx(x);
		goto e3Bioctl_ack;

	case MACIOC_DELALLMCA:	/* disable all multicast addresses */
		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			goto e3Bioctl_nak;
		}

		if (!(device->flags & E3BALLMCA)) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}

		device->flags &= ~E3BALLMCA;
		if (device->mccnt <= 0) {
			x = splstr();
			e3Bmcset(unit, MCOFF);
			splx(x);
		}
		goto e3Bioctl_ack;

	case MACIOC_SETMCA:	/* multicast setup */
		if (data == NULL) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}

		if (iocp->ioc_count != E3COM_ADDR) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}

		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			goto e3Bioctl_nak;
		}

		if (!(*data & 0x01)) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}

		if (mdi_valid_mca(device->dlpi_cookie, data)) {
			goto e3Bioctl_ack;
		}

		x = splstr();
		if (device->mccnt == 0)
			e3Bmcset(unit, MCON);
		device->mccnt++;
		splx(x);

		goto e3Bioctl_ack;

	case MACIOC_DELMCA:	/* multicast delete */
		if (data == NULL) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}

		if (iocp->ioc_count != E3COM_ADDR) {
			iocp->ioc_error = EINVAL;
			goto e3Bioctl_nak;
		}

		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			goto e3Bioctl_nak;
		}

		if (! mdi_valid_mca(device->dlpi_cookie, data)) {
			iocp->ioc_error = ENOENT;
			goto e3Bioctl_nak;
		}

		x = splstr();
		--device->mccnt;
		if (device->mccnt == 0)
			e3Bmcset(unit, MCOFF);
		splx(x);

		goto e3Bioctl_ack;

	default:
		iocp->ioc_error = EINVAL;
		goto e3Bioctl_nak;
	}

e3Bioctl_ack:
	mp->b_datap->db_type = M_IOCACK;
	qreply(q, mp);
	return;

e3Bioctl_nak:
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_count = 0;
	if (mp->b_cont) {
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
	}
	qreply(q, mp);
	return;
}

static char*
eaddrstr(ea)
unsigned char ea[6];
{  
	static char buf[18];
	char* s = "0123456789abcdef";
	int i;

	for (i=0; i<6; ++i) {
		buf[i*3] = s[(ea[i] >> 4) & 0xf];
		buf[i*3 + 1] = s[ea[i] & 0xf];
		buf[i*3 + 2] = ':';
	}
	buf[17] = 0;
	return buf;
}

int
e3Bstrncmp(s1,s2,n)
register char *s1,*s2;
register int n;
{
	++n;
	while (--n > 0 && *s1++ == *s2++);
	return (n);
}
