/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/cpu.c	1.7.1.3"
#ident "$Header$"


/*
 * cpu.c
 *
 * cpu metrics - processes SAR_CPU_P records.
 *
 * Note: This module differs from the other processing modules
 * in that it maintains the tdiff which is accessable to other sar 
 * modules.
 *
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include "../sa.h"
#include "sar.h"

/*
 * tdiff = total of number of clock ticks (per-processor) in the
 *         interval since the last sample.  Used by most of the
 *         other modules.
 */

time_t   *tdiff = NULL;
time_t   *total_tdiff = NULL;
time_t   tdiff_max;
time_t   total_tdiff_max;

static struct metp_cpu     *cpu = NULL;
static struct metp_cpu     *old_cpu = NULL;
static struct metp_cpu     *first_cpu = NULL;
static struct cpu_info     temp;
static char    first_sample = TRUE;

static sarout_t	item_out(int item, 
			 int devnum, 
			 struct metp_cpu *start_time,
			 struct metp_cpu *end_time,
			 time_t *timediff);

static sarout_t	nodal_out(int item, int devnum, struct metp_cpu *start_time,
			 struct metp_cpu *end_time, time_t *timediff);

static sarout_t	total_out(int item,
			  struct metp_cpu *start_time,
			  struct metp_cpu *end_time,
			  time_t *timediff);

static sarout_t	raw_out(int item, 
			int devnum, 
			struct metp_cpu *start_time,
			struct metp_cpu *end_time);

static sarout_t	nodal_raw_out(int item, int devnum, struct metp_cpu *start_time,
			 struct metp_cpu *end_time);

static sarout_t	total_raw_out(int item,
			      struct metp_cpu *start_time,
			      struct metp_cpu *end_time);



flag
sar_cpu_init(void)
{
        int      i;
        int      j;
        
        if (cpu == NULL) {
                cpu = malloc(machinfo.num_engines * sizeof(*cpu));
                old_cpu = malloc(machinfo.num_engines * sizeof(*old_cpu));
                first_cpu = malloc(machinfo.num_engines * sizeof(*first_cpu));
                tdiff = malloc(machinfo.num_engines * sizeof(*tdiff));
                total_tdiff = malloc(machinfo.num_engines * sizeof(*total_tdiff));
                
                if (cpu == NULL || old_cpu == NULL
                    || first_cpu == NULL
                    || tdiff == NULL
                    || total_tdiff == NULL) {
                        return(FALSE);
                }
        }
        
        for (i = 0; i < machinfo.num_engines; i++) {
                for (j = MET_CPU_USER; j <= MET_CPU_IDLE; j++) {
                        cpu[i].mpc_cpu[j] = 0;
                }
                
                old_cpu[i] = cpu[i];
                
                tdiff[i] = 0;
                total_tdiff[i] = 0;
        }
        
        first_sample = TRUE;
        
        return(TRUE);
}


/*ARGSUSED*/

int
sar_cpu_p(FILE *infile, sar_header sarh, flag32 of)
{
        int     i,j;
	int	p;
        
        if (sarh.item_size != sizeof(struct cpu_info)) {
                metric_warn[sarh.item_code] = TRUE;
        }
        
        tdiff_max = 0;
        total_tdiff_max = 0;
        
        memcpy(old_cpu, cpu, machinfo.num_engines * sizeof(*old_cpu));
        
        for (i = 0; i < sarh.item_count; i++) {
                get_item(&temp, sizeof(struct cpu_info), sarh.item_size, infile);
		p = temp.id;
                cpu[p] = temp.data;
                
                if (first_sample == TRUE) {
                        first_cpu[p] = cpu[p];
                }
		collected[sarh.item_code] = TRUE;
                
                tdiff[p] = 0;
                total_tdiff[p] = 0;
                for (j = 0; j < 4; j++) {
                        tdiff[p] += cpu[p].mpc_cpu[j] - old_cpu[p].mpc_cpu[j];
                        total_tdiff[p] += cpu[p].mpc_cpu[j] - first_cpu[p].mpc_cpu[j];
                }
                
                tdiff_max = tdiff[p] > tdiff_max ? tdiff[p] : tdiff_max;
                total_tdiff_max = total_tdiff[p] > total_tdiff_max ? total_tdiff[p] : total_tdiff_max;
        }
        
        first_sample = FALSE;
        
        return(TRUE);
}


