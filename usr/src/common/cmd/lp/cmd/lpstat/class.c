/*		copyright	"%c%" 	*/


#ident	"@(#)class.c	1.2"
#ident	"$Header$"

#include "stdio.h"

#include "lp.h"
#include "class.h"

#define	WHO_AM_I	I_AM_LPSTAT
#include "oam.h"

#include "lpstat.h"


#if	defined(__STDC__)
static void		putcline ( CLASS * );
#else
static void		putcline();
#endif

/**
 ** do_class()
 **/

void
#if	defined(__STDC__)
do_class (
	char **			list
)
#else
do_class (list)
	char			**list;
#endif
{
	register CLASS		*pc;

	printlist_setup ("\t", 0, 0, 0);
	while (*list) {
		if (STREQU(NAME_ALL, *list))
		{	
			while ((pc = getclass(NAME_ALL)) || errno != ENOENT)
				if (pc)
					putcline (pc);

		}
		else if ((pc = getclass(*list)))
			putcline (pc);

		else {
			LP_ERRMSG1 (ERROR, E_LP_NOCLASS, *list);
			exit_rc = 1;
		}
		list++;
	}
	printlist_unsetup ();
	return;
}

/**
 ** putcline()
 **/

static void
#if	defined(__STDC__)
putcline (
	CLASS *			pc
)
#else
putcline (pc)
	register CLASS		*pc;
#endif
{
                LP_OUTMSG1(MM_NOSTD, E_STAT_MEMCLASS, pc->name);
		printlist (stdout, pc->members);
	return;
}
