/*		copyright	"%c%" 	*/

#ident	"@(#)delsystem.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

# include	<string.h>
# include	<stdlib.h>
# include	<errno.h>

# include	"lp.h"
# include	"systems.h"
# include	"printers.h"

# define	SEPCHARS	":\n"

/**
 ** delsystem()
 **/

#if	defined(__STDC__)
int delsystem ( const char * name )
#else
int delsystem ( name )
char	*name;
#endif
{
    FILE	*fpi;
    FILE	*fpo;
    char	*cp;
    char	*file;
    char	buf[BUFSIZ];
    char	c;
    int		all = 0;
    int		errtmp;
    int		n;
    level_t	lid;

    if ((file = tempnam(ETCDIR, "lpdat")) == NULL)
    {
	errno = ENOMEM;
	return(-1);
    }

    if ((fpi = open_lpfile(Lp_NetData, "r", MODE_READ)) == NULL)
    {
	Free(file);
	return(-1);
    }

    if ((fpo = open_lpfile(file, "w", MODE_READ)) == NULL)
    {
	errtmp = errno;
	(void) close_lpfile(fpi);
	Free(file);
	errno = errtmp;
	return(-1);
    }

    if (STREQU(NAME_ALL, name))
	all = 1;

    while (fgets(buf, BUFSIZ, fpi) != NULL)
    {
	if (*buf != '#' && *buf != '\n')
	    if ((cp = strpbrk(buf, SEPCHARS)) != NULL)
	    {
		if (all)
		    continue;
		
		c = *cp;
		*cp = '\0';
		if (STREQU(name, buf))
		    continue;
		*cp = c;
	    }

	if (fputs(buf, fpo) == EOF)
	{
	    errtmp = errno;
	    (void) close_lpfile(fpi);
	    (void) close_lpfile(fpo);
	    (void) Unlink(file);
	    Free(file);
	    errno = errtmp;
	    return(-1);
	}
    }

    (void) close_lpfile(fpi);
    (void) close_lpfile(fpo);
    (void) _Rename(file, Lp_NetData);
    Free(file);

    lid = PR_SYS_PUBLIC;
    while ((n=lvlfile (Lp_NetData, MAC_SET, &lid)) < 0 && errno == EINTR)
	continue;
    
    if (n < 0 && errno != ENOSYS)
	return -1;
    return 0;
}
