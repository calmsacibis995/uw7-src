#ident	"@(#)libthread:i386/lib/libthread/sys/hrestime.c	1.1.2.4"

#include <sys/time.h>
#include <libthread.h>


/*
 * int hrestime(timestruc_t *timestrucp)
 *
 *	Simulate hrestime system call.
 *
 * Calling/Exit state:
 *
 *	Called without holding any locks.
 */
int
hrestime(timestruc_t *timestrucp)
{
	struct timeval timeval;
	int rval;
	ASSERT(timestrucp != (timestruc_t *)NULL);

	if ((rval = gettimeofday(&timeval, (struct timezone *) NULL)) == 0) {
		timestrucp->tv_sec = timeval.tv_sec;
		timestrucp->tv_nsec = timeval.tv_usec * 1000;
	}
	return (rval);
}
