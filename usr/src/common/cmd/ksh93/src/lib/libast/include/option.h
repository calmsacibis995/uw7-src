#ident	"@(#)ksh93:src/lib/libast/include/option.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * command line option parse assist definitions
 */

#ifndef _OPTION_H
#define _OPTION_H

#if _DLL_INDIRECT_DATA && !_DLL
#define opt_info	(*_opt_info_)
#else
#define opt_info	_opt_info_
#endif

/*
 * obsolete interface mappings
 */

#define opt_again	opt_info.again
#define opt_arg		opt_info.arg
#define opt_argv	opt_info.argv
#define opt_char	opt_info.offset
#define opt_index	opt_info.index
#define opt_msg		opt_info.msg
#define opt_num		opt_info.num
#define opt_option	opt_info.option
#define opt_pindex	opt_info.pindex
#define opt_pchar	opt_info.poffset

typedef struct
{
	int		again;		/* see optjoin()		*/
	char*		arg;		/* {:,#} string argument	*/
	char**		argv;		/* most recent argv		*/
	int		index;		/* argv index			*/
	char*		msg;		/* error/usage message buffer	*/
	long		num;		/* # numeric argument		*/
	int		offset;		/* char offset in argv[index]	*/
	char		option[3];	/* current flag {-,+} + option  */
	int		pindex;		/* prev index for backup	*/
	int		poffset;	/* prev offset for backup	*/
#ifdef _OPT_INFO_PRIVATE
	_OPT_INFO_PRIVATE
#endif
} Opt_info_t;

extern Opt_info_t	opt_info;	/* global option state		*/

extern int		optget(char**, const char*);
extern int		optjoin(char**, ...);
extern char*		optusage(const char*);

#endif
