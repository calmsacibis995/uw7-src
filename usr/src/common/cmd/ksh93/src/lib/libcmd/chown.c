#ident	"@(#)ksh93:src/lib/libcmd/chown.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * chown
 */

static const char id[] = "\n@(#)chown (AT&T Bell Laboratories) 06/22/93\0\n";

#include <cmdlib.h>

extern int	b_chgrp(int, char**);

int
b_chown(int argc, char *argv[])
{
	NoP(id[0]);
	return(b_chgrp(argc, argv));
}
