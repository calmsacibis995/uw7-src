/*		copyright	"%c%" 	*/

#ident	"@(#)dashos.c	1.2"
#ident	"$Header$"

#include "string.h"

#include "lp.h"

#define issep(X)	strchr(LP_WS, X)

/**
 ** dashos() - PARSE -o OPTIONS, (char *) --> (char **)
 **/

char **
#if	defined(__STDC__)
dashos (
	char *			o
)
#else
dashos (o)
	register char		*o;
#endif
{
	register char		quote,
				c,
				*option;

	char			**list	= 0;


	if (!o)
		return	(char **) 0;

	while (*o)
	{

		while (*o && issep(*o))
			o++;

		for (option = o; *o && !issep(*o); o++)
			if (strchr(LP_QUOTES, (quote = *o)))
				for (o++; *o && *o != quote; o++)
					if (*o == '\\' && o[1])
						o++;

		if (option < o) {
			c = *o;
			*o = 0;
			(void)addlist (&list, option);
			*o = c;
		}

	}
	return (list);
}
