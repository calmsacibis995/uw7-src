#ident	"@(#)pcintf:bridge/pcidebug.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)pcidebug.c	6.4	LCC);	/* Modified: 21:08:16 6/12/91 */

/*****************************************************************************

	Copyright (c) 1985 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <lmf.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "log.h"

/* Defines for NLS */
/* N = file name, L = Lang, C = Codeset */
#ifdef MSDOS
#define NLS_PATH "c:/pci/%N/%L.%C;c:/usr/lib/merge/%N/%L.%C"
#else
#define NLS_PATH "/usr/pci/%N/%L.%C:/usr/lib/merge/%N/%L.%C"
#endif

#define NLS_FILE "dosmsg"
#define NLS_FILE_MRG "mergemsg"
#define NLS_LANG "En"

#define NLS_DOMAIN "LCC.PCI.UNIX.PCIDEBUG"

#define	strequ(s1, s2)	(strcmp((s1), (s2)) == 0)

#ifdef	SYS5
#define	SIG_DBG1	SIGUSR1
#endif	/* SYS5 */

#ifndef	SIG_DBG1
#include	"Unknown Unix version"
#endif	/* SIG_DBG1 */

char *progname;

int		main		PROTO((int, char **));
LOCAL long	getBitList	PROTO((char *));


main(argc, argv)
int
	argc;
