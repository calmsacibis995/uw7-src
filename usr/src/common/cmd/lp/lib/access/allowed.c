/*		copyright	"%c%" 	*/


#ident	"@(#)allowed.c	1.2"
#ident  "$Header$"
#include <string.h>
#include <unistd.h>
#include <sys/secsys.h>

#include "lp.h"
#include "access.h"

/**
 ** is_user_admin() - CHECK IF CURRENT USER IS AN ADMINISTRATOR
 **/

int
#if	defined(__STDC__)
is_user_admin (
	void
)
#else
is_user_admin ()
#endif
{
	return (Access("/usr/sbin/lpshut", X_OK) < 0 ? 0 : 1);
}

/**
 ** is_user_allowed() - CHECK USER ACCESS ACCORDING TO ALLOW/DENY LISTS
 **/

int
#if	defined(__STDC__)
is_user_allowed (
	char *			user,
	char **			allow,
	char **			deny
)
#else
is_user_allowed (user, allow, deny)
	char			*user,
				**allow,
				**deny;
#endif
{
    /*
     **  If we are in an ID based privilege mechanism then
     **  uid 'lp' is as privileged as the system defined
     **  privileged uid, for this sub-system and neither 
     **  are denied access to forms or printers.  abs s20.1
     */
    if (secsys (ES_PRVID, 0) >= 0 && 
	(bangequ(user, LOCAL_LPUSER) || bangequ(user, LOCAL_ROOTUSER)))
	   return (1);
    
    return (allowed(user, allow, deny));
}

/**
 ** is_user_allowed_form() - CHECK USER ACCESS TO FORM
 **/

int
#if	defined(__STDC__)
is_user_allowed_form (
	char *			user,
	char *			form
)
#else
is_user_allowed_form (user, form)
	char			*user,
				*form;
#endif
{
	char			**allow,
				**deny;

	if (loadaccess(Lp_A_Forms, form, "", &allow, &deny) == -1)
		return (-1);

	return (is_user_allowed(user, allow, deny));
}

/**
 ** is_user_allowed_printer() - CHECK USER ACCESS TO PRINTER
 **/

int
#if	defined(__STDC__)
is_user_allowed_printer (
	char *			user,
	char *			printer
)
#else
is_user_allowed_printer (user, printer)
	char			*user,
				*printer;
#endif
{
	char			**allow,
				**deny;

	if (loadaccess(Lp_A_Printers, printer, UACCESSPREFIX, &allow, &deny) == -1)
		return (-1);

	return (is_user_allowed(user, allow, deny));
}

/**
 ** is_form_allowed_printer() - CHECK FORM USE ON PRINTER
 **/

int
#if	defined(__STDC__)
is_form_allowed_printer (
	char *			form,
	char *			printer
)
#else
is_form_allowed_printer (form, printer)
	char			*form,
				*printer;
#endif
{
	char			**allow,
				**deny;

	if (loadaccess(Lp_A_Printers, printer, FACCESSPREFIX, &allow, &deny) == -1)
		return (-1);

	return (allowed(form, allow, deny));
}

/**
 ** allowed() - GENERAL ROUTINE TO CHECK ALLOW/DENY LISTS
 **/

int
#if	defined(__STDC__)
allowed (
	char *			item,
	char **			allow,
	char **			deny
)
#else
allowed (item, allow, deny)
	char			*item,
				**allow,
				**deny;
#endif
{
	if (deny) {
		if (bang_searchlist(item, deny))
			return (0);
	}

	if (allow) {
		if (bang_searchlist(item, allow))
			return (1);
		else		/* item not listed in either file */
			return (0);
	}
	else
		if (deny)	/* only deny file and item not listed there */
			return (1);

	return (0);
}
