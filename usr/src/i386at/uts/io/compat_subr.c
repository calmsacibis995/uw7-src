#ident	"@(#)kern-i386at:io/compat_subr.c	1.2"

/*
 * misc routines required for compat with SVR4 i386AT drivers
 *
 */

#include <util/param.h>
#include <fs/vfs.h>

void fshadbad(dev_t, daddr_t);


/* ARGSUSED1 */
/*
 * void
 * fshadbad(dev_t dev, daddr_t bno)
 *	called from hdb_mapbad to mark fs dirty
 *
 * Calling/Exit State:
 *	called from UP driver, bound to cpu
 *	sets VFS_BADBLOCK
 */
void
fshadbad(dev_t dev, daddr_t bno)
{
 	register struct vfs *vfsp;

        if ((vfsp = vfs_devsearch(dev)) != (struct vfs *)NULL)
		vfsp->vfs_flag |= VFS_BADBLOCK;
}
