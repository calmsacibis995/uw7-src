#ident	"@(#)OSRcmds:ksh/include/col_value.h	1.1"
#pragma comment(exestr, "@(#) col_value.h 25.1 93/01/20 ")
/*
 *      Copyright (C) 1992-1993 The Santa Cruz Operation, Inc.
 *              All rights reserved.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification History
 * created	scol!markhe	30 Dec 92
 * 	- used by strmatch.c for bracket expressions, copied from the devsys
 */

#ifndef _COL_VALUE_H
#define _COL_VALUE_H

extern short	_col_value (const char *);
extern short	_next_equiv (const char *, int *);
extern short	_longest_col_value (const char **);
extern char *	_col_to_element (char *, short);

#endif /* _COL_VALUE_H */
