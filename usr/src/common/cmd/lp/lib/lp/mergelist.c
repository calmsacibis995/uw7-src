/*		copyright	"%c%" 	*/

#ident	"@(#)mergelist.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

/**
 ** mergelist() - ADD CONTENT OF ONE LIST TO ANOTHER
 **/

int
#if	defined(__STDC__)
mergelist (
	char ***		dstlist,
	char **			srclist
)
#else
mergelist (dstlist, srclist)
	register char		***dstlist,
				**srclist;
#endif
{
	if (!srclist || !*srclist)
		return (0);

	while (*srclist)
		if (addlist(dstlist, *srclist++) == -1)
			return (-1);
	return (0);
}
