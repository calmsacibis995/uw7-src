/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/cpumtr.c	1.1.1.2"
#ident "$Header$"


/*
 * cpumtr.c
 *
 * cpumtr metrics - processes SAR_HWMETRIC_CPU records.
 *
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include "../sa.h"
#include "sar.h"

extern cgid_t *cpu_to_cgid;
extern int *cpu_to_fakeid;

static struct metp_cpumtr *cpumtr = NULL;
static struct metp_cpumtr *old_cpumtr = NULL;
static cpumtr_cpu_stats_t  temp;

static int initialized = 0;

/* output helper functions */
static sarout_t	item_out(int, int, struct metp_cpumtr *,
				struct metp_cpumtr *, time_t *);
static sarout_t	nodal_out(int, int, struct metp_cpumtr *,
				struct metp_cpumtr *, time_t *);
static sarout_t	total_out(int, int, struct metp_cpumtr *,
				struct metp_cpumtr *, time_t *);

static sarout_t	raw_out(int, int, struct metp_cpumtr *,
				struct metp_cpumtr *);
static sarout_t	nodal_raw_out(int, int, struct metp_cpumtr *,
				struct metp_cpumtr *);
static sarout_t	total_raw_out(int, int, struct metp_cpumtr *,
				struct metp_cpumtr *);

/* allocate memory used by this module */
flag
sar_cpumtr_init(void) {
        int      i;
        int      j;
	int	 k;
        
#ifdef DEBUG
	printf("sar_cpumtr_init()\n");
	fflush(stdout);
#endif /* DEBUG */
        if (cpumtr == NULL) {
                cpumtr = malloc(machinfo.num_engines * sizeof(*cpumtr));
                old_cpumtr = malloc(machinfo.num_engines * sizeof(*old_cpumtr));
                if (cpumtr == NULL || old_cpumtr == NULL) {
                        return FALSE;
                }
        }
        
        for (i = 0; i < machinfo.num_engines; i++) {
		for (k = 0; k <= CPUMTR_NUM_METERS; k++) {
                	for (j = 0; j < CPUMTR_NUM_STATES; j++) {
                        	cpumtr[i].mpcm_count[j][k] = 0;
			}
                        cpumtr[i].mpcm_name[k][0] = '\0';
                }
                
                old_cpumtr[i] = cpumtr[i];
                
        }
        
        return TRUE;
}


/*ARGSUSED*/

int
sar_cpumtr_p(FILE *infile, sar_header sarh, flag32 of) {
        int     i;
	int	k;
        
#ifdef DEBUG
        printf("sar_cpumtr_p()\n"); fflush(stdout);
	fflush(stdout);
#endif /* DEBUG */
        if (sarh.item_size != sizeof(cpumtr_cpu_stats_t)) {
                metric_warn[sarh.item_code] = TRUE;
        }
        
        memcpy(old_cpumtr, cpumtr, machinfo.num_engines * sizeof(*old_cpumtr));
        
        for (i = 0; i < sarh.item_count; i++) {
                get_item(&temp, sizeof(cpumtr_cpu_stats_t), sarh.item_size,
			infile);

                cpumtr[i].mpcm_count[CPUMTR_USER][0]
			= temp.ccs_time[CPUMTR_STATE_USER].dl_lop;
                cpumtr[i].mpcm_count[CPUMTR_SYS][0]
			= temp.ccs_time[CPUMTR_STATE_SYSTEM].dl_lop;
                cpumtr[i].mpcm_count[CPUMTR_IDLE][0]
			= temp.ccs_time[CPUMTR_STATE_IDLE].dl_lop;
                cpumtr[i].mpcm_count[CPUMTR_UNKNOWN][0]
			= temp.ccs_time[CPUMTR_STATE_UNKNOWN].dl_lop;
               	strncpy(cpumtr[i].mpcm_name[0], "(cycles)",
			CPUMTR_MAX_NAME_LENGTH);

		for (k = 0; k < CPUMTR_NUM_METERS; k++) {
                	cpumtr[i].mpcm_count[CPUMTR_USER][k+1]
				= temp.ccs_count[CPUMTR_STATE_USER][k].dl_lop;
                	cpumtr[i].mpcm_count[CPUMTR_SYS][k+1]
				= temp.ccs_count[CPUMTR_STATE_SYSTEM][k].dl_lop;
                	cpumtr[i].mpcm_count[CPUMTR_IDLE][k+1]
				= temp.ccs_count[CPUMTR_STATE_IDLE][k].dl_lop;
                	cpumtr[i].mpcm_count[CPUMTR_UNKNOWN][k+1]
			       = temp.ccs_count[CPUMTR_STATE_UNKNOWN][k].dl_lop;
			if (temp.ccs_setup_flags[k]
				== CPUMTR_SETUP_DEACTIVATE) {
                		strncpy(cpumtr[i].mpcm_name[k+1], "(idle)",
					CPUMTR_MAX_NAME_LENGTH);
			} else {
                		strncpy(cpumtr[i].mpcm_name[k+1],
					temp.ccs_setup.css_name[k],
					CPUMTR_MAX_NAME_LENGTH);
			}
		}
        }
	collected[sarh.item_code] = TRUE;
        
#ifdef DEBUG
        printf("sar_cpumtr_p() returns\n"); fflush(stdout);
	fflush(stdout);
#endif /* DEBUG */
        return TRUE;
} /* sar_cpumtr_p() */

