#ident	"@(#)ksh93:src/lib/libast/string/fmtdev.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * idevice() formatter
 */

#include <ast.h>
#include <ctype.h>
#include <ls.h>

char*
fmtdev(struct stat* st)
{
	unsigned int	ma = major(idevice(st));
	unsigned int	mi = minor(idevice(st));

	static char	buf[18];

	if (ma == '#' && isalnum(mi))
	{
		/*
		 * Plan? Nein!
		 */

		buf[0] = ma;
		buf[1] = mi;
		buf[2] = 0;
	}
	else sfsprintf(buf, sizeof(buf), "%03d,%03d", ma, mi);
	return(buf);
}
