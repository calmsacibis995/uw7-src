#ident "@(#)Nonconform.c	29.1"
#ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#define _DDI_C

#include <util/types.h>
#include <util/param.h>
#include <util/mod/moddefs.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <mem/immu.h>
#include <mem/vmparam.h>
#include <proc/cg.h>
#include <io/conf.h>
#include <io/ddi.h>


#define DRVNAME "dlpibase - nonddi conforming routines"
static int dlpibase_load(), dlpibase_unload();
MOD_MISC_WRAPPER(dlpibase, dlpibase_load, dlpibase_unload, DRVNAME);

/* ndcfg information */
static char _ndversion[]="29.1";

static int
dlpibase_load()
{
	return(0);
}

static int
dlpibase_unload()
{
	return(0);
}

/*
 * Function : mdi_end_of_contig_segment
 *
 * Purpose :
 *   Given a range of virtual address space, return the last address
 *   that is physically contiguous with the starting address. 
 *
 * Algorithm :
 *   Check each page boundary in the range.  If there is discontiguity,
 *   return the last address in the previous page.
 *
 * Note:  This routine will only work for compatibility drivers (< DDI8)
 *        and will panic if physical is above 4G (pre DDI8 drivers can't
 *        handle physical > 4GB)
 *
 * Enabling P6 PAE mode (enable_4gb_mem=y boot option) -- only takes effect on 
 * an MP system -- sets enable_4gb_mem and using_pae kernel variables which 
 * was causing panic when routine used kvtophys macro.
 */
u_char *
mdi_end_of_contig_segment(u_char *vstart, u_char *vend)
{
	u_char *first_page, *last_page, *scan_page;

	first_page = (u_char *) (((unsigned int) vstart) / NBPP * NBPP + NBPP);
	last_page  = (u_char *) (((unsigned int) vend) / NBPP * NBPP);

	for (scan_page = first_page; scan_page <= last_page; scan_page += NBPP)
		if ((vtop((caddr_t) scan_page, NULL) - 
			vtop((caddr_t) scan_page - 1, NULL)) != (paddr_t) 1)
			return (scan_page - 1);
	return (vend);
}

#if 0
/* successor to mdi_end_of_contig_segment that works for both ddi7 and ddi8.  
 * originally thought not usable because LOCK_CFGLIST called by find_cfgp_key 
 * obtains a spin lock at PLDDI=PLSTR=PL6=6.  Since we can be calling 
 * mdi_end_of_contig_segment_new from interrupt context (but not STREAMS 
 * wput/srv context any more since LOCK_CFGLIST fixes that) we may already 
 * have this lock held: deadlock, oops.  However, all MDI drivers have
 * 6 for IPL in their System file so calling from interrupt context safe too.  
 * We assume the driver's System file says 6 for IPL...
 *
 * not relevant for ddi8 drivers as they should be using msgscgth(D3str) or
 * scgth(D4) so we #if 0 the whole thing for now.
 */
u_char *
mdi_end_of_contig_segment_new(rm_key_t rmkey, u_char *vstart, u_char *vend)
{
	u_char *first_page, *last_page, *scan_page;
	cfg_info_t *cfgp;

	if ((cfgp=find_cfgp_key(rmkey)) == NULL) {
		/*
		 *+ Kernel could not find configuration information for the resmgr key
		 *+ The MDI driver should be fixed to use the proper resmgr key.
		 */
		cmn_err(CE_PANIC,"mdi_end_of_contig_segment_new: no cfgp for rmkey %d",
			rmkey);
		/* NOTREACHED */
	}

	first_page = (u_char *) (((unsigned int) vstart) / NBPP * NBPP + NBPP);
	last_page  = (u_char *) (((unsigned int) vend) / NBPP * NBPP);

	/* vtop/vtop64 takes care of PAE_ENABLED() */
	if (CFG_PRE_DDI8(cfgp)) {
		/* vtop will panic if physical > 4G as driver couldn't handle it */
		for (scan_page = first_page; scan_page <= last_page; scan_page += NBPP) {
			if ((vtop((caddr_t) scan_page, NULL) - 
				vtop((caddr_t) (scan_page - 1), NULL)) != (paddr_t) 1) {
				return (scan_page - 1);
			}
		}
	} else {
		/* ddi8 driver can handle physical above 4G, call vtop64 */
		for (scan_page = first_page; scan_page <= last_page; scan_page += NBPP) {
			if ((vtop64((caddr_t) scan_page, NULL) - 
				vtop64((caddr_t) (scan_page - 1), NULL)) != (paddr64_t) 1) {
				return (scan_page - 1);
			}
		}
	}
	return (vend);
}
#endif
