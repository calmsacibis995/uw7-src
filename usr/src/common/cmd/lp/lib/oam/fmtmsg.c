/*		copyright	"%c%" 	*/

#ident	"@(#)fmtmsg.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

#include "stdio.h"
#include "string.h"
#include "oam.h"
#include <locale.h>

/**
 ** fmtmsg()
 **/

void
#if	defined(__STDC__)
fmtmsg (
	char *			label,
	int			severity,
	int                     seqnum,
	long int                arraynum,
        int                     logind,
                                ...
)
#else
fmtmsg ( label, severity, seqnum, arraynum, logind, va_alist)
	char			*label;
	int			severity;
	int                     seqnum;
	long int                arraynum;
        int                     logind;
        va_dcl
#endif
{
	stva_list		moreargs,
				skipfmts();
        va_list                 args;
        char                    buf[MSGSIZ];
        char                    text[MSGSIZ];
        char                    msg_text[MSGSIZ];

#ifdef  __STDC__
        va_start(args, logind);
#else
        va_start(args);
#endif

        (void)setlocale(LC_ALL, "");
        setcat("uxlp");
        setlabel(label);
        sprintf(msg_text,":%d:%s\n",seqnum,agettxt(arraynum,buf,MSGSIZ));
        if (logind != LOG)
           vpfmt(stderr,severity,msg_text,args);
        else
           vlfmt(stderr,severity,msg_text,args);
	moreargs = skipfmts(msg_text, args);
        sprintf(text,"%s",agettxt(arraynum + 1,buf,MSGSIZ));
        if (strncmp(text, "", 1) != 0)  {
           sprintf(msg_text,":%d:%s\n",seqnum + 1,text);
           vpfmt(stderr,MM_ACTION,msg_text,moreargs.ap);
        }
}

