#ident	"@(#)kern-i386:io/postwait/postwait.c	1.3"
#ident	"$Header$"

#include <io/conf.h>
#include <io/postwait/postwait.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/inline.h> 
#include <util/ksynch.h> 
#include <util/param.h> 
#include <util/types.h> 

/*
 * The post-wait driver is used for process synchronization.
 * A process will wait (with an optional timeout) for another
 * process to post it (wake it up).  This is used by some
 * Application programs instead of setting a timer, doing 
 * a semaphore operation and then cancelling the timer.
 * For some Applications reducing the 3 system calls to one
 * really matters.
 */

#define PWTIME 		0x1	/* post-wait timeout pending */
#define PWPOST 		0x2	/* pw_post before pw_wait */

int pwdevflag = D_MP;

int pwopen(dev_t *devp, int flags, int otyp, struct cred *cred_p);
int pwclose(dev_t dev, int flag, cred_t *cr);
STATIC void pwtime(proc_t *p);
STATIC int pwwait(int time, int *rvalp);
STATIC int pwpost(int pid, int *rvalp);
int pwioctl(dev_t dev, int cmd, int arg, int flag, cred_t *cr, int *rvalp);

/*
 * int
 * pwopen(dev_t *devp, int flags, int otyp, struct cred *cred_p);
 *
 * Description:
 *	Open routine for post-wait.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
pwopen(dev_t *devp, int flags, int otyp, struct cred *cred_p)
{
        return 0;
}

/*
 * int
 * pwclose(dev_t dev, int flag, cred_t *cr)
 *
 * Description:
 *	Close routine for post-wait.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
pwclose(dev_t dev, int flag, cred_t *cr)
{
        return 0;
}

/*
 * STATIC void
 * pwtime(proc_t *p)
 *
 * Description:
 *	Called when post-wait timeout expires.
 *	Turn off flag so the waiter can tell
 *	that the timeout has expired.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
pwtime(proc_t *p)
{
	pl_t pl;

	pl = LOCK(&p->p_mutex, PLHI);
	p->p_pwflag &= ~PWTIME;
	UNLOCK(&p->p_mutex, pl);

	SV_BROADCAST(&p->p_pwsv, 0);
}

/*
 * STATIC int
 * pwwait(int time, int *rvalp)
 * 
 * Description:
 * 	Wait for 'time' ticks, or until posted
 * 	or signalled.  A 'time' of -1 is an
 * 	infinite wait. The process remembers if
 * 	it was posted before it entered pwwait, and 
 * 	will return 1 just like the post came in
 * 	after sleeping in pwwait.
 * 
 * Calling/Exit State:
 * 	None.
 * 
 * Returns:
 * 	0 - timer has expired; *rvalp was set to 0 in systrap.
 * 	1 - process was posted;  *rvalp set to 1;
 * 	EINVAL - time is < -1
 * 	EINTR  - process was signaled
 * 
 */ 
