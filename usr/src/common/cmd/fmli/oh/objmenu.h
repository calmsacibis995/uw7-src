/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/objmenu.h	1.2.4.3"

typedef struct {
	int flags;
	int numactive;		/* number of inactive menu items */
	struct fm_mn fm_mn;
	int *visible;
	int *slks;
	char **args;
} menuinfo;
