/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/cgmtr.c	1.1"
#ident "$Header$"


/*
 * cgmtr.c
 *
 * cgmtr metrics - processes SAR_HWMETRIC_CG records.
 *
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include "../sa.h"
#include "sar.h"

static struct metp_cgmtr *cgmtr = NULL;
static struct metp_cgmtr *old_cgmtr = NULL;
static cgmtr_cg_stats_t  temp;

static sarout_t	raw_out(int, struct metp_cgmtr *, struct metp_cgmtr *);

static sarout_t	total_raw_out(int, struct metp_cgmtr *,
			 struct metp_cgmtr *end_time);


flag
sar_cgmtr_init(void) {
        int      i;
	int	 k;
        
#ifdef DEBUG
	printf("sar_cgmtr_init(%d)\n", Num_cgs);
	fflush(stdout);
#endif /* DEBUG */

        if (cgmtr == NULL) {
                cgmtr = malloc(Num_cgs * sizeof(*cgmtr));
                old_cgmtr = malloc(Num_cgs * sizeof(*old_cgmtr));
                if (cgmtr == NULL || old_cgmtr == NULL) {
                        return FALSE;
                }
        }
        
        for (i = 0; i < Num_cgs; i++) {
		for (k = 0; k < CGMTR_NUM_METERS; k++) {
                        cgmtr[i].mpcg_count[k] = 0;
                        cgmtr[i].mpcg_name[k][0] = '\0';
                }
                old_cgmtr[i] = cgmtr[i];
        }
        
        return TRUE;
} /* sar_cgmtr_init() */


/*ARGSUSED*/

int
sar_cgmtr_p(FILE *infile, sar_header sarh, flag32 of) {
        int     i;
	int	k;
        
#ifdef DEBUG
	printf("sar_cgmtr_p(%d)\n", Num_cgs);
	fflush(stdout);
#endif /* DEBUG */
        if (sarh.item_size != sizeof(cgmtr_cg_stats_t)) {
                metric_warn[sarh.item_code] = TRUE;
        }
        
        memcpy(old_cgmtr, cgmtr, Num_cgs * sizeof(*old_cgmtr));
        
        for (i = 0; i < sarh.item_count; i++) {
                get_item(&temp, sizeof(cgmtr_cg_stats_t), sarh.item_size, infile);

		for (k = 0; k < CGMTR_NUM_METERS; k++) {
                	cgmtr[i].mpcg_count[k] = temp.pcs_count[k].dl_lop;
			if (temp.pcs_setup_flags[k] == CGMTR_SETUP_DEACTIVATE) {
                		strncpy(cgmtr[i].mpcg_name[k], "(idle)",
					CGMTR_MAX_NAME_LENGTH);
			} else {
                		strncpy(cgmtr[i].mpcg_name[k],
					temp.pcs_setup.pss_name[k],
					CGMTR_MAX_NAME_LENGTH);
			}
		}
                
		collected[sarh.item_code] = TRUE;
                
        }
        return TRUE;
} /* sar_cgmtr_p() */

/* ARGSUSED */

sarout_t
sar_cgmtr_out(int column, int mode, int devnum) {
#ifdef DEBUG
        printf("sar_cgmtr_out(%d %d)\n", mode, devnum);
	fflush(stdout);
#endif /* DEBUG */
        switch (mode) {
              case OUTPUT_DATA:
	      case OUTPUT_RAW:
		return raw_out(devnum, old_cgmtr, cgmtr);

              case OUTPUT_TOTAL:
	      case OUTPUT_RAW_TOTAL:
		return total_raw_out(devnum, old_cgmtr, cgmtr);

              default:
                sarerrno = SARERR_OUTMODE;
                return (sarout_t)-1;
        }
} /* sar_cgmtr_out() */


static sarout_t
raw_out(int devnum, struct metp_cgmtr *start, struct metp_cgmtr *end) {
	int icg;
	int meter_index;

	icg = devnum / 256;
	meter_index = devnum % 256;

	if (meter_index < 0 || meter_index >= CGMTR_NUM_METERS) {
        	sarerrno = SARERR_OUTFIELD;
        	return (sarout_t)-1;
	}
        return DIFF_P(icg,mpcg_count[meter_index]);
} /* raw_out() */


static sarout_t
total_raw_out(int meter_index, struct metp_cgmtr *start,
	struct metp_cgmtr *end)
{
        float  total = 0;
        int    icg;

	if (meter_index < 0 && meter_index >= CGMTR_NUM_METERS) {
        	sarerrno = SARERR_OUTFIELD;
        	return (sarout_t)-1;
	}
        for (icg = 0; icg < Num_cgs; icg++) {
       		total += DIFF_P(icg,mpcg_count[meter_index]);
        }
        return (sarout_t)total;
} /* total_raw_out() */


void
sar_cgmtr_cleanup(void) {
        free(cgmtr);
        free(old_cgmtr);

	cgmtr = NULL;
	old_cgmtr = NULL;
} /* sar_cgmtr_cleanup() */

const char *
cgmtr_get_metric_name(int point, int mtype, int inst) {
	int meter_index;

	if (point < 0 || point >= Num_cgs) {
		return "error in cgmtr_get_metric_name";
	}
	meter_index = cg_meter_index(mtype, inst);
	if (meter_index < 0 || meter_index >= CGMTR_NUM_METERS) {
		return "error in cgmtr_get_metric_name";
	}
	return cgmtr[point].mpcg_name[meter_index];
} /* cgmtr_get_metric_name() */
