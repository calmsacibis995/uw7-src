/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/is_objtype.c	1.3.3.3"

#include <stdio.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "terror.h"
#include "typetab.h"
#include "detabdefs.h"

extern struct odft_entry Detab[MAXODFT];

bool
is_objtype(obj)
char *obj;
{
	register int i;

	for (i = 0; Detab[i].objtype[0]; i++)
		if (strCcmp(obj, Detab[i].objtype) == 0)
			return(TRUE);
	return(FALSE);
}
