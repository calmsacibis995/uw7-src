/*		copyright	"%c%" 	*/


#ident	"@(#)GetProcLevel.c	1.2"
#ident  "$Header$"

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <mac.h>


#ifdef	__STDC__
char *
GetProcLevel (int format)
#else
int
GetProcLevel (format)

int	format;
#endif
{
	int	lvlsize;
	char	*lvlbufp;
	level_t	level;

	if (lvlproc (MAC_GET, &level) < 0)
		return	NULL;

	lvlsize = lvlout (&level, NULL, 0, format);

	if (lvlsize < 0)
	{
		switch (errno) {
		case	EACCES:		/*  Assume ENOPKG  */
			errno = ENOPKG;
			return	NULL;

		default:
			/*
			**  Leave 'errno' set as is.
			*/
			return	NULL;
		}
	}
	lvlbufp = (char *) malloc (lvlsize);

	if (!lvlbufp)
	{
		/*
		**  'errno' is likely ENOSPC.
		*/
		return	NULL;
	}
	if (lvlout (&level, lvlbufp, lvlsize, format) < 0)
	{
		switch (errno) {
		case	EACCES:		/*  Assume ENOPKG  */
			errno = ENOPKG;
			return	NULL;

		default:
			/*
			**  Leave 'errno' set as is.
			*/
			return	NULL;
		}
	}
	return	lvlbufp;
}