char
	*argv[];
{
register int
	argN;				/* Current argument number */
register char
	*arg;				/* Current argument */
long
	setChans = 0,			/* Use this channel set */
	onChans = 0,			/* Turn these channels on */
	offChans = 0,			/* Turn these channels off */
	flipChans = 0,			/* Invert these channels */
	changeTypes = 0;		/* Types of changes requested */
int
	chanDesc,			/* Descriptor of debug channel file */
	svrPid;				/* Process id of server */
char
	chanName[32];			/* Name of debug channel file */
	int lmf_fd;

	lmf_fd = lmf_open_file(NLS_FILE, NLS_LANG, NLS_PATH);
#ifndef MSDOS	/* for MERGE */
	if ( lmf_fd < 0 )
		lmf_open_file(NLS_FILE_MRG, NLS_LANG, NLS_PATH);
#endif
	if ( lmf_fd >= 0 && lmf_push_domain(NLS_DOMAIN)) {
		fprintf(stderr,
			"%s: Can't push domain \"%s\", lmf_errno %d\n",
			argv[0], NLS_DOMAIN, lmf_errno);
	} 

	progname = argv[0];

	/* Validate usage -- check to see that first arg is a number */
	if (argc < 3 || !isdigit(*argv[1])) {
		fputs(lmf_format_string((char *)NULL, 0, 
			lmf_get_message("PCIDEBUG1",
			"%1: Usage: %1 <process ID> <[=+-~]chanList|on|off|close> [...]\n\t\tchanList = chanNum1[,chanNum2[...]]\n"),
			"%s", progname),
			stderr);
		exit(1);
	}

	/* Check to see that the number is the pid of an existing process */
	svrPid = atoi(argv[1]);
	if (kill(svrPid, 0) < 0) {
		if (errno == ESRCH)
			fputs(lmf_format_string((char *)NULL, 0,
				lmf_get_message("PCIDEBUG20",
				"%1: Process %2 doesn't exist\n"),
				"%s%d", progname, svrPid),
				stderr);
		else if (errno == EPERM)
			fputs(lmf_format_string((char *)NULL, 0,
				lmf_get_message("PCIDEBUG21",
				"%1: No permission to signal process %2\n"),
				"%s%d", progname, svrPid),
				stderr);

		else
			fputs(lmf_format_string((char *)NULL, 0,
				lmf_get_message("PCIDEBUG22",
				"%1: Can't signal process %2\n"),
				"%s%d" , progname, svrPid),
				stderr);

		exit(1);
	}

	/* Scan rest of argument and get new debug channel assignments */
	for (argN = 2; argN < argc; argN++) {
		arg = argv[argN];

		if (strequ(arg, "child"))		/* Change child */
			changeTypes |= CHG_CHILD;
		else if (strequ(arg, lmf_get_message("PCIDEBUG10", "off"))) {
			setChans = 0;
			changeTypes |= CHG_SET;
		}
		else if (strequ(arg, lmf_get_message("PCIDEBUG11", "on"))) {
		/* Turn everything on */
			setChans = 0xffffL;
			changeTypes |= CHG_SET;
		}
		else if (strequ(arg, lmf_get_message("PCIDEBUG12", "close")))
		/* Close log file */
			changeTypes |= CHG_CLOSE;
		else if (*arg == '=') {			/* Set abs. channels */
			setChans = getBitList(&arg[1]);
			changeTypes |= CHG_SET;
		}
		else if (*arg == '+') {			/* Add channels */
			onChans = getBitList(&arg[1]);
			changeTypes |= CHG_ON;
		}
		else if (*arg == '-') {			/* Remove channels */
			offChans = getBitList(&arg[1]);
			changeTypes |= CHG_OFF;
		}
		else if (*arg == '~') {			/* Toggle channels */
			flipChans = getBitList(&arg[1]);
			changeTypes |= CHG_INV;
		}
		else {					/* else Bad argument */
			fputs(lmf_format_string((char *)NULL, 0, 
				lmf_get_message("PCIDEBUG2",
				"%1: Invalid argument: \"%2\"\n"),
				"%s%s", progname, arg),
				stderr);
			exit(1);
		}
	}

	/* Invent file name and create it */
	sprintf(chanName, chanPat, svrPid);
	if ((chanDesc = open(chanName, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
	{
		fputs(lmf_format_string((char *)NULL, 0,
			lmf_get_message("PCIDEBUG23",
			"Cannot create channel file %1 (%2)\n"),
			"%s%d" , chanName, errno),
			stderr);
		exit(1);
	}

	/* Write the debug bits to channel file */
	write(chanDesc, &changeTypes, sizeof changeTypes);
	write(chanDesc, &setChans, sizeof setChans);
	write(chanDesc, &onChans, sizeof onChans);
	write(chanDesc, &offChans, sizeof offChans);
	write(chanDesc, &flipChans, sizeof flipChans);
	close(chanDesc);

	/* Signal the server to read the channel file */
	if (kill(svrPid, SIG_DBG1) < 0) {
		if (errno == ESRCH)
			fputs(lmf_format_string((char *)NULL, 0,
				lmf_get_message("PCIDEBUG24",
				"pcidebug: Process %1 disappeared\n"),
				"%d", svrPid),
				stderr);
		else if (errno == EPERM)
			fputs(lmf_format_string((char *)NULL, 0,
				lmf_get_message("PCIDEBUG25",
				"%1: Lost permission to signal process %2\n"),
				"%s%d" , progname, svrPid),
				stderr);

		else
			fputs(lmf_format_string((char *)NULL, 0,
				lmf_get_message("PCIDEBUG26",
				"%1: Can't signal process %2 (%3)\n"),
				"%s%d%d", progname, svrPid, errno),
				stderr);

		exit(1);
	}
	return 0;
}


/*
   getBitList: Decode a list of bit numbers
		return a long with the indicated bits set
*/

long
getBitList(bitString)
register char
	*bitString;
{
register long
	theBits = 0;			/* Decoded bits */
register int
	bitNum;				/* A bit number */
char
	*bitEnd;			/* Char that ended number conversion */

	/* Scan list of numbers and accumulate bits */
	for (;;) {
		bitNum = strtol(bitString, &bitEnd, 0x0);
		/* If a number was converted, add it's bit to mask */
		if (bitEnd != bitString)
			if (bitNum <= 0 || bitNum > 32)
				fputs(lmf_format_string((char *)NULL, 0,
					lmf_get_message("PCIDEBUG27",
					"%1: Bit %2 is out of range\n"),
					"%s%d" , progname, bitNum),
					stderr);
			else
				theBits |= 1L << bitNum - 1;

		bitString = bitEnd;

		while (!isdigit(*bitString))
			if (*bitString++ == '\0')
				return theBits;
	}
}
