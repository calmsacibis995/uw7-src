#ident	"@(#)libc-i386:sys/lsemainit.c	1.1"
/*
 * LWP semaphore initialization
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/usync.h>
#include <sys/errno.h>

/*
 * int
 * _lwp_sema_init(_lwp_sema_t *semap, int count)
 *
 *	Initialize a semaphore.
 *
 * Calling/Exit State:
 *
 *	No locking required on entry, no locks held on exit.
 *	Returns 0 on success, EINVAL if the count is negative.
 *
 * Description:
 *
 *	This function initializes the semaphore to the supplied count
 *	value. Sanity checking is performed on the count value to
 *	ensure that negative value is not supplied. No checking is
 *	is performed on the validity of the semaphore address. 
 */
int
_lwp_sema_init(_lwp_sema_t *semap, int count)
{
	if (count < 0)  /* invalid count */
		return (EINVAL);
	semap->count = count;
	return (0);
}
