#ident "@(#)mdilib.c	28.1"
#ident "$Header$"

/*
 *	  Copyright (C) The Santa Cruz Operation, 1993-1997.
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 */

#ifdef  _KERNEL_HEADERS
#include <io/nd/dlpi/include.h>
#else
#include "include.h"
#endif


/*
 * Function: mdi_addrs_equal
 *
 * Purpose:
 *   Compare the two given mac addresses.
 *
 * Returns:
 *   TRUE if they are equal, FALSE otherwise.
 */
int
mdi_addrs_equal(macaddr_t addr1, macaddr_t addr2)
{
	register int n = sizeof(macaddr_t);

	while (n) {
		if (*addr1++ != *addr2++)
			break;
		n--;
	}
	return(n ? FALSE : TRUE);
}


/*
 * Sends multicast address state to mdi driver.
 */
mblk_t *
mdi_get_mctable(void *cp)
{
	mac_mcast_t	*mcast;
	card_mca_t	*cm;
	mblk_t		*mp;
	ulong		cnt;
	pl_t		s;

	/* block out add/delete MCAs */
	if (((per_card_info_t *)cp)->dlpi_iocblk != (mblk_t *)0xBF) {
		/* dlpi_iocblk set to 0xBF from dlpiclose as part of 
		 * sap close multicasting cleanup. In addition, if there are no other 
		 * consumers of sap then dlpiclose also does a LOCK_DEALLOC on the 
		 * RDLOCK so we can't call RDLOCK_MCA since lock itself doesn't 
		 * exist any more.  The real reason is that if dlpi_iocblk is set to
		 * 0xBF then we're still holding the write lock from 
		 * dlpi_clear_sap_multicasts and the MDI driver is calling this 
		 * routine.
		 */
		s = RDLOCK_MCA((per_card_info_t *)cp);
	} else {
		/* write lock still held from dlpi_clear_sap_multicasts */
	}

	/* dlpi_mca_count expects some sort of lock (RDLOCK or WRLOCK) to be 
	 * held upon entry:
	 *  readlock=set here
	 *  writelock=set in dlpi_clear_sap_multicasts and we're called from 
	 *            within MDI driver
	 */
	cnt = dlpi_mca_count((per_card_info_t *)cp, (per_sap_info_t *)0);

	DLPI_PRINTF400(("mpi_get_mctable(%x) cnt %d\n", cp, cnt));

	mp = allocb(sizeof(mac_mcast_t)+(cnt*sizeof(macaddr_t)), BPRI_MED);
	if (! mp) {
		if (((per_card_info_t *)cp)->dlpi_iocblk != (mblk_t *)0xBF) {
			UNLOCK_MCA((per_card_info_t *)cp, s);
		}
		return(0);
	}
	dlpi_get_mctable((per_card_info_t *)cp, (per_sap_info_t *)0, mp->b_wptr + sizeof(mac_mcast_t), cnt);
	if (((per_card_info_t *)cp)->dlpi_iocblk != (mblk_t *)0xBF) {
		UNLOCK_MCA((per_card_info_t *)cp, s);
	}
	mcast = (mac_mcast_t *)mp->b_rptr;
	mcast->mac_all_mca = ((per_card_info_t *)cp)->all_mca;
	mcast->mac_mca_count = cnt;
	mcast->mac_mca_length = cnt * sizeof(macaddr_t);
	mcast->mac_mca_offset = sizeof(mac_mcast_t);
	mp->b_wptr += sizeof(mac_mcast_t) + mcast->mac_mca_count;
	return(mp);
}


/*
 * Function: mdi_valid_mca
 *
 * Purpose:
 *   Verify that a given multicast address has been set on this adapter.
 *
 * Returns:
 *   TRUE if multicast address is valid for this adapter, FALSE otherwise.
 */
int
mdi_valid_mca(void *cp, macaddr_t addr)
{
	uint hsh;
	pl_t s;
	int ret;

	if (cp == (void *)0)	/* shouldn't happen - mdi_unit test hook */
		return(FALSE);

	if (((per_card_info_t *)cp)->dlpi_iocblk == (mblk_t *)0xBF) {
		/* called from sap close multicasting cleanup */
		return(TRUE);
	}

	hsh = dlpi_hsh(addr, sizeof(macaddr_t));
	DLPI_PRINTF400(("mdi_valid_mca(%x, %b:%b:%b:%b:%b:%b) hsh %x tbl %x\n",
		cp, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
		hsh, ((per_card_info_t *)cp)->mcahashtbl[hsh]));
	s = RDLOCK_MCA((per_card_info_t *)cp);
	ret = ((per_card_info_t *)cp)->mcahashtbl[hsh] ? TRUE : FALSE;
	UNLOCK_MCA((per_card_info_t *)cp, s);
	return(ret);
}

