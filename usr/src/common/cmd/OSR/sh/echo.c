#ident	"@(#)OSRcmds:sh/echo.c	1.1"
#pragma comment(exestr, "@(#) echo.c 23.1 91/09/05 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1991 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/* #ident	"@(#)sh:echo.c	1.4.1.2" */
/*
 *	UNIX shell
 */

/*
 *	MODIFICATION HISTORY
 *
 *	L000	21 Sep 89	scol!howardf
 *	- Fixed pointer problem with the inbuilt echo command.
 *	L001	04sep91		scol!hughd
 *	- 3.2.4 DS ctype.h declares extern int toascii(int),
 *	  but mac.h has #defined toascii: therefore #undef it
 */
#include	"defs.h"
#undef		toascii					/* L001 */
#include	<ctype.h>				/* L000 */
#include	"../include/osr.h"

#define	exit(a)	flushb();return(a)

extern int exitval;

echo(argc, argv)
int argc;				
unsigned char **argv;
{
	register unsigned char	*cp;
	register int	i, wd;
	int	j, nonl = 0;
	
	if(--argc == 0) {
		prc_buff('\n');
		exit(0);
	}
	
	if (!strncmp((char *)argv[1], "-n", 3))
        {
 		nonl++;
		*++argv;
		argc--;
	}

	for(i = 1; i <= argc; i++) 
	{
		sigchk();
		for(cp = argv[i]; *cp; cp++) 
		{
		  if(*cp == '\\')
			switch(*++cp) 
			{
				case 'b':
					prc_buff('\b');
					continue;

				case 'c':
					exit(0);

				case 'f':
					prc_buff('\f');
					continue;

				case 'n':
					prc_buff('\n');
					continue;

				case 'r':
					prc_buff('\r');
					continue;

				case 't':
					prc_buff('\t');
					continue;

				case 'v':
					prc_buff('\v');
					continue;

				case '\\':
					prc_buff('\\');
					continue;
				case '0':		/* vv L000 vv */
					j = wd = 0;
					cp++;
					while (j++ < 3) {
						if (!isdigit(*cp))
							break;
						else if (toint(*cp) > 7)
							break;
						wd <<= 3;
						wd |= toint(*cp);
						cp++;
					}
					prc_buff(wd);
					--cp;
					continue;	/* ^^ L000 ^^ */

				default:
					cp--;
			}
			prc_buff(*cp);
		}
		if (!nonl || i != argc)
			prc_buff(i == argc ? '\n' : ' ');
	}
	exit(0);
}
