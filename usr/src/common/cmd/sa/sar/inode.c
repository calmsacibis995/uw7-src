/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/inode.c	1.7"
#ident	"$Header$"

/* inode.c
 * 
 * inode metrics.  Processes SAR_FS_INODE records 
 */

#include <stdio.h>
#include <malloc.h>
#include "../sa.h"
#include "sar.h"

static struct mets_inodes     *in = NULL;
static struct mets_inodes     *old_in = NULL;

/* Node: acc_in, stores accumulated INUSE, SIZE, and and CURRENT
 *  values, but accumulated DIFFERENCES for FAIL.
 */

static struct mets_inodes     *acc_in = NULL;
static struct inodes_info     inodes_info;

static flag    first_sample = TRUE;
static int     num_samples = 0;


flag
sar_inodes_init(void)
{
	int   i;
	int   j;
	
	if (in == NULL) {
		in = malloc(machinfo.num_fs * sizeof(*in));
		old_in = malloc(machinfo.num_fs * sizeof(*old_in));
		acc_in = malloc(machinfo.num_fs * sizeof(*acc_in));
		
		if (in == NULL || old_in == NULL || acc_in == NULL) {
			return(FALSE);
		}
	}
	
	for (j = 0; j < machinfo.num_fs; j++) {
		for (i = MET_FAIL; i <= MET_CURRENT; i++) {
			in[j].msi_inodes[i] = 0;
		}
		old_in[j] = in[j];
		acc_in[j] = in[j];
	}
	
	first_sample = TRUE;
	num_samples = 0;
	
	return(TRUE);
}


/*ARGSUSED*/

int
sar_inodes(FILE *infile, sar_header sarh, flag32 of)
{
	int   i;
	int   j;
	int   id;

	if (sarh.item_size != sizeof(struct inodes_info)) {
		metric_warn[sarh.item_code] = TRUE;
	}
	
	for (i = 0; i < machinfo.num_fs; i++) {
		old_in[i] = in[i];
	}
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&inodes_info, sizeof(inodes_info), sarh.item_size, infile);
		id = inodes_info.id;
		in[id] = inodes_info.data;
		if (first_sample == FALSE) {
			for (j = MET_INUSE; j <= MET_CURRENT; j++) {
				acc_in[id].msi_inodes[j] += in[id].msi_inodes[j];
			}
			acc_in[id].msi_inodes[MET_FAIL] += in[id].msi_inodes[MET_FAIL] - 
			old_in[id].msi_inodes[MET_FAIL];
		}
	}

	if (first_sample == FALSE) {
		num_samples++;
	}
	else {
		first_sample = FALSE;
	}

	collected[sarh.item_code] = TRUE;
	
	
	return(TRUE);
}


/*ARGSUSED*/

sarout_t
sar_inodes_out(int column, int mode, int devnum)
{
	int      total = 0;

	struct mets_inodes	*start = old_in;
	struct mets_inodes	*end = in;
	
	
	if (mode == OUTPUT_DATA || mode == OUTPUT_RAW) {
		switch (column) {
		      case INODE_FAIL:
			return(DIFF_FS(devnum, msi_inodes[MET_FAIL]));
			break;
			
		      case INODE_INUSE:
			return(end[devnum].msi_inodes[MET_INUSE]);
			break;

		      case INODE_MAX:
			return(end[devnum].msi_inodes[MET_MAX]);
			break;

		      case INODE_CURRENT:
			return(end[devnum].msi_inodes[MET_CURRENT]);
			break;
		}
	}
	else if (mode == OUTPUT_TOTAL || mode == OUTPUT_RAW_TOTAL) {
		switch(column) {
		      case INODE_FAIL:
			SUM_DIFF_FS(total, msi_inodes[MET_FAIL]);
			return(total);
			break;

		      case INODE_INUSE:
			SUM_FS(total, msi_inodes[MET_INUSE]);
			return(total);
			break;

		      case INODE_MAX:
			SUM_FS(total, msi_inodes[MET_MAX]);
			return(total);
			break;

		      case INODE_CURRENT:
			SUM_FS(total, msi_inodes[MET_CURRENT]);
			return(total);
			break;
			
		      default:
			sarerrno = SARERR_OUTFIELD;
			return(-1);
			break;
		}
	}
	else if (mode == OUTPUT_FINAL_DATA || mode == OUTPUT_FINAL_RAW) {
		switch (column) {
		      case INODE_FAIL:
			return(acc_in[devnum].msi_inodes[MET_FAIL]/num_samples);
		      case INODE_INUSE:
			return(acc_in[devnum].msi_inodes[MET_INUSE]/num_samples);
		      case INODE_MAX:
			return(acc_in[devnum].msi_inodes[MET_MAX]/num_samples);
		      case INODE_CURRENT:
			return(acc_in[devnum].msi_inodes[MET_CURRENT]/num_samples);
		      default:
			sarerrno = SARERR_OUTFIELD;
			return(-1);
			break;
		}
	}
	else if (mode == OUTPUT_FINAL_TOTAL || 
		 mode == OUTPUT_FINAL_RAW_TOTAL) {
		end = acc_in;

		switch (column) {
		      case INODE_FAIL:
			SUM_FS(total, msi_inodes[MET_FAIL]);
			return(total/num_samples);
			break;

		      case INODE_INUSE:
			SUM_FS(total, msi_inodes[MET_INUSE]);
			return(total/num_samples);
			break;

		      case INODE_MAX:
			SUM_FS(total, msi_inodes[MET_MAX]);
			return(total/num_samples);
			break;

		      case INODE_CURRENT:
			SUM_FS(total, msi_inodes[MET_CURRENT]);
			return(total/num_samples);
			break;

		      default:
			sarerrno = SARERR_OUTFIELD;
			return(-1);
			break;
		}
	}
	else {
		sarerrno = SARERR_OUTMODE;
		return(-1);
	}
}


void
sar_inodes_cleanup(void)
{
	free(in);
	free(old_in);
	free(acc_in);

	in = NULL;
	old_in = NULL;
	acc_in = NULL;
}
