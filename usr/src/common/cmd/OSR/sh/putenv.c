#ident	"@(#)OSRcmds:sh/putenv.c	1.1"
/*
 *	@(#) putenv.c 21.1 89/09/25 
 *
 *   Portions Copyright 1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sh:putenv.c	1.5.1.1" */
#pragma comment(exestr, "@(#) putenv.c 21.1 89/09/25 ")
/*
 * UNIX shell
 */

/*
 *	MODIFICATION HISTORY
 *
 *	L000	21 Sep 89	scol!howardf
 *	- created
 *	- this putenv routine is required by setname in name.c for
 *	  dynamically changing the locale information within the
 *	  running shell's environment.
 */

/*	putenv - change environment variables

	input - char *change = a pointer to a string of the form
			       "name=value"

	output - 0, if successful
		 1, otherwise
*/

#include "defs.h"

#define NULL 0
static reall = 0;		/* flag to reallocate space, if putenv is called
				   more than once */

int
putenv(change)
char *change;
{
	char **newenv;		    /* points to new environment */
	register int which;	    /* index of variable to replace */

	if ((which = find(change)) < 0)  {
		/* if a new variable */
		/* which is negative of table size, so invert and
		   count new element */
		which = (-which) + 1;
		if (reall)  {
			/* we have expanded environ before */
			newenv = (char **)alloc(which*sizeof(char *));
			if (newenv == NULL)  return -1;
			(void)memcpy((char *)newenv, (char *)environ,
 				(int)(which*sizeof(char *)));
			free(environ);
			/* now that we have space, change environ */
			environ = (unsigned char **)newenv;
		} else {
			/* environ points to the original space */
			reall++;
			newenv = (char **)alloc(which*sizeof(char *));
			if (newenv == NULL)  return -1;
			(void)memcpy((char *)newenv, (char *)environ,
 				(int)(which*sizeof(char *)));
			environ = (unsigned char **)newenv;
		}
		environ[which-2] = (unsigned char *)change;
		environ[which-1] = NULL;
	}  else  {
		/* we are replacing an old variable */
		environ[which] = (unsigned char *)change;
	}
	return 0;
}

/*	find - find where s2 is in environ
 *
 *	input - str = string of form name=value
 *
 *	output - index of name in environ that matches "name"
 *		 -size of table, if none exists
*/
static
find(str)
register char *str;
{
	register int ct = 0;	/* index into environ */

	while(environ[ct] != NULL)   {
		if (match(environ[ct], str)  != 0)
			return ct;
		ct++;
	}
	return -(++ct);
}
/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of 1,
 *	else return 0
 */

static
match(s1, s2)
register char *s1, *s2;
{
	while(*s1 == *s2++)  {
		if (*s1 == '=')
			return 1;
		s1++;
	}
	return 0;
}
