#ident	"@(#)devadp.c	1.4"
#ident	"$Header$"

/*
 * Indirect driver for controlling tty.
 */

#include <util/types.h>
#include <util/param.h>
#include <mem/immu.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <io/conf.h>
#include <io/stream.h>
#include <proc/cred.h>
#include <io/kd/kd.h>
#include <io/termio.h>
#include <io/termios.h>
#include <io/strtty.h>
#include <io/xque/xque.h>
#include <io/ws/ws.h>
#include <fs/specfs/specfs.h>
#include <io/ddi.h>


#define	MONO_MINOR	1
#define	CGA_MINOR	2
#define	EGA_MINOR	4
#define	VGA_MINOR	5

static struct {
	int	model;
	int	minor;
	int	type;
} videos[] = {
	KD_MONO, MONO_MINOR, KD_MONO,
	KD_HERCULES, MONO_MINOR, KD_MONO,
	KD_CGA, CGA_MINOR, KD_CGA,
	KD_EGA, EGA_MINOR, KD_EGA,
	KD_VGA, VGA_MINOR, KD_VGA,
	KD_VDC400, VGA_MINOR, KD_VGA,
	KD_VDC750, VGA_MINOR, KD_VGA,
	KD_VDC600, VGA_MINOR, KD_VGA,
#ifdef EVGA
	KD_EVGA, VGA_MINOR, KD_VGA,
#endif
	0, 0, 0
};


int adpdevflag = 0;

extern wstation_t	Kdws;


/*
 * int
 * adpopen(dev_t *, int, int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
adpopen(dev_t *devp, int flag, int otyp, struct cred *cr)
{
	int	dev; 
	int	error = ENAVAIL;
	vidstate_t	*vp = &Kdws.w_vstate;
	register i;

	
	for (i = 0; videos[i].model; i++) 
		if (videos[i].minor == getminor(*devp) &&
			(videos[i].model == vp->v_type ||
			videos[i].type  == vp->v_type )) {
			error = 0;
			break;
		}

	if (error)
		return(error);
		
	if (get_vt_dev(&dev, cr, 0) < 0)
		return(-1);

	return(ws_open(dev, flag, otyp, cr));
}


/*
 * int
 * adpclose(dev_t, int, int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
adpclose(dev_t dev, int flag, int otyp, struct cred *cr)
{
	if (get_vt_dev(&dev, cr, 1) < 0)
		return(-1);

	return(ws_close(dev, flag, otyp, cr));
}


/*
 * int
 * adpread(dev_t, struct uio *, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
adpread(dev_t dev, struct uio *uiop, struct cred *cr)
{
	if (get_vt_dev(&dev, cr, 1) < 0)
		return(-1);

	return(ws_read(dev, uiop, cr));
}


/*
 * int
 * adpwrite(dev_t, struct uio *, struct cred *)
 *
 * Calling/Exit State: 
 *	None.
 */
/* ARGSUSED */
int
adpwrite(dev_t dev, struct uio *uiop, struct cred *cr)
{
	if (get_vt_dev(&dev, cr, 1) < 0)
		return(-1);

	return(ws_write(dev, uiop, cr));
}


/*
 * int
 * adpioctl(dev_t, int, int, int, struct cred *, int *)
 *
 * Calling/Exit State: 
 *	None.
 */
/* ARGSUSED */
int
adpioctl(dev_t dev, int cmd, int arg, int mode, struct cred *cr, int *rvalp)
{
	if (get_vt_dev(&dev, cr, 1) < 0)
		return(-1);

	return(ws_ioctl(dev, cmd, arg, mode, cr, rvalp));
}


/*
 * int
 * get_vt_dev(dev_t *, struct cred *, int)
 *
 * Calling/Exit State:
 *	- Return -1 if there does not exist a controlling tty, otherwise
 *	  return 0.
 */
int
get_vt_dev(dev_t *devp, struct cred *cr, int flag)
{
	int ws_getvtdev();


	if (ws_getvtdev(devp) < 0)
		return(-1);

	if (flag) {
		int	rval;

		if (ws_ioctl(*devp, CONSADP, 0, 0, cr, &rval) != -1)
			*devp = makedevice(getmajor(*devp), rval);		
	}

	return(0);
}
