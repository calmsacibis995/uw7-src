/*		copyright	"%c%" 	*/

#ident	"@(#)mrecv.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */
# include	<errno.h>

# include	"lp.h"
# include	"msgs.h"

extern MESG	*lp_Md;

/*
** mrecv() - RECEIVE A MESSAGE
*/

int
mrecv (msgbuf, size)
char	*msgbuf;
int	size;
{
    int		n;

    /*
    **	Restart interrupted reads for binary compatibility.
    */
    do
	n = mread(lp_Md, msgbuf, size);
    while (n < 0 && errno == EINTR);
    
    /*
    **	Return EIDRM on disconnect for binary compatibility.
    */
    if (errno == EPIPE)
	errno = EIDRM;

    if (n <= 0)
	return(-1);

    return(getmessage(msgbuf, I_GET_TYPE));
}
