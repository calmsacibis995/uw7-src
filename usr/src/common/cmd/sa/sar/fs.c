/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/fs.c	1.7"
#ident	"$Header$"

/*
 * fs.c
 *
 * file system table metrics.  Processes SAR_FS_TABLE records.
 */

#include <stdio.h>
#include "../sa.h"
#include "sar.h"

static struct mets_files   fs;
static struct mets_files   old_fs;
static struct mets_files   first_fs;
static struct mets_files   acc_fs;

static flag    first_sample = TRUE;
static int     num_samples;

flag
sar_fs_init(void)
{
	int   i;
	
	for (i = MET_FAIL; i <= MET_MAX; i++) {
		fs.msf_file[i] = 0;
		fs.msf_flck[i] = 0;
	}
	old_fs = fs;
	acc_fs = fs;
	first_sample = TRUE;
	num_samples = 0;
	
	return(TRUE);
}


/*ARGSUSED*/

int
sar_fs(FILE *infile, sar_header sarh, flag32 of)
{
	int   i;

	if (sarh.item_size != sizeof(struct mets_files)) {
		metric_warn[sarh.item_code] = TRUE;
	}
	
	old_fs = fs;
	
	get_item(&fs, sizeof(fs), sarh.item_size, infile);
	if (first_sample == TRUE) {
		first_fs = fs;
		first_sample = FALSE;
	}
	else {
		for (i = MET_FAIL; i <= MET_INUSE; i++) {
			acc_fs.msf_file[i] += fs.msf_file[i];
			acc_fs.msf_flck[i] += fs.msf_flck[i];
		}
		num_samples++;
	}
	collected[sarh.item_code] = TRUE;
	
	return(TRUE);
}

/*ARGSUSED*/

sarout_t
sar_fs_out(int column, int mode, int devnum)
{
	sarerrno = 0;
	
	if (mode == OUTPUT_DATA || mode == OUTPUT_TOTAL
	                        || mode == OUTPUT_RAW
	                        || mode == OUTPUT_RAW_TOTAL) {
		switch (column) {
		      case FS_FILEUSE:
			return(fs.msf_file[MET_INUSE]);
			break;
			
		      case FS_FILEFAIL:
			return(fs.msf_file[MET_FAIL] - old_fs.msf_file[MET_FAIL]);
			break;
			
		      case FS_FILEMAX:
			sarerrno = SARERR_OUTBLANK;
			return(-1);
			break;
			
		      case FS_FLCKUSE:
			return(fs.msf_flck[MET_INUSE]);
			break;

		      case FS_FLCKMAX:
			sarerrno = SARERR_OUTBLANK;
			return(-1);
			break;
			
		      default:
			sarerrno = SARERR_OUTFIELD;
			return(-1);
			break;
		}
	}
	
	if (mode == OUTPUT_FINAL_DATA || mode == OUTPUT_FINAL_TOTAL
	                              || mode == OUTPUT_FINAL_RAW
	                              || mode == OUTPUT_FINAL_RAW_TOTAL) {
		int	denom;

		if (mode == OUTPUT_FINAL_DATA || mode == OUTPUT_FINAL_TOTAL) {
			denom = num_samples;
		}
		else {
			denom = 1;
		}

		switch (column) {
		      case FS_FILEUSE:
			return(acc_fs.msf_file[MET_INUSE]/denom);
			break;
			
		      case FS_FILEFAIL:
			return((fs.msf_file[MET_FAIL] - first_fs.msf_file[MET_FAIL])/denom);
			break;

		      case FS_FILEMAX:
			sarerrno = SARERR_OUTBLANK;
			return(-1);
			break;

		      case FS_FLCKUSE:
			return(acc_fs.msf_flck[MET_INUSE]/denom);
			break;

		      case FS_FLCKMAX:
			sarerrno = SARERR_OUTBLANK;
			return(-1);
			break;
			
		      default:
			sarerrno = SARERR_OUTFIELD;
			return(-1);
			break;
		}
	}
	
	sarerrno = SARERR_OUTMODE;
	return(-1);
}
