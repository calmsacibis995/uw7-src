#ident "@(#)bind.c	25.1"
#ident "$Header$"

/*
 *	  Copyright (C) The Santa Cruz Operation, 1993-1996.
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>

#else

#include "include.h"

#endif


/******************************************************************************
 *
 * Generic BIND Processing Code.
 *
 ******************************************************************************/

/*
 * Function: dlpi_dobind
 *
 * Purpose:
 *   Determine the framing type to use based on sap and framing
 *   types supported by the media.
 *
 * Description of Logic:
 *   If SNAP is supported, and the sap is the SNAP sap,
 *   then this bind will use SNAP framing type.  We don't mark the
 *   connection as bound until the subsequent bind to the SNAP id.
 *
 *   If XNS is supported and the sap is the XNS sap,
 *   then use XNS frames.
 *
 *   If LLC is supported and the sap is the default
 *   LLC sap, and llcmode is OFF, then use LLC.
 *
 *   If ETHERNET II is supported, the sap is in the range
 *   1501...65535, and ETHERNET II isn't disabled for this
 *   netX instance, use ETHERNET II frames.
 *
 *   On ODT TBird, TCP/IP uses SNAP frames with sap 0x800 and 0x806,
 *   with llcmode off.  If these frames are supported, then use LLC
 *   frames.
 *
 *   If LLC supported and the sap is even numbered in the range
 *   0x02..0xFE, and llcmode is either off or LLC_1, then use
 *   LLC frames, and keep xidtest_flg on.
 */

int
dlpi_dobind(per_sap_info_t *sp, ulong sap, ulong llcmode, ulong xidtestflg)
{
	int flags;
	int s;

	DLPI_PRINTF20(("dlpi_dobind: sp %x, card_info %x media_specific %x\n", sp, sp->card_info, sp->card_info->media_specific));
	if (!sp || !sp->card_info || !sp->card_info->media_specific)
		return 0;

	flags = sp->card_info->media_specific->media_flags;
	DLPI_PRINTF20(("dlpi_dobind: flags %x sap %x llcmode %x\n", flags, sap, llcmode));

	if (llcmode & DL_CLDLS)
		llcmode = LLC_1;
	else
		llcmode = 0;

	if ((flags & MEDIA_SNAP) && sap == SNAP_SAP) {
		sp->sap_type = FR_SNAP;
		sp->llcmode = LLC_SNAP;
		sp->llcxidtest_flg = 0;
		/* SAP is marked as bound when the DL_SUBS_BIND_REQ is issued */
		return(1);				/* SUCCESS */
	}
	if ((flags & MEDIA_XNS) && sap == XNS_SAP && llcmode == 0) {
		sp->sap_type = FR_XNS;
		sp->llcxidtest_flg = 0;
		sap = 0;
		goto unique_ok;
	}
	if ((flags & MEDIA_LLC) && sap == LLC_DEFAULT_SAP && llcmode == 0) {
		sp->sap_type = FR_LLC;
		sp->llcxidtest_flg = 0;
		goto unique_ok;
	}
	if ((flags & MEDIA_ETHERNET_II) && sap > 1500 && sap <= 0xffff &&
		llcmode == 0) {
		sp->sap_type = sp->card_info->disab_ether ? FR_LLC : FR_ETHER_II;
		sp->llcxidtest_flg = 0;
		goto unique_ok;
	}
	/* UNIXWARE_TCP: allow TCP/IP binds with dl_service_mode == DL_CLDLS */
	if ((flags & MEDIA_ETHERNET_II) && sap > 1500 && sap <= 0xffff &&
		llcmode == DL_CLDLS) {
		sp->sap_type = sp->card_info->disab_ether ? FR_LLC : FR_ETHER_II;
		llcmode = 0;
		sp->llcxidtest_flg = 0;
		goto unique_ok;
	}
#ifdef ODT_TBIRD
	if ((flags & MEDIA_SNAP) 
		&& (sap==0x800 || sap==0x806) && llcmode == 0) {
		sp->sap_type = FR_SNAP;
		llcmode = LLC_SNAP;
		sp->llcxidtest_flg = 0;
		sp->sap_protid = 0;
		goto unique_ok;
	}
#endif
	if ((flags & MEDIA_LLC) &&
		sap >= 0x02 && sap <= 0xfe && !(sap & 0x01) &&
		(llcmode == 0 || llcmode == LLC_1)) {
		sp->sap_type = FR_LLC;
		sp->llcxidtest_flg = xidtestflg;
		goto unique_ok;
	}
	return(0);					/* FAIL */

unique_ok:
	sp->sap = sap;
	sp->llcmode = llcmode;
	/* Check that no-one is already bound to this SAP */
	s = LOCK_SAP_ID(sp->card_info);
	if (dlpi_findsap(sp->sap_type, sp->sap, sp->card_info)) {
		UNLOCK_SAP_ID(sp->card_info, s);
		DLPI_PRINTF20(("dlpi_dobind: sap already bound\n"));
		return(0);						/* FAIL */
	}
	sp->sap_state = SAP_BOUND;
	UNLOCK_SAP_ID(sp->card_info, s);
	return(1);					/* SUCCESS */
}
