/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/lwpres.c	1.2"
#ident "$Header$"


/* procres.c
 *
 * Lwp resource metrics.  Processes SAR_LWP_RESRC_P records. 
 * Data is collected per-processor, but reported system wide.
 * 
 */

#include <stdio.h>
#include <malloc.h>
#include "../sa.h"
#include "sar.h"

static struct metp_lwp_resrc	lwp;
static struct metp_lwp_resrc	old_lwp;
static struct metp_lwp_resrc	acc_lwp;

static int first_sample = TRUE;

static uint_t	num_samples = 0;

flag
sar_lwp_init(void)
{
	memset(&lwp, (char) 0, sizeof(old_lwp));
	old_lwp = lwp;
	acc_lwp = lwp;

	first_sample = TRUE;
	num_samples = 0;

	return(TRUE);
}


/* ARGSUSED */

int
sar_lwp(FILE *infile, sar_header sarh, flag32 of)
{	
	int			i;
	struct metp_lwp_resrc	temp;

	if (sarh.item_size != sizeof(struct lwp_resrc_info)) {
		metric_warn[sarh.item_code] = TRUE;
	}

	old_lwp = lwp;
	memset(&lwp, '\0', sizeof(lwp));
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(temp), sarh.item_size, infile);
		lwp.mpr_lwp[MET_MAX] += temp.mpr_lwp[MET_MAX];
		lwp.mpr_lwp[MET_INUSE] += temp.mpr_lwp[MET_INUSE];
		lwp.mpr_lwp[MET_FAIL] += temp.mpr_lwp[MET_FAIL];
	}

	if (first_sample == FALSE) {
		acc_lwp.mpr_lwp[MET_MAX] += lwp.mpr_lwp[MET_MAX];
		acc_lwp.mpr_lwp[MET_INUSE] += lwp.mpr_lwp[MET_INUSE];
		acc_lwp.mpr_lwp[MET_FAIL] += lwp.mpr_lwp[MET_FAIL] - 
					     old_lwp.mpr_lwp[MET_FAIL];
		num_samples++;
	}
	else {
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;

	return(TRUE);
}
	
		
sarout_t
sar_lwp_out(int column, int mode, int devnum)
{
	if (mode == OUTPUT_DATA || mode == OUTPUT_TOTAL
	                        || mode == OUTPUT_RAW
                                || mode == OUTPUT_RAW_TOTAL) {
		switch (column) {
		      case LWP_MAX:
			return(lwp.mpr_lwp[MET_MAX]);
			break;
		      case LWP_USE:
			return(lwp.mpr_lwp[MET_INUSE]);
			break;
		      case LWP_FAIL:
			return(lwp.mpr_lwp[MET_FAIL] - old_lwp.mpr_lwp[MET_FAIL]);
			break;
		      default:
			sarerrno = SARERR_OUTFIELD;
			return(-1);
			break;
		}
	}
	else if (mode == OUTPUT_FINAL_DATA || mode == OUTPUT_FINAL_TOTAL
					   || mode == OUTPUT_FINAL_RAW
					   || mode == OUTPUT_FINAL_RAW_TOTAL) {
		switch (column) {
		      case LWP_MAX:
			return(acc_lwp.mpr_lwp[MET_MAX]/num_samples);
			break;
		      case LWP_USE:
			return(acc_lwp.mpr_lwp[MET_INUSE]/num_samples);
			break;
		      case LWP_FAIL:
			return(acc_lwp.mpr_lwp[MET_FAIL]/num_samples);
			break;
		      default:
			sarerrno = SARERR_OUTFIELD;
			return(-1);
			break;
		}
	}
	else {
		sarerrno = SARERR_OUTMODE;
		return(-1);
	}
}
			
