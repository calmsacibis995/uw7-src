/*		copyright	"%c%" 	*/

#ident	"@(#)sh:common/cmd/sh/expand.c	1.18.8.5"
#ident "$Header$"
/*
 *	UNIX shell
 *
 */

#include	"defs.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>



/*
 * globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */
static void	addg();

extern int	gmatch();

expand(as, rcnt)
	unsigned char	*as;
{
	int	count; 
	DIR	*dirf;
	unsigned char	*rescan = 0;
	unsigned char 	*slashsav = 0;
	register unsigned char	*s, *cs;
	unsigned char *s2 = 0;
	struct argnod	*schain = gchain;
	struct stat	st;
	BOOL	slash;

	if (trapnote & SIGSET)
		return(0);
	s = cs = as;
	/*
	 * check for meta chars
	 */
	{
		register BOOL open;

		slash = 0;
		open = 0;
		do
		{
			switch (*cs++)
			{
			case 0:
				if (rcnt && slash)
					break;
				else
					return(0);

			case '/':
				slash++;
				open = 0;
				continue;

			case '[':
				open++;
				continue;

			case ']':
				if (open == 0)
					continue;

			case '?':	/*FALLTHROUGH*/
			case '*':
				if (rcnt > slash)
					continue;
				else
					cs--;
				break;


			case '\\':
				cs++;
			default:	/*FALLTHROUGH*/
				continue;
			}
			break;
		} while (TRUE);
	}

	for (;;)
	{
		if (cs == s)
		{
			s = (unsigned char *)nullstr;
			break;
		}
		else if (*--cs == '/')
		{
			*cs = 0;
			if (s == cs)
				s = (unsigned char *)"/";
			else {
			/* push trimmed copy of directory prefix
			   onto stack */
				s2 = cpystak(s);
				trim(s2);
				s = s2;
			}
			break;
		}
	}

	/*
	 * opendir() first tries to open its argument and then checks to see
	 * if it is a directory.  We want to avoid opening non-directories
	 * because of possible side effects, e.g. if the argument is a device
	 * special file; therefore, we check here before calling opendir().
	 */
	if (*s == '\0')
		dirf = opendir(".");
	else if (stat((char *)s, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR)
		dirf = opendir((char *)s);
	else
		dirf = 0;

	/* Let s point to original string because it will be trimmed later */
	if(s2)
		s = as;
	count = 0;
	if (*cs == 0)
		slashsav = cs++; /* remember where first slash in as is */

	/* check for rescan */
	if (dirf)
	{
		register unsigned char *rs;
		struct dirent *e;

		rs = cs;
		do /* find next / in as */
		{
			if (*rs == '/')
			{
				rescan = rs;
				*rs = 0;
				gchain = 0;
			}
		} while (*rs++);

		while ((e = readdir(dirf)) && (trapnote & SIGSET) == 0)
		{
			if (e->d_name[0] == '.'
			    && !(*cs == '.' || (*cs == '\\' && *(cs+1) == '.')))
				continue;

			if (gmatch(e->d_name, cs))
			{
				addg(s, e->d_name, rescan, slashsav);
				count++;
			}
		}
		(void)closedir(dirf);

		if (rescan)
		{
			register struct argnod	*rchain;

			rchain = gchain;
			gchain = schain;
			if (count)
			{
				count = 0;
				while (rchain)
				{
					count += expand(rchain->argval, slash + 1);
					rchain = rchain->argnxt;
				}
			}
			*rescan = '/';
		}
	}

	if(slashsav)
		*slashsav = '/';
	return(count);
}


static void
addg(as1, as2, as3, as4)
unsigned char	*as1, *as2, *as3, *as4;
{
	register unsigned char	*s1, *s2;

	s2 = locstak() + BYTESPERWORD;
	s1 = as1;
	if(as4) {
		while (*s2 = *s1++)
			s2++; 
	/* Restore first slash before the first metacharacter if as1 is not "/" */
		if(as4 + 1 == s1)
			*s2++ = '/';
	}
/* add matched entries, plus extra \\ to escape \\'s */
	s1 = as2;
	while (*s2 = *s1++) {
		if(*s2 == '\\')
			*++s2 = '\\';
		s2++;
	}
	s1 = as3;
	if (s1)
	{
		*s2++ = '/';
		while (*s2++ = *++s1);
	}
	makearg(endstak(s2));
}

void
makearg(args)
	register struct argnod *args;
{
	args->argnxt = gchain;
	gchain = args;
}


