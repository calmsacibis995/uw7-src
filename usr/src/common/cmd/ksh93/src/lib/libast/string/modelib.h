#ident	"@(#)ksh93:src/lib/libast/string/modelib.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * mode_t common definitions
 */

#ifndef _MODELIB_H
#define _MODELIB_H

#include <ast.h>
#include <ls.h>
#include <modex.h>

#define MODELEN	10
#define PERMLEN	24

#define modetab	_mode_table_	/* data hiding				*/
#define permmap	_mode_permmap_	/* data hiding				*/

struct modeop			/* ops for each char in mode string	*/
{
	int	mask1;		/* first mask				*/
	int	shift1;		/* first shift count			*/
	int	mask2;		/* second mask				*/
	int	shift2;		/* second shift count			*/
	char*	name;		/* mode char using mask/shift as index	*/
};

extern struct modeop	modetab[];
extern int		permmap[];

#endif
