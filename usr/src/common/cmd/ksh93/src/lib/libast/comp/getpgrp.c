#ident	"@(#)ksh93:src/lib/libast/comp/getpgrp.c	1.1"
#pragma prototyped

/*
 * bsd		int getpgrp(int);
 * s5		int getpgrp(void);
 * posix	pid_t getpgrp(void);
 * user		SOL
 */

extern int	getpgrp(int);

int
_ast_getpgrp(void)
{
	return(getpgrp(0));
}
