/*		copyright	"%c%" 	*/

#ident	"@(#)f_head.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "sys/types.h"

#include "lp.h"
#include "form.h"

struct {
	char			*v;
	short			len;
	short			infile;
}			formheadings[FO_MAX] = {

#define	ENTRY(X)	X, sizeof(X)-1

	ENTRY("page length:"),		1,	/* FO_PLEN */
	ENTRY("page width:"),		1,	/* FO_PWID */
	ENTRY("number of pages:"),	1,	/* FO_NP */
	ENTRY("line pitch:"),		1,	/* FO_LPI */
	ENTRY("character pitch:"),	1,	/* FO_CPI */
	ENTRY("character set choice:"),	1,	/* FO_CHSET */
	ENTRY("ribbon color:"),		1,	/* FO_RCOLOR */
	ENTRY("comment:"),		0,	/* FO_CMT */
	ENTRY("alignment pattern:"),	1,	/* FO_ALIGN */

#undef	ENTRY

};

/**
 ** _search_fheading()
 **/

int
#if	defined(__STDC__)
_search_fheading (
	char *			buf
)
#else
_search_fheading (buf)
	char *			buf;
#endif
{
	int			fld;


	for (fld = 0; fld < FO_MAX; fld++)
		if (
			formheadings[fld].v
		     && formheadings[fld].len
		     && CS_STRNEQU(
				buf,
				formheadings[fld].v,
				formheadings[fld].len
			)
		)
			break;

	return (fld);
}
