/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/facc.c	1.6.1.3"
#ident "$Header$"


/* facc.c
 *
 * file access metrics.  Processes SAR_FS_ACCESS_P records.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"

/* 
 * make these look like 2D arrays
 *
 */

#define FACC(p,f)       (facc[(p) * machinfo.num_fs + f])
#define OLD_FACC(p,f)   (old_facc[(p) * machinfo.num_fs + f])
#define FIRST_FACC(p, f)   (first_facc[(p) * machinfo.num_fs + f])
#define START(p, f)   (start[(p) * machinfo.num_fs + f])
#define END(p, f)   (end[(p) * machinfo.num_fs + f])


static struct metp_fileaccess    *facc = NULL;
static struct metp_fileaccess    *old_facc = NULL;
static struct metp_fileaccess    *first_facc = NULL;
static struct facc_info          temp;

static flag first_sample = TRUE;

static sarout_t  item_out(int item, 
                     int devnum,
                     struct metp_fileaccess *start_interval,
                     struct metp_fileaccess *end_interval,
                     time_t *tick_difference);

static sarout_t  nodal_out(int item, int devnum,
                      struct metp_fileaccess *start_interval,
                      struct metp_fileaccess *end_interval,
                      time_t *tick_difference);

static sarout_t  total_out(int item,
                      struct metp_fileaccess *start_interval,
                      struct metp_fileaccess *end_interval,
                      time_t *tick_difference);

static sarout_t  raw_out(int item, 
		    int devnum,
		    struct metp_fileaccess *start_interval,
		    struct metp_fileaccess *end_interval);

static sarout_t  nodal_raw_out(int item, int devnum,
                      struct metp_fileaccess *start_interval,
                      struct metp_fileaccess *end_interval,
		      time_t                 *tick_difference);

static sarout_t  total_raw_out(int item,
			  struct metp_fileaccess *start_interval,
			  struct metp_fileaccess *end_interval,
			  time_t		 *tick_difference);



flag
sar_facc_init(void)
{
	int      i;
	int      j;
	
	if (facc == NULL) {
		facc = malloc(machinfo.num_engines * machinfo.num_fs * sizeof(*facc));
		old_facc = malloc(machinfo.num_engines * machinfo.num_fs * sizeof(*old_facc));
		first_facc = malloc(machinfo.num_engines * machinfo.num_fs * sizeof(*first_facc));
		if (facc == NULL || old_facc == NULL || first_facc == NULL) {
			return(FALSE);
		}
	}
	
	
	for (i = 0; i < machinfo.num_engines; i++) {
		for (j = 0; j < machinfo.num_fs; j++) {
			FACC(i,j).mpf_iget = 0;
			FACC(i,j).mpf_iget = 0;
			FACC(i,j).mpf_dirblk = 0;
			FACC(i,j).mpf_ipage = 0;
			FACC(i,j).mpf_inopage = 0;
			
			OLD_FACC(i,j) = FACC(i,j);
		}
	}
	
	first_sample = TRUE;
	
	return(0);
}


/*ARGSUSED*/

int 
sar_facc_p(FILE *infile, sar_header sarh, flag32 of)
{
	int      i;
	
	if (sarh.item_size != sizeof(struct facc_info)) {
		metric_warn[sarh.item_code] = TRUE;
	}

	memcpy(old_facc, facc, 
	       machinfo.num_engines * machinfo.num_fs * sizeof(*old_facc));

	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(temp), sarh.item_size, infile);
		
		FACC(temp.id1,temp.id2) = temp.data;
	}
	if (first_sample == TRUE) {
		memcpy(first_facc, facc, 
		       machinfo.num_engines * machinfo.num_fs * sizeof(*old_facc));
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;

	return(TRUE);
}


