/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:form/fcustom.c	1.5.4.3"

#include <stdio.h>
#include <curses.h>
#include "wish.h"
#include	"token.h"
#include	"winp.h"
#include	"form.h"
#include "var_arrays.h"

form_id
form_custom(vid, flags, rows, cols, disp, ptr)
vt_id vid;
unsigned flags;
int rows, cols;
formfield (*disp)();
char *ptr;
{
	register int	num;
	register struct form	*f;

	/* find a free form structure */
	for (f = FORM_array, num = array_len(FORM_array); num > 0; f++, num--)
		if (!(f->flags & FORM_USED))
			break;
	if (num <= 0) {
		var_append(struct form, FORM_array, NULL);
		f = &FORM_array[array_len(FORM_array) - 1];
	}
	/* set up f */
	f->display = disp;
	f->argptr = ptr;
	f->flags = FORM_USED | FORM_DIRTY;
	f->vid = vid;
	f->curfldnum = 0;
	f->rows = rows;
	f->cols = cols;

	return(f - FORM_array);
}
