/*		copyright	"%c%" 	*/

#ident	"@(#)vfmtmsg.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

#include "stdio.h"
#include "string.h"
#include "oam.h"
#include <locale.h>

/**
 ** vfmtmsg()
 **/

void
#if	defined(__STDC__)
vfmtmsg (
	char *			label,
	int			severity,
	int                     seqnum,
	long int                arraynum,
        int                     logind,
        va_list                 argp
)
#else
vfmtmsg ( label, severity, seqnum, arraynum, logind, argp)
	char			*label;
	int			severity;
	int                     seqnum;
	long int                arraynum;
        int                     logind;
        va_list                 argp;
#endif
{
	stva_list		moreargp,
				skipfmts();
        char                    buf[MSGSIZ];
        char                    text[MSGSIZ];
        char                    msg_text[MSGSIZ];

        (void)setlocale(LC_ALL, "");
        setcat("uxlp");
        setlabel(label);
        sprintf(msg_text,":%d:%s\n",seqnum,agettxt(arraynum,buf,MSGSIZ));
        if (logind != LOG)
           vpfmt(stderr,severity,msg_text,argp);
        else
           vlfmt(stderr,severity,msg_text,argp);
	moreargp = skipfmts(msg_text,argp);
        sprintf(text,"%s",agettxt(arraynum + 1,buf,MSGSIZ));
        if (strncmp(text, "", 1) != 0)  {
           sprintf(msg_text,":%d:%s\n",seqnum + 1,text);
           vpfmt(stderr,MM_ACTION,msg_text,moreargp.ap);
        }
}
