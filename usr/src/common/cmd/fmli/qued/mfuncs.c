/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/mfuncs.c	1.8.6.1"

#include <curses.h>
#include <string.h>
/*#include "curses.h"*/
#include "wish.h"
/* #include "vtdefs.h" */
/* #include "token.h" */
#include "fmacs.h"
#include "winp.h"
#include "vt.h"

fdelline(num)
int num;
{
	register int saverow, i;
	register struct vt *v = &VT_array[VT_curid];

	saverow = Cfld->currow;
	if (Cfld->flags & I_FULLWIN) {
		/*
		 * Use the subwindow to delete lines
		 */
		if (v->subwin) {
			wmove(v->subwin, saverow + Cfld->frow, Cfld->fcol);
			winsdelln(v->subwin, -num);
			wsyncup(v->subwin);
		}
		else
			winsdelln (v->win, -num);
	}
	else {
		/*
		 * only a partial window (scroll field)
		 * don't use a subwindow
		 */
		for (i = saverow; i <= LASTROW; i++) {
			if ((i + num) <= LASTROW)
				fcopyline(i + num, i);
			else {
				fgo(i, 0);
				fclearline();
			}
		}
	}
	fgo(saverow, 0);
}

finsline(num, after)
int num, after;
{
	register int saverow, start, i;
	register struct vt      *v = &VT_array[VT_curid];
	WINDOW *subwin;

	start = saverow = Cfld->currow;
	if (after == TRUE)
		start++;
	fgo(start, 0);
	if (Cfld->flags & I_FULLWIN) {	
		if (v->subwin) {
			wmove(v->subwin, start + Cfld->frow, Cfld->fcol);
			winsdelln(v->subwin, num);
			wsyncup(v->subwin);
		}
		else
			winsdelln(v->win, num);
	}
	else {
		/*
		 * only a partial window (scroll field)
		 * don't use a subwindow
		 */
		for (i = LASTROW; i >= start; i--) {
		  	if ((i - num) >= start)
				fcopyline(i - num, i);
			else {
				fgo(i, 0);
				fclearline();
			}
		}
	}
	fgo(start, 0);
}

#define STR_SIZE	256

static
fcopyline(src, dest)
int src, dest;
{
	register struct vt      *v = &VT_array[VT_curid];
	register int len=0;
	chtype ch_string[STR_SIZE];

	/*
	 * Call winchnstr() to get a line and
	 * waddchnstr() to copy it to dest
	 */
	fgo (src, 0);
	winchnstr(v->win, ch_string, LASTCOL + 1);
	while ((len<(LASTCOL+1)) && (ch_string[len] != 0))
		len++;
	fgo (dest, 0);
	waddchnstr(v->win, ch_string, len);
}