sarout_t
sar_cpumtr_out(int column, int mode, int devnum) {
#ifdef DEBUG
        printf("sar_cpumtr_out(%d %d %d)\n", column, mode, devnum);
	fflush(stdout);
#endif /* DEBUG */

        switch (mode) {
              case OUTPUT_DATA:
                return item_out(column, devnum, old_cpumtr, cpumtr, tdiff);
              case OUTPUT_DATA|NODAL:
                return nodal_out(column, devnum, old_cpumtr, cpumtr, tdiff);
              case OUTPUT_TOTAL:
                return total_out(column, devnum, old_cpumtr, cpumtr, tdiff);
	      case OUTPUT_RAW:
		return raw_out(column, devnum, old_cpumtr, cpumtr);
	      case OUTPUT_RAW|NODAL:
		return nodal_raw_out(column, devnum, old_cpumtr, cpumtr);
	      case OUTPUT_RAW_TOTAL:
		return total_raw_out(column, devnum, old_cpumtr, cpumtr);
              default:
                sarerrno = SARERR_OUTMODE;
                return (sarout_t)-1;
        }
} /* sar_cpumtr_out() */


/* 
 * Produces per-processor output.  'start' and 'end' are 
 * arrays of structures (one per processor) containing metric 
 * information and the start and end of the desired interval.  
 * The array 'td' contains the number of clock ticks (for each
 * processor) occuring during the interval.
 * 
 */

static sarout_t
item_out(int state, int devnum, struct metp_cpumtr *start, 
         struct metp_cpumtr *end, time_t *td) {
	int icpu;
	int meter_index;

	icpu = devnum / 256;
	meter_index = devnum % 256;

	if (meter_index < 0 || meter_index > CPUMTR_NUM_METERS) {
        	sarerrno = SARERR_OUTFIELD;
        	return (sarout_t)-1;
	} else if (state >= 0 && state < CPUMTR_NUM_STATES) {
        	return DIFF_P(icpu,mpcm_count[state][meter_index])/td[icpu];
	} else if (state == CPUMTR_TOTAL) {
		double total = 0;
        	double tdifftot = 0;
		for (state = 0; state < CPUMTR_NUM_STATES; state++) {
        		total += DIFF_P(icpu,mpcm_count[state][meter_index]);
			tdifftot += td[icpu];
		}
        	return (sarout_t)(total/tdifftot);
	}
       	sarerrno = SARERR_OUTFIELD;
       	return (sarout_t)-1;
} /* item_out() */


/* 
 * Produces nodal output for metric data that may also be reported
 * on a per-processor basis.  Parameters have the same meaning as in
 * item_out().
 */

