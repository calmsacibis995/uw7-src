#ident	"@(#)ksh93:src/lib/libast/misc/optget.c	1.2"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * command line option parse assist
 */

#define _OPT_INFO_PRIVATE \
	char*	opts[8];		/* cached opts for optjoin()	*/ \
	int	nopt;			/* opts[] index			*/

#include <ast.h>
#include <error.h>

extern char	*gettxt();

#define OPT_MSG		512

#define error		opterror

#if _DLL_INDIRECT_DATA && !_DLL
static Opt_info_t	opt_info_data;
Opt_info_t		opt_info = &opt_info_data;
#else
Opt_info_t		opt_info = { 0 };
#endif

#if !OBSOLETE_IN_96

/*
 * drop this shared lib compatibility code in 96
 */

#undef	opt_again
int		opt_again;
#undef	opt_arg
char*		opt_arg;
#undef	opt_argv
char**		opt_argv;
#undef	opt_index
int		opt_index;
#undef	opt_msg
char*		opt_msg;
#undef	opt_num
long		opt_num;
#undef	opt_char
int		opt_char;
#undef	opt_option
char		opt_option[3];
#undef	opt_pindex
int		opt_pindex;
#undef	opt_pchar
int		opt_pchar;

int
optget(char** argv, const char* oopts)
{
	int		c;

	static int	opt_char_old;
	static int	opt_index_old;

	if (opt_char_old != opt_char)
		opt_info.offset = opt_char_old = opt_char;
	if (opt_index_old != opt_index)
		opt_info.index = opt_index_old = opt_index;
	c = _optget(argv, oopts);
	opt_char = opt_char_old = opt_info.offset;
	opt_index = opt_index_old = opt_info.index;
	if (opt_info.argv && !opt_argv)
		opt_argv = opt_info.argv;
	return(c);
}

#define optget	_optget

#endif

/*
 * point opt_info.arg to an error message for opt_info.option
 * p points to opts location for opt_info.option
 * optget() return value is returned
 */

static int
error(register char* p)
{
	register char*	s;
	register char*	t;
	int		n;

	if (!opt_info.msg && !(opt_info.msg = newof(0, char, OPT_MSG, 0))) opt_info.arg = ERROR_translate(gettxt("ksh93:225","[* out of space *]"),0);
	else
	{
		s = opt_info.arg = opt_info.msg;
		*s++ = opt_info.option[0];
		*s++ = opt_info.option[1];
		*s++ = ':';
		*s++ = ' ';
		if (*p == '#' || *p == ':')
		{
			if (*p == '#')
			{
				t = ERROR_translate(gettxt("ksh93:226","numeric"), 0);
				while (*s = *t++) s++;
				*s++ = ' ';
			}
			if (*++p == '[')
			{
				n = 1;
				while (*s = *++p)
				{
					if (*s == '[') n++;
					else if (*s == ']' && !--n) break;
					s++;
				}
				*s++ = ' ';
			}
			p = ERROR_dictionary(gettxt("ksh93:9","argument expected"));
		}
		else { 
			p = ERROR_dictionary(gettxt("ksh93:228","unknown option"));
		}
		p = ERROR_translate(p, 0);
		while (*s++ = *p++);
	}
	return(':');
}

#define OPT_ignore	(1<<0)
#define OPT_plus	(1<<1)

/*
 * add opts to opt_info.opts for optusage()
 */

static void
optadd(const char* opts)
{
	register int	n;

	if ((!opt_info.nopt || opt_info.opts[opt_info.nopt - 1] != (char*)opts) && opt_info.nopt < elementsof(opt_info.opts) - 1)
	{
		if (opt_info.nopt && *opt_info.opts[opt_info.nopt - 1] == ':') opt_info.nopt--;
		n = 0;
		do if (n >= opt_info.nopt)
		{
			opt_info.opts[opt_info.nopt++] = (char*)opts;
			break;
		} while (opt_info.opts[n++] != (char*)opts);
	}
}

/*
 * argv:	command line argv where argv[0] is command name
 *
 * opts:	option control string
 *
 *	':'		option takes string arg
 *	'#'		option takes numeric arg (concat option may follow)
 *	'?'		(first) following options not in usage
 *			(following # or :) optional arg
 *			(following option or ]) remaining not in usage
 *	'['...']'	(following # or :) optional option arg description
 *			(otherwise) mutually exclusive option grouping
 *	' '...		optional argument(s) description (to end of string)
 *
 * return:
 *
 *	0		no more options
 *	'?'		usage: opt_info.arg points to message sans `Usage: command'
 *	':'		error: opt_info.arg points to message sans `command:'
 *
 * -- terminates option list and returns 0
 *
 * + as first opts char makes + equivalent to -
 *
 * -? puts usage message sans command name in opt_info.arg and returns '?'
 *
 * if any # option is specified then numeric options (e.g., -123)
 * are associated with the leftmost # option in opts
 */

