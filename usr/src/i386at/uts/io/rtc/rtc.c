/* 
 * Driver for PC AT Calendar clock chip
 *	
 * PC-DOS compatibility requirements:
 *      1. must use local time, not GMT (sigh!)
 *      2. must use bcd mode, not binary
 *
 * To really use this device effectively there should be a program
 * called e.g. rtclock that works like date does.  i.e. "rtclock"
 * would return local time and "rtclock arg" would set local time arg
 * into the chip.  To set the system clock on boot /etc/rc should do
 * something like "date `rtclock`".
 *
 * Modified to use the PSM V2 interfaces for access to the date/time.
 */

#include <fs/file.h>
#include <io/rtc/rtc.h>
#include <io/uio.h>
#include <proc/signal.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <svc/psm.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/ipl.h>
#include <svc/psm.h>

#include <io/ddi.h>	/* must come last */

#define LOCK_HIER_BASE 5
#define PLHI plhi

STATIC lock_t *rtc_mutex=NULL;
STATIC LKINFO_DECL(rtc_lkinfo, "KU::rtc_mutex", 0);

int rtcdevflag = 0;

/*
 * void
 * rtcinit(void)
 *	Initialize real time clock to interrupt us when the chip updates.
 *	(approx. once a second)
 *
 * Calling/Exit State:
 *	none
 */

void
rtcinit(void)
{
	struct rtc_t rtcbuf;

	if (rtc_mutex == NULL)
		rtc_mutex = LOCK_ALLOC(LOCK_HIER_BASE, PLHI, &rtc_lkinfo, 0);

	if (!rtcget(&rtcbuf)) {
		/*
		 * reinitialize the real time clock.
		 * Nothing special required with PSM V2 - handled in PSM as
		 * a sideeffect of the rtcget.
		 */
	}
}

/*
 * int
 * rtcopen(dev_t *devp, int flags, int otyp, void *cred_p)
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcopen(dev_t *devp, int flags, int otyp, void *cred_p)
{
	int error;

	if ((flags & FWRITE) && (error = drv_priv(cred_p)) != 0)
		return error;

	return 0;
}

/*
 * int
 * rtcread(dev_t dev, struct uio *uio_p, void *cred_p)
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcread(dev_t dev, struct uio *uio_p, void *cred_p)
{
	return 0;
}

/*
 * int
 * rtcwrite(dev_t dev, struct uio *uio_p, void *cred_p)
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcwrite(dev_t dev, struct uio *uio_p, void *cred_p)
{
	return 0;
}

/*
 * int
 * rtcioctl(dev_t dev, int cmd, void *addr, int mode,
 *	    void *cred_p, int *rval_p)
 *	This is used to read and write the clock.
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcioctl(dev_t dev, int cmd, void *addr, int mode,
	 void *cred_p, int *rval_p)
{
	struct rtc_t rtcbuf;
	int ecode = 0;

	switch (cmd) {
	case RTCRTIME:
		if (rtcget(&rtcbuf)) {
			ecode = EIO;
			break;
		}
		if (copyout(&rtcbuf, addr, RTC_NREG) != 0)
			ecode = EFAULT;
		break;
	case RTCSTIME:
		if ((ecode = drv_priv(cred_p)) != 0) {
			break;
		}
		if (copyin(addr, &rtcbuf, RTC_NREGP) != 0) {
			ecode = EFAULT;
			break;
		}
		rtcput(&rtcbuf);
		break;
	default:
		ecode = EINVAL;
		break;
	}

	return ecode;
}

/*
 * int
 * rtcclose(dev_t dev, int flags, int otyp, void *cred_p)
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcclose(dev_t dev, int flags, int otyp, void *cred_p)
{
	return 0;
}

/*
 * int
 * rtcget(struct rtc_t *buf)
 *	Call PSM to read the current date/time. Then convert it to
 *	the appropriate type.
 *
 * Calling/Exit State:
 *	Returns -1 if clock not valid, else 0.
 *	Returns RTC_NREG (which is 15) bytes of data in (*buf), as given
 *	in the technical reference data.  This data includes both the time
 *	and the status registers.
 */

int
rtcget(struct rtc_t *buf)
{
	pl_t		pl;
	ms_bool_t	ok;

	if (rtc_mutex == NULL)
		rtcinit();

	pl = LOCK(rtc_mutex, PLHI);

	/*
	 * ZZZ KLUDGE - the PSM routines currently use the WRONG TYPE.
	 * This must be fixed once conversion routines are written.
	 */
	ok = ms_rtodc ((ms_daytime_t*) buf);

	UNLOCK(rtc_mutex, pl);

	if (!ok)
		cmn_err(CE_NOTE, "rtcget: failed to read clock\n");

	return ((ok==MS_TRUE) ? 0 : -1);
}

/*
 * void
 * rtcput(struct rtc_t *buf)
 *	This routine writes the contents of the given buffer to the real time
 *	clock.
 *
 * Calling/Exit State:
 *	Takes RTC_NREGP bytes of data, which are the 10 bytes
 *	used to write the time and set the alarm.
 */

void
rtcput(struct rtc_t *buf)
{
	pl_t		pl;
	ms_bool_t	ok;

	if (rtc_mutex == NULL)
		rtcinit();

	pl = LOCK(rtc_mutex, PLHI);

	/*
	 * ZZZ KLUDGE - the PSM routines currently use the WRONG TYPE.
	 * This must be fixed once conversion routines are written.
	 */
	ok = ms_wtodc ((ms_daytime_t*) buf);

	UNLOCK(rtc_mutex, pl);

	if (!ok)
		cmn_err(CE_NOTE, "rtcput: failed to write clock\n");
}

/*
 * void
 * rtcintr(int ivect)
 *	handle interrupt from real time clock
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
void
rtcintr(int ivect)
{

}


