/*		copyright	"%c%" 	*/

#ident	"@(#)putsystem.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

# include	<sys/types.h>
# include	<stdio.h>
# include	<string.h>
# include	<errno.h>
# include	<stdlib.h>

# include	"lp.h"
# include	"systems.h"
# include	"printers.h"


/**
 ** putsystem() - WRITE SYSTEM STRUCTURE TO DISK FILES
 **/


#if	defined(__STDC__)
int putsystem ( const char * name, const SYSTEM * sysbufp )
#else
int putsystem ( name, sysbufp )
char	*name;
SYSTEM	*sysbufp;
#endif
{
    FILE	*fp;
    level_t	lid;
    int		n;
    char	*systype;

    /*
    **	Validate the arguments.
    **	Must have a name, but not "all".
    **	SYSTEM must have a provider, address, and a protocol.
    */
    if (!name || !*name || STREQU(name, NAME_ALL))
    {
	errno = EINVAL;
	return(-1);
    }

    if (!sysbufp ||
       (sysbufp->protocol != S5_PROTO && sysbufp->protocol != BSD_PROTO
			&& sysbufp->protocol != NUC_PROTO))
    {
	errno = EINVAL;
	return(-1);
    }

    /*
    **	Since, this may be an update of an existing entry, delsystem
    **	is called to prevent any duplication.
    */
    (void) delsystem(name);

    if ((fp = open_lpfile(Lp_NetData, "a", MODE_READ)) == NULL)
	return(-1);

    if (sysbufp->protocol == S5_PROTO)
        systype = NAME_S5PROTO;
    else if (sysbufp->protocol == BSD_PROTO)
         systype = NAME_BSDPROTO;
    else
         systype = NAME_NUCPROTO;

    (void) fprintf(fp, "%s:", name);
    (void) fprintf(fp, "%s:", "x");	/* passwd */
    (void) fprintf(fp, "%s:", "-");	/* reserved1 */
    (void) fprintf(fp, "%s:", systype);
    (void) fprintf(fp, "-:");	/* reserved2 */
    if (sysbufp->timeout < 0)
	(void) fprintf(fp, "n:");
    else
	(void) fprintf(fp, "%d:", sysbufp->timeout);
    
    if (sysbufp->retry < 0)
	(void) fprintf(fp, "n:");
    else
    	(void) fprintf(fp, "%d:", sysbufp->retry);

    (void) fprintf(fp, "-:");	/* reserved3 */
    (void) fprintf(fp, "-:");	/* reserved4 */
    if (sysbufp->comment)
    	(void) fprintf(fp, "%s\n", sysbufp->comment);
    else
        (void) fprintf(fp, "\n");
    (void) close_lpfile(fp);
    lid = PR_SYS_PUBLIC;
    while ((n=lvlfile (Lp_NetData, MAC_SET, &lid)) < 0 && errno == EINTR)
	continue;
    
    if (n < 0 && errno != ENOSYS)
	return -1;

    return 0;
}
