/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 *
 */
#ident	"@(#)fmli:inc/menudefs.h	1.3.4.3"

#define MENU_UNDEFINED	(-1)
#define MENU_MRK	(1)
#define MENU_INACT	(2)

struct menu_line {
	char	*highlight;
	char	*lininfo;
	char	*description;
	short	flags;
};