sarout_t
sar_cpu_out(int column, int mode, int devnum)
{
        switch (mode) {
              case OUTPUT_DATA:
                return(item_out(column, devnum, old_cpu, cpu, tdiff));
                break;
                
              case OUTPUT_DATA|NODAL:
                return nodal_out(column, devnum, old_cpu, cpu, tdiff);
		break;

              case OUTPUT_TOTAL:
                return(total_out(column, old_cpu, cpu, tdiff));
                break;
                
              case OUTPUT_FINAL_DATA:
                return(item_out(column, devnum, first_cpu, cpu, total_tdiff));
                break;
                
              case OUTPUT_FINAL_DATA|NODAL:
                return nodal_out(column, devnum, first_cpu, cpu, total_tdiff);
		break;

              case OUTPUT_FINAL_TOTAL:
                return(total_out(column, first_cpu, cpu, total_tdiff));
                break;
                
	      case OUTPUT_RAW:
		return(raw_out(column, devnum, old_cpu, cpu));
		break;

              case OUTPUT_RAW|NODAL:
                return nodal_raw_out(column, devnum, old_cpu, cpu);
		break;

	      case OUTPUT_RAW_TOTAL:
		return(total_raw_out(column, old_cpu, cpu));
		break;

	      case OUTPUT_FINAL_RAW:
		return(raw_out(column, devnum, first_cpu, cpu));
		break;
		      		
              case OUTPUT_FINAL_RAW|NODAL:
                return nodal_raw_out(column, devnum, first_cpu, cpu);
		break;

	      case OUTPUT_FINAL_RAW_TOTAL:
		return(total_raw_out(column, first_cpu, cpu));
		break;
		      		
              default:
                sarerrno = SARERR_OUTMODE;
                return(-1);
                break;
        }
}


/* 
 * Produces per-processor output.  'start' and 'end' are 
 * arrays of structures (one per processor) containing metric 
 * information and the start and end of the desired interval.  
 * The array 'td' contains the number of clock ticks (for each
 * processor) occuring during the interval.
 * 
 */

static sarout_t
item_out(int column, 
         int devnum, 
         struct metp_cpu *start, 
         struct metp_cpu *end,
         time_t *td)
{
        switch (column) {
              case CPU_IDLE:
		return(DIFF_P(devnum, mpc_cpu[MET_CPU_IDLE]) * 100/td[devnum]);
                break;
                
              case CPU_WAIT:
		return(DIFF_P(devnum, mpc_cpu[MET_CPU_WAIT]) * 100/td[devnum]);
                break;
                
              case CPU_USER:
		return(DIFF_P(devnum, mpc_cpu[MET_CPU_USER]) * 100/td[devnum]);
                break;
                
              case CPU_SYS:
		return(DIFF_P(devnum, mpc_cpu[MET_CPU_SYS]) * 100/td[devnum]);
                break;
                
              default:
                sarerrno = SARERR_OUTFIELD;
                return(-1);
                break;
        }
}


/* 
 * Produces per-node output for metric data that may also be reported
 * on a per-processor basis.  Parameters have the same meaning as in
 * item_out().
 */
static sarout_t
nodal_out(int item, int devnum, struct metp_cpu *start, struct metp_cpu *end,
	time_t *td) {
        float  total = 0;
        float  tdifftot = 0;
        int    i;
        switch (item) {
              case CPU_IDLE:
                for (i = 0; i < machinfo.num_engines; i++)
		if (which_node(i) == devnum) {
                        total += DIFF_P(i,mpc_cpu[MET_CPU_IDLE]);
                        tdifftot += td[i];  
                }
                return total * 100/tdifftot;
              case CPU_WAIT:
                for (i = 0; i < machinfo.num_engines; i++)
		if (which_node(i) == devnum) {
                        total += DIFF_P(i,mpc_cpu[MET_CPU_WAIT]);
                        tdifftot += td[i];  
                }
                return total * 100/tdifftot;
              case CPU_USER:
                for (i = 0; i < machinfo.num_engines; i++)
		if (which_node(i) == devnum) {
                        total += DIFF_P(i,mpc_cpu[MET_CPU_USER]);
                        tdifftot += td[i];  
                }
                return total * 100/tdifftot;
              case CPU_SYS:
                for (i = 0; i < machinfo.num_engines; i++)
		if (which_node(i) == devnum) {
                        total += DIFF_P(i,mpc_cpu[MET_CPU_SYS]);
                        tdifftot += td[i];  
                }
                return total * 100/tdifftot;
              default:
                sarerrno = SARERR_OUTFIELD;
                return (sarout_t)-1;
        }
} /* nodal_out() */

