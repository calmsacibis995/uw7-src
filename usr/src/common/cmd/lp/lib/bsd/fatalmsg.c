/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*		copyright	"%c%" 	*/

#ident	"@(#)fatalmsg.c	1.2"
#ident	"$Header$"

#include <stdio.h>
#include <string.h>
#include <locale.h>
#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#define WHO_AM_I	I_AM_OZ		/* to get oam.h to unfold */
#include "oam.h"
#include "lpd.h"

/*
 * Report fatal error and exit
 */
/*VARARGS1*/
void
#if defined (__STDC__)
fatal(char *fmt, ...)
#else
fatal(fmt, va_alist)
char	*fmt;
va_dcl
#endif
{
	va_list	argp;

	if (Rhost)
		(void)printf("%s: ", Lhost);
	printf("%s: ", Name);
	if (Printer)
		(void)printf("%s: ", Printer);
#if defined (__STDC__)
	va_start(argp, fmt);
#else
	va_start(argp);
#endif
	(void)vprintf(fmt, argp);
	va_end(argp);
	putchar('\n');
	fflush(stdout);
	done(1);		/* defined by invoker */
	/*NOTREACHED*/
}

/*
 * Format lp error message to stderr
 * (this will probably change to remain compatible with LP)
 */
/*VARARGS1*/
void
#if defined (__STDC__)
_lp_msg(int seqnum, long msgid, int logind, va_list args)
#else
_lp_msg(seqnum, msgid, logind, args)
int     seqnum;
long	msgid;
int     logind;
va_list	args;
#endif
{
	stva_list		moreargs,
				skipfmts();
	char			label[20];
	char                    buf[MSGSIZ];
	char                    text[MSGSIZ];
	char                    msg_text[MSGSIZ];


	strcpy(label, "UX:");
	strcat(label, basename(Name));

        (void)setlocale(LC_ALL, "");
        setcat("uxlp");
        setlabel(label);
        sprintf(msg_text,":%d:%s\n",seqnum,agettxt(msgid,buf,MSGSIZ));
	if (logind != LOG)
	    vpfmt(stderr,ERROR,msg_text,args);
	else
	    vlfmt(stderr,ERROR,msg_text,args);
	moreargs = skipfmts(msg_text, args);
	sprintf(text,"%s",agettxt(msgid + 1,buf,MSGSIZ));
	if (strncmp(text, "", 1) != 0)  {
	    sprintf(msg_text,":%d:%s\n",seqnum + 1,text);
	    vpfmt(stderr,MM_ACTION,msg_text,moreargs.ap);
	}

}

/*
 * Format lp error message to stderr
 */
/*VARARGS1*/
void
#if defined (__STDC__)
lp_msg(int seqnum, long msgid, int logind, ...)
#else
lp_msg(seqnum, msgid, logind, va_alist)
int     seqnum;
long	msgid;
int     logind;
va_dcl
#endif
{
	va_list	argp;

#if defined (__STDC__)
	va_start(argp, msgid);
#else
	va_start(argp);
#endif
        _lp_msg(seqnum, msgid, logind, argp);
	va_end(argp);
}

/*
 * Report lp error message to stderr and exit
 */
/*VARARGS1*/
void
#if defined (__STDC__)
lp_fatal(int seqnum, long msgid, int logind, ...)
#else
lp_fatal(seqnum, msgid, logind, va_alist)
int     seqnum;
long	msgid;
int     logind;
va_dcl
#endif
{
	va_list	argp;

#if defined (__STDC__)
	va_start(argp, msgid);
#else
	va_start(argp);
#endif
        _lp_msg(seqnum, msgid, logind, argp);
	va_end(argp);

	done(1);			/* Supplied by caller */
	/*NOTREACHED*/
}
