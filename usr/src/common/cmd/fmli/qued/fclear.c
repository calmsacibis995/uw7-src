/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/fclear.c	1.3.4.4"

#include <stdio.h>
#include <curses.h>
#include "token.h"
#include "winp.h"
#include "fmacs.h"

/*
 * FCLEAR will clear the field from the current cursor position to
 * the end of the field
 */
fclear()
{
	register int row, col;
	register int saverow, savecol;

	saverow = Cfld->currow;
	savecol = Cfld->curcol;
	for (row = saverow, col = savecol; row <= LASTROW; row++, col = 0) {
		fgo(row, col);
		for (; col <= LASTCOL; col++)
			fputchar(' ');
		if (Cfld->flags & I_SCROLL )
			fputchar(' ');
	}
	fgo(saverow, savecol);
}

fclearline()
{
	register int col, savecol;

	savecol = Cfld->curcol;
	for (col = savecol; col <= LASTCOL; col++)
		fputchar(' ');
	fgo(Cfld->currow, savecol);
}
