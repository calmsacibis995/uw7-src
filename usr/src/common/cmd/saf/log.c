/*		copyright	"%c%" 	*/

#ident  "@(#)log.c	1.6"
#ident  "$Header$"

# include <stdio.h>
# include <unistd.h>
# include <sys/types.h>
# include <sac.h>
# include <priv.h>
# include <pfmt.h>
# include "extern.h"
# include "misc.h"
# include "msgs.h"
# include "structs.h"

static	FILE	*Lfp;	/* log file */
# ifdef DEBUG
static	FILE	*Dfp;	/* debug file */
# endif



/* New messages for log.c */

static char 
*MSG19 = ":19:Could not open logfile.\n",
*MSG20 = ":20:Could not lock logfile.\n",
*MSG21 = ":21:Could not open debugfile.\n";

const char 
MSGID36[] = ":36:",  MSG36[] = "Can not open _pid file for <%s>",
MSGID187[] = ":187:", MSG187[] = "*** SAC exiting ***",
MSGID37[] = ":37:",  MSG37[] = "Could not open _sactab",
MSGID38[] = ":38:",  MSG38[] = "Malloc failed",
MSGID39[] = ":39:",  MSG39[] = "_sactab file is corrupt",
MSGID40[] = ":40:",  MSG40[] = "_sactab version # is incorrect",
MSGID41[] = ":41:",  MSG41[] = "Can not chdir to home directory",
MSGID42[] = ":42:",  MSG42[] = "Could not open _sacpipe",
MSGID43[] = ":43:",  MSG43[] = "Internal error - bad state",
MSGID44[] = ":44:",  MSG44[] = "Read of _sacpipe failed",
MSGID45[] = ":45:",  MSG45[] = "Fattach failed",
MSGID46[] = ":46:",  MSG46[] = "I_SETSIG failed",
MSGID47[] = ":47:",  MSG47[] = "Read failed",
MSGID48[] = ":48:",  MSG48[] = "Poll failed",
MSGID49[] = ":49:",  MSG49[] = "System error in _sysconfig",
MSGID50[] = ":50:",  MSG50[] = "Error interpreting _sysconfig",
MSGID51[] = ":51:",  MSG51[] = "Pipe failed",
MSGID52[] = ":52:",  MSG52[] = "Could not create _cmdpipe",
MSGID53[] = ":53:",  MSG53[] = "Could not set ownership",
MSGID54[] = ":54:",  MSG54[] = "Could not set level of process",
MSGID55[] = ":55:",  MSG55[] = "Could not get level of process",
MSGID56[] = ":56:",  MSG56[] = "Could not get level identifiers";
	

/*
 * Procedure: openlog - open log file, sets global file pointer Lfp
 *
 *
 * Restrictions:
                 fprintf: none
                 lockf: none
 *               fopen: none
 */


void
openlog()
{
	FILE *fp;		/* scratch file pointer for problems */

	(void) rename(LOGFILE, OLOGFILE);
	umask(022);
	Lfp = fopen(LOGFILE, "a+");
	if (Lfp == NULL) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt (fp, MM_ERROR, MSG19);
		}
		exit(1);
	}

/*
 * lock logfile to indicate presence
 */

	if (lockf(fileno(Lfp), F_LOCK, 0) < 0) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt (fp, MM_ERROR, MSG20);
		}
		exit(1);
	}
}


/*
 * Procedure: log - put a message into the log file
 *
 * Args:	msg - message to be logged
 *
 * Restrictions:
                 ctime: none
                 sprintf: none
                 fprintf: none
                 fflush: none
 */


void
log(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	char buf[SIZE];		/* scratch buffer */

	(void) time(&clock);
	timestamp = ctime(&clock);
	*(strchr(timestamp, '\n')) = '\0';
	(void) sprintf(buf, "%s; %ld; %s\n", timestamp, getpid(), msg);
	(void) fprintf(Lfp, buf);
	(void) fflush(Lfp);
}



/*
 * Procedure: error - put an error message into the log file and exit if indicated
 *
 * Args:	msgid - id of message to be output
 *		action - action to be taken (EXIT or not)
 */


void
error(msgid, action)
int msgid;
int action;
{
	if (msgid < 0 || msgid > E_LVLIN)
		return;
	switch (msgid)
	{
		case E_SACOPEN:
			log (gettxt(MSGID36, MSG36));
			break;
	
		case E_MALLOC:
			log (gettxt(MSGID37, MSG37));
			break;

		case E_BADFILE:
			log (gettxt(MSGID38, MSG38));
			break;

		case E_BADVER:
			log (gettxt(MSGID39, MSG39));
			break;

		case E_CHDIR:
			log (gettxt(MSGID40, MSG40));
			break;

		case E_NOPIPE:
			log (gettxt(MSGID41, MSG41));
			break;
	
		case E_BADSTATE:
			log (gettxt(MSGID42, MSG42));
			break;

		case E_BADREAD:
			log (gettxt(MSGID43, MSG43));
			break;

		case E_FATTACH:
			log (gettxt(MSGID44, MSG44));
			break;

		case E_SETSIG:
			log (gettxt(MSGID45, MSG45));
			break;

		case E_READ:
			log (gettxt(MSGID46, MSG46));
			break;

		case E_POLL:
			log (gettxt(MSGID47, MSG47));
			break;

		case E_SYSCONF:
			log (gettxt(MSGID48, MSG48));
			break;

		case E_BADSYSCONF:
			log (gettxt(MSGID49, MSG49));
			break;

		case E_PIPE:
			log (gettxt(MSGID50, MSG50));
			break;

		case E_CMDPIPE:
			log (gettxt(MSGID51, MSG51));
			break;

		case E_CHOWN:
			log (gettxt(MSGID52, MSG52));
			break;

		case E_SETPROC:
			log (gettxt(MSGID53, MSG53));
			break;

		case E_GETPROC:
			log (gettxt(MSGID54, MSG54));
			break;

		case E_LVLIN:
			log (gettxt(MSGID55, MSG55));
			break;
	}

	if (action == EXIT) {
		log(gettxt(MSGID187, MSG187));
		exit(msgid);
	}

	return;
}



# ifdef DEBUG

/*
 * Procedure: opendebug - open debugging file, sets global file pointer Dfp
 */


void
opendebug()
{
	FILE *fp;	/* scratch file pointer for problems */

	Dfp = fopen(DBGFILE, "a+");
	if (Dfp == NULL) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt (fp, MM_ERROR, MSG21);
		}
		exit(1);
	}
}


/*
 * Procedure: debug - put a message into debug file
 *
 * Args:	msg - message to be output
 */


void
debug(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	char buf[SIZE];		/* scratch buffer */

	(void) time(&clock);
	timestamp = ctime(&clock);
	*(strchr(timestamp, '\n')) = '\0';
	(void) sprintf(buf, "%s; %ld; %s\n", timestamp, getpid(), msg);
	(void) fprintf(Dfp, buf);
	(void) fflush(Dfp);
}

# endif