sarout_t
sar_facc_out(int column, int mode, int devnum)
{
	switch (mode) {   
	      case OUTPUT_DATA:
		return(item_out(column, devnum, old_facc, facc, tdiff));
		break;
		
	      case OUTPUT_DATA|NODAL:
		return nodal_out(column, devnum, old_facc, facc, tdiff);
		break;

	      case OUTPUT_TOTAL:
		return(total_out(column, old_facc, facc, tdiff));
		break;
		
	      case OUTPUT_FINAL_DATA:
		return(item_out(column, devnum, first_facc, facc, total_tdiff));
		break;
		
	      case OUTPUT_FINAL_DATA|NODAL:
		return nodal_out(column, devnum, old_facc, facc, total_tdiff);
		break;

	      case OUTPUT_FINAL_TOTAL:
		return(total_out(column, first_facc, facc, total_tdiff));
		break;

	      case OUTPUT_RAW:
		return(raw_out(column, devnum, old_facc, facc));
		break;
		
	      case OUTPUT_RAW|NODAL:
		return nodal_raw_out(column, devnum, old_facc, facc, tdiff);
		break;

	      case OUTPUT_RAW_TOTAL:
		return(total_raw_out(column, old_facc, facc, tdiff));
		break;
		
	      case OUTPUT_FINAL_RAW:
		return(raw_out(column, devnum, first_facc, facc));
		break;
		
	      case OUTPUT_FINAL_RAW|NODAL:
		return nodal_raw_out(column, devnum, first_facc, facc,
			total_tdiff);
		break;

	      case OUTPUT_FINAL_RAW_TOTAL:
		return(total_raw_out(column, first_facc, facc, total_tdiff));
		break;
		
	      default:
		sarerrno = SARERR_OUTMODE;
		return(-1);
		break;
	}
}


