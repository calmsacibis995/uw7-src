#ident	"@(#)genvid.c	1.7"
#ident	"$Header$"

/*
 * Indirect driver for /dev/video.
 */

#include <util/types.h>
#include <util/param.h>
#include <mem/immu.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <io/conf.h>
#include <mem/kmem.h>
#include <io/stream.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <fs/vnode.h>
#include <io/gvid/genvid.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <fs/specfs/specfs.h>

#include <io/ddi.h>	/* must come last */


#define		GVIDHIER	1


STATIC int	gviddev(dev_t, dev_t *);

int		gvidinit(void);
int		gvidopen(dev_t *, int, int, cred_t *);
int		gvidclose(dev_t, int, int, cred_t *);
int		gvidread(dev_t, struct uio *, cred_t *);
int		gvidwrite(dev_t, struct uio *, cred_t *);
int		gvidioctl(dev_t, int, int, int, cred_t *, int *);


/*
 * Global variables.
 */
int	gviddevflag = 0;		/* See comments in kdstr.c */

gvid_t	Gvid = {0};
int	gvidflg = 0;

lock_t	*gvid_mutex;
sv_t	*gvidsv;

LKINFO_DECL(gvid_mutex_lkinfo, "GVID::gvid_mutex", 0);


/*
 * STATIC int
 * gviddev(dev_t, dev_t *)
 *
 * Calling/Exit State:
 *	- gvid_mutex lock is held on entry/exit.
 *	- Return EINVAL if its an illegal major/minor number, otherwise
 *	  if devp references a NODEV then return ENODEV, else
 *	  return 0.
 */
STATIC int
gviddev(dev_t ttydev, dev_t *devp)
{
	major_t majnum;
	minor_t minnum;


	majnum = getmajor(ttydev);
	if (majnum != Gvid.gvid_maj)
		return (EINVAL);

	minnum = getminor(ttydev);
	if (minnum >= Gvid.gvid_num)
		return (EINVAL);

	*devp = *(Gvid.gvid_buf + minnum);
	if (*devp == NODEV) 
		return (ENODEV);

	return (0);
}

/*
 * int
 * gvidinit(void)
 *
 * Calling/Exit State:
 *	- Return 0, if the locks were successfully allocated and initialized.
 */
int
gvidinit(void)
{
	gvid_mutex = LOCK_ALLOC(GVIDHIER, plhi, &gvid_mutex_lkinfo, KM_NOSLEEP);
	gvidsv = SV_ALLOC(KM_NOSLEEP);
	
	if (!gvid_mutex || !gvidsv)
		/*
		 *+ Their isn't enough memory available to allocate
		 *+ space for gvid mutex basic lock.
		 */
		cmn_err(CE_PANIC, 
			"Not enough memory available for lock allocation");

	return (0);
}


/*
 * int
 * gvidopen(dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	- Return error number on failure, otherwise return 0.
 */
/* ARGSUSED */
int
gvidopen(dev_t *devp, int flag, int otyp, cred_t *cr)
{
	dev_t	gdev, cttydev;
	minor_t	gen_minor;
	int	error;
	pl_t	pl;


	gen_minor = getminor(*devp);

	if (gen_minor == 1)
		return (0);	/* success if administrative open */

	if (error = ws_getctty(&cttydev)) 
		return (error);

	/*
	 * Enforce mutual exclusion 
	 */
	pl = LOCK(gvid_mutex, plhi);

	if (!(gvidflg & GVID_SET)) {
		UNLOCK(gvid_mutex, pl);
		return (EBUSY);	/* fail opens until table is loaded */
	}

	while (gvidflg & GVID_ACCESS) {
		if (!SV_WAIT_SIG(gvidsv, primed - 3, gvid_mutex))
			return (EINTR);
		pl = LOCK(gvid_mutex, plhi);
	}

	gvidflg |= GVID_ACCESS;

	if (error = gviddev(cttydev, &gdev)) {
		gvidflg &= ~GVID_ACCESS;
		SV_SIGNAL(gvidsv, 0);
		UNLOCK(gvid_mutex, pl);
		return error;
	}
	
	/*
	 * Release access to the flag -- we could sleep in open.
	 */
	gvidflg &= ~GVID_ACCESS;
	SV_SIGNAL(gvidsv, 0);
	UNLOCK(gvid_mutex, pl);

	error = drv_devopen(&gdev, flag, VCHR, B_TRUE, cr);

	if (!error) 
		*devp = gdev;		/* clone!! */

	return error;
}


/*
 * int
 * gvidclose(dev_t, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
gvidclose(dev_t dev, int flag, int otyp, cred_t *cr)
{
	return 0;
}


/*
 * int
 * gvidread(dev_t, struct uio *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
gvidread(dev_t dev, struct uio *uiop, cred_t *cr)
{
	return (ENXIO);
}


/*
 * int
 * gvidwrite(dev_t, struct uio *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
gvidwrite(dev_t dev, struct uio *uiop, cred_t *cr)
{
	return (ENXIO);
}


/*
 * int
 * gvidioctl(dev_t, int, int, int, cred_t *, int *)
 * 
 * Calling/Exit State:
 *	- Return 0 on success, otherwise return the following error number:
 *		- EINVAL, if its an illegal ioctl
 *		- ENOMEM, if could not allocate memory
 *		- EFAULT, if copyin/copyout is unsuccessful
 * 
 * Description:
 *	The only way we can get in here is when /dev/vidadm is opened.
 *	Ioctls on /dev/video go to the underlying video driver.
 */
