/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/sar.c	1.7.2.1"
#ident	"$Header$"


  
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
  
#include "../sa.h"
#include "sar.h"
  
#include <unistd.h>

typedef struct option_entry {
	char     option;
	flag32   flags;
} option_entry;

/*
 * Maps options specifying output (single character letters) 
 * to flags.  Other options (i.e., input file name, start time,
 * etc) are not on this table.
 *
 * This scheme may cause minor problems in the unlikely event
 * that there are ever more than 32 output tables.
 *
 */

static option_entry   output_options[] = {
	{ 'a', REPORT_FACCESS },
	{ 'b', REPORT_BUFFER },
	{ 'c', REPORT_SYSCALL },
	{ 'd', REPORT_DISK },
	{ 'g', REPORT_PGOUT },
	{ 'k', REPORT_KMA },
	{ 'm', REPORT_IPC },
	{ 'p', REPORT_PGIN },
	{ 'q', REPORT_QUEUE },
	{ 'r', REPORT_FREEMEM },
	{ 'u', REPORT_CPU },
	{ 't', REPORT_INODE },
	{ 'v', REPORT_SYSTAB },
	{ 'w', REPORT_SWAP },
	{ 'y', REPORT_TERM },
	{ 'A', 0xFFFFFFFF } 
};

#define NUM_OUTPUT_OPTIONS  (sizeof(output_options)/sizeof(option_entry))

/* Keep this array in sync with the SAR_ constants! */ 

static char *metric_names[] = {
	"Initialization information",
	"KMA pool sizes",
	"File system types",
	"Start sample marker",
	"CPU metrics",
	"Per-processor scheduling metrics",
	"System wide scheduling metrics",
	"Buffer cache metrics",
	"System call metrics",
	"Process resource metrics",
	"File lookup metrics",
	"File system table metrics",
	"File access metrics",
	"Inode metrics",
	"TTY metrics",
	"IPC metrics",
	"VM metrics",
	"KMA metrics",
	"Free memory metrics",
	"Disk metrics",
	"LWP metrics",
	"Hardware CPU metrics",
	"Per-LWP HW CPU metrics",
	"CG Hardware metrics",
	"CG Count and Type",
	"PS Info",
	"CPU to CG Mapping"
  };


static char *ao_option_strings[] = {
#define AO_QUERY              0
      "?",
#define AO_OUTPUT_RAW         1
      "raw",
#define AO_NO_BLANKS          2
      "noblanks",
#define AO_CSV                3
      "csv",
#define AO_LWP                4
      "lwp",
      NULL
};

struct hwmetric_init_info hwmetric_init_info =
	{ 1, CPUMTR_NOT_SUPPORTED, CGMTR_NOT_SUPPORTED };

flag  metric_warn[sizeof(metric_names)/sizeof(char *)];
     
static int   num_warn = sizeof(metric_names)/sizeof(char *);
     
     
     
/* 
 * The Module Table
 *
 * This is the central sar structure.  Each subsidiary module
 * must have an entry in this table.  Subsidiary modules are
 * invoked ONLY through this table, they are never called directly.
 *
 * See sar.h for more details about this table.
 *
 * This array must be kept in sync with the SAR_ constants.  Any
 * changes to those constants must be reflected here, and vice versa.
 *
 */
     
     
