/*		copyright	"%c%" 	*/

#ident	"@(#)done.c	1.2"
#ident	"$Header$"

#include "lpstat.h"

/**
 ** done() - CLEAN UP AND EXIT
 **/

void
#if	defined(__STDC__)
done (
	int			rc
)
#else
done (rc)
	int			rc;
#endif
{
	(void)mclose ();

	if (!rc && exit_rc)
		exit (exit_rc);
	else
		exit (rc);
}
