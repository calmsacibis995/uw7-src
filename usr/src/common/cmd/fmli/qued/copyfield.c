/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/copyfield.c	1.12.3.3"

#include <stdio.h>
#include <memory.h>
#include <curses.h>
#include "wish.h"
#include "token.h"
#include "winp.h"
#include "fmacs.h"
#include "moremacros.h"
#include "terror.h"

/*
 * HIDEFIELD will remove the field from screen WITHOUT destroying the
 * ifield structure.
 */
hidefield(fld)
ifield *fld;
{
	ifield *savefield;
	int flags;

	savefield = Cfld;
	if (fld != NULL)
		Cfld = fld;
	flags = fld->flags;
	setfieldflags(fld, (fld->flags & ~I_FILL));
	fgo(0, 0);
	fclear();
	setfieldflags(fld, flags);
	Cfld = savefield;
}