static sarout_t
nodal_out(int state, int devnum, struct metp_cpumtr *start,
	struct metp_cpumtr *end, time_t *td) {
	int nodei;
	int meter_index;
        double  total = 0;
        double  tdifftot = 0;
        int    icpu;

	nodei = devnum / (CPUMTR_NUM_METERS + 1);
	meter_index = devnum % (CPUMTR_NUM_METERS + 1);

        for (icpu = 0; icpu < machinfo.num_engines; icpu++)
	if (which_node(icpu) == nodei) {
		if (state >= 0 && state < CPUMTR_NUM_STATES) {
        		total += DIFF_P(icpu,mpcm_count[state][meter_index]);
		} else if (state == CPUMTR_TOTAL) {
			int istate;
			for (istate = 0; istate < CPUMTR_NUM_STATES; istate++) {
        			total +=
				DIFF_P(icpu,mpcm_count[istate][meter_index]);
			}
		}
        	tdifftot += td[icpu];
        }
        return (sarout_t)(total/tdifftot);
} /* nodal_out() */

/* 
 * Produces system-wide output for metric data that may also be reported
 * on a per-processor basis.  Parameters have the same meaning as in
 * item_out().
 */

static sarout_t
total_out(int state, int meter_index, struct metp_cpumtr *start,
	struct metp_cpumtr *end, time_t *td) {
        double  total = 0;
        double  tdifftot = 0;
        int    icpu;

	if (meter_index < 0 && meter_index > CPUMTR_NUM_METERS) {
        	sarerrno = SARERR_OUTFIELD;
        	return (sarout_t)-1;
	}
        for (icpu = 0; icpu < machinfo.num_engines; icpu++) {
		if (state >= 0 && state < CPUMTR_NUM_STATES) {
        		total += DIFF_P(icpu,mpcm_count[state][meter_index]);
		} else if (state == CPUMTR_TOTAL) {
			int istate;
			for (istate = 0; istate < CPUMTR_NUM_STATES; istate++) {
        			total +=
				DIFF_P(icpu,mpcm_count[istate][meter_index]);
			}
		}
        	tdifftot += td[icpu];
        }
        return (sarout_t)(total/tdifftot);
} /* total_out() */

static sarout_t
raw_out(int state, int devnum, struct metp_cpumtr *start, 
         struct metp_cpumtr *end)
{
	int icpu;
	int meter_index;

	icpu = devnum / 256;
	meter_index = devnum % 256;

	if (meter_index < 0 || meter_index > CPUMTR_NUM_METERS) {
        	sarerrno = SARERR_OUTFIELD;
        	return (sarout_t)-1;
	} else if (state >= 0 && state < CPUMTR_NUM_STATES) {
        	return DIFF_P(icpu,mpcm_count[state][meter_index]);
	} else if (state == CPUMTR_TOTAL) {
		double total = 0;
		for (state = 0; state < CPUMTR_NUM_STATES; state++) {
        		total += DIFF_P(icpu,mpcm_count[state][meter_index]);
		}
        	return (sarout_t)total;
	}
       	sarerrno = SARERR_OUTFIELD;
       	return (sarout_t)-1;
} /* raw_out() */


/* 
 * Produces per-node output for metric data that may also be reported
 * on a per-processor basis.  Parameters have the same meaning as in
 * item_out().
 */

static sarout_t
nodal_raw_out(int state, int devnum, struct metp_cpumtr *start,
	struct metp_cpumtr *end) {
	int nodei;
	int meter_index;
        double  total = 0;
        int    icpu;

	nodei = devnum / (CPUMTR_NUM_METERS + 1);
	meter_index = devnum % (CPUMTR_NUM_METERS + 1);

        for (icpu = 0; icpu < machinfo.num_engines; icpu++)
	if (which_node(icpu) == nodei) {
		if (state >= 0 && state < CPUMTR_NUM_STATES) {
        		total += DIFF_P(icpu,mpcm_count[state][meter_index]);
		} else if (state == CPUMTR_TOTAL) {
			int istate;
			for (istate = 0; istate < CPUMTR_NUM_STATES; istate++) {
        			total +=
				DIFF_P(icpu,mpcm_count[istate][meter_index]);
			}
		}
        }
        return (sarout_t)total;
} /* nodal_raw_out() */

