/*		copyright	"%c%" 	*/

#ident	"@(#)accept.c	1.2"
#ident  "$Header$"

#include "stdio.h"

#include "lp.h"
#include "class.h"
#include "msgs.h"

#define	WHO_AM_I	I_AM_LPSTAT
#include "oam.h"

#include "lpstat.h"

/*
 * Procedure:     do_accept
 *
 * Restrictions:
                 isclass: None
*/

void
#if	defined(__STDC__)
do_accept (
	char **			list
)
#else
do_accept (list)
	char			**list;
#endif
{
	while (*list) {
		if (STREQU(*list, NAME_ALL)) {
			send_message (S_INQUIRE_CLASS, "");
			(void)output (R_INQUIRE_CLASS);
			send_message (S_INQUIRE_PRINTER_STATUS, "");
			(void)output (R_INQUIRE_PRINTER_STATUS);

		} else if (isclass(*list)) {
			send_message (S_INQUIRE_CLASS, *list);
			(void)output (R_INQUIRE_CLASS);

		} else {
			send_message (S_INQUIRE_PRINTER_STATUS, *list);
			switch (output(R_INQUIRE_PRINTER_STATUS)) {
			case MNODEST:
				LP_ERRMSG1 (ERROR, E_LP_BADDEST, *list);
				exit_rc = 1;
				break;
			}

		}
		list++;
	}
	return;
}

/**
 ** putqline()
 **/

void
#if	defined(__STDC__)
putqline (
	char *			dest,
	int			rejecting,
	char *			reject_date,
	char *			reject_reason
)
#else
putqline (dest, rejecting, reject_date, reject_reason)
	char			*dest;
	int			rejecting;
	char			*reject_date,
				*reject_reason;
#endif
{
	if (!rejecting)
            LP_OUTMSG2(MM_NOSTD, E_STAT_ACCEPTING, dest, reject_date);
	else
            LP_OUTMSG3(MM_NOSTD, E_STAT_NOTACCEPTING, dest,
			reject_date,
			reject_reason
		);
	return;
}
