/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/procres.c	1.6"
#ident "$Header$"


/* procres.c
 *
 * Process resource metrics.  Processes SAR_PROCRESOURCE records. 
 */

#include <stdio.h>
#include "../sa.h"
#include "sar.h"

static struct mets_proc_resrc    proc;
static struct mets_proc_resrc    old_proc;
static struct mets_proc_resrc	 acc_proc;

static flag       first_sample;

static uint_t     num_samples = 0;


flag
sar_proc_init(void)
{
	int   i;
	
	for (i = MET_FAIL; i <= MET_MAX; i++) {
		proc.msr_proc[i] = 0;
	}
	old_proc = proc;
	acc_proc = proc;
	
	first_sample = TRUE;
	num_samples = 0;
	
	return(TRUE);
}


/*ARGSUSED*/

int
sar_proc(FILE *infile, sar_header sarh, flag32 of)
{
	if (sarh.item_size != sizeof(struct mets_proc_resrc)) {
		metric_warn[sarh.item_code] = TRUE;
	}

	old_proc = proc;
	get_item(&proc, sizeof(proc), sarh.item_size, infile);
	
	if (first_sample == FALSE) {
		acc_proc.msr_proc[MET_MAX] += proc.msr_proc[MET_MAX];
		acc_proc.msr_proc[MET_INUSE] += proc.msr_proc[MET_INUSE];
		acc_proc.msr_proc[MET_FAIL] += proc.msr_proc[MET_FAIL] - 
					       old_proc.msr_proc[MET_FAIL];

		num_samples++;
	}
	collected[sarh.item_code] = TRUE;
	first_sample = FALSE;

	return(TRUE);
}


/*ARGSUSED*/

sarout_t
sar_proc_out(int column, int mode, int devnum)
{
	if (mode == OUTPUT_DATA || mode == OUTPUT_TOTAL
	    			|| mode == OUTPUT_RAW
	    			|| mode == OUTPUT_RAW_TOTAL) {
		switch (column) {
		      case PROC_MAX:
			return(proc.msr_proc[MET_MAX]);  
			break;
		      case PROC_USE:
			return(proc.msr_proc[MET_INUSE]);  
			break;
		      case PROC_FAIL:
			return(proc.msr_proc[MET_FAIL] - old_proc.msr_proc[MET_FAIL]);  
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
		      case PROC_MAX:
			return(acc_proc.msr_proc[MET_MAX]/num_samples);  
			break;
		      case PROC_USE:
			return(acc_proc.msr_proc[MET_INUSE]/num_samples);  
			break;
		      case PROC_FAIL:
			return(acc_proc.msr_proc[MET_FAIL]/num_samples);
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