proc_entry  proc_table[] = {
/* SAR_INIT */      { 0xFFFFFFFF, NULL, sar_init, NULL, NULL },
/* SAR_KMEM_SIZES */{ 0xFFFFFFFF, sar_skmem_init, sar_skmem, NULL, NULL },
/* SAR_FS_NAMES */  { 0xFFFFFFFF, sar_names_init, sar_names, NULL, NULL },
/* SAR_START_REC */ { 0xFFFFFFFF, sar_start_init, sar_start, NULL, NULL },
/* SAR_CPU_P */     { 0xFFFFFFFF, sar_cpu_init, sar_cpu_p, sar_cpu_out, sar_cpu_cleanup },
/* SAR_LOCSCHED_P */ { REPORT_SWAP | REPORT_QUEUE, sar_locsched_init, sar_locsched_p, sar_locsched_out, sar_locsched_cleanup },
/* SAR_GLOBSCHED */  { REPORT_QUEUE, sar_gsched_init, sar_gsched_p, sar_gsched_out, NULL },
/* SAR_BUFFER_P */   { REPORT_BUFFER, sar_cache_init, sar_cache, sar_cache_out, sar_cache_cleanup },
/* SAR_SYSCALL_P */  { REPORT_SYSCALL, sar_syscall_init, sar_syscall_p, sar_syscall_out, sar_syscall_cleanup },
/* SAR_PROCRESOURCE */ { REPORT_SYSTAB, sar_proc_init, sar_proc, sar_proc_out, NULL },
/* SAR_FS_LOOKUP_P */ { REPORT_FACCESS, sar_flook_init, sar_flook_p, sar_flook_out, sar_flook_cleanup },
/* SAR_FS_TABLE */   { REPORT_SYSTAB, sar_fs_init, sar_fs, sar_fs_out, NULL },
/* SAR_FS_ACCESS_P */ { REPORT_FACCESS | REPORT_INODE, sar_facc_init, sar_facc_p, sar_facc_out, sar_facc_cleanup },
/* SAR_FS_INODES */  { REPORT_SYSTAB | REPORT_INODE, sar_inodes_init, sar_inodes, sar_inodes_out, sar_inodes_cleanup },
/* SAR_TTY_P */   { REPORT_TERM, sar_tty_init, sar_tty, sar_tty_out, sar_tty_cleanup },
/* SAR_IPC_P */   { REPORT_IPC, sar_ipc_init, sar_ipc, sar_ipc_out, NULL },
 /* SAR_VM_P */    { REPORT_SWAP | REPORT_PGIN | REPORT_PGOUT, sar_vm_init, sar_vm, sar_vm_out, NULL },
/* SAR_KMEM_P */  { REPORT_KMA, sar_kmem_init, sar_kmem, sar_kmem_out, sar_kmem_cleanup },
/* SAR_MEM */     { REPORT_FREEMEM, sar_mem_init, sar_mem, sar_mem_out, NULL },
/* SAR_DISK */	  { REPORT_DISK, sar_disk_init, sar_disk, sar_disk_out, sar_disk_cleanup },
/* SAR_LWP_RESRC_P */ { REPORT_SYSTAB, sar_lwp_init, sar_lwp, sar_lwp_out, NULL}
/* SAR_HWMETRIC_CPU */ ,{ REPORT_CPUMTR|REPORT_CPUMTR_STATE|REPORT_CPUMTR_LWP|REPORT_CPUMTR_LWP_STATE,
  sar_cpumtr_init, sar_cpumtr_p, sar_cpumtr_out, sar_cpumtr_cleanup },
/* SAR_HWMETRIC_LWP */ { REPORT_CPUMTR_LWP|REPORT_CPUMTR_LWP_STATE,
  NULL, sar_cpumtr_lwp_p, sar_cpumtr_lwp_out, sar_cpumtr_lwp_cleanup },
/* SAR_HWMETRIC_CG */ { REPORT_CGMTR,
  NULL, sar_cgmtr_p, sar_cgmtr_out, sar_cgmtr_cleanup },
/* SAR_HWMETRIC_INIT */ { (flag32)0xFFFFFFFF,
  NULL, sar_hwmetric_init, NULL, NULL },
/* SAR_PS */ { REPORT_CPUMTR_LWP|REPORT_CPUMTR_LWP_STATE,
  NULL, sar_psdata_p, NULL, sar_psdata_cleanup },
/* SAR_CPUCG_P */ { (flag32) 0xFFFFFFFF, NULL, sar_cpusg, 
		    NULL, NULL }
};                     



int  num_item_id = sizeof(proc_table)/sizeof(proc_entry);

