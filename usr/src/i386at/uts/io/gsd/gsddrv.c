/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#ident	"@(#)gsddrv.c	1.3"
#ident	"$Header$"

/*
 * gsddrv.c, Graphics System Driver Module driver code.
 */

/*
 *	L000	7/3/97		rodneyh@sco.com
 *	- Remove Novell copyright, add SCO copyright.
 *	  Remove cmn_err sign on message
 *
 */


#include <util/types.h>
#include <io/ldterm/euc.h>
#include <io/ldterm/eucioctl.h>
#include <io/ws/ws.h>
#include <io/gsd/gsd.h>
#include <util/cmn_err.h>

extern wstation_t Kdws;

/* int	channel_ref_count = 0;	/* number of active graphical channels */

/*
 * int gsdinit(void) - called from gsd_load()
 *	Initializes gsd.
 *
 * Calling/Exit status:
 *
 */
int
gsdinit(void)
{

	Kdws.w_consops->cn_gcl_norm = gcl_norm;
	Kdws.w_consops->cn_gcl_handler = gcl_handler;
	Kdws.w_consops->cn_gdv_scrxfer = gdv_scrxfer;
	Kdws.w_consops->cn_gs_chinit = gs_chinit;
	Kdws.w_consops->cn_gs_alloc = gs_alloc;
	Kdws.w_consops->cn_gs_free = gs_free;
	Kdws.w_consops->cn_gs_seteuc = gs_seteuc;
	Kdws.w_consops->cn_gs_ansi_cntl = gs_ansi_cntl;

	gs_init_flg = 1;
}