STATIC int
pwwait(int time, int *rvalp)
{
	int id;
	proc_t *p = u.u_procp;
	pl_t pl;


	if (time == 0) {
		/*
		 * if a pw_post happened before
		 * we did the pw_wait, return 1
		 * just like we were posted.
		 */
		if (p->p_pwflag & PWPOST) {
			p->p_pwflag &= ~PWPOST;
			*rvalp = 1;
			return 0;
		}
		/*
		 * If possible, give up the processor.
		 */
		(void) CL_YIELD(u.u_lwpp, u.u_lwpp->l_cllwpp, B_TRUE);
		/*
		 * See if post happened
		 * while we gave up processor.
		 */
		if (p->p_pwflag & PWPOST) {
			p->p_pwflag &= ~PWPOST;
			*rvalp = 1;
		}
		return 0;
	}

	pl = LOCK(&p->p_mutex, PLHI);
	/*
	 * if a pw_post happened before
	 * we did the pw_wait, return 1
	 * just like we were posted.
	 */
	if (p->p_pwflag & PWPOST) {
		p->p_pwflag &= ~PWPOST;
		UNLOCK(&p->p_mutex, pl);
		*rvalp = 1;
		return 0;
	}
	
	/*
	 * Sleep until we are posted, the optional
	 * timeout goes off, or we are signalled.
	 * The timeout firing will clear PWTIME.
	 * A pw_post will not.
	 */

	if (time > 0) {
		p->p_pwflag |= PWTIME;
		id = itimeout((void(*)())pwtime, (caddr_t)p, time, PLHI);
		if (id == 0) {
			p->p_pwflag &= ~(PWPOST|PWTIME);
			UNLOCK(&p->p_mutex, pl);
			return EAGAIN;
		}
	} else if (time < -1) {
		UNLOCK(&p->p_mutex, pl);
		return EINVAL;
	}


	if (SV_WAIT_SIG(&p->p_pwsv, PRIMED, &p->p_mutex) == B_FALSE) {
		/*
		 * We were signaled.
		 */
		if (time > 0)
			untimeout(id);
		p->p_pwflag &= ~(PWPOST|PWTIME);
		return EINTR;
	}

	if (time > 0) {
		pl = LOCK(&p->p_mutex, PLHI);
		if ((p->p_pwflag & PWTIME) == 0) {
			/*
			 * The timer has expired.
			 */
			p->p_pwflag &= ~PWPOST;
			UNLOCK(&p->p_mutex, pl);
			return 0;
		} else {
			/*
			 * We were awakened by the post
			 * before the timeout fired.
			 * Cancel the timeout, and
			 * fall thru.
			 */
			UNLOCK(&p->p_mutex, pl);
			untimeout(id);
		}
	}
	/*
	 * We have been posted.
	 */
	p->p_pwflag &= ~(PWPOST|PWTIME);
	*rvalp = 1;
	return 0;
}

/*
 * STATIC int
 * pwpost(int pid, int *rvalp)
 *
 * Description:
 *	Post (wakeup) the process pid.
 *	If the process has done a pwwait,
 *	arrange for it to start running again.
 *	If the process is not waiting,
 *	record the post so that the process
 *	will get the post the next time
 *	it does a pwwait.  We do nothing
 *	if we are posting ourself.
 *
 * Calling/Exit State:
 *	None.
 *
 * Returns:
 *	0 - process was posted, was sleeping in pw_wait
 *	1 - process was posted, was not in pw_wait
 *	EINVAL - process 'pid' does not exist
 */
STATIC int
pwpost(int pid, int *rvalp)
{
	proc_t *p;

	/*
	 * prfind does ASSERT(pid >= 0).
	 */
	if (pid < 0)
		return EINVAL;

	p = prfind(pid);	/* returns with p->p_mutex held */
	if (p == NULL)
		return EINVAL;

	if (SV_BLKD(&p->p_pwsv)) {
		SV_BROADCAST(&p->p_pwsv, KS_NOPRMPT);
		UNLOCK(&p->p_mutex, PLBASE);
	} else {
		/*
		 * If a pw_post happens before
		 * the process does the pw_wait,
		 * remember it for later; except
		 * we don't set our own flag.
		 * Let the caller know the process
		 * was not waiting.
		 */
		if (pid != u.u_procp->p_pidp->pid_id) {
			p->p_pwflag |= PWPOST;
		}
		UNLOCK(&p->p_mutex, PLBASE);
		*rvalp = 1;
	}

	return 0;
}

/*
 * int
 * pwioctl(dev_t dev, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
 *
 * Description:
 *	Ioctl routine for post-wait.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
pwioctl(dev_t dev, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
{
	int error;

	switch(cmd) {
		case PWIOC_POST:
			error = pwpost(arg, rvalp);
			break;
		case PWIOC_WAIT:
			error = pwwait(arg, rvalp);
			break;
		default:
			error = EINVAL;
	}

	return error;
}