/* 
 * collected[] is an array of flags, one for each record type.
 * At the start of each sample the flags are cleared, and as
 * each record type is read its flag is set.  (This is done only for
 * the record types containing metric data.  Although flags are
 * present for the special record types, those flags are not used).
 * When preparing to produce an output table, the groups used by that
 * table are checked, if one of them has not been read then the table
 * is supressed.  This allows a version of sar that processes a 
 * given record type to function with data generated by versions of
 * sadc that do not produce that.  Without this check, the command
 * 'sar -A' could produce tables containing unpredictable values.
 */

flag	collected[sizeof(proc_table)/sizeof(proc_entry)];

flag32    output_flags = 0;
char    *ofile_name = NULL;
static char    *infile_name = NULL;
static char    *option_string = "uybdvcwaqmtpgrkxARo:s:e:i:f:P:O:H:N:";


/* variables for -s, -e, and -i options */

flag     st_flag = FALSE;
flag     et_flag = FALSE;
flag     interval_flag = FALSE;
time_t   st_time;
time_t   et_time;
int     interval;

char  *proc_list_string = NULL;
char  aggregate_only = TRUE;
char  do_aggregate = TRUE;

int   rewound = TRUE;   /* Set to true each time file is scanned */

static int      write_mode = WRITE_BY_ITEM;

flag	output_raw = FALSE;  


char   *meter_list_string = "";
char   *node_list_string = NULL;
flag    ao_no_blanks = FALSE;
flag    ao_csv_flag = FALSE;
static flag    ao_lwp_flag = FALSE;
static flag    hwsar_flag = FALSE;
char    do_aggregate_node = TRUE;

struct sar_init_info    machinfo;
static struct tm  start_time;
static struct tm  end_time;

extern int     record_num;
extern time_t  last_time;

extern int     optind;

static void    process_options(int argc, char **argv);



/* skipping_record is set to TRUE when sar needs to skip an entire
 * sample.  When skipping_record is TRUE, sar will read only 
 * headers.  Data is skipped for all record types except SAR_START and
 * SAR_INIT.  This is used to by sar to match timestamps (from 
 * SAR_START records) against command line options
 */


int   skipping_record = FALSE;

/*
 * do_headers is used to suppress the printing of headers after
 * a reboot has been detected.  See start.c for details.
 *
 */

extern flag	do_headers;

int   sarerrno;

static flag   realtime_flag = FALSE;

