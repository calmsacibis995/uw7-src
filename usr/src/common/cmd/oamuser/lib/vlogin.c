#ident	"@(#)vlogin.c	1.2"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<pwd.h>
#include	<shadow.h>
#include	<nwusers.h>
#include	<userdefs.h>
#include	<users.h>
#include    <limits.h>

extern	struct	passwd	*getpwnam(), *nis_getpwnam();
extern	struct	spwd	*getspnam();
extern	struct	passwd	*getunixnam();

/*
 * validate string given as login name.
 */
int
valid_login(login, pptr)
	char	*login;
	struct passwd	**pptr;
{
	register struct	passwd	*t_pptr;
	register struct	spwd	*s_pptr;
	register char *ptr = login;

	if (!login || !*login) {
		return INVALID;
	}

	if (!strcmp(login, ".") || !strcmp(login, "..")) {
		return INVALID;
	}
		
	if (strlen(login) >= (size_t) LOGNAME_MAX) {
		return INVALID;
	}

	for ( ; *ptr != NULL; ptr++) {
		if (!isprint(*ptr) || (*ptr == ':'))
			return INVALID;
	}
	if (t_pptr = nis_getpwnam(login)) {
		if (pptr) {
			*pptr = t_pptr;
		}
		return NOTUNIQUE;
	}
	if (s_pptr = getspnam(login)) {
		return NOTUNIQUE;
	}
	return UNIQUE;
}


int
all_numeric(strp)
	char	*strp;
{
	register char *ptr = strp;

	for (; *ptr != NULL; ptr++) {
		if (!isdigit(*ptr)) {
			return 0;
		}
	}
	return 1;
}


/*
 * validate string given as ndsname.
 */
int
valid_ndsname(ndsname)
	char	*ndsname;
{
	register struct	passwd	*s_pptr;
	register char *ptr = ndsname;
	char equivname[NDSFULNAME_MAX]; /* For generating a equivalent NDS name. */

	for ( ; *ptr != NULL; ptr++) {
		if (!isprint(*ptr) || (*ptr == ':'))
			return INVALID;
	}

	if ( ! parse_ndsname(ndsname,equivname) ) 
		return INVALID;

	if (s_pptr = getunixnam(ndsname)) {
		return NOTUNIQUE;
	}

	if (s_pptr = getunixnam(equivname)) {
		return NOTUNIQUE;
	}

	return UNIQUE;
}

#define ILLEGAL_NDS_NAME	0

#define FIRST 0
#define MIDDLE 1
#define LAST 2

int parse_ndsname(ndsname,equivname)
	char *ndsname;
	char *equivname;
{
	char *comp_end;
	int typed, pos, lbl_len, ret = 1;
	static char *labels[] = { "CN=", "OU=", "O=" };

	typed = !strncmp(ndsname, labels[FIRST], strlen(labels[FIRST]));
	if (strlen(ndsname) > (typed ? NDSFULNAME_MAX : NDSNAME_MAX))
		return ILLEGAL_NDS_NAME;
	for (pos = FIRST ; pos != LAST ; ) {
		/* isolate current component */
		if ((comp_end = strchr(ndsname, '.')) == NULL) {
			if (pos == FIRST) { /* 2 components (CN and O) minimum */
				ret = ILLEGAL_NDS_NAME;
				break;
			}
			pos = LAST;
		} else
			*comp_end = '\0';

		/* parse this component; add it to equivname */
		if (typed) {
			if (strncmp(ndsname, labels[pos], lbl_len = strlen(labels[pos]))) {
				ret = ILLEGAL_NDS_NAME;
				break;
			}
			ndsname += lbl_len;
			equivname += sprintf(equivname, "%s", ndsname);
		} else /* typeless */
			equivname += sprintf(equivname, "%s%s", labels[pos],
					     ndsname);
		if (!*ndsname || (strchr(ndsname, '=') != NULL)) {
			/* component is zero length, or has invalid char */
			ret = ILLEGAL_NDS_NAME;
			break;
		}

		/* restore the component delimiter, point to next component */
		if (pos != LAST) {
			*equivname++ = *comp_end = '.';
			ndsname = comp_end + 1;
			pos = MIDDLE;
		}
	}
	if (comp_end != NULL)
		*comp_end = '.';
	return ret;
}
