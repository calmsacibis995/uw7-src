#ident	"@(#)call_udel.c	1.4"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>

extern	pid_t	fork(),
		wait();

extern	int	execvp();

extern	void	exit();

/*
 * Procedure:	call_udelete
 *
 * Restrictions:
 *		freopen:	None
 *		execl:		None
*/

int
call_udelete(nargv)
	char	*nargv;
{
	int	ret;
	char	*cmd = "/usr/sbin/userdel",
		*cmd_opt = "-r -n 0";	/* don't add uid to ageduid file */

	switch (fork()) {
	case 0:
		/* CHILD */
		if (freopen("/dev/null", "w+", stdout) == NULL
			|| freopen("/dev/null", "w+", stderr) == NULL
			|| execl(cmd, cmd, cmd_opt, nargv, (char *) NULL) == -1)
			exit(EX_FAILURE);
		break;
	case -1:
		/* ERROR */
		return EX_FAILURE;
	default:
		/* PARENT */	
		if (wait(&ret) == -1)
			return EX_FAILURE;
		ret = (ret >> 8) & 0xff;
	}
	return ret;
		
}
