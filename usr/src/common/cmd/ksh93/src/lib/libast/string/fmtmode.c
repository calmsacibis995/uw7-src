#ident	"@(#)ksh93:src/lib/libast/string/fmtmode.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return ls -l style file mode string given file mode bits
 * if external!=0 then mode is modex canonical
 */

#include "modelib.h"

char*
fmtmode(register int mode, int external)
{
	register char*		s;
	register struct modeop*	p;

	static char		buf[MODELEN + 1];

	if (!external) mode = modex(mode);
	s = buf;
	for (p = modetab; p < &modetab[MODELEN]; p++)
		*s++ = p->name[((mode & p->mask1) >> p->shift1) | ((mode & p->mask2) >> p->shift2)];
	return(buf);
}
