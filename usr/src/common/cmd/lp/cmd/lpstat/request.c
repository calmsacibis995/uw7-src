/*		copyright	"%c%" 	*/

#ident	"@(#)request.c	1.3"
#ident  "$Header$"

#include "stdio.h"
#include "pwd.h"
#include "sys/types.h"

#include "lp.h"
#include "msgs.h"
#include "requests.h"

#define	WHO_AM_I	I_AM_LPSTAT
#include "oam.h"

#include "lpstat.h"


/*
 * Procedure:     do_request
 *
 * Restrictions:
                 isprinter: None
                 isclass: None
*/

void
#if	defined(__STDC__)
do_request (
	char **			list
)
#else
do_request (list)
	char			**list;
#endif
{
	while (*list) {
		if (STREQU(NAME_ALL, *list)) {
			send_message (S_INQUIRE_REQUEST_RANK, 1,
				      "", "", "", "", "");
			(void)output (R_INQUIRE_REQUEST_RANK);
		} else if (isrequest(*list)) {
			send_message (S_INQUIRE_REQUEST_RANK, 1,
				      "", "", *list, "", "");
			switch (output(R_INQUIRE_REQUEST_RANK)) {
			case MNOINFO:
				LP_ERRMSG1 (ERROR,
				E_STAT_DONE, *list);
				exit_rc = 1;
				break;
			}
		} else {
			send_message (S_INQUIRE_REQUEST_RANK, 1,
				      "", *list, "", "", "");
			switch (output(R_INQUIRE_REQUEST_RANK)) {
			case MNOINFO:
				if (!isprinter(*list) &&
			            !isclass(*list)) {
					LP_ERRMSG1 (ERROR,
					E_STAT_BADSTAT, *list);
					exit_rc = 1;
				}
				break;
			}
		}
		list++;
	}
	return;
}

/*
 * Procedure:     do_user
 *
 * Restrictions:
                 getpwnam: None
*/

void
#if	defined(__STDC__)
do_user (
	char **			list
)
#else
do_user (list)
	char			**list;
#endif
{
	while (*list) {
		if (STREQU(NAME_ALL, *list)) {
			send_message (S_INQUIRE_REQUEST_RANK, 1,
				      "", "", "", NAME_ALL, "");
			(void)output (R_INQUIRE_REQUEST_RANK);
		} else {
			send_message (S_INQUIRE_REQUEST_RANK, 1,
				      "", "", "", *list, "");
			switch (output(R_INQUIRE_REQUEST_RANK)) {
			case MNOINFO:
				if (!getpwnam(*list))
					LP_ERRMSG1 (WARNING,
					E_STAT_USER, *list);
				break;
			}
		}
		list++;
	}
	return;
}

/*
 * Procedure:     putoline
 *
 * Restrictions:
                 lvlout: None
*/

void
#if	defined(__STDC__)
putoline (
	char *			request_id,
	char *			user,
	long			size,
	char *			date,
	int			state,
	char *			printer,
	char *			form,
	char *			character_set,
	int			rank,
	level_t			lid
)
#else
putoline (request_id, user, size, date, state, printer, form, character_set, rank, lid)
	char			*request_id,
				*user,
				*date,
				*printer,
				*form,
				*character_set;
	long			size;
	int			state;
	int			rank;
	level_t			lid;
