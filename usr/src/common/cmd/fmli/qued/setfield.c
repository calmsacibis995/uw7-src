/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/setfield.c	1.10.4.1"

#include <stdio.h>
#include <curses.h>
/* #include <malloc.h>  abs s19 */
#include "token.h"
#include "winp.h"
#include "fmacs.h"
#include "terror.h"
#include "attrs.h"
#include "wish.h"		/* abs s16 */

#define FSIZE(x)	(x->rows * (x->cols + 1))

setfieldflags(fld, flags)
register ifield *fld;
register int flags;
{
    fld->flags = (flags & I_CHANGEABLE) | (fld->flags & ~(I_CHANGEABLE));
    fld->fieldattr = (fld->flags & I_FILL ? Attr_underline: Attr_normal);
}

