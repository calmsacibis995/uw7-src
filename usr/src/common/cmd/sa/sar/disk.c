/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/disk.c	1.7"
#ident	"$Header$"

/* disk.c 
 *
 * Disk usage metrics.  Processes SAR_DISK records.
 * 
 * Note: The computations for Average values will only
 * be correct if there are no changes in devices present
 * during the run.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <limits.h>
#include "../sa.h"
#include <sys/metdisk.h>

#include "sar.h"

#define	MEM_INCREASE	5	/* increase array sizes by MEM_INCREASE */


/* 
 * extra cast shuts up warning about unsigned long to double conversion
 *
 */

#define DLTOD(x)  ((double) (x).dl_hop * (double) (unsigned long) ULONG_MAX + (double) (x).dl_lop)


/* disk_name maps names to indices in the disk[] array.  Must be
 * visible to the output module
 */

char		**disk_name = NULL;


/*
 * This structure maintains information to determine whether a disk is 
 * online or not and whether two consecutive samples have been read.  This
 * record keeping is complicated by the fact that reading the
 * SAR_DISK records, and producing table output occur at separate
 * times.
 * 
 * read_one is set to TRUE when the first sample for a disk is
 * read.  It is set back to FALSE as soon as a record occurs
 * that does not contain data for that disk (and may later be
 * set to TRUE if data for that disk reappears).  Essentially 
 * if this field is TRUE then disk[index] contains valid data 
 * for the most recent sample.  If read_one is TRUE and read_two
 * is FALSE, and this is not the first record in the file, then
 * the disk has just come on-line.
 * 
 * read_two is set to TRUE whenever two consecutive samples for
 * the disk have been read, i.e. disk[index] is valid data for
 * the current samples, and old_disk[index] is valid data for
 * the previous sample.  
 * 
 * report_offline is set to TRUE when a disk goes off-line, and
 * remains TRUE for only one sample (i.e., it is set to FALSE when
 * the next sample is read).  This happens when read_two is
 * TRUE, and the SAR_DISK record being read does not contain
 * data for this disk (in which case read_one and read_two are
 * both set to FALSE, and report_offline is set to TRUE).  This
 * flag is needed to enable the output function to report when
 * a disk goes off-line.
 *
 * this_sample is set to FALSE at the start of processing a SAR_DISK
 * record, and is set to TRUE for all disks having data present in
 * that record.  After the record is procesed the other flags are
 * adjusted for disks where this_sample is still FALSE.
 */

typedef struct {
	flag	read_one;
	flag	read_two;
	flag	report_offline;
	flag	this_sample;
} ds_info_t;

static ds_info_t	*ds_info = NULL;

static met_disk_stats_t		*disk = NULL;
static met_disk_stats_t		*old_disk = NULL;
static met_disk_stats_t		*first_disk = NULL;
static met_disk_stats_t		temp;

int	num_disks = 0;	/* can't be static, needed by output.c */

static int	first_sample = TRUE;


static void insert(met_disk_stats_t ds);
static void new_disk(met_disk_stats_t ds, int index);


static sarout_t
item_out(int item, 
         int devnum, 
         met_disk_stats_t *start_interval,
         met_disk_stats_t *end_interval,
         time_t tick_diff);

static sarout_t	
raw_out(int item,
	int devnum,
	met_disk_stats_t *start_interval,
	met_disk_stats_t *end_interval);


flag
sar_disk_init(void)
{
	int	i;

/*
 * Unlike many of the other modules, memory is not allocated in
 * the initialization function.  Since the number of disks may
 * change, space is allocated as needed in sar_disk()
 */

	first_sample = TRUE;
	
	num_disks = 0;

	return(TRUE);
}
 

/*ARGSUSED*/

int
sar_disk(FILE *infile, sar_header sarh, flag32 of)
{
	int      i;

	if (disk != NULL) {
		memcpy(old_disk, disk, num_disks * sizeof(*old_disk));
	}

	/* Note: num_disks may be 0 */

	for (i = 0; i < num_disks; i++) {
		ds_info[i].this_sample = FALSE;
		ds_info[i].report_offline = FALSE; /* don't carry this over */
	}
	
	for (i = 0; i < sarh.item_count; i++) {
		get_item(&temp, sizeof(temp), sarh.item_size, infile);
		insert(temp);
	}

	if (first_sample == TRUE) {
		first_disk = malloc(num_disks * sizeof(*first_disk));
		memcpy(first_disk, disk, num_disks * sizeof(*first_disk));
		first_sample = FALSE;
	}

	collected[sarh.item_code] = TRUE;

	for (i = 0; i < num_disks; i++) {
		if (disk_name[i] != NULL && ds_info[i].this_sample == FALSE) {
			/* no data for this disk */
			
			if (ds_info[i].read_one == TRUE) {
				ds_info[i].report_offline = TRUE;
			}
			ds_info[i].read_one = FALSE;
			ds_info[i].read_two = FALSE;
		}
	}
	
	return(TRUE);
}