/*
 * Function: mdi_do_loopback
 *
 * Purpose:
 *   Loop the given message back to the other queue.
 *   Calls pullupmsg to collate the data into a single 
 *   STREAMS message.
 */
void
mdi_do_loopback(queue_t * q, mblk_t * mp, int min_packet_size)
{
	mblk_t *mp1, *tmp_mp;
	int	len;

	if (canputnext(RD(q)) && (mp1 = dupmsg(mp))) {
		if ((len = msgdsize(mp1)) < min_packet_size) {
			mblk_t *	mp2;
			int		padlen = min_packet_size - len;

			mp2 = allocb(padlen, BPRI_MED);
			if (mp2) {
				mp2->b_wptr += padlen;
				bzero(mp2->b_rptr, padlen);
				linkb(mp1, mp2);
			}
		}
		tmp_mp = mp1;
		mp1 = msgpullup(mp1, msgdsize(mp1));
		freemsg(tmp_mp);
		if (mp1 != NULL) {
			putnext(RD(q), mp1);
		}
	}
}

/* mdi_get_unit solves the problem when you have multiple smart-bus boards 
 * (PCI, EISA, MCA) of the * same type, cm_getnbrd will return a value higher 
 * than has been configured, and then user starts pulling out boards.
 * The driver needs to know if this particular instance has been configured or
 * not and (for ddi7) the associated minor number that it should use,
 * otherwise frames end up going out the wrong interface or driver refuses to
 * open the device
 * this routine retrieves the CM_NDCFG_UNIT parameter that ndcfg set in the 
 * resmgr.  ndcfg saves this as a number so we don't need to call dlpi_atoi.
 * While tempting, we can't use CM_UNIT as this is driver dependant parameter.
 * ddi<8:  corresponds to numboards at the time the board was installed
 * ddi8:   corresponds to .INSTNUM parameter at the time board was installed
 *         which corresponds to the %i in the device name in the Node(4) file
 *         Depends on .INSTNUM not changing at key after it's assigned.
 *         unit argument should be NULL for ddi8 drivers.
 * returns B_TRUE or B_FALSE, indicating if this board instance has been 
 * previously configured with netcfg/ndcfg
 */

/* from ndcfg's common.h */
#define CM_DEV_NAME "DEV_NAME"
#define CM_NDCFG_UNIT "NDCFG_UNIT"

boolean_t
mdi_get_unit(rm_key_t rmkey, uint_t *unit)
{
	cm_num_t tmpunit;
	cm_args_t cma;
	char devname[20];

	cma.cm_key = rmkey;
	cma.cm_param = CM_NDCFG_UNIT;
	cma.cm_val = &tmpunit;
	cma.cm_vallen = sizeof(cm_num_t);
	cma.cm_n = 0;

	if (cm_getval(&cma)) {
		/* NDCFG_UNIT parameter not set.  normal for ddi8 drivers 
		 * because the moment ndcfg adds MODNAME our CFG_ADD code 
		 * runs which calls this routine.  However, could be a 
		 * bogus ddi8 driver too.  Check for DEV_NAME (added by 
		 * ndcfg just before MODNAME) to tell the difference
		 * between the two
		 */
		cma.cm_key = rmkey;
		cma.cm_param = CM_DEV_NAME;
		cma.cm_val = devname;
		cma.cm_vallen = sizeof(devname);
		cma.cm_n = 0;

		if (cm_getval(&cma)) {
			/* both NDCFG_UNIT and DEV_NAME not found in resmgr
			 * this resmgr key has not been configured by ndcfg
			 * could be either ddi7 or 8 driver to get here
			 */
			return(B_FALSE);
		} else {
			/* ddi8 driver where DEV_NAME is set but NDCFG_UNIT 
			 * not added yet.  This driver has been configured 
			 * with ndcfg, let it through.  We could read 
			 * .INSTNUM directly from resmgr since 
			 * _cm_apply_instnums sets it prior to calling our 
			 * driver but this could be made an illegal cm_getval 
			 * parameter.  Note that ddi8 drivers should set 
			 * the unit argument to NULL so they won't be 
			 * expecting to use it(since we don't read 
			 * .INSTNUM here).
			 */
			return(B_TRUE);
		}
	}

	/* could be ddi7 or ddi8.  ddi8 drivers should set unit to NULL */
	if (unit != (uint_t *)NULL) {
		*unit = tmpunit;
	}

	return(B_TRUE);
}
