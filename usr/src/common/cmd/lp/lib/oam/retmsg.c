/*		copyright	"%c%" 	*/

#ident	"@(#)retmsg.c	1.3"
#ident	"$Header$"
/* LINTLIBRARY */

#include <stdio.h>
#include <string.h>
#include "oam.h"
#include <locale.h>
#include <unistd.h>

/**
 ** retmsg()
 **/

char *
#if	defined(__STDC__)
retmsg (
	int                     seqnum,
	long int                arraynum
)
#else
retmsg ( seqnum, arraynum )
	int                     seqnum;
	long int                arraynum;
#endif
{
        static char             buf[MSGSIZ];
        char                    msg_text[MSGSIZ];
	char			*msg;

        (void)setlocale(LC_ALL, "");
        setcat("uxlp");
        sprintf(msg_text,":%d",seqnum);
        msg = gettxt(msg_text,agettxt(arraynum,buf,MSGSIZ));
        return (msg);
}
