#ifndef	_RegExp_h_
#define _RegExp_h_

#ident	"@(#)debugger:inc/common/RegExp.h	1.1"

// Shell style pattern matching and regular expression
// compile/execute

// Provide a common interface that takes care of the difference
// between old libgen style routines and the new XPG routines

#ifdef OLD_REGEXP
#include <libgen.h>
#include <regexpr.h>
#else
#include <fnmatch.h>
#include <regex.h>
#endif

// shell style pattern matching
// 3 types of match - the old gmatch from libgen, the new
// X/Open fnmatch, and the faster non-standard fnmcomp/fnmexec
// (the fast versions implement the standards-conforming pattern
// matching but have non-standard API's)

class PatternMatch {
#if defined(OLD_REGEXP) || !defined(FNM_REUSE)
	const char	*saved_pattern;
#else
	fnm_t		fnm;
	int		first;
#endif
public:
#if defined(OLD_REGEXP) || !defined(FNM_REUSE)
			PatternMatch() { saved_pattern = 0; }
			~PatternMatch() { delete (char *)saved_pattern; }
#else
			PatternMatch() { first = 1; }
			~PatternMatch() {}
#endif
	int		compile(const char *pattern);
	int		match(const char *name);
};

// Regular expression compile/execute; the libgen version
// supports only the grep style of regular expressions; 
// the newer version also supports egrep style extended
// regular expressions.

class RegExp {
#ifdef OLD_REGEXP
	const char	*re;
#else
	regex_t		*re;
#endif
	int		match_begin;
	int		match_end;
	int		save_extent;
public:
			RegExp() { re = 0; }
			~RegExp();
	int		re_compile(const char *, int extended,
				int save_extent);
	int		re_execute(const char *, int not_begin = 0);
	int		get_begin() { return match_begin; }
	int		get_end() { return match_end; }
};
#endif
