/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/flook.c	1.6"
#ident "$Header$"


/* flook.c
 *
 * file lookup metrics.  Processes SAR_FS_LOOKUP_P records.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"

static struct metp_filelookup    *flook = NULL;
static struct metp_filelookup    *old_flook = NULL;
static struct metp_filelookup    *first_flook = NULL;
static struct flook_info         temp;

static int  first_sample = TRUE;

flag
sar_flook_init(void)
{
	int      i;
	
	if (flook == NULL) {
		flook = malloc(machinfo.num_engines * sizeof(*flook));
		old_flook = malloc(machinfo.num_engines * sizeof(*old_flook));
		first_flook = malloc(machinfo.num_engines * sizeof(*first_flook));
		
		if (flook == NULL || old_flook == NULL || first_flook == NULL) {
			return(FALSE);
		}
	}
	
	for (i = 0; i < machinfo.num_engines ; i++) {
		flook[i].mpf_lookup = 0;
		flook[i].mpf_dnlc_hits = 0;
		flook[i].mpf_dnlc_misses = 0;
		
		old_flook[i] = flook[i];
	}
	first_sample = TRUE;
	
	return(TRUE);
}


/*ARGSUSED*/

int
sar_flook_p(FILE *infile, sar_header sarh, flag32 of)
{
	int      i;
	int      p;

	if (sarh.item_size != sizeof(struct flook_info)) {
		metric_warn[sarh.item_code] = TRUE;
	}
	
	memcpy(old_flook, flook, machinfo.num_engines * sizeof(*old_flook));
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(struct flook_info), sarh.item_size, infile);
		p = temp.id;
		flook[p] = temp.data;
	}
	
	if (first_sample == TRUE) {
		memcpy(first_flook, flook, machinfo.num_engines * sizeof(*first_flook));
		first_sample = FALSE;
	}
	collected[sarh.item_code] = TRUE;
	
	return(TRUE);
}

sarout_t
sar_flook_out(int column, int mode, int devnum)
{
	struct metp_filelookup	*start;
	struct metp_filelookup	*end; 
	time_t		      	*td;
	sarout_t		answer;
	sarout_t		hits;
	sarout_t		misses;

	SET_INTERVAL(mode, flook, old_flook, first_flook);

	switch (column) {
	      case FLOOK_LOOKUP:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpf_lookup);
		return(answer);
		break;

	      case FLOOK_DNLC_PERCENT:
		GENERIC_DIFF(hits, mode, devnum, mpf_dnlc_hits);
		GENERIC_DIFF(misses, mode, devnum, mpf_dnlc_misses);
		if (hits + misses == 0) {
			return(100);
		}
		else {
			return(100 * hits/(hits + misses));
		}
		break;

	      case FLOOK_DNLC_HITS:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpf_dnlc_hits);
		return(answer);
		break;

	      case FLOOK_DNLC_MISSES:
		COMPUTE_GENERIC_TIME(answer, mode, devnum, mpf_dnlc_misses);
		return(answer);
		break;

	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


void
sar_flook_cleanup(void)
{
	free(flook);
	free(old_flook);
	free(first_flook);

	flook = NULL;
	old_flook = NULL;
	first_flook = NULL;
}
