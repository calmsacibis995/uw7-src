/*		copyright	"%c%" 	*/

#ident	"@(#)ksh:shlib/optget.c	1.3.6.2"
#ident "$Header$"

/*
 * G. S. Fowler
 * AT&T Bell Laboratories
 *
 * command line option parse assist
 *
 *	-- or ++ terminates option list
 *
 *	return:
 *		0	no more options
 *		'?'	unknown option opt_option
 *		':'	option opt_option requires an argument
 *		'#'	option opt_option requires a numeric argument
 *
 *	conditional compilation:
 *
 *		KSHELL	opt_num and opt_argv disabled
 */

int		opt_index;		/* argv index			*/
int		opt_char;		/* char pos in argv[opt_index]	*/
#ifndef KSHELL
long		opt_num;		/* # numeric argument		*/
char**		opt_argv;		/* argv				*/
#endif
char*		opt_arg;		/* {:,#} string argument	*/
char		opt_option[3];		/* current flag {-,+} + option	*/

int		opt_pindex;		/* prev opt_index for backup	*/
int		opt_pchar;		/* prev opt_char for backup	*/

#ifdef KSHELL
#include	"sh_config.h"
#endif
extern char*	strchr();

#ifndef KSHELL
extern long	strtol();
#endif

int
optget(argv, opts)
register char**	argv;
char*		opts;
{
	register int	c;
	register char*	s;
#ifndef KSHELL
	char*		e;
#endif

	opt_pindex = opt_index;
	opt_pchar = opt_char;
	for (;;)
	{
		if (!opt_char)
		{
			if (!opt_index)
			{
				opt_index++;
#ifndef KSHELL
				opt_argv = argv;
#endif
			}
			if (!(s = argv[opt_index]) || (opt_option[0] = *s++) != '-' && opt_option[0] != '+' || !*s)
				return(0);
			if (*s++ == opt_option[0] && !*s)
			{
				opt_index++;
				return(0);
			}
			opt_char++;
		}
		if (opt_option[1] = c = argv[opt_index][opt_char++]) break;
		opt_char = 0;
		opt_index++;
	}
	opt_arg = 0;
#ifndef KSHELL
	opt_num = 0;
#endif
	if (c == ':' || c == '#' || c == '?' || !(s = strchr(opts, c)))
	{
#ifdef KSHELL
		return('?');
#else
		if (c < '0' || c > '9' || !(s = strchr(opts, '#')) || s == opts) return('?');
		c = *--s;
#endif
	}
	if (*++s == ':' || *s == '#')
	{
		if (!*(opt_arg = &argv[opt_index++][opt_char]))
		{
			if (!(opt_arg = argv[opt_index]))
			{
				if (*(s + 1) != '?') c = ':';
			}
			else
			{
				opt_index++;
				if (*(s + 1) == '?')
				{
					if (*opt_arg == '-' || *opt_arg == '+')
					{
						if (*(opt_arg + 1)) opt_index--;
						opt_arg = 0;
					}
#ifndef KSHELL
					else if (*s++ == '#')
					{
						opt_num = strtol(opt_arg, &e, 0);
						if (*e) opt_arg = 0;
					}
#endif
				}
			}
		}
#ifndef KSHELL
		if (*s == '#' && opt_arg)
		{
			opt_num = strtol(opt_arg, &e, 0);
			if (*e) c = '#';
		}
#endif
		opt_char = 0;
	}
	return(c);
}
