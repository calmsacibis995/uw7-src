#ident	"@(#)ksh93:src/lib/libast/include/namval.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * common name-value struct support
 */

#ifndef _NAMVAL_H
#define _NAMVAL_H

typedef struct
{
	char*		name;
	int		value;
#ifdef _NAMVAL_PRIVATE_
	_NAMVAL_PRIVATE_
#endif
} Namval_t;

#endif
