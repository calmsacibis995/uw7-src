#ident	"@(#)kern-i386:util/kdb/scodb/steps.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include	"sys/types.h"
#include	"sys/tss.h"
#include	"sys/reg.h"
#include	"dbg.h"
#include	"sent.h"
#include	"dis.h"
#include	"histedit.h"

extern int scodb_nregl;		/* number of register display lines */

NOTSTATIC
c_singstep(c, v)
	int c;
	char **v;
{
	extern int scodb_ssregs;

	if (c == 2) {
		if (!strcmp(v[1], "-r")) {	/* toggle ssregs */
			scodb_ssregs = !scodb_ssregs;
			return DB_CONTINUE;
		}
		else
			return DB_USAGE;
	}
	setspec(0);
	return DB_RETURN;
}

/*
*	print registers for single stepping
*/
NOTSTATIC
p_ssregs(col)
	int col;
{
	int i;
	extern int *REGP;
	extern int scodb_ssregs;

	if (scodb_ssregs == 0)
		return;
	/*
	*	dump the registers,
	*	and bring cursor back up to our prompt
	*/
	putchar('\n');	/* a blank line between dis and regs */
	putchar('\n');
	dpregs(REGP, 1);
	for (i = 0;i < scodb_nregl+1+1;i++)	/* 1 + 1blank */
		up();
	for (i = 0;i < col;i++)
		right();
}

/*
*	erase registers for single stepping
*/
NOTSTATIC
e_ssregs(col)
	register int col;
{
	register int i;
	extern int scodb_ssregs;

	if (scodb_ssregs == 0)
		return;

	/* erase registers
	*	registers were six lines
	*/
	down();
	down();	/* blank line */
	for (i = 0;i < scodb_nregl;i++)
		delline();
	up();	/* blank line */
	up();
}
