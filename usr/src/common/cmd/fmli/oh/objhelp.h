/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/objhelp.h	1.2.4.3"

typedef struct {
	int    flags;
	struct fm_mn fm_mn;
	char **holdptrs;
	int   *slks;
	char **args;
} helpinfo;
