/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/bufcache.c	1.6"
#ident "$Header$"


/* 
 * bufcache.c 
 * 
 * buffer cache metrics.  Processes SAR_BUFFER_P records 
 */


#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"

static struct metp_buf  *bi = NULL;
static struct metp_buf  *old_bi = NULL;
static struct metp_buf  *first_bi = NULL;

static struct metp_buf  total_bi;
static struct metp_buf	acc_bi;

static struct buf_info	temp;

static int  first_sample = TRUE;


#define COMPUTE_CACHE(diffb, diffl)  (((diffl) > 0) ? (100 - 100 * (diffb)/(diffl)) : 100)



flag
sar_cache_init(void)
{
	int      i;
	
	if (bi == NULL) {
		bi = malloc(machinfo.num_engines * sizeof(*bi));
		old_bi = malloc(machinfo.num_engines * sizeof(*old_bi));
		first_bi = malloc(machinfo.num_engines * sizeof(*first_bi));
		
		if (bi == NULL || old_bi == NULL || first_bi == NULL) {
			return(FALSE);
		}
	}
	
	for (i = 0; i < machinfo.num_engines; i++) {
		bi[i].mpb_bread = 0;
		bi[i].mpb_bwrite = 0;
		bi[i].mpb_lread = 0;
		bi[i].mpb_lwrite = 0;
		bi[i].mpb_phread = 0;
		bi[i].mpb_phwrite = 0;
		
		old_bi[i] = bi[i];
	}
	total_bi = bi[0];
	acc_bi = bi[0];
	
	first_sample = TRUE;
	
	return(TRUE);
}


/*ARGSUSED*/

int
sar_cache(FILE *infile, sar_header sarh, flag32 of)
{
	int      i;
	int      p;
	
	if (sarh.item_size != sizeof(struct buf_info)) {
		metric_warn[sarh.item_code] = TRUE;
	}
	
	memcpy(old_bi, bi, machinfo.num_engines * sizeof(*old_bi));
	
	total_bi.mpb_bread = 0;
	total_bi.mpb_bwrite = 0;
	total_bi.mpb_lread = 0;
	total_bi.mpb_lwrite = 0;
	total_bi.mpb_phread = 0;
	total_bi.mpb_phwrite = 0;
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(struct buf_info), sarh.item_size, infile);
		p = temp.id;
		bi[p] = temp.data;

		if (first_sample == TRUE) {
			continue;
		}

		total_bi.mpb_bread += bi[p].mpb_bread - old_bi[p].mpb_bread;
		total_bi.mpb_bwrite += bi[p].mpb_bwrite - old_bi[p].mpb_bwrite;
		total_bi.mpb_lread += bi[p].mpb_lread - old_bi[p].mpb_lread;
		total_bi.mpb_lwrite += bi[p].mpb_lwrite - old_bi[p].mpb_lwrite;
		total_bi.mpb_phread += bi[p].mpb_phread - old_bi[p].mpb_phread;
		total_bi.mpb_phwrite += bi[p].mpb_phwrite - old_bi[p].mpb_phwrite;
	}

	
	if (first_sample == TRUE) {
		memcpy(first_bi, bi, machinfo.num_engines * sizeof(*first_bi));
		first_sample = FALSE;
	}
	else {
		acc_bi.mpb_bread += total_bi.mpb_bread;
		acc_bi.mpb_bwrite += total_bi.mpb_bwrite;
		acc_bi.mpb_lread += total_bi.mpb_lread;
		acc_bi.mpb_lwrite += total_bi.mpb_lwrite;
		acc_bi.mpb_phread += total_bi.mpb_phread;
		acc_bi.mpb_phwrite += total_bi.mpb_phwrite;
	}

	collected[sarh.item_code] = TRUE;
	
	return(TRUE);
}


sarout_t
sar_cache_out(int column, int mode, int devnum)
{
	struct metp_buf *start;
	struct metp_buf *end;
	time_t		*td;
	sarout_t	 answer;
	int		diffb;
	int		diffl;

	SET_INTERVAL(mode, bi, old_bi, first_bi);

	switch (column) {
	      case CACHE_BREAD:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpb_bread);
		return(answer);
		break;

	      case CACHE_LREAD:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpb_lread);
		return(answer);
		break;

	      case CACHE_RCACHE:
		GENERIC_DIFF(diffb, mode, devnum, mpb_bread);
		GENERIC_DIFF(diffl, mode, devnum, mpb_lread);
		return(COMPUTE_CACHE(diffb, diffl));
		break;

	      case CACHE_WCACHE:
		GENERIC_DIFF(diffb, mode, devnum, mpb_bwrite);
		GENERIC_DIFF(diffl, mode, devnum, mpb_lwrite);
		return(COMPUTE_CACHE(diffb, diffl));
		break;

	      case CACHE_BWRITE:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpb_bwrite);
		return(answer);
		break;

	      case CACHE_LWRITE:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpb_lwrite);
		return(answer);
		break;

	      case CACHE_PREAD:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpb_phread);
		return(answer);
		break;

	      case CACHE_PWRITE:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpb_phwrite);
		return(answer);
		break;
		
	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


void
sar_cache_cleanup(void)
{
	free(bi);
	free(old_bi);
	free(first_bi);

	bi = NULL;
	old_bi = NULL;
	first_bi = NULL;
}

