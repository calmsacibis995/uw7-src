#ident	"@(#)ksh93:src/lib/libast/string/stropt.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 */

#include <ast.h>
#include <ctype.h>
#include <hash.h>

/*
 * parse option expression in s using options in tab with element size siz
 * siz==0 implies Hash_table_t*tab
 * options match
 *
 *	[no]name[[:]=['"]value["']][, ]...
 *
 * f is called for each option
 *
 *	(*f)(void* a, char* p, int n, char* v)
 *
 *	a	from stropt
 *	p	matching tab entry
 *	n	0 if option had ``no'' prefix, -1 if :=, 1 otherwise
 *	v	option value pointer
 *
 * for unmatched options p is 0 and v is the offending option
 *
 * names in s may be shorter than tab names
 * longer names must have a prefix that matches a tab name
 * the first match is returned
 * \ escapes value using chresc()
 */

int
stropt(const char* as, const void* tab, int siz, int(*f)(void*, const void*, int, const char*), void* a)
{
	register int	c;
	register char*	s;
	register char*	v;
	register char*	t;
	char**		p;
	char*		u;
	char*		x;
	char*		e;
	int		n;
	int		q;

	if (!as) n = 0;
	else if (!(x = s = strdup(as))) n = -1;
	else
	{
		for (;;)
		{
			while (isspace(*s) || *s == ',') s++;
			if (*s == 'n' && *(s + 1) == 'o')
			{
				s += 2;
				n = 0;
			}
			else n = 1;
			if (!*s)
			{
				n = 0;
				break;
			}
			if (!siz)
			{
				v = s;
				while ((c = *v++) && !isspace(c) && c != '=' && c != ':' && c != ',');
				*--v = 0;
				t = (p = (char**)hashget((Hash_table_t*)tab, s)) ? s : (char*)0;
				if ((*v = c) == ':' && *(v + 1) == '=')
				{
					v++;
					n = -1;
				}
			}
			else for (p = (char**)tab; t = *p; p = (char**)((char*)p + siz))
			{
				for (v = s; *t && *t++ == *v; v++);
				if (!*t || isspace(*v) || *v == ',' || *v == '=') break;
				if (*v == ':' && *(v + 1) == '=')
				{
					v++;
					n = -1;
					break;
				}
			}
			if (!t)
			{
				u = v = s;
				p = 0;
			}
			while (*v && !isspace(*v) && *v != '=' && *v != ',')
				if (*v++ == ':' && *v == '=')
				{
					if (!t) *(v - 1) = 0;
					n = -n;
					break;
				}
			if (*v == '=')
			{
				if (!t) *v = 0;
				t = s = ++v;
				q = 0;
				for (;;)
				{
					switch (c = *s++)
					{
					case 0:
						break;
					case '\\':
						*t++ = chresc(s - 1, &e);
						s = e;
						continue;
					case '"':
					case '\'':
						if (!q) q = c;
						else if (c == q) q = 0;
						else *t++ = c;
						continue;
					default:
						if (!q && (c == ',' || isspace(c))) break;
						*t++ = c;
						continue;
					}
					break;
				}
				*t = 0;
			}
			else
			{
				s = v;
				c = *s;
				*s++ = 0;
			}
			n = (*f)(a, p, n, p ? v : u);
			if (n || !c) break;
		}
		free(x);
	}
	return(n);
}