#endif
{
	char			*msg;
	char			*retmsg(int, long int);
	char			*lidout;
	int			lidsize;

	if (verbosity & V_RANK)
		(void) printf("%3d ", rank);

	(void) printf(
		"%-*s %-*s %*ld %s%s",
		((verbosity & V_RANK)? IDSIZE - 2 : IDSIZE),
		request_id,
		LOGMAX-1,
		user,
		OSIZE,
		size,
		((verbosity & V_RANK)? "" : "  "),
		date
	);

/* lpstat -z */
	if (lvlformat & LVL_ALIAS ){   
		lidsize = lvlout( &lid, (char *)NULL, (int) 0, LVL_ALIAS);
		lidout = (char *) malloc (lidsize);
		(void) lvlout( &lid, lidout, lidsize, LVL_ALIAS);
		(void) printf(" %s", lidout);
		}

/* lpstat -Z */
	if (lvlformat & LVL_FULL ){   
		lidsize = lvlout( &lid, (char *)NULL, (int) 0, LVL_FULL);
		lidout = (char *) malloc (lidsize);
		(void) lvlout( &lid, lidout, lidsize, LVL_FULL);
		(void) printf(" %s", lidout);
		}

	if (!(verbosity & (V_LONG|V_BITS))) {

		/*
		 * Unless the -l option is given, we show the CURRENT
		 * status. Check the status bits in reverse order of
		 * chronology, i.e. go with the bit that would have been
		 * set last. Old bits don't get cleared by the Spooler.
		 * We only have space for 21 characters!
		 */

		if (state & RS_NOTIFYING)
		   {
			msg = retmsg(E_STAT_NOTIF);
			(void) printf(msg);
		   }

		else if (state & RS_CANCELLED)
		   {
			msg = retmsg(E_STAT_CANCELED);
			(void) printf(msg);
		   }

		else if (state & RS_PRINTED)
		   {
			msg = retmsg(E_STAT_FINISHED);
			(void) printf(msg);
		   }

		else if (state & RS_PRINTING)
		   {
			msg = retmsg(E_STAT_ONPR);
			(void) printf(msg, printer);
		   }

		else if (state & RS_ADMINHELD)
		   {
			msg = retmsg(E_STAT_HBADM);
			(void) printf(msg);
		   }

		else if (state & RS_HELD)
		   {
			msg = retmsg(E_STAT_BHELD);
			(void) printf(msg);
		   }

		else if (state & RS_FILTERED)
		   {
			msg = retmsg(E_STAT_FILT);
			(void) printf(msg);
		   }

		else if (state & RS_FILTERING)
		   {
			msg = retmsg(E_STAT_BEFILT);
			(void) printf(msg);
		   }

		else if (state & RS_CHANGING)
		   {
			msg = retmsg(E_STAT_HELDCH);
			(void) printf(msg);
		   }

	} else if (verbosity & V_BITS) {
		register char		*sep	= "\n	";

			BITPRINT (state, RS_HELD);
			BITPRINT (state, RS_FILTERING);
			BITPRINT (state, RS_FILTERED);
			BITPRINT (state, RS_PRINTING);
			BITPRINT (state, RS_PRINTED);
			BITPRINT (state, RS_CHANGING);
			BITPRINT (state, RS_CANCELLED);
			BITPRINT (state, RS_IMMEDIATE);
			BITPRINT (state, RS_FAILED);
			BITPRINT (state, RS_SENDING);
			BITPRINT (state, RS_NOTIFY);
			BITPRINT (state, RS_NOTIFYING);
			BITPRINT (state, RS_SENT);
			BITPRINT (state, RS_ADMINHELD);
			BITPRINT (state, RS_REFILTER);
			BITPRINT (state, RS_STOPPED);

	} else if (verbosity & V_LONG) {

		register char		*sep	= "\n	";

		/*
		 * Here we show all the interesting states the job
		 * has gone through. Left to right they are in
		 * chronological order.
		 */

		if (state & RS_CANCELLED) {
			msg = retmsg(E_STAT_SCANC);
			(void) printf(msg, sep);
			sep = ", ";

		} else if (state & RS_PRINTING) {
			msg = retmsg(E_STAT_SONPR);
			(void) printf(msg, sep, printer);
			sep = ", ";

		} else if (state & RS_FAILED) {
			msg = retmsg(E_STAT_SFAILED);
			(void) printf(msg, sep);
			sep = ", ";

		} else if (state & RS_PRINTED) {
			msg = retmsg(E_STAT_SFINISHED);
			(void) printf(msg, sep, printer);
			sep = ", ";

		/*
		 * WATCH IT! We make the request ID unusable after
		 * the next line.
		 */
		} else if (!STREQU(strtok(request_id, "-"), printer)) {
			msg = retmsg(E_STAT_SASS);
			(void) printf(msg, sep, printer);
			sep = ", ";

		} else {
			if (state & RS_SENT)
			   {
				msg = retmsg(E_STAT_SQREM);
				(void) printf(msg, sep, printer);
			   }
			else
			   {
				msg = retmsg(E_STAT_SQFOR);
				(void) printf(msg, sep, printer);
			   }
			sep = ", ";
		}

		if (!(state & RS_DONE)) {
			if (form && *form) {
				msg = retmsg(E_STAT_SFORM);
				(void) printf(msg, sep, form);
				sep = ", ";
			}
			if (character_set && *character_set) {
				msg = retmsg(E_STAT_SCHARS);
				(void) printf(msg, sep, character_set);
				sep = ", ";
			}
		}

		if (state & RS_NOTIFYING) {
			msg = retmsg(E_STAT_SNOTIF);
			(void) printf(msg, sep);
			sep = ", ";

		} else if (state & RS_CHANGING) {
			msg = retmsg(E_STAT_SHELDCH);
			(void) printf(msg, sep);
			sep = ", ";

		} else if (state & RS_ADMINHELD) {
			msg = retmsg(E_STAT_SHBADM);
			(void) printf(msg, sep);
			sep = ", ";

		} else if (state & RS_HELD) {
			msg = retmsg(E_STAT_SBHELD);
			(void) printf(msg, sep);
			sep = ", ";

		}

		if (state & RS_FILTERED) {
			msg = retmsg(E_STAT_SFILT);
			(void) printf(msg, sep);
			sep = ", ";

		} else if (state & RS_FILTERING) {
			msg = retmsg(E_STAT_SBEFILT);
			(void) printf(msg, sep);
			sep = ", ";
		}

	}
	(void) printf("\n");
	return;
}
