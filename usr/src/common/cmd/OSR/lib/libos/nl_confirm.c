#ident	"@(#)OSRcmds:lib/libos/nl_confirm.c	1.1"
#pragma comment(exestr, "@(#) nl_confirm.c 25.1 92/08/12 ")
/*
 *	Copyright (C) 1992 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * The code marked with symbols from the list below, is owned
 * by The Santa Cruz Operation Inc., and represents SCO value
 * added portions of source code requiring special arrangements
 * with SCO for inclusion in any product.
 *  Symbol:		 Market Module:
 * SCO_BASE 		Platform Binding Code
 * SCO_ENH 		Enhanced Device Driver
 * SCO_ADM 		System Administration & Miscellaneous Tools
 * SCO_C2TCB 		SCO Trusted Computing Base-TCB C2 Level
 * SCO_DEVSYS 		SCO Development System Extension
 * SCO_INTL 		SCO Internationalization Extension
 * SCO_BTCB 		SCO Trusted Computing Base TCB B Level Extension
 * SCO_REALTIME 	SCO Realtime Extension
 * SCO_HIGHPERF 	SCO High Performance Tape and Disk Device Drivers
 * SCO_VID 		SCO Video and Graphics Device Drivers (2.3.x)
 * SCO_TOOLS 		SCO System Administration Tools
 * SCO_FS 		Alternate File Systems
 * SCO_GAMES 		SCO Games
 */

/* BEGIN SCO_BASE */

/* MODIFICATION HISTORY
 *
 * 	11 Feb 92	scol!harveyt
 * 	- Created, supports YESEXPR (a regular expression) if defined.
 */

#include <nl_types.h>
#include <langinfo.h>
#include <string.h>

#define CONFBUFSIZ	32		/* Buffer size for confirm string */

#ifdef YESEXPR
#  define DEFAULT_YES	"^[Yy]"
#  define YES_ITEM	((nl_item) YESEXPR)
#  include <regex.h>
   static regex_t yes_re;
#else
#  define DEFAULT_YES	"y"
#  define YES_ITEM	((nl_item) YESSTR)
#endif

static char *yesexpr;			/* String or regex to match for yes */

/*
 * Reads a string from file descriptor `fd', upto the newline character.
 * If it matches the YES_ITEM of the current locale, it returns 1, otherwise
 * returns 0.  If an error occurs before reading an answer, -1 is returned.
 *
 * Assumes the locale isn't going to change within the lifetime of the current
 * process.
 */
int
nl_confirm (int fd)
{
	char confbuf[CONFBUFSIZ+1];
	char *cp;
	int n;

	if (yesexpr == NULL) {
#ifdef INTL
		if ((yesexpr = nl_langinfo (YES_ITEM)) == NULL
		    || *yesexpr == '\0')
			yesexpr = DEFAULT_YES;
#else
		yesexpr = DEFAULT_YES;
#endif
#ifdef YESEXPR
		while (regcomp (&yes_re, yesexpr,
				REG_EXTENDED | REG_NOSUB) != 0) {
			if (yesexpr == DEFAULT_YES)
				return -1;
			else
				yesexpr = DEFAULT_YES;
		}
#endif
	}

	/* Read upto a newline, place at most CONFBUFSIZ bytes in confbuf */
	for (cp = confbuf;
	     (n = read (fd, cp, 1)) == 1 && *cp != '\n';
	     cp += (cp < (confbuf + CONFBUFSIZ)))
		;
	*cp = '\0';			/* NUL terminate it */

	/* Return -1 if a read error occurred, 0 if no string was entered */
	if (n == -1)
		return -1;
	if (cp == confbuf || *confbuf == '\0')
		return 0;

	/* Return 1 if response matches yesexpr, 0 otherwise */
#ifdef YESEXPR
	return (regexec (&yes_re, confbuf, (size_t) 0, NULL, 0) == 0);
#else
	return (strncmp (confbuf, yesexpr, strlen (yesexpr)) == 0);
#endif
}
/* END SCO_BASE */
