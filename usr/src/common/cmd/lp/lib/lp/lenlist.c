/*		copyright	"%c%" 	*/

#ident	"@(#)lenlist.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

/**
 ** lenlist() - COMPUTE LENGTH OF LIST
 **/

int
#if	defined(__STDC__)
lenlist (
	char **			list
)
#else
lenlist (list)
	char			**list;
#endif
{
	register char **	pl;

	if (!list)
		return (0);
	for (pl = list; *pl; pl++)
		;
	return (pl - list);
}