int
optget(register char** argv, const char* oopts)
{
	register int	c;
	register char*	s;
	char*		e;
	char*		opts;
	int		n;
	int		flags = 0;

	opt_info.pindex = opt_info.index;
	opt_info.poffset = opt_info.offset;
	if (!opt_info.index)
	{
		opt_info.index = 1;
		opt_info.offset = 0;
		opt_info.nopt = 0;
	}
	opt_info.opts[opt_info.nopt] = (char*)oopts;
	for (n = 0; n <= opt_info.nopt; n++)
		for (opts = opt_info.opts[n];; opts++)
		{
			switch (*opts)
			{
			case '?':
				if (n == opt_info.nopt) flags |= OPT_ignore;
				continue;
			case '+':
				flags |= OPT_plus;
				continue;
			case ':':
				continue;
			}
			break;
		}
	for (;;)
	{
		if (!opt_info.offset)
		{
			if (opt_info.index == 1)
			{
				opt_info.argv = argv;
				if (!(flags & OPT_ignore)) optadd(oopts);
			}
			if (!(s = argv[opt_info.index]) || (c = *s++) != '-' && (c != '+' || !(flags & OPT_plus) && (*s < '0' || *s > '9' || !strchr(opts, '#'))) || !*s)
				return(0);
			if (*s++ == c && !*s)
			{
				opt_info.index++;
				return(0);
			}
			opt_info.offset++;
		}
		if (c = opt_info.option[1] = argv[opt_info.index][opt_info.offset++])
		{
			opt_info.option[0] = argv[opt_info.index][0];
			break;
		}
		opt_info.offset = 0;
		opt_info.index++;
	}
	if (!(flags & OPT_ignore)) optadd(oopts);
	if (c == '?')
	{
		opt_info.arg = optusage(NiL);
		return(c);
	}
	e = 0;
	s = opts;
	if (c == ':' || c == '#' || c == ' ' || c == '[' || c == ']')
	{
		if (c != *s) s = "";
	}
	else while (*s)
	{
		if (*s == c) break;
		if (*s == ' ')
		{
			s = "";
			break;
		}
		if (*s == ':' || *s == '#')
		{
			if (!e && *s == '#' && s > opts) e = s - 1;
			if (*++s == '?') s++;
			if (*s == '[')
			{
				n = 1;
				while (*++s)
				{
					if (*s == '[') n++;
					else if (*s == ']' && !--n)
					{
						s++;
						break;
					}
				}
				n = 0;
			}
		}
		else s++;
	}
	if (!*s)
	{
		if (c < '0' || c > '9' || !e) return(error(""));
		c = opt_info.option[1] = *(s = e);
		opt_info.offset--;
	}
	opt_info.arg = 0;
	if (*++s == ':' || *s == '#')
	{
		if (*(opt_info.arg = &argv[opt_info.index++][opt_info.offset]))
		{
			if (*s == '#')
			{
				opt_info.num = strtol(opt_info.arg, &e, 0);
				if (e == opt_info.arg)
				{
					if (*(s + 1) == '?')
					{
						opt_info.arg = 0;
						opt_info.index--;
						return(c);
					}
					else c = error(s);
				}
				else if (*e)
				{
					opt_info.offset += e - opt_info.arg;
					opt_info.index--;
					return(c);
				}
			}
		}
		else if (opt_info.arg = argv[opt_info.index])
		{
			opt_info.index++;
			if (*(s + 1) == '?' && (*opt_info.arg == '-' || (flags & OPT_plus) && *opt_info.arg == '+') && *(opt_info.arg + 1))
			{
				opt_info.index--;
				opt_info.arg = 0;
			}
			else if (*s == '#')
			{
				opt_info.num = strtol(opt_info.arg, &e, 0);
				if (*e)
				{
					if (*(s + 1) == '?')
					{
						opt_info.arg = 0;
						opt_info.index--;
					}
					else c = error(s);
				}
			}
		}
		else if (*(s + 1) != '?') c = error(s);
		opt_info.offset = 0;
	}
	else if (!argv[opt_info.index][opt_info.offset])
	{
		opt_info.offset = 0;
		opt_info.index++;
	}
	return(c);
}

#include <sfstr.h>

/*
 * return pointer to usage message sans `Usage: command'
 * if opts is 0 then opt_info.opts is used
 */