/* 
 * Produces system wide output for metric data that may also be reported
 * on a per-processor basis.  Parameters have the same meaning as in
 * item_out()
 */

static sarout_t
total_out(int item, struct metp_cpu *start, struct metp_cpu *end, time_t *td)
{
        float  total = 0;
        float  tdifftot = 0;
        int    i;
        
        switch (item) {
              case CPU_IDLE:
                for (i = 0; i < machinfo.num_engines; i++) {
                        total += DIFF_P(i,mpc_cpu[MET_CPU_IDLE]);
                        tdifftot += td[i];  
                }
                return(total * 100/tdifftot);
                break;
                
              case CPU_WAIT:
                for (i = 0; i < machinfo.num_engines; i++) {
                        total += DIFF_P(i,mpc_cpu[MET_CPU_WAIT]);
                        tdifftot += td[i];  
                }
                return(total * 100/tdifftot);
                break;
                
              case CPU_USER:
                for (i = 0; i < machinfo.num_engines; i++) {
                        total += DIFF_P(i,mpc_cpu[MET_CPU_USER]);
                        tdifftot += td[i];  
                }
                return(total * 100/tdifftot);
                break;
                
              case CPU_SYS:
                for (i = 0; i < machinfo.num_engines; i++) {
                        total += DIFF_P(i,mpc_cpu[MET_CPU_SYS]);
                        tdifftot += td[i];  
                }
                return(total * 100/tdifftot);
                break;
                
              default:
                sarerrno = SARERR_OUTFIELD;
                return(-1);
                break;
        }
}


static sarout_t
raw_out(int item, int devnum, struct metp_cpu *start, struct metp_cpu *end)
{
	switch (item) {	
	      case CPU_IDLE:
		return(DIFF_P(devnum, mpc_cpu[MET_CPU_IDLE]));
		break;

	      case CPU_WAIT:
		return(DIFF_P(devnum, mpc_cpu[MET_CPU_WAIT]));
		break;

	      case CPU_USER:
		return(DIFF_P(devnum, mpc_cpu[MET_CPU_USER]));
		break;

	      case CPU_SYS:
		return(DIFF_P(devnum, mpc_cpu[MET_CPU_SYS]));
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


static sarout_t
nodal_raw_out(int item, int devnum, struct metp_cpu *start,
	struct metp_cpu *end) {
	sarout_t	answer;

	switch (item) {	
	      case CPU_IDLE:
		SUM_DIFF_N(devnum, answer, mpc_cpu[MET_CPU_IDLE]);
		return answer;
	      case CPU_WAIT:
		SUM_DIFF_N(devnum, answer, mpc_cpu[MET_CPU_WAIT]);
		return answer;
	      case CPU_USER:
		SUM_DIFF_N(devnum, answer, mpc_cpu[MET_CPU_USER]);
		return answer;
	      case CPU_SYS:
		SUM_DIFF_N(devnum, answer, mpc_cpu[MET_CPU_SYS]);
		return answer;
	      default:
		sarerrno = SARERR_OUTFIELD;
		return (sarout_t)-1;
	}
} /* nodal_raw_out() */



static sarout_t
total_raw_out(int item, struct metp_cpu *start, struct metp_cpu *end)
{
	sarout_t	answer;

	switch (item) {	
	      case CPU_IDLE:
		SUM_DIFF_P(answer, mpc_cpu[MET_CPU_IDLE]);
		return(answer);
		break;

	      case CPU_WAIT:
		SUM_DIFF_P(answer, mpc_cpu[MET_CPU_WAIT]);
		return(answer);
		break;

	      case CPU_USER:
		SUM_DIFF_P(answer, mpc_cpu[MET_CPU_USER]);
		return(answer);
		break;

	      case CPU_SYS:
		SUM_DIFF_P(answer, mpc_cpu[MET_CPU_SYS]);
		return(answer);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


void
sar_cpu_cleanup(void)
{
        free(tdiff);
        free(total_tdiff);
        free(cpu);
        free(old_cpu);
        free(first_cpu);

	tdiff = NULL;
	total_tdiff = NULL;
	cpu = NULL;
	old_cpu = NULL;
	first_cpu = NULL;
}
