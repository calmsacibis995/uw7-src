/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:menu/mclose.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"menu.h"
#include	"var_arrays.h"

void
menu_close(mid)
menu_id	mid;
{
	register struct menu	*m;

	if (MNU_curid == mid)
		MNU_curid = -1;
	_menu_cleanup();
	m = &MNU_array[mid];
	vt_close(m->vid);
	m->flags = 0;
}