static sarout_t
item_out(int item, 
         int devnum,
         struct metp_fileaccess *start, 
         struct metp_fileaccess *end,
         time_t *td)
{
	int   total = 0;
	int   total2 = 0;
	int   old_total = 0;
	int   i;
	
	switch (item) {
	      case FACC_IGET:
		for (i = 0; i < machinfo.num_fs; i++) {
			total += END(devnum,i).mpf_iget;
			old_total += START(devnum,i).mpf_iget;
		}
		return(DO_DIFF_TIME(old_total, total, td[devnum]));
		break;
		
	      case FACC_DIRBLK:
		for (i = 0; i < machinfo.num_fs; i++) {
			total += END(devnum,i).mpf_dirblk;
			old_total += START(devnum,i).mpf_dirblk;
		}
		return(DO_DIFF_TIME(old_total, total, td[devnum]));
		break;

	      case FACC_IPF:	/* devnum = filesystem to report on*/
		for (i = 0; i < machinfo.num_engines; i++) {
			total += END(i,devnum).mpf_ipage - START(i,devnum).mpf_ipage;
			total2 += END(i,devnum).mpf_inopage - START(i,devnum).mpf_inopage;
		}

		if (total + total2 == 0) {
			return(100);
		}
		else {
			return((total * 100)/(total + total2));
		}
		
	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}

static sarout_t
nodal_out(int item, int devnum, struct metp_fileaccess *start, 
          struct metp_fileaccess *end, time_t *td) {
	int   total = 0;
	int   devtot = 0;
	int   old_devtot = 0;
	int   i;
	int   j;
	
	switch (item) {
	      case FACC_IGET:
		for (i = 0; i < machinfo.num_engines; i++) {
			if (td[i] == 0 || which_node(i) != devnum)
				continue;
			devtot = 0;
			old_devtot = 0;
			for (j = 0; j < machinfo.num_fs; j++) {
				devtot += END(i,j).mpf_iget;
				old_devtot += START(i,j).mpf_iget;
			}
			total += DO_DIFF_TIME(old_devtot, devtot, td[i]);
		}
		return total;
	      case FACC_DIRBLK:   
		for (i = 0; i < machinfo.num_engines; i++) {
			if (td[i] == 0 || which_node(i) != devnum)
				continue;
			devtot = 0;
			old_devtot = 0;
			for (j = 0; j < machinfo.num_fs; j++) {
				devtot += END(i,j).mpf_dirblk;
				old_devtot += START(i,j).mpf_dirblk;
			}
			total += DO_DIFF_TIME(old_devtot, devtot, td[i]);
		}
		return total;
	      default:
		sarerrno = SARERR_OUTFIELD;
		return (sarout_t)-1;
	}
} /* nodal_out() */


static sarout_t
total_out(int item, 
          struct metp_fileaccess *start, 
          struct metp_fileaccess *end,
          time_t *td)
{
	
	int   total = 0;
	int   devtot = 0;
	int   old_devtot = 0;
	int   i;
	int   j;
	
	switch (item) {
	      case FACC_IGET:
		for (i = 0; i < machinfo.num_engines; i++) {
			if (td[i] == 0) {
				continue;
			}
			
			devtot = 0;
			old_devtot = 0;
			
			for (j = 0; j < machinfo.num_fs; j++) {
				devtot += END(i,j).mpf_iget;
				old_devtot += START(i,j).mpf_iget;
			}
			total += DO_DIFF_TIME(old_devtot, devtot, td[i]);
		}
		return(total);
		break;
		
	      case FACC_DIRBLK:   
		for (i = 0; i < machinfo.num_engines; i++) {
			if (td[i] == 0) {
				continue;
			}
			
			devtot = 0;
			old_devtot = 0;
			for (j = 0; j < machinfo.num_fs; j++) {
				devtot += END(i,j).mpf_dirblk;
				old_devtot += START(i,j).mpf_dirblk;
			}
			total += DO_DIFF_TIME(old_devtot, devtot, td[i]);
		}
		return(total);
		break;
		
	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


static sarout_t
raw_out(int item,
	int devnum,
	struct metp_fileaccess *start,
	struct metp_fileaccess *end)
{
	int	i;
	int	total = 0;

	switch (item) {
	      case FACC_IGET:
		for (i = 0; i < machinfo.num_fs; i++) {
			total += END(devnum, i).mpf_iget - 
			         START(devnum, i).mpf_iget;
		}
		return(total);
		break;

	      case FACC_DIRBLK:
		for (i = 0; i < machinfo.num_fs; i++) {
			total += END(devnum, i).mpf_dirblk - 
			         START(devnum, i).mpf_dirblk;
		}
		return(total);
		break;

	      case FACC_IPAGE:
		for (i = 0; i < machinfo.num_engines; i++) {
			total += END(i, devnum).mpf_ipage - 
			         START(i, devnum).mpf_ipage;
		}
		return(total);
		break;

	      case FACC_INOPAGE:
		for (i = 0; i < machinfo.num_engines; i++) {
			total += END(i, devnum).mpf_inopage - 
			         START(i, devnum).mpf_inopage;
		}
		return(total);
		break;
	}
}


static sarout_t  
nodal_raw_out(int item, int devnum, struct metp_fileaccess *start,
	      struct metp_fileaccess *end, time_t *td) {
	int	total = 0;
	int	i;
	int	j;
	switch (item) {
	      case FACC_IGET:
		for (i = 0; i < machinfo.num_engines; i++) {
			if (td[i] == 0 || which_node(i) != devnum)
				continue;
			for (j = 0; j < machinfo.num_fs; j++) {
				total += END(i,j).mpf_iget - 
				         START(i,j).mpf_iget;
			}
		}
		return total;
	      case FACC_DIRBLK:
		for (i = 0; i < machinfo.num_engines; i++) {
			if (td[i] == 0 || which_node(i) != devnum)
				continue;
			for (j = 0; j < machinfo.num_fs; j++) {
				total += END(i,j).mpf_dirblk - 
				         START(i,j).mpf_dirblk;
			}
		}
		return total;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return (sarout_t)-1;
	}
} /* nodal_raw_out() */


static sarout_t  
total_raw_out(int item,
	      struct metp_fileaccess *start,
	      struct metp_fileaccess *end,
	      time_t *td)
{
	int	total = 0;
	int	i;
	int	j;

	switch (item) {
	      case FACC_IGET:
		for (i = 0; i < machinfo.num_engines; i++) {
			if (td[i] == 0) {
				continue;
			}
			
			for (j = 0; j < machinfo.num_fs; j++) {
				total += END(i,j).mpf_iget - 
				         START(i,j).mpf_iget;
			}
		}
		return(total);
		break;

	      case FACC_DIRBLK:
		for (i = 0; i < machinfo.num_engines; i++) {
			if (td[i] == 0) {
				continue;
			}

			for (j = 0; j < machinfo.num_fs; j++) {
				total += END(i,j).mpf_dirblk - 
				         START(i,j).mpf_dirblk;
			}
		}
		return(total);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


void
sar_facc_cleanup(void)
{
	free(facc);
	free(old_facc);
	free(first_facc);

	facc = NULL;
	old_facc = NULL;
	first_facc = NULL;
}
