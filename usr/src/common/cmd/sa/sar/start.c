/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/start.c	1.6.1.1"
#ident "$Header$"

/* start.c
 *  
 * Processes SAR_START records.  A SAR_START record marks the
 * beginning of data for an interval.
 *
 * Note that this module calls the output routines, and determines
 * whether or not to process this sample.
 */


#include <stdio.h>
#include "../sa.h"
#include "sar.h"


#define INTERVAL_ADJUST   2

extern int   skipping_record;

time_t      last_time = 0;
int      record_num = 0;


flag	do_headers = TRUE;


extern flag    st_flag;
extern flag    et_flag;
extern flag    interval_flag;
extern time_t   st_time;
extern time_t  et_time;
extern int    interval;

extern int	num_item_id;


flag
sar_start_init(void)
{
	skipping_record = FALSE;
	record_num = 0;
	last_time = 0;
	return(TRUE);
}


int
sar_start(FILE *infile, sar_header sarh, flag32 of)
{
	struct sar_start_info   start_info;
	time_t      	temp_time;
	struct tm   	*tm;
	long        	seconds;
	int   		i;
	static		do_sys_info = TRUE;
	
	get_item(&start_info, sizeof(struct sar_start_info), sarh.item_size, infile);
	
	temp_time = start_info.timestamp;
	tm = localtime(&temp_time);
	seconds = tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
	
	if (st_flag && seconds < st_time) {
		skipping_record = TRUE;
		return(TRUE);
	}
	if (et_flag && seconds > et_time) {
		skipping_record = TRUE;
		return(TRUE);
	}
	
	if (interval_flag && last_time != 0 && temp_time - last_time < interval - INTERVAL_ADJUST) {
		skipping_record = TRUE;
		return(TRUE);
	}
	
	skipping_record = FALSE;

	record_num++;

	/*
	 * We output the system information (sysname, nodename, etc)
	 * exactly once.  
	 */

	if (do_sys_info == TRUE) {
		output_sysinfo(start_info.timestamp);
		do_sys_info = FALSE;
	}

	/*
	 * Headers are output after the first sample, but only
	 * if 'do_headers' is TRUE.  do_headers is set to TRUE at
	 * the start of processing, and is set to FALSE after 
	 * header output.  This prevents headers from being printed
	 * after a reboot (INIT record with a different boot_time).
	 *
	 */

        if (record_num == 2 && do_headers == TRUE) {
		output_headers(of, last_time);
		do_headers = FALSE;
	}
	else if (record_num > 2) { /* note, doesn't cover LAST sample */
		output_data(of, last_time, FALSE);
	}

	for (i = 0; i < num_item_id; i++) {
		collected[i] = FALSE;
	}
	
	last_time = temp_time;
	return(TRUE);
}

