/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/dtcu.c	1.1.1.1"
#endif

#include <stdio.h>
#include <wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/procset.h>
#include <limits.h>

#include <X11/Intrinsic.h>
#include <OpenLook.h>
#include <Gizmos.h>
#include "error.h"

static void doExec(char **args);
static void catch_sig();
static char *newargs[15];
static int status;
	
	/* this program is called from DialupMgr and dtcall.
		It's first argument is either 1 for turn on
		debugging for cu, or 0 to leave debugging off.
		/usr/bin/cu is called with the rest of the arguments
		untouched. e.g.
		dtcu is invoked with:
		/usr/X/desktop/rft/dtcu 1 -cACU -b7 -e 94434444
		then dtcu will call:
		/usr/bin/cu -d -cACU -b7 -e 94434444

	*/
void
main(argc, argv)
int argc;
char *argv[];
{
	
	
	int i, j;

#ifdef DEBUG
	fprintf(stderr,"dtcu: argc=%d\n",argc);
	for (i=0; i < argc; i++) {
		fprintf(stderr,"%s%c", argv[i], (i<argc-1)? ' ': '\n');
	}
#endif
	if ((argc < 3) || (argc > 15)) {
		fprintf(stderr,GGT(cu_badargs));
		exit(2);
	}

	newargs[0] = strdup("/usr/bin/cu");

	j = 1;
	if (*argv[1] == '1') {
		newargs[j++] = strdup("-d");	
	} 
	
	for (i=2; i < argc; i++) {
		newargs[j++] = argv[i];
	 }
	sigset(SIGINT, catch_sig) ;
	sigset(SIGQUIT, catch_sig) ;
	sigset(SIGTERM, catch_sig) ;
	sigset(SIGCLD, catch_sig) ;
		/* always show the cu commandline */
	for (i=0; i < j; i++) {
		fprintf(stdout,"%s%c", newargs[i], (i<j-1)? ' ': '\n');
	}
	(void) doExec(newargs);
	wait(&status);
		
	
} /* main */


static void
catch_sig()
{
	/* routine to catch the death of the cu process
		need to exit with appropriate exit code */

	int  c, return_code;

#ifdef DEBUG
	fprintf(stderr,"catch_sig\n");
	fflush(stderr);
#endif
	if ((wait(&status)) == -1)
		exit(0);
	return_code = WEXITSTATUS(status);
#ifdef DEBUG
	fprintf(stderr,"catch_sig: return_code=%d\n", return_code);
	fflush(stderr);
#endif
	if (return_code != 0)  {
		fprintf(stdout, GGT(cu_error));
	}

	fprintf(stdout, GGT(cu_newline));
	switch (return_code) {
		case 0:
			break;;
		case 29:
			fprintf(stdout, GGT(string_killedCU));
			break;
		case 39:
			fprintf(stdout, GGT(string_badPhone));
			break;
		case 40:
			fprintf(stdout, GGT(string_usageCU));
			break;
		case 41:
			fprintf(stdout, GGT(string_exceed58));
			break;
		case 43:
			fprintf(stdout, GGT(string_connectFail));
			break;
		case 45:
			fprintf(stdout, GGT(string_lostConnect));
			break;
		case 47:
			fprintf(stdout, GGT(string_lostCarrier));
			break;
		case 49:
			fprintf(stdout, GGT(string_dialFail));
			break;
		case 50:
			fprintf(stdout, GGT(string_scriptFail));
			break;
		case 51:
			fprintf(stdout, GGT(string_dialFail));
			break;
		case 52:
			fprintf(stdout, GGT(string_noDevice));
			break;
		case 53:
			fprintf(stdout, GGT(string_noSystem));
			break;
		default:
			fprintf(stdout, GGT(string_unknownFail));
			break;
		}
		/* insert some newlines */
	fprintf(stdout, GGT(cu_newline));
	fprintf(stdout, GGT(cu_msg));
	/* wait for user to press enter key */
	while ((c = getc(stdin)) != EOF) {
		c = toascii(c);
		if ((c == '\r')  || (c == '\n')) break;
	}

	exit(return_code);

}

static void
doExec(args)
char *args[];
{
	id_t pid;
#ifdef DEBUG
	fprintf(stderr,"dtcu execProgram %s\n",args[0]);
#endif
	switch (pid = fork()) {
		case 0: /* child */
			execvp(args[0], args);
			perror(args[0]);
			exit(255);
		default:
			/* maybe set the button insensitive */
			return;
	}
			
} /* doExec */


