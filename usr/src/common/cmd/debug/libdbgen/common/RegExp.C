#ident	"@(#)debugger:libdbgen/common/RegExp.C	1.1"

#include "RegExp.h"
#include "str.h"

#ifdef OLD_REGEXP
#include <libgen.h>
#include <regexpr.h>
#include <stdlib.h>
#else
#include <fnmatch.h>
#include <regex.h>
#include <string.h>
#endif

// shell style pattern matching
// 3 types of match - the old gmatch from libgen, the new
// X/Open fnmatch, and the faster non-standard fnmcomp/fnmexec
// (the fast versions implement the standards-conforming pattern
// matching but have non-standard API's)

#if defined(OLD_REGEXP) || !defined(FNM_REUSE)

int
PatternMatch::compile(const char *pattern) 
{
	delete (char *)saved_pattern;
	saved_pattern = makestr(pattern);
	return 1;
}

#else
int 
PatternMatch::compile(const char *pattern)
{
	// behave more like old gmatch
	int	flags = FNM_BADRANGE|FNM_BKTESCAPE;

	if (first)
		first = 0;
	else
		flags |= FNM_REUSE;
	return(_fnmcomp(&fnm, (const unsigned char *)pattern, flags) == 0);
}
#endif

#ifdef OLD_REGEXP
int
PatternMatch::match(const char *name)
{
	return(gmatch(name, saved_pattern));
}

#elif !defined(FNM_REUSE)
int
PatternMatch::match(const char *name)
{
	return(fnmatch(saved_pattern, name, 0) == 0);
}

#else
int
PatternMatch::match(const char *name)
{
	return(_fnmexec(&fnm, (const unsigned char *)name) == 0);
}
#endif

// grep/egrep style regular expression handling - for the
// old libgen style model, we don't support extended regular
// expressions

#ifdef OLD_REGEXP

RegExp::~RegExp()
{
	if (re)
		free((void *)re);
}

int
RegExp::re_compile(const char *pattern, int, int se)
{
	save_extent = se;
	if (re)
		free((void *)re);
	if ((re = compile(pattern, 0, 0)) == 0)
		return 0;
	return 1;
}

int
RegExp::re_execute(const char *string, int not_begin)
{
	if (not_begin)
	{
		// not beginning of line
		locs = loc2 = (char *)string;
	}
	if (!step(string, re))
		return 0;
	if (save_extent)
	{
		match_begin = loc1 - string;
		match_end = loc2 - string;
	}
	return 1;
}

#else 	// XPG4 interfaces

RegExp::~RegExp()
{
	if (re)
	{
		regfree(re);
		delete re;
	}
}

int
RegExp::re_compile(const char *pattern, int extended, int se)
{
	int	flags = 0;

	save_extent = se;

	if (re)
		regfree(re);
	else
	{
		re = new regex_t;
		memset(re, 0, sizeof(regex_t));
	}
	if (extended)
	{
		flags = REG_OLDBRE & ~REG_ESCNL; // Posix.2 allows 
		flags |= REG_EXTENDED|REG_MTPARENBAD|REG_NLALT;
	}
	if (save_extent)
		flags |= REG_ONESUB;
	else
		flags |= REG_NOSUB;
	return(regcomp(re, pattern, flags) == 0);
}

int
RegExp::re_execute(const char *string, int not_begin)
{
	int	flags = not_begin ? REG_NOTBOL : 0;
	if (save_extent)
	{
		regmatch_t	pmatch;
		if (regexec(re, string, 1, &pmatch, flags) == 0)
		{
			match_begin = pmatch.rm_so;
			match_end = pmatch.rm_eo;
			return 1;
		}
		else
			return 0;
	}
	else
	{
		return(regexec(re, string, 0, 0, flags) == 0);
	}
}
#endif
