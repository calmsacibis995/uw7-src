/*		copyright	"%c%" 	*/


#ident	"@(#)bang.c	1.2"
#ident  "$Header$"

#include "string.h"
#include "unistd.h"
#include "stdlib.h"
#include "sys/utsname.h"

#include "lp.h"
#include "debug.h"

/*
 * The rules:
 *
 *	Key:	A - some system
 *		X - some user
 *
 *	X	a user named X on the local system
 *	A!X	the user named X from the system A
 *	all!X	all users named X from any system
 *	all	all users from local system
 *	A!all	all users from the system A
 *	all!all	all users from any system
 */


/**
 ** bangequ() - LIKE STREQU, BUT HANDLES system!name CASES
 **/

int
#ifdef	__STDC__
bangequ (char *user1p, char *user2p)
#else
bangequ (user1p, user2p)

char *	user1p;
char *	user2p;
#endif
{
	DEFINE_FNNAME (bangequ)

	int	sysname1_all	= 0,
		username1_all	= 0;
	char	sysname1 [32],
		sysname2 [32],
		username1 [32],
		username2 [32];
	char *	sp;

	static	char *Nodenamep = (char *) 0;

	ENTRYP
	TRACEs (user1p)
	TRACEs (user2p)

	if (! user1p || ! user2p)
		return	1;

	if (! Nodenamep)
	{
		struct utsname	utsbuf;

		(void)	uname (&utsbuf);
		Nodenamep = Strdup (utsbuf.nodename);
	}
	if (STREQU (NAME_ALL, user1p) && ! (strchr (user2p, '!')))
	{
		TRACEP ("STREQU (NAME_ALL, user1p) && ! (strchr (user2p, '!'))")
		EXITP
		return	1;
	}

	if (! (sp = strchr (user1p, '!')))
	{
		TRACEP ("! (sp = strchr (user1p, '!'))")
		EXITP
		return	STREQU (user1p, user2p);
	}
	*sp = '\0';
	(void)	strcpy (sysname1, user1p);
	*sp++ = '!';
	(void)	strcpy (username1, sp);

	sysname1_all = STREQU (NAME_ALL, sysname1);
	username1_all = STREQU (NAME_ALL, username1);

	if (sysname1_all && username1_all)
	{
		TRACEP ("sysname1_all && username1_all")
		EXITP
		return	1;
	}
	if (! sysname1_all && ! username1_all)
	{
		TRACEP ("! sysname1_all && ! username1_all")
		EXITP
		return	STREQU (user1p, user2p);
	}
	if (sp = strchr (user2p, '!'))
	{
		*sp = '\0';
		(void)	strcpy (sysname2, user2p);
		*sp++ = '!';
		(void)	strcpy (username2, sp);
	}
	else
	{
		(void)	strcpy (sysname2, Nodenamep);
		(void)	strcpy (username2, user2p);
	}
	if (sysname1_all)
	{
		TRACEP ("STREQU (username1, username2)")
		EXITP
		return	STREQU (username1, username2);
	}
	else	/*  username1_all  */
	{
		TRACEP ("STREQU (sysname1, sysname2)")
		EXITP
		return	STREQU (sysname1, sysname2);
	}
	/*NOTREACHED*/
}

/**
 ** bang_searchlist() - SEARCH (char **) LIST FOR "system!user" ITEM
 **/

int
#if	defined(__STDC__)
bang_searchlist (
	char *			item,
	char **			list
)
#else
bang_searchlist (item, list)
	register char		*item;
	register char		**list;
#endif
{
	if (!list || !*list)
		return (0);

	/*
	 * This is a linear search--we believe that the lists
	 * will be short.
	 */
	while (*list) {
		if (bangequ(*list, item))
			return (1);
		list++;
	}
	return (0);
}

/**
 ** bang_dellist() - REMOVE "system!name" ITEM FROM (char **) LIST
 **/

int
#if	defined(__STDC__)
bang_dellist (
	char ***		plist,
	char *			item
)
#else
bang_dellist (plist, item)
	register char		***plist,
				*item;
#endif
{
	register char **	pl;
	register char **	ql;

	register int		n;

				/*
				 * "hole" is a pointer guaranteed not
				 * to point to anyplace malloc'd.
				 */
	char *			hole	= "";


	/*
	 * There are two ways this routine is different from the
	 * regular "dellist()" routine: First, the items are of the form
	 * ``system!name'', which means there is a two part matching
	 * for ``all'' cases (all systems and/or all names). Second,
	 * ALL matching items in the list are deleted.
	 *
	 * Now suppose the list contains just the word ``all'', and
	 * the item to be deleted is the name ``fred''. What will
	 * happen? The word ``all'' will be deleted, leaving the list
	 * empty (null)! This may sound odd at first, but keep in mind
	 * that this routine is paired with the regular "addlist()"
	 * routine; the item (``fred'') is ADDED to an opposite list
	 * (we are either deleting from a deny list and adding to an allow
	 * list or vice versa). So, to continue the example, if previously
	 * ``all'' were allowed, removing ``fred'' from the allow list
	 * does indeed empty that list, but then putting him in the deny
	 * list means only ``fred'' is denied, which is the effect we
	 * want.
	 */

	if (*plist) {

		for (pl = *plist; *pl; pl++)
			if (bangequ(*pl, item)) {
				Free (*pl);
				*pl = hole;
			}

		for (n = 0, ql = pl = *plist; *pl; pl++)
			if (*pl != hole) {
				*ql++ = *pl;
				n++;
			}

		if (n == 0) {
			Free ((char *)*plist);
			*plist = 0;
		} else {
			*plist = (char **)Realloc(
				(char *)*plist,
				(n + 1) * sizeof(char *)
			);
			if (!*plist)
				return (-1);
			(*plist)[n] = 0;
		}
	}

	return (0);
}
