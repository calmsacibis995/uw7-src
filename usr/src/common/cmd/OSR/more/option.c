#ident	"@(#)OSRcmds:more/option.c	1.1"
#pragma comment(exestr, "@(#) option.c 55.1 96/05/28 ")
/*
 *	Copyright (C) 1994-1996 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential. 
 */
/*
 * Copyright (c) 1988 Mark Nudleman
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Modification History
 * L000	scol!anthonys	15 Apr 94
 * - Created from BSD4.4 sources, with modifications to support some old
 *   functionality of the previous implementation, plus modifications
 *   to support the requirements of POSIX.2 and XPG4.
 *
 * L001	scol!anthonys	17 Oct 94
 * - Fixed bug in recognition of "--" as the end of options.
 *
 * S002 johng@sco.com   17 MAY 96
 * - restored -n and -number option. BUG HUA-1-23
 * - restored -r functionality       BUG HUA-1-6
 */

#ifndef lint
static char sccsid[] = "@)#(option.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <stdio.h>
#include "less.h"

#ifdef INTL
#  include      <locale.h>
#  include      "more_msg.h"
   extern nl_catd catd;
#else
#  define MSGSTR(id,def) (def)
#endif

int top_scroll;			/* Repaint screen from top */
int bs_mode = 0;		/* How to process backspaces */
int caseless;			/* Do "caseless" searches */
int cbufs = 10;			/* Current number of buffers */
int linenums = 1;		/* Use line numbers */
int quit_at_eof = 0;
int squeeze;			/* Squeeze multiple blank lines into one */
int tabstop = 8;		/* Tab settings */
int tagoption;
int dflag = 0;			/* Set for additional text at the prompt */
int vflag = 0;			/* Translate non printable characters */
int rflag = 0;			/* Translate \r to ^M            S002 */
int lflag = 1;			/* Translate \f to  ^L           S002 */
int Rflag = 0;			/* Restrict Shell escape	 S002 */


char *firstcommand = NULL;
char *firstsearch = NULL;
extern int sc_height;

option(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	static int sc_window_set = 0;
	int ch;
	char *p;
	char *plus_arg = NULL;

	/* backward compatible processing for "+/search" */
	char **a;
	for (a = argv; *a; ++a){
		if (!strcmp(*a, "--"))			/* L001 */
			break;
		if ((*a)[0] == '+'){
			if ((*a)[1] == '/')
				(*a)[0] = '-';
			else{
				int alldigit = 1;
				plus_arg = *a;
				while(*plus_arg = *(plus_arg +1)){
					if (!isdigit(*plus_arg++))
						alldigit = 0;
				}
				if (alldigit)
					*plus_arg = 'g';
				plus_arg = *a;
				*a = "-P";
				break;
			}
		}
	}

	optind = 1;		/* called twice, re-init getopt. */
	while ((ch = getopt(argc, argv, "0123456789/:cdeilmn:p:Pst:urRvwx:f")) != EOF)
		switch((char)ch) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			/*
			 * kludge: more was originally designed to take
			 * a number after a dash.
			 */
			if (!sc_window_set) {
				p = argv[optind - 1];
				if (p[0] == '-' && p[1] == ch && !p[2])
					sc_height = atoi(++p);
				else
					sc_height = atoi(argv[optind] + 1);
				sc_window_set = 1;
			}
			break;
		case '/':
			firstsearch = optarg;
			break;
		case 'c':
			top_scroll = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'e':
			quit_at_eof = 1;
			break;
		case 'i':
			caseless = 1;
			break;
		case 'm': /* turn off line number reporting  ^g cmd */
			linenums = 0;
			break;
		case 'n': /* if not set here look in screen.c line 264 */
			sc_height = atoi( optarg);	/* S002 */
			if ( sc_height < 5 )
				sc_height = -1;
			sc_window_set = 1;              /* S002 */
			break;
		case 'p':
			firstcommand = optarg;
			break;
		case 's':
			squeeze = 1;
			break;
		case 't':
			tagoption = 1;
			findtag(optarg);
			break;
		case 'u':
			bs_mode = 1;
			break;
		case 'l':
			lflag = 0;  /* reversed of real flag */
			break;
		case 'r':
			rflag = 1;
			break;
		case 'R':	/* retrict shell escape via ! vi or ed */
			Rflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'w':	/* Ignore, since this is now the default behaviour */
			break;
		case 'x':
			tabstop = atoi(optarg);
			if (tabstop <= 0)
				tabstop = 8;
			break;
		case 'f':	/* ignore -f, compatability with old more */
			break;
		case 'P':
			if (plus_arg){			/* Internally generated -P */
				firstcommand = plus_arg;
				break;
			}
			/* FALLTHROUGH */
		case '?':
		default:
			fprintf(stderr, MSGSTR(MORE_ERR_USAGE,
"usage: more [-cdeiurRlsv] [-n ##] [-t tag] [-x tabs] \n\t    [+command | -p command] [-/ pattern] [-#] [file ...]\n"));
			exit(1);
		}
	return(optind);
}