/* ARGSUSED */
int
gvidioctl(dev_t dev, int cmd, int arg, int mode, cred_t *crp, int *rvalp)
{
	pl_t	pl;


	switch (cmd) {

	case GVID_SETTABLE: {
		gvid_t	tmpmap;
		dev_t	*devbufp, *tmpbuf;
		int	tmpnum;

		/*
		 * Protect against stream head access. 
		 */
		pl = LOCK(gvid_mutex, plhi);

		while (gvidflg & GVID_ACCESS) {
			if (!SV_WAIT_SIG(gvidsv, primed - 3, gvid_mutex))
				return (EINTR);
			pl = LOCK(gvid_mutex, plhi);
		}	

		gvidflg |= GVID_ACCESS;
		UNLOCK(gvid_mutex, pl);

		if (copyin((caddr_t) arg, (caddr_t) &tmpmap, 
				sizeof(gvid_t)) == -1) {
#ifdef DEBUG
			cmn_err(CE_WARN, 
				"gvid: could not copy to tmpmap");
#endif /* DEBUG */
			pl = LOCK(gvid_mutex, plhi);
			gvidflg &= ~GVID_ACCESS;
			SV_SIGNAL(gvidsv, 0);
			UNLOCK(gvid_mutex, pl);
			return (EFAULT);
		}

		devbufp = (dev_t *)kmem_alloc(
				tmpmap.gvid_num * sizeof(dev_t), KM_SLEEP);

		if (devbufp == (dev_t *) NULL) {
			pl = LOCK(gvid_mutex, plhi);
			gvidflg &= ~GVID_ACCESS;
			SV_SIGNAL(gvidsv, 0);
			UNLOCK(gvid_mutex, pl);
			return (ENOMEM);
		}

		if (copyin(tmpmap.gvid_buf, devbufp, 
				tmpmap.gvid_num * sizeof(dev_t)) == -1) {
			kmem_free(devbufp, sizeof(dev_t) * tmpmap.gvid_num);
			pl = LOCK(gvid_mutex, plhi);
			gvidflg &= ~GVID_ACCESS;
			SV_SIGNAL(gvidsv, 0);
			UNLOCK(gvid_mutex, pl);
#ifdef DEBUG
			cmn_err(CE_WARN, 
				"gvid: could not copy to tmpbuf");
#endif /* DEBUG */
			return (EFAULT);
		}

		pl = LOCK(gvid_mutex, plhi);
		tmpbuf = Gvid.gvid_buf;
		tmpnum = Gvid.gvid_num;

		Gvid.gvid_buf = devbufp;
		Gvid.gvid_num = tmpmap.gvid_num;
		Gvid.gvid_maj = tmpmap.gvid_maj;
		gvidflg &= ~GVID_ACCESS;
		gvidflg |= GVID_SET;
		SV_SIGNAL(gvidsv, 0);
		UNLOCK(gvid_mutex, pl);

		if (tmpbuf)
			kmem_free(tmpbuf, sizeof(dev_t) * tmpnum);

		return (0);
	}

	case GVID_GETTABLE: {
		gvid_t	tmpmap;

		/*
		 * Protect against stream head access. 
		 */
		pl = LOCK(gvid_mutex, plhi);

		if (!(gvidflg & GVID_SET)) {
			UNLOCK(gvid_mutex, pl);
			return EBUSY;
		}

		while (gvidflg & GVID_ACCESS)  {
			if (!SV_WAIT_SIG(gvidsv, primed - 3, gvid_mutex))
				return (EINTR);
			pl = LOCK(gvid_mutex, plhi);
		}

		gvidflg |= GVID_ACCESS;
		UNLOCK(gvid_mutex, pl);

		if (copyin((caddr_t ) arg, (caddr_t) &tmpmap, 
				sizeof(gvid_t)) == -1) {
#ifdef DEBUG
			cmn_err(CE_WARN,
				"gvid: could not copy to tmpmap");
#endif
			pl = LOCK(gvid_mutex, plhi);
			gvidflg &= ~GVID_ACCESS;
			SV_SIGNAL(gvidsv, 0);
			UNLOCK(gvid_mutex, pl);
			return (EFAULT);
		}

		pl = LOCK(gvid_mutex, plhi);
		tmpmap.gvid_num = min(tmpmap.gvid_num, Gvid.gvid_num);
		tmpmap.gvid_maj = Gvid.gvid_maj;
		UNLOCK(gvid_mutex, pl);

		if (copyout(Gvid.gvid_buf, tmpmap.gvid_buf, 
				tmpmap.gvid_num * sizeof(dev_t)) == -1) {
#ifdef DEBUG
			cmn_err(CE_WARN, 
				"gvid: could not copy to tmpbuf");
#endif
			pl = LOCK(gvid_mutex, plhi);
			gvidflg &= ~GVID_ACCESS;
			SV_SIGNAL(gvidsv, 0);
			UNLOCK(gvid_mutex, pl);
			return (EFAULT);
		}

		if (copyout((caddr_t) &tmpmap, (caddr_t) arg, 
				sizeof(gvid_t)) == -1) {
#ifdef DEBUG
			cmn_err(CE_WARN,
				"gvid: could not copy to tmpbuf");
#endif
			pl = LOCK(gvid_mutex, plhi);
			gvidflg &= ~GVID_ACCESS;
			SV_SIGNAL(gvidsv, 0);
			UNLOCK(gvid_mutex, pl);
			return (EFAULT);
		}

		pl = LOCK(gvid_mutex, plhi);
		gvidflg &= ~GVID_ACCESS;
		SV_SIGNAL(gvidsv, 0);
		UNLOCK(gvid_mutex, pl);
		return (0);
	}

	default:
		return EINVAL;

	}
}
