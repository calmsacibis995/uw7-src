/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/putfield.c	1.10.4.6"

#include <stdio.h>
#include <string.h>
#include <curses.h>
#include "wish.h"
#include "token.h"
#include "winp.h"
#include "fmacs.h"
#include "moremacros.h"
#include "terror.h"
#define FSIZE(x)        (x->rows * (x->cols + 1))

extern 	char *fputstring();

putfield(fld, str)
ifield *fld;
char *str;
{
	ifield *savefield;
	chtype *sbuf_ptr;
	char *v_ptr;

	if (str == NULL)
		return;
	savefield = Cfld;
	if (fld != NULL)
		Cfld = fld;
	else if (!Cfld)			/* no current field */
		return;
	if (Flags & I_INVISIBLE) {
		strncpy(Value, str, FSIZE(Cfld) - 1);
		Cfld = savefield;
		return;
	}
	Flags |= I_CHANGED;
	fgo(0, 0);			/* home the cursor */

	/*
	 * Free remains of a previous field value
	 */
	if (Value)
		free(Value);
	if (Scrollbuf)
		free_scroll_buf(Cfld);	/* if used, reset scroll buffers */

	/*
	 * If Value is LESS than the visible field size
	 * then allocate at least the field size
	 * otherwise strsave the passed value.
	 */
	if ((int)strlen(str) < FIELDBYTES) {
		if ((Value = malloc(FIELDBYTES +1)) == NULL) /* +1 abs k15 */
			fatal(NOMEM, nil);
		strcpy(Value, str);
	}
	else
		Value = strsave(str);

	Valptr = fputstring(Value);	/* update pointer into value */
	fclear();			/* clear the rest of field */
	fgo(0, 0);			/* home the cursor */
	if ((Flags & I_SCROLL) && Currtype == SINGLE) {
		/*
		 * HORIZONTAL SCROLLING
		 * initialize scroll buffer and copy string to it
		 */
		unsigned vallength, maxlength;

		vallength = strlen(Value);
		maxlength = max(vallength, FIELDBYTES);	/* removed +1 abs k15 */
		growbuf(maxlength);
/*		strcpy(Scrollbuf, Value);  abs */
		/* THE following is >>> WRONG <<< it does not
		 * process  characters like tabs. it should be
		 * handled like vertical scroll fields.
		 */
		sbuf_ptr = Scrollbuf;
		v_ptr = Value;
		while (*v_ptr!= '\0')
			{
                                int col = 0;
                                int numspaces;
                                if (*v_ptr == '\\') {
                                        switch(*++v_ptr) {
                                        case 't':
                                                *v_ptr = '\t';
                                                break;
                                        }
                                }
                                switch(*v_ptr) {
                                case '\t':
                                        for (numspaces = ((col + 8) & ~7) -col; numspaces > 0; numspaces--, col++)
                                        *sbuf_ptr++ = ((chtype) *v_ptr++ ) | Fieldattr;
                                        break;
                                default:
                                        *sbuf_ptr++ = ((chtype) *v_ptr++) | Fieldattr;
                                        col++;
                                        break;
                                }
                        }
		free(Value);
		Valptr = Value = NULL;
	}
	setarrows();
	Cfld = savefield;
}
