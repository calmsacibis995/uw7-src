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
#ident	"@(#)fmli:sys/scrclean.c	1.2.3.4"

#include <stdio.h>
#include <curses.h>		/* abs s14 */

extern int Refresh_slks;

screen_clean()
{
	Refresh_slks = 0;
	slk_clear();
	vt_close_all();
	copyright();
	vt_flush();
	endwin();
	putchar('\n');
}
