/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/cpumtr_lwp.c	1.1"
#ident "$Header$"



/*
 * cpumtr_lwp.c
 *
 * cpumtr_lwp metrics - processes SAR_HWMETRIC_LWP records.
 *
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include "../sa.h"
#include "sar.h"

static struct metp_cpumtr_lwp *cpumtr_lwp = NULL;
static struct metp_cpumtr_lwp *old_cpumtr_lwp = NULL;
static cpumtr_lwp_stats_t      temp;
static int num_lwps = 0;
static int old_num_lwps = 0;

static sarout_t total_raw_out(int, int);


/*ARGSUSED*/

int
sar_cpumtr_lwp_p(FILE *infile, sar_header sarh, flag32 of) {
        int     i;
        int     k;
        
        if (sarh.item_size != sizeof(cpumtr_lwp_stats_t)) {
                metric_warn[sarh.item_code] = TRUE;
        }
	if (old_cpumtr_lwp != NULL)
		free (old_cpumtr_lwp);
	old_num_lwps = num_lwps;
	old_cpumtr_lwp = cpumtr_lwp;
	num_lwps = sarh.item_count;
	cpumtr_lwp = malloc(num_lwps * sizeof(*cpumtr_lwp));
        
        for (i = 0; i < sarh.item_count; i++) {
                get_item(&temp, sizeof(cpumtr_lwp_stats_t), sarh.item_size,
			infile);
		
                cpumtr_lwp[i].mpcml_pid = temp.cls_pid;
                cpumtr_lwp[i].mpcml_lwpid = temp.cls_lwpid;

                cpumtr_lwp[i].mpcml_count[CPUMTR_LWP_USER][0]
                        = temp.cls_time[CPUMTR_STATE_USER].dl_lop;
                cpumtr_lwp[i].mpcml_count[CPUMTR_LWP_SYS][0]
                        = temp.cls_time[CPUMTR_STATE_SYSTEM].dl_lop;
                cpumtr_lwp[i].mpcml_count[CPUMTR_LWP_UNKNOWN][0]
                        = temp.cls_time[CPUMTR_STATE_UNKNOWN].dl_lop;

                for (k = 0; k < CPUMTR_NUM_METERS; k++) {
                        cpumtr_lwp[i].mpcml_count[CPUMTR_LWP_USER][k+1]
                                = temp.cls_count[CPUMTR_STATE_USER][k].dl_lop;
                        cpumtr_lwp[i].mpcml_count[CPUMTR_LWP_SYS][k+1]
                                = temp.cls_count[CPUMTR_STATE_SYSTEM][k].dl_lop;
                        cpumtr_lwp[i].mpcml_count[CPUMTR_LWP_UNKNOWN][k+1]
                               = temp.cls_count[CPUMTR_STATE_UNKNOWN][k].dl_lop;
		}
        }
	collected[sarh.item_code] = TRUE;
        return TRUE;
} /* sar_cputmr_lwp_p() */

sarout_t
sar_cpumtr_lwp_out(int column, int mode, int devnum) {
#ifdef DEBUG
        printf("sar_cpumtr_lwp_out(%d %d %d)\n", column, mode, devnum);
	fflush(stdout);
#endif /* DEBUG */

        switch (mode) {
              case OUTPUT_TOTAL:
              case OUTPUT_RAW_TOTAL:
                return total_raw_out(column, devnum);

              default:
                sarerrno = SARERR_OUTMODE;
                return (sarout_t)-1;
        }
} /* sar_cpumtr_lwp_out() */

static sarout_t
total_raw_out(int column, int devnum) {
	int i, j;
	int meter_index;
	int pid, lwpid;

	pid = (devnum / 65536) - 2;
	devnum = devnum % 65536;
	lwpid = (devnum / 256) - 2;
	meter_index = devnum % 256;

        if (meter_index < 0 || meter_index > CPUMTR_NUM_METERS) {
                sarerrno = SARERR_OUTFIELD;
                return (sarout_t)-1;
        }

	if (column == CPUMTR_LWP_PID)
		return (sarout_t)pid;
	else if (column == CPUMTR_LWP_LWPID)
		return (sarout_t)lwpid;

	{
		int k;

		i = -1;
		for (k = 0; k < num_lwps; k++) {
			if (cpumtr_lwp[k].mpcml_pid == pid
			 && cpumtr_lwp[k].mpcml_lwpid == lwpid) {
				i = k;
				break;
			}
		}
		j = -1;
		for (k = 0; k < old_num_lwps; k++) {
			if (old_cpumtr_lwp[k].mpcml_pid == pid
			 && old_cpumtr_lwp[k].mpcml_lwpid == lwpid) {
				j = k;
				break;
			}
		}
	}

	if (i == -1) {
                sarerrno = SARERR_OUTFIELD;
                return (sarout_t)-1;
	}

	if (column >= 0 && column < CPUMTR_NUM_LWP_STATES) {
		if (j == -1) {
			return (sarout_t)
				cpumtr_lwp[i].mpcml_count[column][meter_index];
		} else {
			return (sarout_t)(
				cpumtr_lwp[i].mpcml_count[column][meter_index]
		          - old_cpumtr_lwp[j].mpcml_count[column][meter_index]);
		}
        } else if (column == CPUMTR_LWP_TOTAL) {
                double total = 0;
		int ist;
		for (ist = 0; ist < CPUMTR_NUM_LWP_STATES; ist++) {
		  if (j == -1) {
			total += cpumtr_lwp[i].mpcml_count[ist][meter_index];
		  } else {
			total += (cpumtr_lwp[i].mpcml_count[ist][meter_index]
		            - old_cpumtr_lwp[j].mpcml_count[ist][meter_index]);
		  }
		}
                return (sarout_t)total;
        }
        sarerrno = SARERR_OUTFIELD;
        return (sarout_t)-1;
} /* total_raw_out() */

void
sar_cpumtr_lwp_cleanup(void) {
        free(cpumtr_lwp);
        free(old_cpumtr_lwp);

	cpumtr_lwp = NULL;
	old_cpumtr_lwp = NULL;
} /* sar_cpumtr_lwp_cleanup() */

int
sar_cpumtr_lwp_each (int *pnext_pid, int *pnext_lwpid) {
	static i = 0;

#ifdef DEBUG
	printf("sar_cpumtr_lwp_each() i=%d\n", i);
	fflush(stdout);
#endif /* DEBUG */

  	if (i >= num_lwps) {
    		i = 0;
    		return FALSE;
  	}
	*pnext_pid = cpumtr_lwp[i].mpcml_pid;
	*pnext_lwpid = cpumtr_lwp[i].mpcml_lwpid;
	i++;
	return TRUE;
} /* sar_cpumtr_lwp_each() */
