/*		copyright	"%c%" 	*/


#ident	"@(#)TruncLevel.c	1.2"
#ident	"$Header$"

#include	<errno.h>
#include	<string.h>
/*
**  o  We do not truncate aliased levels.
**  o  We assume the unaliased format of:
**	h_name:[c_name[,c_name]...]
**  o  The smallest we can truncate to is:
**	X:(...)
**     This is 7 chars long.
*/

extern	int	errno;

#ifdef	__STDC__
int
TruncateLevel (char *lvlbufp, int trunclen)
#else
int
TruncateLevel (lvlbufp, trunclen)

char	*lvlbufp;
int	trunclen;
#endif
{
	register
	char	*cp;
	int	lvllen;

#ifdef	__STDC__
	static	const int	Elen = 5;
	static	const char	Elipsis[] = "(...)";
#else
	static	int	Elen = 5;
	static	char	Elipsis[] = "(...)";
#endif

	/*
	**  Common errors
	*/
	if (!lvlbufp || trunclen < 7)
	{
		errno = EINVAL;
		return	0;
	}
	/*
	**  Do we need to truncate at all?
	*/
	lvllen = strlen (lvlbufp);
	if (lvllen <= trunclen)
	{
		errno = 0;
		return	0;
	}
	/*
	**  Do we have just an aliased level?
	*/
	cp = lvlbufp;
	while (*cp && *cp != ':')
		cp++;

	if (!*cp)
	{
		errno = EINVAL;
		return	0;
	}
	if ((((int) cp - (int) lvlbufp) + Elen + 1) > trunclen)
	{
		errno = ENOMEM;
		return	0;
	}
	/*
	**  This will point to the last char of the string.
	*/
	cp = lvlbufp + trunclen - Elen - 1;
	while (*cp != ',' && *cp != ':')
		cp--;

	cp++;
	*cp = '\0';
	(void)	strcat (lvlbufp, Elipsis);

	return	1;
}
