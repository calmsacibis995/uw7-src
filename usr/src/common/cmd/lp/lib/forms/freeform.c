/*		copyright	"%c%" 	*/

#ident	"@(#)freeform.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "sys/types.h"
#include "stdlib.h"

#include "lp.h"
#include "form.h"

/**
 **  freeform() - FREE MEMORY ALLOCATED FOR FORM STRUCTURE
 **/

void
#if	defined(__STDC__)
freeform (
	FORM *			pf
)
#else
freeform (pf)
	FORM *			pf;
#endif
{
	if (!pf)
		return;
	if (pf->chset)
		Free (pf->chset);
	if (pf->rcolor)
		Free (pf->rcolor);
	if (pf->comment)
		Free (pf->comment);
	if (pf->conttype)
		Free (pf->conttype);
	if (pf->name)
		Free (pf->name);
	pf->name = 0;

	return;
}
