/*		copyright	"%c%" 	*/

#ident	"@(#)fold:fold.c	1.1.8.1"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
*/

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#define _XOPEN_SOURCE 1
#include <stdio.h>
#include <sys/types.h>	/* for off_t,  etc...			*/
#include <sys/euc.h>	/* for ISASCII, ISSET?, etc...		*/
#include <stdlib.h>	/* for strtol, etc...  			*/
#include <errno.h>	/* for errno, etc... 			*/
#include <string.h>	/* for strerror, etc...			*/
#include <getwidth.h>	/* for eucwidth_t, getwidth, etc... 	*/
#include <locale.h>	/* for LC_*, setlocale, etc... 	  	*/
#include <pfmt.h>	/* for pfmt, MM_*, etc...		*/
#include <limits.h>	/* for LINE_MAX, etc...			*/
#include <ctype.h>	/* for isdigit, __ctype, etc...		*/
#include <wchar.h>	/* for iswctype etc...			*/

/*
 * fold - fold long lines for finite output devices
 *
 */

static int		fold =  80;
static int		bflag ;		/* for -b option */
static int		sflag ;		/* for -s option */
static char  		*wflag ;	/* for -w option */
static eucwidth_t	wp;
static wctype_t		blank;		/* for blank detection */

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

static char		*wtmp;		/* segment buffer pointer */
static int 		wlen;		/* size of segment buffer */
static int 		l_optind = 1;

static int 		width_type ;
#define			OLD_WIDTH 	1
#define			NEW_WIDTH 	2

static const char bad_opt1[] =
	"uxlibc:1:Illegal option -- %c\n";
static const char bad_opt2[] =
	"uxlibc:2:Option requires an argument -- %c\n";
static const char incorrect[] =
	":2:Incorrect usage\n" ;

static const char nomem[] =
	":87:Out of memory: %s\n" ;
static const char Usage[] =
	":88:Usage: fold [ -bs ] [ -w width | -width ] [ file ... ]\n";
static const char bad_num[] =
	":89:Bad number for fold\n";

#ifdef __STDC__
static void dofold(void) ;
#else
static void dofold() ;
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	register 	optc;
	char 		*badc ;		/* for strtol */
	int 		errflag = 0;	/* option error flag */
	char 		*arg ;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:fold");

	blank = wctype("blank");

	while(argc > l_optind ) {
		arg = argv[l_optind] ;

		if ('-' != arg[0] || '\0' == arg[1])
			break;

		if ('-' == arg[1]) {
			l_optind++;
			break;
		}
		arg++ ;
		while (arg && (optc = *arg)) {
			if(isdigit(optc)) {
				if (width_type  == NEW_WIDTH) {
					errflag++ ;
					pfmt(stderr, MM_ERROR, incorrect);
					break ;
				}
				width_type = OLD_WIDTH;
				wflag = arg;
				break;
			}
			arg++ ;
			switch(optc) {
			case 'b':
				bflag++;
				break;
			case 's':
				sflag++;
				break;
			case 'w':
				if (width_type  == OLD_WIDTH) {
					errflag++ ;
					pfmt(stderr, MM_ERROR, incorrect);
					break ;
				}
				width_type = NEW_WIDTH;
				if (!(*arg) && (l_optind + 1 >= argc)){
					pfmt(stderr, MM_ERROR,
						bad_opt2, optc) ;
					errflag++ ;
				}
				else {
					if (*arg) {
						wflag = arg ;
						arg = NULL ;
					}
					else {
						l_optind++ ;
						wflag = argv[l_optind];
					}
				}
				break;
			default:
				pfmt(stderr, MM_ERROR, bad_opt1, optc);
				errflag++ ;
			}
			if(errflag)
				goto endopt;
		}
		l_optind++ ;
	}
