/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/locsched.c	1.7"
#ident "$Header$"


/* locsched.c
 * 
 * Per processor schedule information.  Processes SAR_LOCSCHED_P records. 
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"

static struct metp_sched   *psched = NULL;
static struct metp_sched   *old_psched = NULL;
static struct metp_sched   *first_psched = NULL;
static struct lsched_info  temp;

static flag    first_sample = TRUE;


flag
sar_locsched_init(void)
{
	int   i;
	
	if (psched == NULL) {
		psched = malloc(machinfo.num_engines * sizeof(*psched));
		old_psched = malloc(machinfo.num_engines * sizeof(*old_psched));
		first_psched = malloc(machinfo.num_engines * sizeof(*first_psched));
		
		if (psched == NULL || old_psched == NULL || first_psched == NULL) {
			return(FALSE);
		}
	}
	
	for (i = 0; i < machinfo.num_engines; i++) {
		psched[i].mps_pswitch = 0;
		psched[i].mps_runque = 0;
		psched[i].mps_runocc = 0;
		
		old_psched[i] = psched[i];
	}
	
	first_sample = TRUE;
	return(TRUE);
}

/*ARGSUSED*/

int
sar_locsched_p(FILE *infile, sar_header sarh, flag32 of)
{
	int   p;
	int   i;


	if (sarh.item_size != sizeof(struct lsched_info)) {
		metric_warn[sarh.item_code] = TRUE;
	}

	memcpy(old_psched, psched, machinfo.num_engines * sizeof(*old_psched));
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(struct lsched_info), sarh.item_size, infile);
		p = temp.id;
		psched[p] = temp.data;
	}
	
	if (first_sample == TRUE) {
		memcpy(first_psched, psched, machinfo.num_engines * sizeof(*first_psched));
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;
		
	return(TRUE);
}


sarout_t
sar_locsched_out(int column, int mode, int devnum)
{
	struct metp_sched	*start;
	struct metp_sched	*end;
	time_t			*td;
	sarout_t		answer;
	sarout_t     		runque;
	sarout_t		runocc;
	
	SET_INTERVAL(mode, psched, old_psched, first_psched);

	switch (column) {
	      case PSCHED_RUNQ:
		GENERIC_DIFF(runque, mode, devnum, mps_runque);
		GENERIC_DIFF(runocc, mode, devnum, mps_runocc);
		if ((mode & RAW) != 0) {
			return(runque);
		} 
		else {
			if (runocc == (sarout_t) 0) {
				sarerrno = SARERR_OUTBLANK;
				return((sarout_t) -1);
			}
			else {
				return(runque/runocc);
			}
		}
		break;
			
	      case PSCHED_RUNOCC:
		GENERIC_DIFF(runocc, mode, devnum, mps_runocc);
		if ((mode & RAW) != 0) {
			return(runocc);
		}
		else {
			if (runocc == (sarout_t) 0) {
				sarerrno = SARERR_OUTBLANK;
				return((sarout_t) -1);
			}
			else {
				if ((mode & TOTAL) != 0) {
					time_t ticks;

					ticks = (mode & FINAL) ? total_tdiff_max : tdiff_max;
					return((100 * DO_DIFF_TIME(0, runocc, ticks))/machinfo.num_engines);

				}
				else {	
					return(100 * DIFF_TIME_P(devnum, mps_runocc));
				}
			}
		}
		break;

	      case PSCHED_PSWITCH:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mps_pswitch);
		return(answer);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}



void
sar_locsched_cleanup(void)
{
	free(psched);
	free(old_psched);
	free(first_psched);

	psched = NULL;
	old_psched = NULL;
	first_psched = NULL;
}
