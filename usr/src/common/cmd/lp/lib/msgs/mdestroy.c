/*		copyright	"%c%" 	*/

#ident	"@(#)mdestroy.c	1.2"
#ident	"$Header$"
# include	<string.h>
# include	<stropts.h>
# include	<errno.h>
# include	<stdlib.h>
# include	<unistd.h>

# include	"lp.h"
# include	"msgs.h"

#if	defined(__STDC__)
int mdestroy ( MESG * md )
#else
int mdestroy (md)
    MESG	*md;
#endif
{
    if (md->type != MD_MASTER || md->file == NULL)
    {
	errno = EINVAL;
	return(-1);
    }

    if (fdetach(md->file) != 0)
        return(-1);

    if (ioctl(md->writefd, I_POP, "connld") != 0)
        return(-1);

    Free(md->file);
    md->file = NULL;

    (void) mdisconnect(md);

    return(0);
}
