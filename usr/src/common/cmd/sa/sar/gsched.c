/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/gsched.c	1.6"
#ident "$Header$"


/* gsched.c
 * 
 * scheduling metrics (system wide).  Processes SAR_GLOBSCHED records.
 */


#include <stdio.h>

#include "../sa.h"
#include "sar.h"


static struct mets_sched	gsched;
static struct mets_sched	old_gsched;
static struct mets_sched  	first_gsched;

static flag	first_sample = TRUE;

static sarout_t	item_out(int item, 
			 struct mets_sched start,
			 struct mets_sched end,
			 time_t		td);

static sarout_t	raw_out(int item, 
			struct mets_sched start,
			struct mets_sched end);


flag
sar_gsched_init(void)
{
	gsched.mss_runque = 0;
	gsched.mss_runocc = 0;
	gsched.mss_swpque = 0;
	gsched.mss_swpocc = 0;
	
	old_gsched = gsched;
	first_gsched = gsched;

	first_sample = TRUE;
	
	return(TRUE);
}

/*ARGSUSED*/

int
sar_gsched_p(FILE *infile, sar_header sarh, flag32 of)
{
	if (sarh.item_size != sizeof(struct mets_sched)) {
		metric_warn[sarh.item_code] = TRUE;
	}
	
	old_gsched = gsched;
	get_item(&gsched, sizeof(struct mets_sched), sarh.item_size, infile);

	if (first_sample) {
		first_gsched = gsched;
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;
	
	return(TRUE);
}

/*ARGSUSED*/

sarout_t
sar_gsched_out(int column, int mode, int devnum)
{
	sarerrno = 0;
	if ((mode & TOTAL) == 0) {
		sarerrno = SARERR_OUTBLANK;
		return(-1);
	}
	else if (mode == OUTPUT_TOTAL) {
		return(item_out(column, old_gsched, gsched, tdiff_max));
	}
	else if (mode == OUTPUT_FINAL_TOTAL) {
		return(item_out(column, first_gsched, gsched, total_tdiff_max));
	}
	else if (mode == OUTPUT_RAW_TOTAL) {
		return(raw_out(column, old_gsched, gsched));
	}
	else if (mode == OUTPUT_FINAL_RAW_TOTAL) {
		return(raw_out(column, first_gsched, gsched));
	}
	else {
		sarerrno = SARERR_OUTMODE;
		return(-1);
	}
}


static sarout_t
item_out(int item, struct mets_sched start, struct mets_sched end, time_t td)
{
	sarout_t	answer;
	
	switch (item) {
	      case GSCHED_RUNQ:
		if ((DIFF(mss_runocc)) == 0) {
			sarerrno = SARERR_OUTBLANK;
			return(-1);
		}
		else {
			answer = DIFF(mss_runque)/DIFF(mss_runocc);
			return(answer);
		}
		break;
		
	      case GSCHED_RUNOCC:
		if ((DIFF(mss_runocc)) == 0) {
			sarerrno = SARERR_OUTBLANK;
			return(-1);
		}
		else {
			return(100 * DIFF_TIME(mss_runocc));
		}
		break;

	      case GSCHED_SWPQ:
		if ((DIFF(mss_swpocc)) == (sarout_t) 0) {
			sarerrno = SARERR_OUTBLANK;
			return((sarout_t) -1);
		}
		else {
			answer = DIFF(mss_swpque)/DIFF(mss_swpocc);
			return(answer);
		}
		break;
		
	      case GSCHED_SWPOCC:
		if ((DIFF(mss_swpocc)) == (sarout_t) 0) {
			sarerrno = SARERR_OUTBLANK;
			return((sarout_t) -1);
		}
		else {
			return((sarout_t) 100 * DIFF_TIME(mss_swpocc));
		}
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return((sarout_t) -1);
		break;
	}
}


static sarout_t
raw_out(int item, struct mets_sched start, struct mets_sched end)
{
	switch (item) {
	      case GSCHED_RUNQ:
		return(DIFF(mss_runque));
		break;

	      case GSCHED_RUNOCC:
		return(DIFF(mss_runocc));
		break;

	      case GSCHED_SWPQ:
		return(DIFF(mss_swpque));
		break;
	      case GSCHED_SWPOCC:
		return(DIFF(mss_swpocc));
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}