static void
insert(met_disk_stats_t ds) 
{
	int	i;

	for (i = 0; i < num_disks; i++) {
		if (disk_name[i] == NULL) {
			new_disk(ds, i);
			return;
		}
		
		else if (strcmp(disk_name[i], ds.ds_name) == 0) {
			/* found an existing disk */

			if (ds_info[i].read_one == TRUE) {
				ds_info[i].read_two = TRUE;
			}
			ds_info[i].read_one = TRUE;
			ds_info[i].report_offline = FALSE;
			ds_info[i].this_sample = TRUE;	
			disk[i] = ds;
			return;
		}
	}

	/* not found, and no more room in the table */

	disk = realloc(disk, (num_disks + MEM_INCREASE) * sizeof(*disk));
	old_disk = realloc(old_disk, (num_disks + MEM_INCREASE) * sizeof(*old_disk));
	disk_name = realloc(disk_name, (num_disks + MEM_INCREASE) * sizeof(*disk_name));
	ds_info = realloc(ds_info, (num_disks + MEM_INCREASE) * sizeof(*ds_info));

	if (disk == NULL || old_disk == NULL 
	    		 || disk_name == NULL
			 || ds_info == NULL) {
		sar_error(SAR_MEM);
	}

	for (i = 1; i < MEM_INCREASE; i++) {
		disk_name[num_disks + i] = NULL;
	}

	new_disk(ds, num_disks);
	num_disks += MEM_INCREASE;
}


static void
new_disk(met_disk_stats_t ds, int index)
{
	if ((disk_name[index] = strdup(ds.ds_name)) == NULL) {
		sar_error(SARERR_MEM);
	}
	ds_info[index].read_one = TRUE;
	ds_info[index].read_two = FALSE;
	ds_info[index].report_offline = FALSE;
	ds_info[index].this_sample = TRUE;
	disk[index] = ds;
}



sarout_t
sar_disk_out(int column, int mode, int devnum)
{
	dl_t	temp;
	ulong	temp2;

	switch (mode) {   
	      case OUTPUT_INQUIRE:
		if (devnum < 0 || devnum >= num_disks
		               || disk_name[devnum] == NULL) { 
			/* no disk associated with this index */
			return(INQRES_NODATA);
		}
		else if (ds_info[devnum].report_offline) {
			return(INQRES_OFFLINE);
		}
		else if (ds_info[devnum].read_two) {
			temp = lsub(disk[devnum].ds_active, 
				    old_disk[devnum].ds_active);
			temp2 = disk[devnum].ds_op[0] + disk[devnum].ds_op[1] -
				old_disk[devnum].ds_op[0] -
				old_disk[devnum].ds_op[1];

			if ((temp.dl_hop == 0 && temp.dl_lop == 0) 
			    || temp2 == 0) {
				return(INQRES_NODATA);
			}
			else {
				return(INQRES_DATA);
			}
		}
		else if (ds_info[devnum].read_one) {
			return(INQRES_ONLINE);
		}
		else {
			return(INQRES_NODATA);
		}
		break;

	      case OUTPUT_FINAL_INQUIRE:
		
		if (devnum < 0 || devnum >= num_disks 
		    	       || disk_name[devnum] == NULL) {
			return(INQRES_NODATA);
		}
		else {
			temp = lsub(disk[devnum].ds_active,
				    first_disk[devnum].ds_active);
			temp2 = disk[devnum].ds_op[0] + disk[devnum].ds_op[1] -
				first_disk[devnum].ds_op[0] -
				first_disk[devnum].ds_op[1];

			if ((temp.dl_hop == 0 && temp.dl_lop == 0) 
			    || temp2 == 0) {
				return(INQRES_NODATA);
			}
			else {
				return(INQRES_DATA);
			}
		}

	      case OUTPUT_DATA:
		return(item_out(column, devnum, old_disk, disk, tdiff_max));
		break;

	      case OUTPUT_FINAL_DATA:
		return(item_out(column, devnum, first_disk, disk, total_tdiff_max));
		break;

	      case OUTPUT_RAW:
		return(raw_out(column, devnum, old_disk, disk));
		break;

	      case OUTPUT_FINAL_RAW:
		return(raw_out(column, devnum, first_disk, disk));
		break;
		
	      default:
		sarerrno = SARERR_OUTMODE;
		return(-1);
	}
}



