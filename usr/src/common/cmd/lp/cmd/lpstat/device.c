/*		copyright	"%c%" 	*/


#ident	"@(#)device.c	1.2"
#ident  "$Header$"

#include "sys/types.h"
#include "string.h"

#include "lp.h"
#include "printers.h"

#define	WHO_AM_I	I_AM_LPSTAT
#include "oam.h"

#include "lpstat.h"
#include "msgs.h"
#include <mac.h>
#include <unistd.h>

#if	defined(__STDC__)
static void		putdline ( PRINTER * );
#else
static void		putdline();
#endif

/*
 * Procedure:     do_device
 *
 * Restrictions:
                 getprinter: None
*/

void
#if	defined(__STDC__)
do_device (
	char **			list
)
#else
do_device (list)
	char			**list;
#endif
{
	register PRINTER	*pp;


	while (*list) {
		if (STREQU(NAME_ALL, *list))
		{
			while ((pp = getprinter(NAME_ALL)) || errno != ENOENT)
				if (pp)
					putdline (pp);
		}

		else if ((pp = getprinter(*list)))
			putdline (pp);

		else {
			LP_ERRMSG1 (ERROR, E_LP_NOPRINTER, *list);
			exit_rc = 1;
		}

		list++;
	}
	return;
}

/**
 ** putdline()
 **/

static void
#if	defined(__STDC__)
putdline (
	PRINTER *		pp
)
#else
putdline (pp)
	register PRINTER	*pp;
#endif
{
    char *			msg;
    char *			retmsg(int, long int);
    char			range[MSGMAX];
    int				n;
    int				len;
    int				warn = 0;
    int				show_range;

    if (!pp->device && !pp->dial_info && !pp->remote) {
	LP_ERRMSG1 (ERROR, E_LP_PGONE, pp->name);
    } else if (pp->remote) {
	char *			cp = strchr(pp->remote, BANG_C);

	/* if there is a range associated with the printer  and
	 * the -v option was specified and the user is an lp
	 * administrator, then print the range too.
	 * Range is printed using alias name if possible else
	 * the fully qualified level is printed.  ul90-35222 abs s19
	 */

	show_range =  ( lvlformat && v && pp->hilevel > 0 &&
		       (Access("/usr/sbin/lpshut", X_OK) == 0)) ? 1 : 0;
	if (show_range) {
	    range[0] = '\t';
	    range[1] = '\0';
	    while ((n = lvlout(&pp->hilevel, range+1, MSGMAX-4, LVL_ALIAS))
		   < 0 && errno == EINTR)
		;
	    if (n >= 0 && isdigit(range[1])) {
		while ((n = lvlout(&pp->hilevel, range+1, MSGMAX-4, LVL_FULL))
                   < 0 && errno == EINTR)
                ;
	    }
	    if (n < 0) {
		warn++;
		range[1] = '?';
		range[2] = '\0';
	    }
	    if (pp->hilevel != pp->lolevel) {
		(void)strcat(range, " - ");
		len = (int)strlen(range);
		while ((n =
			lvlout(&pp->lolevel, range+len, MSGMAX-len, LVL_ALIAS))
		       < 0 && errno == EINTR)
		    ;
		if (n >= 0 && isdigit(range[len])) {
		    while ((n = lvlout(&pp->lolevel, range+len, MSGMAX-len,
				       LVL_FULL)) < 0 && errno == EINTR)
			;
		}
	    }
	    if (n < 0) {
		warn++;
		range[len] = '?';
		range[len+1] = '\0';
	    }
	    if (warn)
		LP_ERRMSG(WARNING, E_STAT_BADLVL);

	}

	if (cp)
	    *cp++ = 0;
	if (show_range) {
	    if (cp)
		LP_OUTMSG4(MM_NOSTD, E_STAT_SYSASRNG, pp->name,
			   pp->remote, cp, range);
	    else
		LP_OUTMSG3(MM_NOSTD, E_STAT_SYSRNG, pp->name,
			   pp->remote, range);
	}
	else {
	    if (cp)
		LP_OUTMSG3(MM_NOSTD, E_STAT_SYSAS, pp->name,
			   pp->remote, cp);
	    else
		LP_OUTMSG2(MM_NOSTD, E_STAT_SYS, pp->name, pp->remote);
	}

    } else if (pp->dial_info) {
	if (pp->device)
	    LP_OUTMSG3(MM_NOSTD, E_STAT_TOKENON, pp->name, pp->dial_info,
		       pp->device);
	else
	    LP_OUTMSG2(MM_NOSTD, E_STAT_TOKEN, pp->name, pp->dial_info);

    } else {
	msg = retmsg(E_STAT_DEVICE);
	(void)printf (msg, pp->name, pp->device);
#if	defined(CAN_DO_MODULES)
	if (verbosity & V_MODULES)
	    if (
		emptylist(pp->modules)
		|| STREQU(NAME_NONE, pp->modules[0])
		)
	    {
		msg = retmsg(E_STAT_NOMOD);
		(void)printf (msg);
	    }
	    else if (STREQU(NAME_KEEP, pp->modules[0]))
	    {
		msg = retmsg(E_STAT_KEEP);
		(void)printf (msg);
	    }
	    else if (STREQU(NAME_DEFAULT, pp->modules[0]))
	    {
		msg = retmsg(E_STAT_DEF);
		(void)printf (msg,  DEFMODULES);
	    }
	    else {
		(void)printf (" ");
		printlist_setup ("", 0, ",", "");
		printlist (stdout, pp->modules);
		printlist_unsetup ();
	    }
#endif
	(void)printf ("\n");
    }

    return;
}
