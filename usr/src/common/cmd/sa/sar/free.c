/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/free.c	1.8"
#ident	"$Header$"

/* free.c
 * 
 * free memory metrics.  Processes SAR_MEM records.
 *
 */

#include <stdio.h>
#include <time.h>
#include "../sa.h"
#include "sar.h"


static struct mets_mem      mem;
static struct mets_mem      old_mem;
static struct mets_mem      first_mem;

static int  first_sample = TRUE;


flag
sar_mem_init(void)
{
	mem.msm_freemem = lzero;
	mem.msm_freeswap = lone;
	old_mem = mem;
	first_sample = TRUE;
	
	return(TRUE);
}


/*ARGSUSED*/

int
sar_mem(FILE *infile, sar_header sarh, flag32 of)
{
	if (sarh.item_size != sizeof(struct mets_mem)) {
		metric_warn[sarh.item_code] = TRUE;
	}

	old_mem = mem;
	get_item(&mem, sizeof(struct mets_mem), sarh.item_size, infile);
	if (first_sample == TRUE) {
		first_mem = mem;
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;
	
	return(TRUE);
}


/*ARGSUSED*/

sarout_t
sar_mem_out(int column, int mode, int devnum)
{
	dl_t  		answer;
	dl_t		denom;
	dl_t		hz;
	struct mets_mem	start;
	
	sarerrno = 0;
	denom.dl_hop = 0;

	if (mode == OUTPUT_DATA || mode == OUTPUT_TOTAL) {
		start = old_mem;
		denom.dl_lop = (long) tdiff_max/
			       (long) machinfo.mets_native_units.mnu_hz;
	}
	else if (mode == OUTPUT_RAW || mode == OUTPUT_RAW_TOTAL) {
		start = old_mem;
		denom.dl_lop = 1;
	}
	else if (mode == OUTPUT_FINAL_DATA || mode == OUTPUT_FINAL_TOTAL) {
		start = first_mem;
		denom.dl_lop = (long) total_tdiff_max/
		               (long) machinfo.mets_native_units.mnu_hz;
	}
	else if (mode == OUTPUT_FINAL_RAW || mode == OUTPUT_FINAL_RAW_TOTAL) {
		start = first_mem;
		denom.dl_lop = 1;
	}
	else {
		sarerrno = SARERR_OUTMODE;
		return(-1);
	}
			
	switch (column) {
	      case MEM_FREEMEM:
		answer = lsub(mem.msm_freemem, start.msm_freemem);
		answer = ldivide(answer, denom);
		return(answer.dl_lop);
		break;
			
	      case MEM_FREESWAP:
		answer = lsub(mem.msm_freeswap, start.msm_freeswap);
		answer = ldivide(answer, denom);
		return(answer.dl_lop);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}







