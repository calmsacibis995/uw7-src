#ident	"@(#)ksh93:src/lib/libast/string/tokline.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return an Sfio_t* to a file or string that
 *
 *	splices \\n to single lines
 *	checks for "..." and '...' spanning newlines
 *	drops #...\n comments
 *
 * NOTE: seek disabled and string disciplines cannot be composed
 *	 quoted \n translated to \r
 */

#include <ast.h>

typedef struct
{
	Sfdisc_t	disc;
	Sfio_t*		sp;
	int		quote;
	int*		line;
} SPLICE;

/*
 * the splicer
 */

static int
splice(Sfio_t* s, int op, Sfdisc_t* ad)
{
	SPLICE*		d = (SPLICE*)ad;
	register char*	b;
	register int	c;
	register int	n;
	register int	q;
	register char*	e;
	char*		buf;

	switch (op)
	{
	case SF_CLOSE:
		sfclose(d->sp);
		return(0);
	case SF_DPOP:
		free(d);
		return(0);
	case SF_READ:
		do
		{
			if (!(buf = sfgetr(d->sp, '\n', 0)) && !(buf = sfgetr(d->sp, '\n', -1)))
				return(0);
			n = sfslen();
			q = d->quote;
			(*d->line)++;
			if (n > 1 && buf[n - 2] == '\\')
			{
				n -= 2;
				if (q == '#')
				{
					n = 0;
					break;
				}
			}
			else if (q == '#')
			{
				q = 0;
				n = 0;
				break;
			}
			if (n > 0)
			{
				e = (b = buf) + n;
				while (b < e)
				{
					if ((c = *b++) == '\\') b++;
					else if (c == q) q = 0;
					else if (!q)
					{
						if (c == '\'' || c == '"') q = c;
						else if (c == '#' && (b == (buf + 1) || (c = *(b - 2)) == ' ' || c == '\t'))
						{
							if (buf[n - 1] != '\n')
							{
								q = '#';
								n = b - buf - 2;
							}
							else if (n = b - buf - 1) buf[n - 1] = '\n';
							break;
						}
					}
				}
				if (n <= 0) break;
				if (buf[n - 1] != '\n' && (s->flags & SF_STRING))
					buf[n++] = '\n';
				if (q && buf[n - 1] == '\n') buf[n - 1] = '\r';
			}
		} while (n <= 0);
		sfsetbuf(s, buf, n);
		d->quote = q;
		return(1);
	default:
		return(0);
	}
}

/*
 * open a stream to parse lines
 *
 *	flags: 0		arg: open Sfio_t* 
 *	flags: SF_READ		arg: file name
 *	flags: SF_STRING	arg: null terminated char*
 *
 * if line!=0 then it points to a line count that starts at 0
 * and is incremented for each input line
 */

Sfio_t*
tokline(const char* arg, int flags, int* line)
{
	Sfio_t*		f;
	Sfio_t*		s;
	SPLICE*		d;

	static int	hidden;

	if (!(d = newof(0, SPLICE, 1, 0))) return(0);
	if (!(s = sfopen(NiL, NiL, "s")))
	{
		free(d);
		return(0);
	}
	if (!(flags & (SF_STRING|SF_READ))) f = (Sfio_t*)arg;
	else if (!(f = sfopen(NiL, arg, (flags & SF_STRING) ? "s" : "r")))
	{
		free(d);
		sfclose(s);
		return(0);
	}
	d->disc.exceptf = splice;
	d->sp = f;
	*(d->line = line ? line : &hidden) = 0;
	sfdisc(s, (Sfdisc_t*)d);
	return(s);
}
