#ident	"@(#)cscope:common/logdir.c	1.4"
/*
 *	logdir()
 *
 *	This routine does not use the getpwent(3) library routine
 *	because the latter uses the stdio package.  The allocation of
 *	storage in this package destroys the integrity of the shell's
 *	storage allocation.
 */

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#define	BUFSIZ	160

static char line[BUFSIZ+1];

static char *
field(p)
register char *p;
{
	while (*p && *p != ':')
		++p;
	if (*p) *p++ = 0;
	return(p);
}

char *
logdir(name)
char *name;
{
	register char	*p;
	register int	i, j;
	int	pwf;
	
	/* attempt to open the password file */
	if ((pwf = myopen("/etc/passwd", 0, 0)) == -1)
		return(0);
		
	/* find the matching password entry */
	do {
		/* get the next line in the password file */
		i = read(pwf, line, BUFSIZ);
		for (j = 0; j < i; j++)
			if (line[j] == '\n')
				break;
		/* return a null pointer if the whole file has been read */
		if (j >= i)
			return(0);
		line[++j] = 0;			/* terminate the line */
		(void) lseek(pwf, (long) (j - i), 1);	/* point at the next line */
		p = field(line);		/* get the logname */
	} while (*name != *line ||	/* fast pretest */
	    strcmp(name, line) != 0);
	(void) close(pwf);
	
	/* skip the intervening fields */
	p = field(p);
	p = field(p);
	p = field(p);
	p = field(p);
	
	/* return the login directory */
	(void) field(p);
	return(p);
}
