#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:dtamlib/getent.c	1.5"
#endif

#include <limits.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include "dtamlib.h"

static struct passwd *
nis_user(char *, struct passwd *, Boolean, Boolean, Boolean *);

static struct group *
nis_group(char *, struct group *, Boolean, Boolean, Boolean *);

/*
 * _DtamGetpwent : private version of getpwent(3) for dtadmin clients.
 *	Will get list of valid users in the system, filtering the 
 *	NIS non-local users.  
 *
 *	For valid local NIS users ('+user' entries in /etc/passwd):
 * 	    If first argument is STRIP, strip out the '+' from the user name.
 * 	    If 2nd argument is NIS_EXPAND, try to obtain complete user data 
 *	    from  NIS map.
 *
 *	If the third argument, nis_up, is not NULL at function call, this
 *	argument will point to True at return time if the secnod argument 
 *	was NIS_EXPAND, and NIS was up (the user info was expanded from 
 *	NIS map, as desired). It will point to False if NIS was down and
 *	the user info could not be expanded.
 */

struct passwd *
_DtamGetpwent(Boolean strip, Boolean complete, Boolean *nis_up)
{
	char		*u_name;
	struct	passwd	*pw;
	static FILE	*f_pw;

	if (!f_pw)
		if (!(f_pw = fopen ("/etc/passwd", "rb")))
			return (NULL);	    /* on error, return NULL ptr,
					     * like expected from getpwent(3C)
					     */

	while ( pw = fgetpwent(f_pw) ) {

		u_name = STRDUP(pw->pw_name);

		if (*u_name) {

			if (u_name[0] == '+') {

				/* ignore /etc/passwd entries that begin with
				 * "+:" or "+@netgroup:"
				 */
				if  (u_name[1] != '\0' && u_name[1] != '@') {
					pw = nis_user(u_name, pw, 
						strip, complete, nis_up);
					break;		/* local NIS user */
				}

			} else if (u_name[0] != '-')   /* ignore "-usr:" entries */
					break;        /* regular user */
		}
		FREE(u_name);
	}

	if ( pw == NULL ) {	/* end of passwd file */
		fclose(f_pw);
		f_pw = (FILE *)NULL;
	} else
        	FREE(u_name);
	return(pw);
}

/* 
 * nis_user : called from _DtamGetpwent for NIS local users.  
 * 	If strip is True, the '+' sign is stripped out the user name.
 * 	If complete is True, the directory, comment and shell info
 * 	is obtained from NIS map data only if these fields are empty
 * 	in the /etc/passwd file.
 */

static struct passwd *
nis_user(char *u_name, struct passwd *pw, 
	Boolean strip, Boolean complete, Boolean *nis_up)
{
	char 		*u_strip;
	struct	passwd	*t_pw;

	/* strip '+' sign */
	u_strip = &u_name[1];

	/* 
	 * if desire to expand NIS user info,  get the info from the NIS map
	 * (getpwnam will expand - but home directory, comment and shell info  
	 * on the /etc/passwd record will have priority over info in map.)
	 */

	if (complete == NIS_EXPAND) {
		if (!(t_pw = getpwnam(u_strip))) {
			if (nis_up != NULL)
				*nis_up = False;
			t_pw = pw;		/* couldn't expand - NIS down */
		} else {
			if (nis_up != NULL)
				*nis_up = True;
		}
		
	} else 
		t_pw = pw;

	if (strip == STRIP)
		t_pw->pw_name = STRDUP(u_strip);
	else
		t_pw->pw_name = STRDUP(u_name);

	return (t_pw);
}


/*
 * _DtamGetgrent : private version of getgrent(3) for dtadmin clients.
 *	Will get list of valid local groups in the system, filtering the 
 *	NIS non-local groups.  
 *
 *	For valid local NIS groups ('+group' entries in /etc/group):
 * 	    If first argument is STRIP, strip out the '+' from the group name.
 * 	    If second argument is NIS_EXPAND, try to obtain complete group 
 *	    data from  NIS map.
 *
 *	If the third argument, nis_up, is not NULL at function call, this
 *	argument will point to True at return time if the secnod argument 
 *	was NIS_EXPAND, and NIS was up (the group info was expanded from 
 *	NIS map, as desired). It will point to False if NIS was down and
 *	the group info could not be expanded.
 */
struct group *
_DtamGetgrent(Boolean strip, Boolean complete, Boolean *nis_up)
{
	char		*g_name;
	struct	group	*gr;
	static FILE	*f_group;

	if (!f_group)
		if (!(f_group = fopen ("/etc/group", "rb")))
			return (NULL);	    /* on error, return NULL ptr,
					     * like expected from getgrent(3C)
					     */

	while ( gr = fgetgrent(f_group) ) {

		g_name = STRDUP(gr->gr_name);

		if (*g_name) {

			if (g_name[0] == '+') {

				/* ignore /etc/group entries that begin with
				 * "+:" or "+@netgroup:"
				 */
				if  (g_name[1] != '\0' && g_name[1] != '@') {
					gr = nis_group(g_name, gr, 
						strip, complete, nis_up);
					break;		/* local NIS group */
				}

			} else if (g_name[0] != '-')   /* ignore "-grp:" entries */
					break;        /* regular group */
		}
		FREE(g_name);
	}

	if ( gr == NULL ) {	/* end of group file */
		fclose(f_group);
		f_group = (FILE *)NULL;
	} else
		FREE(g_name);

	return(gr);
}

/* 
 * nis_group : called from _DtamGetgrent for NIS local group.  
 * 	If strip is True, the '+' sign is stripped out the group name.
 * 	If complete is True, the group info is expanded using the NIS map.
 */

static struct group *
nis_group(char *g_name, struct group *gr, 
	Boolean strip, Boolean complete, Boolean *nis_up)
{
	char 		*g_strip;
	struct	group	*t_gr;

	/* strip '+' sign */
	g_strip = &g_name[1];

	/* 
	 * if desire to expand NIS group info,  get the info from the NIS map
	 * (getgrnam will expand)
	 */

	if (complete == NIS_EXPAND) {
		if (!(t_gr = getgrnam(g_strip))) {
			if (nis_up != NULL)
				*nis_up = False;
			t_gr = gr;	/* couldn't expand - NIS down */
		} else {
			if (nis_up != NULL)
				*nis_up = True;
		}
		
	} else 
		t_gr = gr;

	if (strip == STRIP)
		t_gr->gr_name = STRDUP(g_strip);
	else 
		t_gr->gr_name = STRDUP(g_name);

	return (t_gr);
}
