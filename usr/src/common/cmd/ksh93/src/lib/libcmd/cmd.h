#ident	"@(#)ksh93:src/lib/libcmd/cmd.h	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * builtin cmd definitions
 */

#ifndef _CMD_H
#define _CMD_H

#include <ast.h>
#include <error.h>
#include <stak.h>

#ifdef	STANDALONE

/*
 * command initialization
 */

static void
cmdinit(register char** argv)
{
	register char*	cp;

	if (cp = strrchr(argv[0], '/')) cp++;
	else cp = argv[0];
	error_info.id = cp;
}

#else

extern int		cmdrecurse(int, char**, int, char**);
extern void		cmdinit(char**);

#endif

#endif
