#ident	"@(#)ksh93:src/lib/libast/misc/procclose.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * close a proc opened by procopen()
 * -1 returned if procopen() had a problem
 * otherwise exit() status of process is returned
 */

#include "proclib.h"

#include <wait.h>

int
procclose(register Proc_t* p)
{
	int	status = -1;

	if (p)
	{
		if (p->rfd >= 0) close(p->rfd);
		if (p->wfd >= 0 && p->wfd != p->rfd) close(p->wfd);
		sigcritical(1);
		while (waitpid(p->pid, &status, 0) == -1 && errno == EINTR);
		sigcritical(0);
		procfree(p);
	}
	return(status);
}
