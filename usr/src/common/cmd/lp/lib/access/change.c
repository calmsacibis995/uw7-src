/*		copyright	"%c%" 	*/

#ident	"@(#)change.c	1.3"
#ident	"$Header$"

/*******************************************************************************
 *
 * FILENAME:    change.c
 *
 * DESCRIPTION: Provides functions to change the access files for the following:
 *                 User access (allow/deny) to user a printer
 *                 User access (allow/deny) to user a form
 *                 Printer access (allow/deny) to user a form
 *
 * SCCS:	change.c 1.3  7/3/97 at 09:24:15
 *
 * CHANGE HISTORY:
 *
 * 03-06-97  Paul Cunningham        ul97-12219
 *           Change function chgaccess() so that if access of "allow!all" or
 *           "deny!all" is specified the "all" entries in both the allow and
 *           deny files are first removed before added it back into the
 *           required access file.
 *
 *******************************************************************************
 */

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "errno.h"
#include "string.h"
#include "stdlib.h"

#include "lp.h"
#include "access.h"


#if	defined(__STDC__)

static int		chgaccess ( int , char ** , char * , char * , char * );
static char **		empty_list ( void );

#else

static int		chgaccess();
static char		**empty_list();

#endif

/**
 ** deny_user_form() - DENY USER ACCESS TO FORM
 **/

int
#if	defined(__STDC__)
deny_user_form (
	char **			user_list,
	char *			form
)
#else
deny_user_form (user_list, form)
	char			**user_list,
				*form;
#endif
{
	return (chgaccess(0, user_list, form, Lp_A_Forms, ""));
}

/**
 ** allow_user_form() - ALLOW USER ACCESS TO FORM
 **/

int
#if	defined(__STDC__)
allow_user_form (
	char **			user_list,
	char *			form
)
#else
allow_user_form (user_list, form)
	char			**user_list,
				*form;
#endif
{
	return (chgaccess(1, user_list, form, Lp_A_Forms, ""));
}

/**
 ** deny_user_printer() - DENY USER ACCESS TO PRINTER
 **/

int
#if	defined(__STDC__)
deny_user_printer (
	char **			user_list,
	char * 			printer
)
#else
deny_user_printer (user_list, printer)
	char			**user_list,
				*printer;
#endif
{
	return (chgaccess(0, user_list, printer, Lp_A_Printers, UACCESSPREFIX));
}

/**
 ** allow_user_printer() - ALLOW USER ACCESS TO PRINTER
 **/

int
#if	defined(__STDC__)
allow_user_printer (
	char **			user_list,
	char *			printer
)
#else
allow_user_printer (user_list, printer)
	char			**user_list,
				*printer;
#endif
{
	return (chgaccess(1, user_list, printer, Lp_A_Printers, UACCESSPREFIX));
}

/**
 ** deny_form_printer() - DENY FORM USE ON PRINTER
 **/

int
#if	defined(__STDC__)
deny_form_printer (
	char **			form_list,
	char *			printer
)
#else
deny_form_printer (form_list, printer)
	char			**form_list,
				*printer;
#endif
{
	return (chgaccess(0, form_list, printer, Lp_A_Printers, FACCESSPREFIX));
}

/**
 ** allow_form_printer() - ALLOW FORM USE ON PRINTER
 **/

int
#if	defined(__STDC__)
allow_form_printer (
	char **			form_list,
	char *			printer
)
#else
allow_form_printer (form_list, printer)
	char			**form_list,
				*printer;
#endif
{
	return (chgaccess(1, form_list, printer, Lp_A_Printers, FACCESSPREFIX));
}

/**
 ** chgaccess() - UPDATE ALLOW/DENY ACCESS OF ITEM TO RESOURCE
 **/

static int
#if	defined(__STDC__)
chgaccess (
	int			isallow,
	char **			list,
	char *			name,
	char *			dir,
	char *			prefix
)
#else
chgaccess (isallow, list, name, dir, prefix)
	int			isallow;
	char			**list,
				*name,
				*dir,
				*prefix;
