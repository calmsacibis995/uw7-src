#ident	"@(#)OSRcmds:lib/misc/f_gethfdo.c	1.1"
#include	<sys/time.h>
#include	<sys/resource.h>
#include	<sys/types.h>
#include	<sys/fcntl.h>
#include	<unistd.h>

int f_gethfdo ()
{
	struct rlimit	rlim;
	int		max;
	int		fstatfl;

	(void) getrlimit (RLIMIT_NOFILE, &rlim);

	for (max = (int) rlim.rlim_cur - 1; max >= 0; max--) {
		fstatfl = fcntl (max, F_GETFL);
		if (fstatfl != -1) break;
	}
	return(max++);
}