endopt:

	if (errflag) {
		pfmt(stderr, MM_ACTION, Usage) ;
		exit(1);
	}
	if (wflag) {
		fold = strtol((char *)wflag, &badc, 10) ;
		if (*badc != '\0' || fold <= 0 ) {
			pfmt(stderr, MM_ERROR, bad_num) ;
			exit(1);
		}
	}

	if (sflag) {
		wlen = LINE_MAX ;
		wtmp = malloc(wlen);
		if (wtmp == NULL) {
			pfmt(stderr, MM_ERROR, nomem, strerror(errno)) ;
			exit(1) ;
		}
	}


	argc -= l_optind ;
	argv = &argv[l_optind] ;
	getwidth(&wp);

	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				pfmt(stderr, MM_ERROR, ":56:%s: %s\n",
					argv[0],strerror(errno));
				exit(1);
			}
			argc--, argv++;
		}
		dofold() ;
	} while (argc > 0);
	exit(0);
	/* NOTREACHED */
}

static void
#ifdef __STDC__
dofold(void)
#else
dofold()
#endif
{
	register ncol = 0;
	register scol = 0;
	register col = 0;
	register int c;
	register eucleft = 0;
	register eucb = 0;
	register scrw = 0;
	register char *bp = NULL; 	/* word buffer pointer */

	while ((c = getc(stdin)) != EOF) {

		if (eucleft > 0) {
			if (c < 0240) {	/* ASCII or C1 byte */
				eucleft = 0;
			} else {
				if (bp)
					*bp++ = c & 0377 ;
				else
					(void )putchar(c);
				eucleft--;
				continue ;
			}
		}

		switch (c) {
		case '\r':
		case '\n':
			ncol = 0;
			break;
		case '\t':
			if(bflag)
				ncol = col + 1 ;
			else
				ncol = (col + 8) &~ 7;
			eucb = 1;
			break;
		case '\b':
			if (bflag)
				ncol = col + 1 ;
			else
				ncol = col ? col - 1 : 0 ;
			eucb = 1;
			break;
		default:
			if (!wp._multibyte || ISASCII(c)) {
				eucb = scrw = 1;
			}
			else if (ISSET2(c)) {
				eucb = wp._eucw2 + 1;/* add SS2 byte */
				scrw = wp._scrw2;
			}
			else if (ISSET3(c)) {
				eucb = wp._eucw3 + 1;/* add SS3 byte */
				scrw = wp._scrw3;
			}
			else if (c < 0240) {		/* C1 char */
				eucb = scrw = 1;
			}
			else {				/* ISSET1 */
				eucb = wp._eucw1;
				scrw = wp._scrw1;
			}
			if ((eucleft = eucb - 1) < 0) {
				eucleft =  0;
				eucb = scrw = 1;
			}
			if (bflag)
				ncol = col + eucb;
			else
				ncol = col + scrw;
			break ;
		} /* switch */

		if (bp) {
			if ( c == '\n' || ncol > fold ) {
				if (c == '\n') {
					if (bp != wtmp) {
						*bp = '\0';
						(void)
						fputs(wtmp, stdout);
					}
					col = 0 ;
				} else {
					(void)putchar('\n');
					if (bp != wtmp) {
						*bp = '\0';
						(void)
						fputs(wtmp, stdout);
						col = col - scol;
					} else
						col = 0 ;
				}
				bp  = NULL;
				(void)putchar(c);
			} else {
				if (iswctype(c,blank)) {
					if (bp != wtmp) {
						*bp = '\0';
						(void)
						fputs(wtmp, stdout);
					}
					(void)	putchar(c);
				} else {
					if(bp + eucb >= &wtmp[wlen-1]){
						off_t woff  = bp - wtmp;
						wlen += 1024;
						wtmp = realloc(wtmp, wlen);
						if(wtmp == NULL) {
							pfmt(stderr, MM_ERROR, nomem, strerror(errno));
							exit(1);
						}
						bp = wtmp + woff;
					}
					*bp++ = c & 0377;
				}
			}
		} else {
			if (ncol > fold) {
				(void)putchar('\n');
				col = 0;
			}
			(void)putchar(c);
		}
		switch (c) {
		case '\r':
		case '\n':
			col = 0;
			break;
		case '\t':
			if (bflag)
				col++ ;
			else {
				col += 8;
				col &= ~7;
			}
			break;
		case '\b':
			if (bflag)
				col++ ;
			else if(col)
				col = col - 1;
			break;
		default:
			if (bflag)
				col += eucb;
			else
				col += scrw;
			break;
		}
		if (sflag && iswctype(c,blank)) {
			bp = wtmp;
			scol = col;
		}
	}
}