#endif
{
	register char		***padd_list,
				***prem_list,
				**a_list,
				**r_list,
				**pl,
				**item;

	char			**allow_list,
				**deny_list,
				*bang_c;

	int			adding_NAME_ALL = 0;


	if (loadaccess(dir, name, prefix, &allow_list, &deny_list) == -1)
		return (-1);

	if (isallow) {
		padd_list = &allow_list;
		prem_list = &deny_list;
		a_list = duplist(allow_list);
		r_list = duplist(deny_list);
	} else {
		padd_list = &deny_list;
		prem_list = &allow_list;
		a_list = duplist(deny_list);
		r_list = duplist(allow_list);
	}
	

	for (pl = list; *pl; pl++)
		if (  STREQU(*pl, NAME_ALL)
		     || STREQU(*pl, NAME_ANY)
		) {
			adding_NAME_ALL = 1; /* We are adding NAME_ALL */
			break;
		}


	for (pl = list; *pl; pl++) {

		/*
		 * Do the ``all'' and ``none'' cases explicitly,
		 * so that we can clean up the lists nicely.
		 */
		if (STREQU(*pl, NAME_NONE)) {
			isallow = !isallow;
			goto AllCase;
		}
		if (
		        STREQU(*pl, ALL_BANG_ALL)
		     || STREQU(*pl, NAME_ALL)
		     || STREQU(*pl, NAME_ANY)
		) {
AllCase:
		     	if (STREQU(*pl, ALL_BANG_ALL)
				|| STREQU(*pl, NAME_NONE)) {
				freelist (allow_list);
				freelist (deny_list);
				allow_list = 0;
				deny_list = 0;
			}
		     	if (STREQU(*pl, ALL_BANG_ALL)) {
				if (addlist(padd_list, *pl) == -1)
					return (-1);
			}
			else
			if (STREQU(*pl, NAME_NONE)) 
			{
				if (addlist(prem_list, ALL_BANG_ALL) == -1)
					return (-1);
			}
			else 
			{
			    /* Remove all local and 'all' items from
			     * the allow and deny lists
			     */
			    if (*a_list)
			    {
				/* process the allow list
				 */
				for (item = a_list; *item; item++) 
				{
					if (strchr( *item, BANG_C))
					{
					  /* don't remove the xxx!all entry
					   */
					  continue;
					}
					if (bang_dellist(padd_list,*item) == -1)
						return (-1);
				}
			    }
			    if (*r_list)
			    {
				/* process the deny list
				 */
				for (item = r_list; *item; item++) 
				{
					if (strchr(*item, BANG_C))
					{
					  /* don't remove the xxx!all entry
					   */
					  continue;
					}
					if (bang_dellist(prem_list,*item) == -1)
						return (-1);
				}
			    }
			    (void) bang_dellist(padd_list, ALL_BANG_ALL);
			    if (addlist(padd_list, NAME_ALL) == -1)
				return (-1);
			}

			if (STREQU(*pl, ALL_BANG_ALL) || STREQU(*pl, NAME_NONE))
				break;

		} 
		else 
		{

			/*
			 * For each regular item in the list,
			 * we add it to the ``add list'' and remove it
			 * from the ``remove list''. This is not
			 * efficient, especially if there are a lot of
			 * items in the caller's list; doing it the
			 * way we do, however, has the side effect
			 * of skipping duplicate names in the caller's
			 * list.
			 *
			 * Do a regular "addlist()"--the resulting
			 * list may have redundancies, but it will
			 * still be correct.
			 */
			(void) bang_dellist(padd_list, ALL_BANG_ALL);
			/*
			 * Delete ALL only if a local regular item is
			 * being added. Add local regular item only if
			 * NAME_ALL is NOT being added.
			*/
			if (((bang_c = strchr (*pl, BANG_C)) == NULL) ||
							bang_c == *pl ) 
			{
			    if (!adding_NAME_ALL) {
				(void) bang_dellist(padd_list, NAME_ALL);
				if (addlist(padd_list, *pl) == -1)
					return (-1);
			    }
			} else
			if (addlist(padd_list, *pl) == -1)
				return (-1);

			if (bang_dellist(prem_list, *pl) == -1)
				return (-1);

		}

	}

	freelist (a_list);
	freelist (r_list);

	return (dumpaccess(dir, name, prefix, &allow_list, &deny_list));
}

/**
 ** empty_list() - CREATE AN EMPTY LIST
 **/

static char **
#if	defined(__STDC__)
empty_list (
	void
)
#else
empty_list ()
#endif
{
	register char		**empty;


	if (!(empty = (char **)Malloc(sizeof(char *)))) {
		errno = ENOMEM;
		return (0);
	}
	*empty = 0;
	return (empty);
}
