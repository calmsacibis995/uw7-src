#ident	"@(#)ksh93:src/lib/libast/misc/optjoin.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * multi-pass commmand line option parse assist
 *
 *	int fun(char** argv, int last)
 *
 * each fun() argument parses as much of argv as
 * possible starting at (opt_info.index,opt_info.offset) using
 * optget()
 *
 * if last!=0 then fun is the last pass to view
 * the current arg, otherwise fun sets opt_info.again=1
 * and another pass will get a crack at it
 *
 * 0 fun() return causes immediate optjoin() 0 return
 *
 * optjoin() returns non-zero if more args remain
 * to be parsed at opt_info.index
 */

#include <ast.h>
#include <option.h>

typedef int (*OPTFUN)(char**, int);

int
optjoin(char** argv, ...)
{
	va_list			ap;
	register OPTFUN		fun;
	register OPTFUN		rep;
	OPTFUN			err;
	int			more;
	int			user;
	int			last_index;
	int			last_offset;
	int			err_index;
	int			err_offset;
#if !OBSOLETE_IN_96

#undef	opt_again
	extern int		opt_again;
	static int		opt_again_old;
#undef	opt_char
	extern int		opt_char;
	static int		opt_char_old;
#undef	opt_index
	extern int		opt_index;
	static int		opt_index_old;

	if (opt_again_old != opt_again)
		opt_info.again = opt_again_old = opt_again;
	if (opt_char_old != opt_char)
		opt_info.offset = opt_char_old = opt_char;
	if (opt_index_old != opt_index)
		opt_info.index = opt_index_old = opt_index;
#endif

	err = rep = 0;
	for (;;)
	{
		va_start(ap, argv);
		while (fun = va_arg(ap, OPTFUN))
		{
			last_index = opt_info.index;
			last_offset = opt_info.offset;
#if !OBSOLETE_IN_96
			opt_again = opt_again_old = opt_info.offset;
			opt_char = opt_char_old = opt_info.offset;
			opt_index = opt_index_old = opt_info.index;
#endif
			user = (*fun)(argv, 0);
#if !OBSOLETE_IN_96
			if (opt_again_old != opt_again)
				opt_info.again = opt_again_old = opt_again;
			if (opt_char_old != opt_char)
				opt_info.offset = opt_char_old = opt_char;
			if (opt_index_old != opt_index)
				opt_info.index = opt_index_old = opt_index;
#endif
			more = argv[opt_info.index] != 0;
			if (!opt_info.again)
			{
				if (!more) return(0);
				if (!user)
				{
					if (*argv[opt_info.index] != '+') return(1);
					opt_info.again = -1;
				}
				else err = 0;
			}
			if (opt_info.again)
			{
				if (opt_info.again > 0 && (!err || err_index < opt_info.index || err_index == opt_info.index && err_offset < opt_info.offset))
				{
					err = fun;
					err_index = opt_info.index;
					err_offset = opt_info.offset;
				}
				opt_info.again = 0;
				if (opt_info.pindex)
					opt_info.index = opt_info.pindex;
				opt_info.offset = opt_info.poffset;
			}
			if (!rep || opt_info.index != last_index || opt_info.offset != last_offset)
				rep = fun;
			else if (fun == rep)
			{
				if (!err) return(1);
				(*err)(argv, 1);
				opt_info.offset = 0;
			}
		}
		va_end(ap);
	}
}
