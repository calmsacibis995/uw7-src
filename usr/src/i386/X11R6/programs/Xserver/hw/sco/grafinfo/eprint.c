/*
 *	@(#) eprint.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Wed Nov 04 10:59:29 PST 1992	buckm@sco.com
 *	- Created.
 */

/*
 * eprint.c - error printf function; just like ErrorF
 */

#include <stdio.h>

Eprint(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
	char *f;
	char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
{
	fprintf(stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
}
