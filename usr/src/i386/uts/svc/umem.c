#ident	"@(#)kern-i386:svc/umem.c	1.1.2.1"
#ident	"$Header$"

/* umem.c - User memory allocation */

/* Enhanced Application Compatibility Support */

#include <proc/user.h>
#include <proc/proc.h>
#include <mem/as.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>

/*
 * void *umem_alloc(size_t size)
 *
 * 	Allocate memory in user space for use by routines in "os/sco.c" and
 * 	"os/isc.c" (or anyone who needs it).  There is an assumption that
 * 	any memory allocated will be freed before the return to user context.
 *	This will keep users from seeing anomalies appear in their address
 *	space if a new segment has to be used.
 *
 * Calling/Exit State:
 *	as_wrlock cannot be held when calling this function.
 *
 */
void *
umem_alloc(size_t size)
{
	void		*addr;
	struct as	*asp = u.u_procp->p_as;

	as_wrlock(asp);
	map_addr((vaddr_t *)&addr, size, 0, 1);
	if(addr != NULL &&
	   as_map(asp, (vaddr_t)addr, size,segvn_create,zfod_argsp))
		addr = NULL;	/* as_map() failed */
	as_unlock(asp);
	return(addr);
}

/*
 * void umem_free(void *addr, size_t size)
 * 	Free space allocated by umem_alloc().  
 *	`size' should be the size requested of umem_alloc().
 *
 * Calling/Exit State:
 *	No locking assumption.
 */
void
umem_free(void *addr, size_t size)
{
	struct as	*asp = u.u_procp->p_as;
	if (addr != NULL) {
		as_wrlock(asp);
		(void)as_unmap(asp, (vaddr_t)addr, size);
		as_unlock(asp);
	}
}

/* End Enhanced Application Compatibility Support */
