#ident	"@(#)kern-i386at:io/kdvm/kdvm.c	1.8.2.1"
#ident	"$Header$"


#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <io/uio.h>
#include <io/kd/kd.h>
#include <io/ws/ws.h>
#include <proc/cred.h>
#include <io/ddi.h>
#include <io/conf.h>


int kdvm_devflag = 0;			/* See comments in kdstr.c */


/*
 * int
 * kdvm_open(dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
kdvm_open(dev_t *devp, int flag, int otyp, cred_t *cr)
{
	int	dev;
	int	error;

	if ((error = ws_getvtdev((dev_t *)&dev)) != 0)
		return (error);

	return (ws_open(dev, flag, otyp, cr));
}


/*
 * int
 * kdvm_close(dev_t, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
kdvm_close(dev_t dev, int flag, int otyp, cred_t *cr)
{
	int	error;

	if ((error = ws_getvtdev((dev_t *)&dev)) != 0)
		return (error);

	return (ws_close(dev, flag, otyp, cr));
}


/*
 * int
 * kdvm_ioctl(dev_t, int, int, int, cred_t *, int *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
kdvm_ioctl(dev_t dev, int cmd, int arg, int mode, cred_t *cr, int *rvalp)
{
	int	error;

	if ((error = ws_getvtdev(&dev)) != 0)
		return (error);

	return (ws_ioctl(dev, cmd, arg, mode, cr, rvalp));
}
