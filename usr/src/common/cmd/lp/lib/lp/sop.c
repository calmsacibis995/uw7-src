/*		copyright	"%c%" 	*/

#ident	"@(#)sop.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** sop_up_rest() - READ REST OF FILE INTO STRING
 **/

char *
#if	defined(__STDC__)
sop_up_rest (
	FILE *			fp,
	char *			endsop
)
#else
sop_up_rest (fp, endsop)
	FILE			*fp;
	register char		*endsop;
#endif
{
	register int		size,
				add_size,
				lenendsop;

	register char		*str;

	char			buf[BUFSIZ];


	str = 0;
	size = 0;
	if (endsop)
		lenendsop = strlen(endsop);

	while (fgets(buf, BUFSIZ, fp)) {
		if (endsop && STRNEQU(endsop, buf, lenendsop))
			break;
		add_size = strlen(buf);
		if (str)
			str = Realloc(str, size + add_size + 1);
		else
			str = Malloc(size + add_size + 1);
		if (!str) {
			errno = ENOMEM;
			return (0);
		}
		(void)strcpy (str + size, buf);
		size += add_size;
	}
	if (ferror(fp)) {
		Free (str);
		return (0);
	}
	return (str);
}