int
main(int argc, char **argv)
{
	long        temp_time;
	struct tm   *current_time;
	FILE        *infile;
	int          real_length;
	int          real_num_intervals;
	sar_header  sarh;
	flag32      flags;
	int         i;
	
	char        *cp;
	/* Get pointer to last element of cmd pathname. */
	cp = strrchr(argv[0], '/');
	if (cp == NULL)
		cp = argv[0];
	else
		cp++;
	/* If invoked by the name of "hwsar", '-A' option is extended.  */
	hwsar_flag = (strcmp(cp, "hwsar") == 0);

	process_options(argc, argv);
	
	
	/*
	 * Determine whether to process real time data or read from a file
	 * and set appropriate variables
	 */
	
	switch (argc - optind) {
	      case 0:        /* Get input from data file */
		if (infile_name == NULL) {    /* use default file */
			temp_time = time((long *) 0);
			current_time = localtime(&temp_time);
			infile_name = malloc(strlen(SARFILE_FMT) + 12);
			if (infile_name == NULL) {
				sar_error(SARERR_INIT);
			}
			sprintf(infile_name, SARFILE_FMT, current_time->tm_mday);
		}
		if ((infile = fopen(infile_name, "r")) == NULL) {
			sar_error(SARERR_INFILE);
		}
		break;
		
	      case 1:        /* real time data, one cycle */
		realtime_flag = TRUE;
		real_length = atoi(argv[optind]);
		real_num_intervals = 2;
		break;
		
	      case 2:        /* real time data; specified cycles */
	      default:
		realtime_flag = TRUE;
		real_length = atoi(argv[optind]);
		real_num_intervals = 1 + atoi(argv[optind + 1]);
		break;
	}
	
	if (realtime_flag == TRUE) {
		char  arg1[10];
		char  arg2[10];
		int   pipe1[2];
		int   pipe2[2];
		int   childid;
		int   childid2;
		
		sprintf(arg1, "%d", real_length);
		sprintf(arg2, "%d", real_num_intervals);
		
		if (pipe(pipe1) == -1) {
			sar_error(SARERR_SADC);
		}
		if ((childid = fork()) == 0) { /* child */
			close(1);
			dup(pipe1[1]);

			if (ofile_name == NULL) {
				if (execlp("/usr/lib/sa/sadc","/usr/lib/sa/sadc",arg1,arg2,NULL) == -1) {
					sar_error(SARERR_SADC);
				}
			}
			else {   /* pipe sadc through tee */
				if (pipe(pipe2) == -1) {
					sar_error(SARERR_SADC);
				}

				if ((childid2 = fork()) == 0) { /* child */
					close(1);
					dup(pipe2[1]);
					if (execlp("/usr/lib/sa/sadc","/usr/lib/sa/sadc",arg1,arg2,NULL) == -1) {

						sar_error(SARERR_SADC);
					}
				}
				else if (childid2 == -1) {
					sar_error(SARERR_SADC);
				}
				else {
					close(0);
					dup(pipe2[0]);
					close(pipe2[1]);
					if (execlp("tee", "tee", ofile_name, NULL) == -1) {
						sar_error(SARERR_SADC);
					}
				}
			}
		}
		else if (childid == -1) {
			sar_error(SARERR_SADC);
		}
		else {
			infile = fdopen(pipe1[0], "r");
			close(pipe1[1]);
		}
		write_mode = WRITE_BY_RECORD;
	}
	else {
		infile = fopen(infile_name, "r");
	}
	
	/* 
	 * Initialize size warnings array.  After the data, warnings 
	 * will be printed for each data item having a size different
	 * than expected.
	 */
	
	for (i = 0; i < num_warn; i++) {
		metric_warn[i] = FALSE;
	}
	
	if (write_mode == WRITE_BY_RECORD) {
		
		/*
		 * Do real-time reporting.  One pass through the
		 * input.  Produce all tables at once.  Probably most
		 * useful for some sort graphical interface.
		 */

		do_headers = TRUE;
		
		while (!feof(infile)) {
			if (fread(&sarh, sizeof(sar_header), 1, infile) != 1) {
				break;
			}
			
			/* A number of conditions that cause sar to skip this
			 * record.  They are:
			 *  
			 * 1. Unrecognized vendor code.
			 * 2. Unrecognized item code.
			 * 3. skipping_record == TRUE and this is not a special record.
			 * 4. This record is not needed for the specified output tables.
			 * 5. The process_fn pointer for this record type is NULL.
			 *  
			 * If none of these are true, the record is passed
			 * process_fn looked up in proc_table.
			 */  
			
			if (sarh.reserved != 0) {
				skip(infile, sarh.item_size * sarh.item_count);
			}
			else if (sarh.item_code >= num_item_id) {
				skip(infile, sarh.item_size * sarh.item_count);
			}
			else if (skipping_record == TRUE && sarh.item_code != SAR_START_REC
				 && sarh.item_code != SAR_INIT) {
				skip(infile, sarh.item_size * sarh.item_count);
			}
			else if ((output_flags & proc_table[sarh.item_code].flags) == 0) {
				skip(infile, sarh.item_size * sarh.item_count);
			}
			else if (proc_table[sarh.item_code].process_fn == NULL) {
				skip(infile, sarh.item_size * sarh.item_count);
			}
			else {
				proc_table[sarh.item_code].process_fn(infile, sarh, output_flags);
			}
		}
		/* output data for last sample and average */
		if (record_num >= 2) {  
			output_data(output_flags, last_time, FALSE);
		}
		if (record_num >= 3) {
			output_data(output_flags, last_time, TRUE);
		}
	}
	else {
		
		/* The non real-time case.  We output one table at a time, 
		 * reprocessing the file for each table.  This is the same
		 * as the code above, but is enclosed in a loop over the
		 * possible output flags.
		 */
		
		for (flags = 1; flags != 0; flags <<= 1) {
			if ((flags & output_flags) != 0) {
				rewind(infile);
				rewound = TRUE;
				do_headers = TRUE;
				
				while (!feof(infile)) {
					if (fread(&sarh, sizeof(sar_header), 1, infile) != 1) {
						break;
					}
					
#if DEBUG
					printf("HEADER: VENDOR: %d, ITEM %d, SIZE: %d, COUNT: %d\n", 
					       sarh.reserved, sarh.item_code, sarh.item_size, 
					       sarh.item_count);
					fflush(stdout);
#endif /* DEBUG */
					
					if (sarh.reserved != 0) {
						skip(infile, sarh.item_size * sarh.item_count);
					}
					else if (sarh.item_code > num_item_id) {
						skip(infile, sarh.item_size * sarh.item_count);
					}
					else if (skipping_record == TRUE 
						 && sarh.item_code != SAR_START_REC
						 && sarh.item_code != SAR_INIT) {
						skip(infile, sarh.item_size * sarh.item_count);
					}
					else if ((flags & proc_table[sarh.item_code].flags) == 0) {
						skip(infile, sarh.item_size * sarh.item_count);
					}
					else if (proc_table[sarh.item_code].process_fn == NULL) {
						skip(infile, sarh.item_size * sarh.item_count);
					}
					else {
						proc_table[sarh.item_code].process_fn(infile, sarh, flags);
					}
				}
				/* output data for last sample and average */
				if (record_num >= 2) {  
					output_data(flags, last_time, FALSE);
				}
				if (record_num >= 3) {
					output_data(flags, last_time, TRUE);
				}
			}
		}
	}
	
	for (i = 0; i < num_warn; i++) {
		if (metric_warn[i] == TRUE) {
			printf("sar: warning: %s size has changed.\n", metric_names[i]);
		}
	}
	
	return(0);
}