static sarout_t
item_out(int item, 
         int devnum, 
         met_disk_stats_t *start,
         met_disk_stats_t *end,
         time_t td)
{
	dl_t	answer;
	dl_t	temp;
	int	i;
	ulong	total = 0;
	double	danswer;
	double	dtemp;

	switch (item) {
	      case DISK_BUSY:

/* 100 * [(diff * HZ)/(1000000 * tdiff)] = (diff * HZ)/(10000 * tdiff) */

		answer = lsub(end[devnum].ds_active, start[devnum].ds_active);

		temp.dl_hop = 0;
		temp.dl_lop = machinfo.mets_native_units.mnu_hz;
		answer = lmul(answer, temp);

		temp.dl_lop = 10000;
		answer = ldivide(answer, temp);

		temp.dl_lop = td;
		answer = ldivide(answer, temp);

		return(answer.dl_lop);
		break;
		
	      case DISK_AVQUE:
		answer = lsub(end[devnum].ds_resp, start[devnum].ds_resp);
		temp = lsub(end[devnum].ds_active, start[devnum].ds_active);
		danswer = DLTOD(answer);
		dtemp = DLTOD(temp);
		danswer /= dtemp;
		
		return((sarout_t) danswer);
		break;
		
	      case DISK_OPS:
		total = 0;
		for (i = 0; i < MET_DS_OPTYPES; i++) {
			total += end[devnum].ds_op[i] - start[devnum].ds_op[i];
		}
		return((total * machinfo.mets_native_units.mnu_hz)/td);
		break;
		
	      case DISK_BLOCKS:
		total = 0;
		for (i = 0; i < MET_DS_OPTYPES; i++) {
			total += end[devnum].ds_opblks[i] - start[devnum].ds_opblks[i];
		}
		return((total * machinfo.mets_native_units.mnu_hz)/td);
		break;
		
	      case DISK_AVWAIT:
		answer = lsub(end[devnum].ds_resp, start[devnum].ds_resp);
		answer = lsub(answer, end[devnum].ds_active);
		answer = ladd(answer, start[devnum].ds_active);

		temp.dl_hop = 0;
		temp.dl_lop = 0;
		for (i = 0; i < MET_DS_OPTYPES; i++) {
			temp.dl_lop += end[devnum].ds_op[i] - start[devnum].ds_op[i];
		}

		danswer = DLTOD(answer);
		dtemp = DLTOD(temp);

		danswer /= (dtemp * 1000.0);
		return((sarout_t) danswer);

		break;
		
	      case DISK_AVSERV:
		answer = lsub(end[devnum].ds_active, start[devnum].ds_active);

		temp.dl_hop = 0;
		temp.dl_lop = 0;
		for (i = 0; i < MET_DS_OPTYPES; i++) {
			temp.dl_lop += end[devnum].ds_op[i] - start[devnum].ds_op[i];
		}

		danswer = DLTOD(answer);
		dtemp = DLTOD(temp);

		danswer /= (dtemp * 1000.0);
		return((sarout_t) danswer);

		break;
		
	      default:
		sarerrno = SARERR_OUTFIELD;
		return(-1);
		break;
	}
}


static sarout_t
raw_out(int item, int devnum, met_disk_stats_t *start, met_disk_stats_t *end)
{
	dl_t	answer;
	int	i;
	ulong	total = 0;

	switch (item) {
	      case DISK_BUSY:
		answer = lsub(end[devnum].ds_active, start[devnum].ds_active);
		return(answer.dl_lop);
		break;

	      case DISK_RESP:
		answer = lsub(end[devnum].ds_resp, start[devnum].ds_resp);
		return(answer.dl_lop);
		break;

	      case DISK_OPS:
		total = 0;
		for (i = 0; i < MET_DS_OPTYPES; i++) {
			total += DIFF_DISK(devnum, ds_op[i]);
		}
		return(total);
		break;

	      case DISK_BLOCKS:
		total = 0;
		for (i = 0; i < MET_DS_OPTYPES; i++) {
			total += DIFF_DISK(devnum, ds_opblks[i]);
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
sar_disk_cleanup(void)
{
	free(disk_name);
	free(ds_info);
	free(disk);
	free(old_disk);
	free(first_disk);
	
	disk_name = NULL;
	ds_info = NULL;
	disk = NULL;
	old_disk = NULL;
	first_disk = NULL;
}
