#ident	"@(#)ksh93:src/lib/libast/include/swap.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * integral representation conversion support definitions
 * supports sizeof(integral_type)<=sizeof(int_max)
 */

#ifndef _SWAP_H
#define _SWAP_H

#include <int.h>

#define SWAP_MAX	8

extern void*		swapmem(int, const void*, void*, size_t);
extern int_max		swapget(int, const void*, int);
extern void*		swapput(int, void*, int, int_max);
extern int		swapop(const void*, const void*, int);

#endif
