#ident	"@(#)kern-i386at:io/hba/blc/blc.cf/Space.c	1.4.2.1"
#ident	"$Header$"

/*
 *	Unixware scsi host adapter device driver
 *	budi -blc Ver. 3.01	3/28/1997
 *	Copyright (c) Mylex Corp.
 */

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include "config.h"

int	 blc_gtol[MAX_EXHAS];	/* map global hba # to local # */
int	 blc_ltog[MAX_EXHAS];	/* map local hba # to global # */

/*
**  blc_hba_max is the maximum jobs that can be run concurrently
**  on the HBA.  The hardware supports much more, however, due
**  to the large size of the ucb structure, we can only fit 44
**  onto a single page, which is required to avoid corruption
**  from non-contiguous DMA.
*/
int blc_hba_max = 44;

/*
**  blc_lu_max is the maximum jobs per LUN concurrently.  It is
**  critical that this number be less than or equal to
**
**	((blc_hba_max / Number_Devices) - 1)
**
**  If the value is greater than or equal to this, a hang can
**  result on any given LUN.
*/
int blc_lu_max = 6;

/*
**  blc_lu_max_sched is the maximum scheduled jobs per LUN
**  concurrently.
*/
int blc_lu_max_sched = 2;

/*
**  blc_dma_max is the maximum number of concurrent DMA requests
**  per host adapter.
*/
int blc_dma_max = 15;

/*
**  blc_hard_reset = 1 will generate scsi reset during card initializatin.
**                 = 0 will not generate scsi reset.
**  For multi initiator application, set blc_hard_reset = 0.
*/
int blc_hard_reset = 1;

/*
**  blc_do_tag value of 1 enables command queueing
**  on devices that support it.  0 disables command queueing.
*/
int blc_do_tag = 1;

/*
**  blc_do_sched value of 1 enables job scheduling
**  in driver.  0 disables job scheduling.
*/
int blc_do_sched = 0;

int	blc_timeout_period 	= 15;	/* number of secs to wait for I/O */
					/*   request before timing out.   */
					/*   0 disables cmd timeout.      */

int	blc_retry_max	= 2;	/* number of times to retry	  */
					/*   request before resetting	  */
					/*   controller			  */

struct	ver_no    blc_sdi_ver;

/* BLC_CNTLS is not set if autoconfiguration is supported.
 * The driver will set it by calling sdi_hba_autoconf().
 */
#ifndef BLC_CNTLS
#define BLC_CNTLS 1
#endif

#ifdef	BLC_CPUBIND

#define	BLC_VER		HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	blc_extinfo = {
	0, BLC_CPUBIND
};

#else

#define	BLC_VER		HBA_UW21_IDATA

#endif	/* BLC_CPUBIND */

struct	hba_idata_v5	_blcidata[]	= {
	{ BLC_VER, "(blc,1st) BLC SCSI",
	  7, 0, -1, 0, -1, 0 }
#ifdef BLC_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&blc_extinfo }
#endif
};

int	blc_cntls	= BLC_CNTLS;