char*
optusage(const char* oopts)
{
	char*			opts = (char*)oopts;
	register Sfio_t*	sp;
	register int		c;
	register char*		p;
	char*			t;
	char*			x;
	char**			o;
	char**			v;
	char**			e;
	int			m;
	int			n;
	int			q;
	int			z;
	int			b;
	Sfio_t*			sp_text = 0;
	Sfio_t*			sp_plus = 0;
	Sfio_t*			sp_args = 0;
	Sfio_t*			sp_excl = 0;

	if (opts)
	{
		o = &opts;
		e = o + 1;
	}
	else
	{
		o = opt_info.opts;
		e = o + opt_info.nopt;
	}
	if (e == o) return(gettxt("ksh93:229","[* call optget() before optusage() *]"));
	if (!opt_info.msg && !(opt_info.msg = newof(0, char, OPT_MSG, 0))) goto nospace;
	if (!(sp_text = sfstropen())) goto nospace;
	x = 0;
	z = 0;
	for (v = o; v < e; v++)
	{
		p = ERROR_translate(*v, 0);
		if (*p == ':') p++;
		if (*p == '+')
		{
			p++;
			if (!(sp = sp_plus) && !(sp = sp_plus = sfstropen())) goto nospace;
		}
		else sp = sp_text;
		while (c = *p++)
		{
			if (c == '[')
			{
				if (!z)
				{
					if (!sp_excl && !(sp_excl = sfstropen())) goto nospace;
					if (sfstrtell(sp_excl)) sfputc(sp_excl, ' ');
					sfputc(sp_excl, '[');
					b = sfstrtell(sp_excl);
				}
				m = 1;
				for (;;)
				{
					if (!(c = *p)) break;
					p++;
					if (c == '[')
					{
						m++;
						if (!z)
						{
							if (sfstrtell(sp_excl) != b)
								sfputc(sp_excl, '|');
							sfputc(sp_excl, '[');
						}
					}
					else if (c == ']')
					{
						if (!z) sfputc(sp_excl, ']');
						if (!--m) break;
					}
					else
					{
						if (!z)
						{
							if (sfstrtell(sp_excl) != b)
								sfputc(sp_excl, '|');
							if (sp == sp_plus)
								sfputc(sp_excl, '+');
							sfputc(sp_excl, '-');
							sfputc(sp_excl, c);
						}
						if (*p == ':' && (t = "arg") || *p == '#' && (t = "#"))
						{
							if (q = (*++p == '?'))
							{
								p++;
								c = '[';
							}
							else c = ' ';
							if (*p == '[')
							{
								if (*(p + 1) == ']') p += 2;
								else
								{
									if (!z) sfputc(sp_excl, c);
									n = 1;
									while (c = *++p)
									{
										if (c == '[') n++;
										else if (c == ']' && !--n)
										{
											p++;
											break;
										}
										if (!z) sfputc(sp_excl, c);
									}
								}
							}
							else if (!z)
							{
								sfputc(sp_excl, c);
								sfputr(sp_excl, t, -1);
							}
						}
						if (!z && q) sfputc(sp_excl, ']');
					}
				}
			}
			else if (c == ' ')
			{
				x = p;
				break;
			}
			else if (c == '?') z = 1;
			else if (*p == ':' && (t = "arg") || *p == '#' && (t = "#"))
			{
				if (!z)
				{
					if (sp_args) sfputc(sp_args, ' ');
					else if (!(sp_args = sfstropen())) goto nospace;
					sfputc(sp_args, '[');
					if (sp == sp_plus) sfputc(sp_args, '+');
					sfputc(sp_args, '-');
					sfputc(sp_args, c);
				}
				if (q = (*++p == '?'))
				{
					p++;
					c = '[';
				}
				else c = ' ';
				if (*p == '[')
				{
					if (*(p + 1) == ']') p += 2;
					else
					{
						if (!z) sfputc(sp_args, c);
						n = 1;
						while (c = *++p)
						{
							if (c == '[') n++;
							else if (c == ']' && !--n)
							{
								p++;
								break;
							}
							if (!z) sfputc(sp_args, c);
						}
					}
				}
				else if (!z)
				{
					sfputc(sp_args, c);
					sfputr(sp_args, t, -1);
				}
				if (!z)
				{
					if (q) sfputc(sp_args, ']');
					sfputc(sp_args, ']');
				}
			}
			else if (!z)
			{
				if (!sfstrtell(sp))
				{
					if (sfstrtell(sp)) sfputc(sp, ' ');
					sfputc(sp, '[');
					if (sp == sp_plus) sfputc(sp, '+');
					sfputc(sp, '-');
				}
				sfputc(sp, c);
			}
		}
	}
	sp = sp_text;
	if (sfstrtell(sp)) sfputc(sp, ']');
	if (sp_plus)
	{
		if (sfstrtell(sp_plus))
		{
			if (sfstrtell(sp)) sfputc(sp, ' ');
			sfputr(sp, sfstruse(sp_plus), ']');
		}
		sfclose(sp_plus);
	}
	if (sp_excl)
	{
		if (sfstrtell(sp_excl))
		{
			if (sfstrtell(sp)) sfputc(sp, ' ');
			sfputr(sp, sfstruse(sp_excl), -1);
		}
		sfclose(sp_excl);
	}
	if (sp_args)
	{
		if (sfstrtell(sp_args))
		{
			if (sfstrtell(sp)) sfputc(sp, ' ');
			sfputr(sp, sfstruse(sp_args), -1);
		}
		sfclose(sp_args);
	}
	if (x)
	{
		if (sfstrtell(sp)) sfputc(sp, ' ');
		sfputr(sp, x, -1);
	}
	if (sfstrtell(sp) >= OPT_MSG) sfstrset(sp, OPT_MSG - 1);
	strcpy(opt_info.msg, sfstruse(sp));
	sfclose(sp);
	return(opt_info.msg);
 nospace:
	if (sp_text) sfclose(sp_text);
	if (sp_plus) sfclose(sp_plus);
	if (sp_args) sfclose(sp_args);
	if (sp_excl) sfclose(sp_excl);
	return(gettxt("ksh93:225","[* out of space *]"));
}