/* 
 * Produces system-wide output for metric data that may also be reported
 * on a per-processor basis.  Parameters have the same meaning as in
 * item_out()
 */

static sarout_t
total_raw_out(int state, int meter_index, struct metp_cpumtr *start,
	struct metp_cpumtr *end) {
        double  total = 0;
        int    icpu;

	if (meter_index < 0 && meter_index > CPUMTR_NUM_METERS) {
        	sarerrno = SARERR_OUTFIELD;
        	return (sarout_t)-1;
	}
        for (icpu = 0; icpu < machinfo.num_engines; icpu++) {
		if (state >= 0 && state < CPUMTR_NUM_STATES) {
        		total += DIFF_P(icpu,mpcm_count[state][meter_index]);
		} else if (state == CPUMTR_TOTAL) {
			int istate;
			for (istate = 0; istate < CPUMTR_NUM_STATES; istate++) {
        			total +=
				DIFF_P(icpu,mpcm_count[istate][meter_index]);
			}
		}
        }
        return (sarout_t)total;
} /* total_raw_out() */


void
sar_cpumtr_cleanup(void) {
        free(cpumtr);
        free(old_cpumtr);

	cpumtr = NULL;
	old_cpumtr = NULL;
} /* sar_cpumtr_cleanup() */

const char *
cpumtr_get_metric_name(int point, int mtype, int inst) {
	int meter_index;

#ifdef DEBUG
	printf("cpumtr_get_metric_name(%d, %d, %d)\n", point, mtype, inst);
	fflush(stdout);
#endif /* DEBUG */
	if (point < 0 || point >= machinfo.num_engines) {
		return "error in cpumtr_get_metric_name";
	}
	meter_index = cpu_meter_index(mtype, inst);
	if (meter_index < 0 || meter_index > CPUMTR_NUM_METERS) {
		return "error in cpumtr_get_metric_name";
	}
	return cpumtr[point].mpcm_name[meter_index];
} /* cpumtr_get_metric_name() */

/*
 * Process SAR_HWMETRIC_INIT records.
 */

char *node_list = NULL;
extern char *node_list_string;
extern char do_aggregate_node;

/* ARGSUSED */
int
sar_hwmetric_init(FILE *infile, sar_header sarh, flag32 of) {
        int      i;
        char     *tempptr;
        int      p;

        if (sarh.item_size != sizeof(hwmetric_init_info))
                metric_warn[sarh.item_code] = TRUE;

        get_item(&hwmetric_init_info, sizeof(hwmetric_init_info),
		sarh.item_size, infile);

	if (initialized) {
	  return TRUE;
	}
	initialized = 1;
	  
	cpu_to_cgid = malloc(Num_cpus * sizeof(*cpu_to_cgid));
	cpu_to_fakeid = malloc(Num_cpus * sizeof(*cpu_to_fakeid));

        node_list = malloc(Num_cgs);
        if (node_list_string == NULL) {     /* -N option not used */
                for (i = 0; i < Num_cgs; i++)
                        node_list[i] = FALSE;
        } else if (strcmp(node_list_string, "ALL") != 0) {
                char *dupstring;
        	/*
        	 * -N option, but not "ALL" keyword, i.e.,
         	 * individual nodes specified.
         	 */
                do_aggregate_node = FALSE;
                for (i = 0; i < Num_cgs; i++)
                        node_list[i] = FALSE;
                /*
                 * duplicate node_list_string before using
                 * strtok() so that this code will work after
                 * a reboot INIT record is detected.
                 */
                dupstring = malloc(strlen(node_list_string) + 1);
                strcpy(dupstring, node_list_string);
                tempptr = strtok(dupstring, ",");
                while (tempptr) {
                        p = atoi(tempptr);
                        if (p >= 0 && p < Num_cgs)
                                node_list[p] = TRUE;
                        tempptr = strtok(NULL, ",");
                }
                free(dupstring);

        } else { /* -N ALL, report data for all nodes */
                do_aggregate_node = TRUE;
                for (i = 0; i < Num_cgs; i++)
                        node_list[i] = TRUE;
        }
	(void)sar_cgmtr_init();
	return TRUE;
} /* sar_hwmetric_init() */

