#ident  "@(#)isbusy.c	1.3"
#ident  "$Header$"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <utmp.h>

#define nisname(n) (*n == '+' || *n == '-')
#define nisnetgr(n) ((*n == '+' || *n == '-') && (*(n+1) == '@'))

/* Maximium logname length - hard coded in utmp.h */
#define	MAXID	8

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

struct utmp *getutent();

/*
 * Procedure:	isbusy
 *
 * Restrictions:
 *		getutent:	None
 *
 * Notes:	Is this login being used? 
*/

isbusy(login)
	char *login;
{
	/*
	 * Minus user will never be logged in
	 */
	if (*login == '-')
		return(0);

	/*
	 * Is this a NIS name 
	 */
	if (nisname(login)) {
		if (nis_check() < 0){
			/*
			 * If we were unable to talk to NIS, 
			 * we'll assume that nis users
			 * were probably not able to login in,
			 * so return false. This may not be true.
			 */
			return(FALSE);
		}
		if (strcmp(login, "+") == 0) {
			/*
			 * See if anyone in the NIS map are logged in.
			 */
			return(nis_getall());
		} 
		if (nisnetgr(login)) {
			/*
			 * See if any users in this netgroup are logged in
			 */
			return(nis_getnetgr(login+2));
		} 
		/*
		 * must be a +user
		 */
		return(logedin(login+1));
	}

	/*
	 * Normal user name
	 */
	return(logedin(login));
}
/*
 * Check /etc/utmp to see if user is logged in.
 */
logedin(login)
	register char *login;
{
	register struct utmp *utptr;

	setutent();
	while ((utptr = getutent()) != NULL){
		if (!strncmp( login, utptr->ut_user, MAXID ) &&
					utptr->ut_type != DEAD_PROCESS)
			return TRUE;
	}
	return FALSE;
}
