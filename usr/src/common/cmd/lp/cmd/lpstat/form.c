/*		copyright	"%c%" 	*/


#ident	"@(#)form.c	1.2"
#ident  "$Header$"

#include "stdio.h"

#include "string.h"

#include "lp.h"
#include "form.h"
#include "access.h"

#define	WHO_AM_I	I_AM_LPSTAT
#include "oam.h"

#include "lpstat.h"


#if	defined(__STDC__)
static void		putfline ( FORM * );
#else
static void		putfline();
#endif

/*
 * Procedure:     do_form
 *
 * Restrictions:
                 getform: None
*/

void
#if	defined(__STDC__)
do_form (
	char **			list
)
#else
do_form (list)
	char			**list;
#endif
{
	FORM			form;
	int			ret = 0;

	while (*list) {
		if (STREQU(NAME_ALL, *list))
		{
			while ((ret = getform(NAME_ALL, &form, (FALERT *)0, (FILE **)0)) == 0 || errno != ENOENT)
			if (ret == 0)
				putfline (&form);
		}

		else if (getform(*list, &form, (FALERT *)0, (FILE **)0) == 0) {
			putfline (&form);

		} else {
			LP_ERRMSG1 (ERROR, E_LP_NOFORM, *list);
			exit_rc = 1;
		}

		list++;
	}
	printsdn_unsetup ();
	return;
}

/*
 * Procedure:     putfline
 *
 * Restrictions:
                 getname: None
*/

static void
#if	defined(__STDC__)
putfline (
	FORM *			pf
)
#else
putfline (pf)
	FORM			*pf;
#endif
{
	register MOUNTED	*pm;

        char 			*msg;
	char			*retmsg(int, long int);

	if (is_user_allowed_form(getname(), pf->name))
 	   msg = retmsg(E_STAT_FORMAVAIL);
	else
	   msg = retmsg(E_STAT_FORMNOTAV);

	(void) printf(msg, pf->name);

	for (pm = mounted_forms; pm->forward; pm = pm->forward)
		if (STREQU(pm->name, pf->name)) {
			if (pm->printers) {
				msg = retmsg(E_STAT_FM);
				(void) printf(msg);
				printlist_setup (0, 0, ",", "");
				printlist (stdout, pm->printers);
				printlist_unsetup();
			}
			break;
		}

	(void) printf("\n");

	if (verbosity & V_LONG) {

		msg = retmsg(E_STAT_PL);
		printsdn_setup (msg, 0, 0);
		printsdn (stdout, pf->plen);

		msg = retmsg(E_STAT_PW);
		printsdn_setup (msg, 0, 0);
		printsdn (stdout, pf->pwid);

		msg = retmsg(E_STAT_NOP);
		(void) printf(msg, pf->np);
		(void) printf("\n");

		msg = retmsg(E_STAT_LPITCH);
		printsdn_setup (msg, 0, 0);
		printsdn (stdout, pf->lpi);

		msg = retmsg(E_STAT_CHARPITCH);
		(void) printf(msg);
		if (pf->cpi.val == N_COMPRESSED)
			(void) printf(" %s\n", NAME_COMPRESSED);
		else {
			printsdn_setup (" ", 0, 0);
			printsdn (stdout, pf->cpi);
		}

		msg = retmsg(E_STAT_CMAN);
		LP_OUTMSG2(MM_NOSTD, E_STAT_SETCHOICE,
			(pf->chset? pf->chset : NAME_ANY),
			(pf->mandatory ? msg : "")
		);

		LP_OUTMSG1(MM_NOSTD, E_STAT_RIBCOL,
			(pf->rcolor? pf->rcolor : NAME_ANY)
		);

		if (pf->comment)
		        LP_OUTMSG1(MM_NOSTD, E_STAT_COMMENT,
			                                    pf->comment);

	}
	return;
}

