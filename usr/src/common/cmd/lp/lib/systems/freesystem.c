/*		copyright	"%c%" 	*/

#ident	"@(#)freesystem.c	1.2"
#ident "$Header$"
/* LINTLIBRARY */

# include	"lp.h"
# include	"systems.h"

/**
 **  freesystem() - FREE MEMORY ALLOCATED FOR SYSTEM STRUCTURE
 **/


#if	defined(__STDC__)
void freesystem ( SYSTEM * sp )
#else
void freesystem ( sp )
SYSTEM	*sp;
#endif
{
    if (!sp)
	return;

    if (sp->name)
	Free(sp->name);

    if (sp->passwd)
	Free(sp->passwd);

    if (sp->comment)
	Free(sp->comment);
}
