/*		copyright	"%c%" 	*/


#ident	"@(#)SetProcLevel.c	1.2"
#ident  "$Header$"

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <mac.h>


#ifdef	__STDC__
int
SetProcLevel (char *lvlbufp)
#else
int
SetProcLevel (lvlbufp)

char *lvlbufp;
#endif
{
	level_t	level;

	if (lvlin (lvlbufp, &level) < 0)
	{
		switch (errno) {
		case	EACCES:		/*  Assume ENOPKG  */
			errno = ENOPKG;
			return	-1;

		default:
			/*
			**  Leave 'errno' set as is.
			*/
			return	-1;
		}
	}
	if (lvlproc (MAC_SET, &level) < 0)
	{
		/*
		**  Leave 'errno' set as is.
		*/
		return	-1;
	}
	return	0;
}
