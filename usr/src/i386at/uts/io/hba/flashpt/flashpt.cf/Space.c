#ident	"@(#)kern-i386at:io/hba/flashpt/flashpt.cf/Space.c	1.1.1.1"
#ident	"$Header$"

/*
 *	Unixware FlashPoint scsi host adapter device driver
 *	budi -flashpt Ver. 3.01	3/28/1997
 *	Copyright (c) Mylex Corp.
 */

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include "config.h"

int	 flashpt_gtol[MAX_EXHAS];	/* map global hba # to local # */
int	 flashpt_ltog[MAX_EXHAS];	/* map local hba # to global # */

/*
**  flashpt_hba_max is the maximum jobs that can be run concurrently
**  on the HBA.  The hardware supports much more, however, due
**  to the large size of the ucb structure, we can only fit 44
**  onto a single page, which is required to avoid corruption
**  from non-contiguous DMA.
*/
int flashpt_hba_max = 44;

/*
**  flashpt_lu_max is the maximum jobs per LUN concurrently.  It is
**  critical that this number be less than or equal to
**
**	((flashpt_hba_max / Number_Devices) - 1)
**
**  If the value is greater than or equal to this, a hang can
**  result on any given LUN.
*/
int flashpt_lu_max = 6;

/*
**  flashpt_lu_max_sched is the maximum scheduled jobs per LUN
**  concurrently.
*/
int flashpt_lu_max_sched = 2;

/*
**  flashpt_dma_max is the maximum number of concurrent DMA requests
**  per host adapter.
*/
int flashpt_dma_max = 15;

/*
**  flashpt_hard_reset = 1 will generate scsi reset during card init.
**                     = 0 will not generate scsi reset.
**  For multi initiator application, set flashpt_hard_reset = 0.
*/
int flashpt_hard_reset = 1;

/*
/*
**  flashpt_do_tag value of 1 enables command queueing
**  on devices that support it.  0 disables command queueing.
*/
int flashpt_do_tag = 1;

/*
**  flashpt_do_sched value of 1 enables job scheduling
**  in driver.  0 disables job scheduling.
*/
int flashpt_do_sched = 0;

int	flashpt_timeout_period 	= 15;	/* number of secs to wait for I/O */
					/*   request before timing out.   */
					/*   0 disables cmd timeout.      */

int	flashpt_retry_max	= 2;	/* number of times to retry	  */
					/*   request before resetting	  */
					/*   controller			  */

struct	ver_no    flashpt_sdi_ver;

/* FLASHPT_CNTLS is not set if autoconfiguration is supported.
 * The driver will set it by calling sdi_hba_autoconf().
 */
#ifndef FLASHPT_CNTLS
#define FLASHPT_CNTLS 1
#endif

#ifdef	FLASHPT_CPUBIND

#define	FLASHPT_VER		HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	flashpt_extinfo = {
	0, FLASHPT_CPUBIND
};

#else

#define	FLASHPT_VER		HBA_UW21_IDATA

#endif	/* FLASHPT_CPUBIND */

struct	hba_idata_v5	_flashptidata[]	= {
	{ FLASHPT_VER, "(flashpt,1st) FLASHPT SCSI",
	  7, 0, -1, 0, -1, 0 }
#ifdef FLASHPT_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&flashpt_extinfo }
#endif
};

int	flashpt_cntls	= FLASHPT_CNTLS;

