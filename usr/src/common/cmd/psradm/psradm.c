#ident	"@(#)psradm:common/cmd/psradm/psradm.c	1.4"

/*
 * Command: psradm
 *
 *	psradm takes processors on/off line on 
 * 	a multiprocessor system
 *
 *	Usage  psradm -f|n [-v] -a|processor_id
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <priv.h> 
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/sysconfig.h>
#include <locale.h>
#include <pfmt.h>
#include <time.h>
#include <utmpx.h>

static int flg_all=0;		/* select all processors */
static int flg_verbose=0;
static int flg_act=0;		/* 1=on (-1)=off 	*/
static int processor=(-1);	


static void usage_exit(), failure(), do_psradm();
static char * time_str();
extern int sysconfig(int);

int error = 0;

int 
main(int argc, char *argv[])
{
	int c;
	char * ptr;
					/* setlocale with "" means use
					 * the LC_* environment variable
					 */
	(void) setlocale(LC_ALL, "");	
	(void) setcat("uxmp");		/* set default message catalog */
	(void) setlabel("UX:psradm");	/* set deault message label */

	if (argc == 1) 
		usage_exit();

	while ((c = getopt(argc, argv, "vfna")) != EOF) {
		switch (c) {

		case 'a':
		flg_all=1;
		break;

		case 'f':
		if (flg_act != 0) usage_exit();
		flg_act=(-1);
		break;

		case 'n':
		if (flg_act != 0) usage_exit();
		flg_act=1;
		break;

		case 'v':
		flg_verbose=1;
		break;

		case '?':
		usage_exit();

		}
	}

	if (flg_all == 0 && flg_act == 1 && argv[optind] == NULL ) 
			flg_all=1;

	if (((argv[optind]  == NULL) && (flg_all == 0)) && (flg_verbose ==0)) 
		usage_exit();

	if (flg_all == 0 && flg_act == 0 && flg_verbose == 1) 
			flg_all=1;

	else if (flg_act == 0)	/* must have action on/off */
		usage_exit();

	if (flg_all == 0 && argv[optind] == NULL)
		usage_exit();


	if (flg_all == 0) {		/* act on processor list */
		
		while (argv[optind]) {
			processor = strtol(argv[optind], &ptr, 10);

			if (ptr == argv[optind])
				usage_exit();

			do_psradm(processor, flg_act, flg_verbose);
			optind++;
		}

	} else {	/* act on all processors 
			 * disallow -a with processor_id  list 
			 */
		
		if (argv[optind] != NULL) 
			usage_exit();

		c = sysconfig(_CONFIG_NENGINE); /* proc's configured */

		if (c <  0){
		     pfmt(stderr, MM_ERROR,
		         ":1:cannot determine number of processors.\n");

		     exit(1);
		}

		/* do it for all the processors */

		for(processor=0; processor < c; processor++)
			do_psradm(processor, flg_act, flg_verbose);
	}
	exit(error);
	/*NOTREACHED*/
}

/*
 * perform the action 'act' on processor 'proc' with possible verbosity
 *
 *
 */

void 
do_psradm(int proc, int act, int verbose)
{
	time_t t;
	int i;


	if (act > 0) {	/* turn on */

		if (p_online(proc, P_QUERY) != P_ONLINE) { /*allready online?*/

			(void)time(&t);

			if ((p_online(proc, P_ONLINE)) < 0)
				failure(proc);
			else {
				log_wtmp(&t,proc,act);
				lfmt(NULL, MM_INFO,
					":14:At %s, %d was brought on-line.\n",
					(time_str(&t)),proc);


				if (verbose) 
				pfmt(stdout, MM_INFO,
					":2:At %s, %d was brought on-line.\n",
					(time_str(&t)), proc);
			}

		} 
		
	} else if (act < 0){	/* turn off */
		if (p_online(proc, P_QUERY) != P_OFFLINE) {/*allready offline?*/

			(void)time(&t);
			if ((p_online(proc, P_OFFLINE)) < 0) {
				failure(proc);
			} else {
				log_wtmp(&t,proc,act);
				lfmt(NULL, MM_INFO,
					":15:At %s, %d was taken off-line.\n",
					time_str(&t), proc);

				if (verbose) 
				pfmt(stdout, MM_INFO,
					":3:At %s, %d was taken off-line.\n",
					time_str(&t), proc);
			}

		} 
	} else if (act == 0  && verbose) { /* just print status */


		i = p_online(proc, P_QUERY);

		switch(i){
		case P_ONLINE:
		pfmt(stdout, MM_INFO, ":4:processor %d online\n", proc);
		break;

		case P_OFFLINE:
		pfmt(stdout, MM_INFO, ":5:processor %d offline\n", proc);
		break;

		default:
		pfmt(stdout, MM_INFO, ":6:unsure about processor %d\n", proc);

		}
	}
}

/*
 * print usage message and return error
 *
 *
 */

void
usage_exit()
{
	char *Usage = ":8:Usage:  psradm -f|n [-v] -a|processor_id\n";

	pfmt(stderr, MM_ERROR, ":7:Incorrect Usage\n");
	pfmt(stderr, MM_ACTION, Usage);
	exit(1);
}

/*
 * print error messages
 *
 *
 */

void
failure(int p)
{
	switch(error=errno) {

	case EPERM:
	pfmt(stderr, MM_ERROR, ":9:Permission denied\n");
	break;

	case EINVAL:
	pfmt(stderr, MM_INFO, ":10:processor %d non-existent\n", p);
	break;

	case EBUSY:
	pfmt(stderr, MM_INFO, ":11:processor %d busy\n", p);
	break;
		
	case EIO:
	pfmt(stderr, MM_INFO, ":12:cannot reach processor %d\n", p);
	break;

	}
}

/*
 *
 * return current time as ascii string of form "MM/DD/YY HH:MM:SS"
 */

char * 
time_str(time_t *tp)
{
	static char t_buf[30];

	(void)cftime(t_buf, "%D %T", tp);
	return(t_buf);
}

/*
 *
 * Write the change information into the wtmp log file
 *
 */

log_wtmp(time_t *stamp,int processor_id,int onoff)
{
	int fd;
	struct utmp wtmp;
	char buf[30];
	
	/* create the record */

	memset(&wtmp,0,sizeof(struct utmp));	/* zero entry */
	strcpy(wtmp.ut_user,"psradm");		/* let them know who did this */
	sprintf(buf,"%03d  %s",processor_id,(onoff>0)?"on":"off");
	strcpy(wtmp.ut_line,buf);
	wtmp.ut_time = *stamp;			/* timestamp */
	wtmp.ut_type = USER_PROCESS;

	/* create if nessary and update both wtmp and wtmpx */
	updwtmp(WTMP_FILE, &wtmp);
}
