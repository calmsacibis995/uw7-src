/*		copyright	"%c%" 	*/

#ident	"@(#)getspooldir.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "lp.h"

char *
#if	defined(__STDC__)
getspooldir (
	void
)
#else
getspooldir ()
#endif
{
	return (Lp_Spooldir);
}
