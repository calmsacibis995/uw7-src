/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:inc/slk.h	1.3.4.3"

struct slk {
	char *label;
	char *msgid;	/* holds the nls message number of this label */
	token tok;
	char *tokstr;
	char *intr;
	char *onintr;
};

#define MAX_SLK	16