static void
process_options(int argc, char **argv)
{
	int      c;
	int      j;
	
	extern char    *optarg;
	char	*ao_options;
	char	*ao_value;
	
	while ((c = getopt(argc, argv, option_string)) != EOF) {
		switch (c) {
		      case 'o':         /* output file */
			ofile_name = strdup(optarg);
			break;
			
		      case 's':         /* starting time for samples */
			if (sscanf(optarg, "%d:%d:%d", &start_time.tm_hour,
				   &start_time.tm_min,
				   &start_time.tm_sec) < 1) {
				sar_error(SARERR_ILLARG_S);
			}
			else {
				st_flag = TRUE;
				st_time = start_time.tm_hour * 3600 + start_time.tm_min * 60
				  + start_time.tm_sec;
			}
			break;
			
		      case 'e':         /* ending time for samples */
			if (sscanf(optarg, "%d:%d:%d", &end_time.tm_hour,
				   &end_time.tm_min,
				   &end_time.tm_sec) < 1) {
				sar_error(SARERR_ILLARG_E);
			}
			else {
				et_flag = TRUE;
				et_time = end_time.tm_hour * 3600 + end_time.tm_min * 60
				  + end_time.tm_sec;
			}
			break;
			
		      case 'i':         /* interval */
			if (sscanf(optarg, "%d", &interval) < 1) {
				sar_error(SARERR_ILLARG_I);
			}
			if (interval > 0) {
				interval_flag = TRUE;
			}
			break;
			
		      case 'f': 
			infile_name = strdup(optarg);
			write_mode = WRITE_BY_ITEM;
			break;
			
		      case 'P':
			/*
			 * We store the processor list as a string.  
			 * This string is parsed in the init module.  
			 * It is not parsed here because we don't know
			 * how many processors there are.
			 */

			if (aggregate_only) {
			  aggregate_only = FALSE;
			  proc_list_string = malloc(strlen(optarg) + 1);
			  strcpy(proc_list_string, optarg); 
			}
			break;
			
		      case 'N':
			/*
			 * We store the node list as a string.  
			 * This string is parsed in the init module.  
			 * It is not parsed here because we don't know
			 * how many nodes there are.
			 */
			if (aggregate_only) {
				aggregate_only = FALSE;
				node_list_string = malloc(strlen(optarg) + 1);
				strcpy(node_list_string, optarg); 
			}
			break;

		      case '?':
			/*
			 * This is handled by the error function, 
			 * so that all messages are done in one place.
			 */

			if (hwsar_flag) {
			  sar_error(SARERR_HWUSAGE);
			}
			else {
			  sar_error(SARERR_USAGE);
			}
			break;

		      case 'R':
			output_raw = TRUE;
			break;
			
		      case 'O':
			/*
			 * Additional options collected here. 
			 */
			ao_options = optarg;
			while (*ao_options != '\0') {
			        switch (getsubopt(&ao_options, ao_option_strings, &ao_value)) {
			              case AO_OUTPUT_RAW:
			                output_raw = TRUE;
			                break;
			              case AO_NO_BLANKS:
			                ao_no_blanks = TRUE;
			                break;
			              case AO_CSV:
			                ao_csv_flag = TRUE;
			                break;
			              case AO_LWP:
			                ao_lwp_flag = TRUE;
			                break;
			              case AO_QUERY:
			              default:
			                fprintf(stderr, "%s: ERROR: Illegal option -- O%s\n", argv[0], optarg);
			                sar_error(SARERR_USAGE);
			                break;
			        }
			}
			break ;
		      case 'H':
			/*
			 * We store the meter list as a string.  
			 * This string is parsed in the output module.  
			 * It is not parsed here because we don't know
			 * how many processors/nodes there are.
			 */
			
			meter_list_string = malloc(strlen(optarg) + 1);
			strcpy(meter_list_string, optarg); 
			/* Treat bare dot as an empty list, for convenience. */
			if (strcmp(meter_list_string, ".") == 0)
				meter_list_string[0] = '\0';
			output_flags |= REPORT_CGMTR;
			break;

		      default:
			for (j = 0; j < NUM_OUTPUT_OPTIONS; j++) {
				if (c == output_options[j].option) {
					if (output_options[j].flags
					  & REPORT_HWMETRICS && !hwsar_flag) {
						output_flags |=
							output_options[j].flags
							&~REPORT_HWMETRICS;
					} else {
						output_flags |=
							output_options[j].flags;
					}
					break;
				}
			}
			
			if (j == NUM_OUTPUT_OPTIONS) {      /* not found */
				sar_error(SARERR_ILLARG_UNKNOWN);
				fprintf(stderr, "sar: Unknown option -%c\n", c);
				exit(-1);
			}
			break;
		}
	}
	
	if (output_flags & REPORT_CGMTR) {
		if (output_flags & REPORT_CPU) {
			output_flags &= ~REPORT_CPU;
			if (ao_lwp_flag)
				output_flags |= REPORT_CPUMTR_LWP_STATE;
			else
				output_flags |= REPORT_CPUMTR_STATE;
		} else {
			if (ao_lwp_flag)
				output_flags |= REPORT_CPUMTR_LWP;
			else
				output_flags |= REPORT_CPUMTR;
		}
	}

	/* if no output options specified, set to the default */
	if (output_flags == 0) { 
		output_flags = REPORT_DEFAULT;
	}
}


int
skip(FILE *infile, unsigned long numbytes) 
{
	if (numbytes == 0) {
		return(0);
	}
	
	if (realtime_flag == TRUE || infile_name == NULL) { /* can't fseek */
		char  dummy[100];
		
		while (numbytes > 100) {
			if (fread(dummy, 1, 100, infile) != 100) {  
				return(-1);
			}
			numbytes -= 100;
		}
		if (fread(dummy, 1, numbytes, infile) != numbytes) {
			return(-1);
		}
		return(0);
	}
	else {
		return(fseek(infile, numbytes, SEEK_CUR));
	}
}
