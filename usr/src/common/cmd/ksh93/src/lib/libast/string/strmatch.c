#ident	"@(#)ksh93:src/lib/libast/string/strmatch.c	1.1"
#include <fnmatch.h>
#include <ast.h>

/*
 * compare the string s with the shell pattern p
 * returns 1 for match 0 otherwise
 */

int
strmatch(const char* s, const char* p)
{
	return fnmatch(p, s, FNM_EXTENDED) == 0;
}

/*
 * leading substring match
 * first char after end of substring returned
 * 0 returned if no match
 *
 * OBSOLETE: use strgrpmatch()
 */

char*
strsubmatch(const char* s, const char* p, int flags)
{
	int n = fnmatch(p, s,
	      FNM_EXTENDED | ((flags & STR_MAXIMAL) ? FNM_RETMAX : FNM_RETMIN));
	if (n < 0)
		return 0;
	if ((flags & STR_RIGHT) && (n != strlen(s)))
		return 0;
	return (char *)&s[n];
}
