#ident	"@(#)ksh93:src/lib/libast/misc/fastfind.c	1.1"
#pragma prototyped
/*
 * fast find algorithm (snarfed from bsd find) by
 *
 *  	James A. Woods, Informatics General Corporation,
 *	NASA Ames Research Center, 6/81.
 *	Usenix ;login:, February/March, 1983, p. 8.
 *
 * open/next/close interface and strmatch() additions by
 *
 *	Glenn Fowler
 *	AT&T Bell Laboratories
 *
 * searches a pre-computed path list (constructed nightly via cron)
 *
 * 'fastfind' scans a file list for the full pathname of a file
 * given only a piece of the name.  The list has been processed with
 * with "front-compression" and bigram coding.  Front compression reduces
 * space by a factor of 4-5, bigram coding by a further 20-25%.
 * The codes are:
 *
 *	0-28	likeliest differential counts + offset to make nonnegative 
 *	30	escape code for out-of-range count to follow in next word
 *	128-255 bigram codes, (128 most common, as determined by 'updatedb')
 *	32-127  single character (printable) ascii residue
 *
 * A novel two-tiered string search technique is employed: 
 *
 * First, a metacharacter-free subpattern and partial pathname is
 * matched BACKWARDS to avoid full expansion of the pathname list.
 * The time savings is 40-50% over forward matching, which cannot efficiently
 * handle overlapped search patterns and compressed path residue.
 *
 * Then, the actual shell glob-style regular expression (if in this form)
 * is matched against the candidate pathnames using the slower strmatch().
 */

#include <ast.h>

#define	OFFSET		14
#define	ESCCODE		30

typedef struct
{
	Sfio_t*	fp;
	char*	end;
	int	count;
	int	peek;
	short	found;
	short	match;
	char	bigram1[(1<<(CHAR_BIT-1))];
	char	bigram2[(1<<(CHAR_BIT-1))];
	char	path[PATH_MAX];
	char	pattern[1];
} Find_t;

/*
 * return a fastfind stream handle for pattern
 */

void*
findopen(const char* pattern)	
{
	register Find_t*	fp;
	register char*		p;
	register char*		s;
	register char*		b;
	register int		i; 
	int			brace = 0;
	int			paren = 0;

	static char*		codes[] =
	{
		0,
		"/usr/local/share/lib/find/find.codes",
		"/usr/lib/find/find.codes"
	};

	if (!(fp = newof(0, Find_t, 1, 2 * (strlen(pattern) + 1)))) return(0);
	if (!codes[0]) codes[0] = getenv("FINDCODES");
	for (i = 0; i < elementsof(codes); i++)
		if (codes[i] && (fp->fp = sfopen(NiL, codes[i], "r")))
			break;
	if (!fp->fp)
	{
		free(fp);
		return(0);
	}
	for (i = 0; i < elementsof(fp->bigram1); i++) 
	{
		fp->bigram1[i] = sfgetc(fp->fp);
		fp->bigram2[i] = sfgetc(fp->fp);
	}

	/*
	 * extract last glob-free subpattern in name for fast pre-match
	 * prepend 0 for backwards match
	 */

	b = fp->pattern;
	p = s = (char*)pattern;
	for (;;)
	{
		switch (*b++ = *p++)
		{
		case 0:
			break;
		case '\\':
			s = p;
			if (!*p++) break;
			continue;
		case '[':
			if (!brace)
			{
				brace++;
				if (*p == ']') p++;
			}
			continue;
		case ']':
			if (brace)
			{
				brace--;
				s = p;
			}
			continue;
		case '(':
			if (!brace) paren++;
			continue;
		case ')':
			if (!brace && paren > 0 && !--paren) s = p;
			continue;
		case '|':
		case '&':
			if (!brace && !paren)
			{
				s = "";
				break;
			}
			continue;
		case '*':
		case '?':
			s = p;
			continue;
		default:
			continue;
		}
		break;
	}
	if (s != pattern) fp->match = 1;
	*b++ = 0;
	if (!*s) *b++ = '/';
	else while (i = *s++) *b++ = i;
	*b-- = 0;
	fp->end = b;
	fp->peek = sfgetc(fp->fp);
	return((void*)fp);
}

/*
 * close an open fastfind stream
 */

void
findclose(void* handle)
{
	register Find_t*	fp = (Find_t*)handle;

	sfclose(fp->fp);
	free(fp);
}

/*
 * return the next fastfind path
 * 0 returned when list exhausted
 */

char*
findnext(void* handle)
{
	register Find_t*	fp = (Find_t*)handle;
	register char*		p;
	register char*		q;
	register char*		s;
	register char*		b;
	register int		c;
	int			n;

	for (c = fp->peek; c != EOF;)
	{
		if (c == ESCCODE)
		{
			if (sfread(fp->fp, (char*)&n, sizeof(n)) != sizeof(n)) break;
			c = n;
		}
		fp->count += c - OFFSET;
		for (p = fp->path + fp->count; (c = sfgetc(fp->fp)) > ESCCODE;)
			if (c & (1<<(CHAR_BIT-1)))
			{
				*p++ = fp->bigram1[c & ((1<<(CHAR_BIT-1))-1)];
				*p++ = fp->bigram2[c & ((1<<(CHAR_BIT-1))-1)];
			}
			else *p++ = c;
		*p-- = 0;
		b = fp->path;
		if (fp->found) fp->found = 0;
		else b += fp->count;
		for (s = p; s >= b; s--) 
			if (*s == *fp->end)
			{
				for (p = fp->end - 1, q = s - 1; *p; p--, q--)
					if (*q != *p)
						break;
				if (!*p)
				{
					fp->found = 1;
					if (!fp->match || strmatch(fp->path, fp->pattern))
					{
						fp->peek = c;
						return(fp->path);
					}
					break;
				}
			}
	}
	fp->peek = c;
	return(0);
}
