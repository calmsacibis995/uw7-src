/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)domacro.c	1.2"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include "ftp_var.h"

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <pfmt.h>

#define	MAX_R_DEPTH	20
int	recursive_depth = 0;


domacro(argc, argv)
	int argc;
	char *argv[];
{
	register int i, j;
	register char *cp1, *cp2;
	int count = 2, loopflg = 0;
	char line2[LINSIZ];
	extern char **glob(), *globerr;
	struct cmd *getcmd(), *c;
	extern struct cmd cmdtab[];

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":108:(macro name) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":118:Usage: %s macro_name.\n", argv[0]);
		code = -1;
		return;
	}
	for (i = 0; i < macnum; ++i) {
		if (!strncmp(argv[1], macros[i].mac_name, 9)) {
			break;
		}
	}
	if (i == macnum) {
		pfmt(stdout, MM_NOSTD, ":119:'%s' macro not found.\n", argv[1]);
		code = -1;
		return;
	}

	if ((recursive_depth++) > MAX_R_DEPTH ) {
		pfmt(stdout, MM_NOSTD, ":120:Reached the maximum of %d recursive calls\n",MAX_R_DEPTH);
		code = -1;
		return;
	}

	(void) strcpy(line2, line);
TOP:
	cp1 = macros[i].mac_start;
	while (cp1 != macros[i].mac_end) {
		while (isspace(*cp1)) {
			cp1++;
		}
		cp2 = line;
		while (*cp1 != '\0') {
		      switch(*cp1) {
		   	    case '\\':
				 *cp2++ = *++cp1;
				 break;
			    case '$':
				 if (isdigit(*(cp1+1))) {
				    j = 0;
				    while (isdigit(*++cp1)) {
					  j = 10*j +  *cp1 - '0';
				    }
				    cp1--;
				    if (argc - 2 >= j) {
					(void) strcpy(cp2, argv[j+1]);
					cp2 += strlen(argv[j+1]);
				    }
				    break;
				 }
				 if (*(cp1+1) == 'i') {
					loopflg = 1;
					cp1++;
					if (count < argc) {
					   (void) strcpy(cp2, argv[count]);
					   cp2 += strlen(argv[count]);
					}
					break;
				}
				/* intentional drop through */
			    default:
				*cp2++ = *cp1;
				break;
		      }
		      if (*cp1 != '\0') {
			 cp1++;
		      }
		}
		*cp2 = '\0';
		makeargv();
		c = getcmd(margv[0]);
		if (c == (struct cmd *)-1) {
			pfmt(stdout, MM_NOSTD, ":4:?Ambiguous command\n");
			code = -1;
		}
		else if (c == 0) {
			pfmt(stdout, MM_NOSTD, ":5:?Invalid command\n");
			code = -1;
		}
		else if (c->c_conn && !connected) {
			pfmt(stdout, MM_NOSTD, ":6:Not connected.\n");
			code = -1;
		}
		else {
			if (verbose) {
				pfmt(stdout, MM_NOSTD, ":32:%s\n",line);
			}
			(*c->c_handler)(margc, margv);
#define CTRL(c) (c & 037)
			if (bell && c->c_bell) {
				(void) putchar(CTRL('g'));
			}
			(void) strcpy(line, line2);
			makeargv();
			argc = margc;
			argv = margv;
		}
		if (cp1 != macros[i].mac_end) {
			cp1++;
		}
	}
	if (loopflg && ++count < argc) {
		goto TOP;
	}
	recursive_depth--;
}
