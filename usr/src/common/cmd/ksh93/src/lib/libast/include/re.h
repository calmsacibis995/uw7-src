#ident	"@(#)ksh93:src/lib/libast/include/re.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * regular expression library definitions
 */

#ifndef _RE_H
#define _RE_H

#include <sfio.h>

#define RE_ALL		(1<<0)	/* substitute all occurrences		*/
#define RE_EDSTYLE	(1<<1)	/* ed(1) style magic characters		*/
#define RE_LOWER	(1<<2)	/* substitute to lower case		*/
#define RE_MATCH	(1<<3)	/* record matches in Re_program_t.match	*/
#define RE_UPPER	(1<<4)	/* substitute to upper case		*/
#define RE_EXTERNAL	8	/* first external flag bit		*/

typedef struct			/* sub-expression match			*/
{
	char*	sp;		/* start in source string		*/
	char*	ep;		/* end in source string			*/
} Re_match_t;

typedef struct			/* compiled regular expression program	*/
{
#ifdef _RE_PROGRAM_PRIVATE_
	_RE_PROGRAM_PRIVATE_
#else
	Re_match_t	match['9'-'0'+1];/* sub-expression match table*/
#endif
} Re_program_t, reprogram;

/*
 * interface routines
 */

extern Re_program_t*	recomp(const char*, int);
extern int		reexec(Re_program_t*, const char*);
extern void		refree(Re_program_t*);
extern void		reerror(const char*);
extern char*		resub(Re_program_t*, const char*, const char*, char*, int);
extern void		ressub(Re_program_t*, Sfio_t*, const char*, const char*, int);

#endif
