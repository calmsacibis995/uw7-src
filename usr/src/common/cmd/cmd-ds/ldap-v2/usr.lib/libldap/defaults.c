
/*
 * Handle defaults for LDAP commands
 *
 * ldap_read_defaults(FILE *fp) reads a file with contents
 * in the form  name=value, as per enviromental variables.
 *
 * char *ldap_getdef(char *name) returns the value associated
 * with the particular name, along the lines of getenv(3)
 *
 * It should be no surprise that the code has mostly been lifted
 * from the getenv() and putenv() library functions.
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>


static const char **ldap_default;	/* pointer to defaults, as per environ */
static reall = 0;		/* flag to reallocate space, if put is called
				   more than once */
static int find(), match();

static int
ldap_putdef(change)
char *change;
{
	char **new_ldap_default;		    /* points to new ldap_default */
	register int which;	    /* index of variable to replace */
	int retval = 0;

	if ((which = find(change)) < 0)  {
		/* if a new variable */
		/* which is negative of table size, so invert and
		   count new element */
		which = (-which) + 1;
		if (reall)  {
			/* we have expanded ldap_default before */
			new_ldap_default = (char **)realloc(ldap_default,
				  which*sizeof(char *));
			if (new_ldap_default == NULL) {
				retval = -1;
				goto out;
			}
			/* now that we have space, change ldap_default */
			ldap_default = (const char **)new_ldap_default;
		} else {
			/* ldap_default points to the original space */
			reall++;
			new_ldap_default = (char **)malloc(which*sizeof(char *));
			if (new_ldap_default == NULL) {
                                retval = -1;
                                goto out;
                        } 
			(void)memcpy((char *)new_ldap_default, (char *)ldap_default,
 				(int)(which*sizeof(char *)));
			ldap_default = (const char **)new_ldap_default;
		}
		ldap_default[which-2] = change;
		ldap_default[which-1] = NULL;
	}  else  {
		/* we are replacing an old variable */
		ldap_default[which] = change;
	}
out:
	return retval;
}

/*	find - find where s2 is in ldap_default
 *
 *	input - str = string of form name=value
 *
 *	output - index of name in ldap_default that matches "name"
 *		 -size of table, if none exists
*/
static
find(str)
register char *str;
{
	register int ct = 0;	/* index into ldap_default */

	while(ldap_default[ct] != NULL)   {
		if (match(ldap_default[ct], str)  != 0)
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



char *
ldap_getdef(char *name)
{
	register const char *q;
	register int ch;
	register char *s;
	register char **p;

	if ((p = (char **)ldap_default) != 0 && (s = *p) != 0)
	{
		q = name;
		ch = *q;
		do
		{
			if (*s != ch)
				continue;
			while (*++q == *++s)
			{
				/*
				* For compatibility, allow "foo=bar" names,
				* but only search for "foo".
				*/
				if (*q == '=')
					return s + 1;
			}
			if (*q == '\0' && *s == '=')
				return s + 1;
			q = name;
		} while ((s = *++p) != 0);
	}
	return 0;
}

void
ldap_read_defaults(FILE *fp)
{
	char buf[1024];
	char *line;
	char *perm;
	char *c;
	int i;

	while ( (line = fgets( buf, sizeof(buf), fp)) != NULL ) {
		/* skip comments and blank lines */
		if ( line[0] == '#' || line[0] == '\0' ) {
			continue;
		}
		i = strlen(line);
		if (line[i-1] == '\n') {
			line[i-1] = '\0';
			i--;
		}
		
		/*
		 * Do a simple minded removal of quotes, so that
		 * var="foo bar"  gets entered as var=foo bar\0
		 */
		c = strchr(line, '=');

		if (c == (char *)0)
			continue;

		if ((*++c == '"') && (line[i-1] == '"')) {
			line[i-1] = '\0';
			while (*(c+1) != '\0') {
				*c = *(c+1);
				c++;
			}
			*c = '\0';
		}


		if (perm = strdup(line) ) {
			if (ldap_putdef(perm) < 0) {
				/*
				 * Maybe an error indication is required,
				 * just ignore for now 
				 */
			}
		}
	}
}
