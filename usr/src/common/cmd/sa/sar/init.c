/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/init.c	1.6.2.1"
#ident "$Header$"


/* init.c
 *
 * Processes SAR_INIT records.  
 *
 * In addition to processing the initialization information in the
 * SAR_INIT record (number of processors, file systems, etc.) this
 * routine invokes the initialization routines for the other modules
 * (via proc_table[]).
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>


#include "../sa.h"
#include "sar.h"

extern int  num_item_id;
extern char  do_aggregate;
extern int	record_num;
extern flag32	output_flags;
extern time_t	last_time;
extern proc_entry    proc_table[];

uint_t   *kmem_sizes;
char     (*fs_name)[MET_FSNAMESZ];
     
uint_t   num_mclass;
     
char     *proc_list;
extern char    *proc_list_string;

static struct sar_init_info	temp_info;
static int	re_init = TRUE;
static time_t	last_boot = 0;

/*ARGSUSED*/

int 
sar_init(FILE *infile, sar_header sarh, flag32 of)
{
	int      i;
	char     *tempptr;
	int      p;
	extern int  rewound;

	
	if (sarh.item_size != sizeof(machinfo)) {
		metric_warn[sarh.item_code] = TRUE;
	}

	get_item(&temp_info, sizeof(machinfo), sarh.item_size, infile);

/*
 * If re_init is FALSE and the boot_time has changed, then the machine
 * has been rebooted.
 */

	if (re_init == FALSE && temp_info.boot_time != last_boot) { 
		/* 
		 * Output the last sample and final data if necessary 
                 */

		if (rewound == FALSE) {
			if (record_num >= 2) {
				output_data(output_flags, last_time, FALSE);
			}
			if (record_num >= 3) {
				output_data(output_flags, last_time, TRUE);
			}
		}

		output_reboot(temp_info.boot_time);

		/* 
		 * Call cleanup functions.  This will deallocate 
		 * memory allocated by init functions.  Note that
		 * parameters such as the number of processors 
		 * may have changed.  Calling the cleanup functions
		 * insures that when the init functions are called	
		 * (in the code below), memory will be reallocated
		 * as necessary.
		 */

		for (i = 0; i < num_item_id; i++) {
			if (proc_table[i].end_fn != NULL) {
				proc_table[i].end_fn();
			}
		}

		/* 
		 * Set re_init and rewound to TRUE so that each modules
		 * init function is called.
		 */

		re_init = TRUE;
		rewound = TRUE;
	}
	else if (re_init == FALSE) {
		/* boot time hasn't changed.  Check INIT record for
		   consistency */
		if (memcmp(&machinfo, &temp_info, sizeof(machinfo)) != 0) {
			sar_error(SARERR_INITREC);
			exit(-1);
		}
	}

	if (re_init == TRUE) {
		machinfo = temp_info;
		last_boot = machinfo.boot_time;
		num_mclass = machinfo.num_kmem_sizes;
		
		proc_list = malloc(machinfo.num_engines);
		
		if (proc_list_string == NULL) {     /* -P option not used */
			for (i = 0; i < machinfo.num_engines; i++) {
				proc_list[i] = FALSE;
			}
		}
		else if (strcmp(proc_list_string, "ALL") != 0) { 
			char	*dupstring;

		/* -P option, but not "ALL" keyword, i.e., 
		 * individual processors specified.
		 */

			do_aggregate = FALSE;
			for (i = 0; i < machinfo.num_engines; i++) {
				proc_list[i] = FALSE;
			}

			/* 
			 * duplicate proc_list_string before using
			 * strtok() so that this code will work after
			 * a reboot INIT record is detected.
			 */

			dupstring = malloc(strlen(proc_list_string) + 1);
			strcpy(dupstring, proc_list_string);
			
			tempptr = strtok(dupstring, ",");
			while (tempptr) {
				p = atoi(tempptr);
				if (p >= 0 && p < machinfo.num_engines) {
					proc_list[p] = TRUE;
				}
				tempptr = strtok(NULL, ",");
			}

			free(dupstring);
		}
		else { /* -P ALL, report data for all processors */
			do_aggregate = TRUE;
			for (i = 0; i < machinfo.num_engines; i++) {
				proc_list[i] = TRUE;
			}
		}
		re_init = FALSE;
	}
	
	if (rewound == TRUE) {
		for (i = 0; i < num_item_id; i++) { 
			if (proc_table[i].init_fn != NULL) {
				proc_table[i].init_fn();
			}
		}
		rewound = FALSE;
	}
	
	return(TRUE);
}
