/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/fgo.c	1.2.4.3"

#include <stdio.h>
#include <curses.h>
#include "token.h"
#include "winp.h"

fgo(row, col)
int row;
int col;
{
	Cfld->currow = row;
	Cfld->curcol = col;
	wgo(row + Cfld->frow, col + Cfld->fcol);
}
