#ident	"@(#)OSRcmds:dspmsg/dspmsg.c	1.1"
#pragma comment(exestr, "@(#) dspmsg.c 26.1 95/07/11 ")
/*
 *      Copyright (C) 1992-1995 The Santa Cruz Operation, Inc.
 *              All rights reserved.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * dspmsg -- A utility that prints out a particular message from
 *	     a message catalogue.
 *	     This program is designed for use by shell scripts
 *	     so that they can print out language dependent messages
 *	     (instead of hardwiring in message using 'echo "Hello"'
 *	      for example.)
 *
 *	Modification History
 *
 *	10 June 92	scol!anthonys	creation
 *
 *	27th January 1994	scol!ashleyb	L001
 *	- Call setlocale() before catopen.
 *	05 Jul 1995	scol!ianw	L002
 *	- Changed the second argument of catopen() to MC_FLAGS so dspmsg
 *	  locates the specified message catalogue in the same way other
 *	  utilities locate their message catalogues.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <nl_types.h>
#include <locale.h>					/* L001 */
/*
#include "errormsg.h"
*/
#include "dspmsg_msg.h"

static void	usage(void);

char *command_name = "";

static nl_catd catd;

main(argc, argv)
char **argv;
{
	char *ptr;
	nl_catd cat_d;

	char *catalogue_name = "";
	int  set_id = 1;
	int  message_number;
	char *default_mess = "";

	setlocale(LC_ALL,"");				/* L001 */
	catd = catopen(MF_DSPMSG, MC_FLAGS);

/*
 * parse the command. Note the problems caused by the OSF syntax
 * of allowing "-s" part way down the command line, while we still
 * permit a more conventional convention of options before other
 * arguments.
 */
	if (argc < 3)
		usage();
	command_name = *argv;
	argc--;
	argv++;
	if ( strcmp(*argv, "-s") == 0){
		argc--;
		argv++;
		errno = 0;
		set_id = strtol(*argv, &ptr, 10);
		if (errno || *ptr != '\0' || set_id < 0 ){
			errorl(MSGSTR(DSPMSG_ERR_ILLSOPT, \
				"Illegal argument %s to -s option"), *argv);
			exit(1);
		}
		argc--;
		argv++;
	}
	catalogue_name = *argv;
	argc--;
	argv++;
	
	if ( strcmp(*argv, "-s") == 0){
		argc--;
		argv++;
		errno = 0;
		set_id = strtol(*argv, &ptr, 10);
		if (errno || *ptr != '\0' || set_id < 0 ){
			errorl(MSGSTR(DSPMSG_ERR_ILLSOPT, \
				"Illegal argument %s to -s option"), *argv);
			exit(1);
		}
		argc--;
		argv++;
	}
	errno = 0;
	message_number = strtol(*argv, &ptr, 10);
	if (errno || *ptr != '\0' || message_number < 0 ){
		errorl(MSGSTR(DSPMSG_ERR_ILLMESSNO, \
			"Illegal message number %s"), *argv);
		exit(1);
	}

	if ( --argc > 0)
		default_mess = *++argv;
/*
 * now we look for the message and print it
 */

	if ((cat_d = catopen(catalogue_name, MC_FLAGS)) == (nl_catd)-1){/*L002*/
		if ( *default_mess == '\0'){
			errorl(MSGSTR(DSPMSG_ERR_NOMSG, "Could not open message catalogue %s, and no default message"), catalogue_name);
			exit(2);
		}
	}

	*argv-- = catgets(cat_d, set_id, message_number, default_mess);
	*argv = "dspmsg";
	execv("/usr/bin/printf", argv);
	psyserrorl(errno, MSGSTR(DSPMSG_ERR_EXECFAIL, "Could not execute command printf"));
	exit(4);
	/* NOTREACHED */
}

static void usage()
{
	(void) fprintf(stderr, MSGSTR(DSPMSG_ERR_USAGE, "Usage - dspmsg catalogue_name [-s set_number] message_number [default message [arguments ...]]\n"));
	exit(1);
	/* NOTREACHED */
}
