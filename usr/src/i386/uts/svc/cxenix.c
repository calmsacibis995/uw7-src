#ident	"@(#)kern-i386:svc/cxenix.c	1.5.6.1"
#ident	"$Header$"

#include <svc/reg.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/types.h>
#include <util/debug.h>
#include <fs/xnamfs/xnamnode.h>

/* Enhanced Application Compatibility Support */
#include <io/termios.h>
#include <svc/sco.h>
/* The functions that require ACP package are declared in sco.h */
/* End Enhanced Application Compatibility Support */

extern	int	nullsys();
extern	int	nosys(), chsize(), creatsem(), ftime(), locking(), nap();
extern	int	nbwaitsem(), opensem(), proctl(), rdchk(), sigsem();
extern	int	waitsem(), execseg(), sdenter(), xsdfree(), sdget();
extern	int	sdgetv(), sdleave(), sdwaitv(), unexecseg();
extern	int	setreuid(), setregid(), setitimer(), getitimer();

int	xnet_sys() { return (0); }

#ifdef DEBUG
/*
 * For now we turn this on whenever DEBUG is defined.
 * XXX - Take this out later.
 */
#define Xdebug
#endif

/*
 * XENIX-special system calls.  In order to save space in the system call
 * table, and to minimize conflicts with other unix systems, all custom
 * XENIX calls are done via the cxenix() call. The cxentry table is the
 * switch used to transfer to the appropriate routine for processing a
 * cxenix sub-type system call.  Each row contains the number of arguments
 * expected and a pointer to the routine.
 */

struct sysent cxentry[] = {
	0, nosys,			/* 0 = obsolete (XENIX shutdown) */
	3, locking,			/* 1 = XENIX file/record lock */
	2, creatsem,			/* 2 = create XENIX semaphore */
	1, opensem,			/* 3 = open XENIX semaphore */
	1, sigsem,			/* 4 = signal XENIX semaphore */
	1, waitsem,			/* 5 = wait on XENIX semaphore */
	1, nbwaitsem,			/* 6 = nonblocking wait on XENIX sem */
	1, rdchk,			/* 7 = read check */
	0, nosys,			/* 8 = obsolete (XENIX stkgrow) */
	0, nosys,			/* 9 = obsolete (XENIX ptrace) */
	2, chsize,			/* 10 = change file size */
	1, ftime,			/* 11 = V7 ftime*/
	1, nap,				/* 12 = nap */
	4, sdget,			/* 13 = create/attach XENIX shdata */
	1, xsdfree,			/* 14 = free XENIX shdata */
	2, sdenter,			/* 15 = enter XENIX shdata */
	1, sdleave,			/* 16 = leave XENIX shdata */
	1, sdgetv,			/* 17 = get XENIX shdata version */
	2, sdwaitv,			/* 18 = wait for XENIX shdata version */
	0, nosys,			/* 19 = obsolete (XENIX brkctl) */
	0, nosys,			/* 20 = unused (reserved for XENIX) */
	0, xnet_sys,			/* 21 = obsolete (XENIX nfs_sys) */
	0, nosys,			/* 22 = obsolete (XENIX msgctl) */
	0, nosys,			/* 23 = obsolete (XENIX msgget) */
	0, nosys,			/* 24 = obsolete (XENIX msgsnd) */
	0, nosys,			/* 25 = obsolete (XENIX msgrcv) */
	0, nosys,			/* 26 = obsolete (XENIX semctl) */
	0, nosys,			/* 27 = obsolete (XENIX semget) */
	0, nosys,			/* 28 = obsolete (XENIX semop) */
	0, nosys,			/* 29 = obsolete (XENIX shmctl) */
	0, nosys,			/* 30 = obsolete (XENIX shmget) */
	0, nosys,			/* 31 = obsolete (XENIX shmat) */
	3, proctl,			/* 32 = proctl */
	0, execseg,			/* 33 = execseg */
	0, unexecseg,			/* 34 = unexecseg */
	0, nosys,			/* 35 = obsolete (XENIX swapadd) */
	/* Enhanced Application Compatibility Support */
	5, select_sco,			/* 36 = SCO select */
	2, eaccess_sco,			/* 37 = SCO eaccess*/
	0, nosys,			/* 38 = SCO RESERVED for paccess*/
	3, sigaction_sco,		/* 39 = SCO sigaction */
	3, sigprocmask_sco,		/* 40 = SCO sigprocmask */
	1, sigpending_sco,		/* 41 = SCO sigpending */
	1, sigsuspend_sco,		/* 42 = SCO sigsuspend */
	2, getgroups_sco,		/* 43 = SCO getgroups */
	2, setgroups_sco,		/* 44 = SCO setgroups */
	1, sysconf_sco,			/* 45 = SCO sysconf */
	2, pathconf_sco,		/* 46 = SCO pathconf */
	2, fpathconf_sco,		/* 47 = SCO fpathconf */
	2, rename_sco,			/* 48 = SCO rename */
	0, nosys,			/* 49 = SCO reserved for security */
	2, scoinfo,			/* 50 = SCO scoinfo */
	0, nosys,			/* 51 = SCO/ALTOS Reserved */
	0, nosys,			/* 52 = SCO/ALTOS Reserved */
	0, nosys,			/* 53 = SCO/ALTOS Reserved */
	0, nosys,			/* 54 = SCO/ALTOS Reserved */
	2, getitimer,			/* 55 = getitimer */
	3, setitimer,			/* 56 = setitimer */
	0, nosys,			/* 57 = SCO TBD */
	/* End Enhanced Application Compatibility Support */
	2, setreuid,			/* 58 = BSD/SCO/UNIX95 setreuid */
	2, setregid,			/* 59 = BSD/SCO/UNIX95 setregid */
	/* Enhanced Application Compatibility Support */
	0, nosys,			/* 60 = Reserved for SCO pmregister */
	0, nosys,			/* 61 = Reserved for SCO probe */
	/* End Enhanced Application Compatibility Support */

};

/* number of cxenix subfunctions */	
int ncxentry = sizeof(cxentry)/sizeof(struct sysent);

#ifdef	Xdebug
#include <util/cmn_err.h>

int	Xdbg = 0;
STATIC	void Xdbprt(int);
#endif	/* Xdebug */

/*
 * int
 * cxenix(char *uap, rval_t *rvp)
 *      XENIX custom system call dispatcher
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
/* ARGSUSED */
int
cxenix(char *uap, rval_t *rvp)
{
	int	subfunc;
	struct	sysent *callp;
	int	*ap;
#ifdef	Xdebug
	u_int	i;
#endif	/* Xdebug */

	ap = (int *)u.u_ar0[T_UESP];
	/*
	 * ap points to the return addr on the user's
	 * stack. bump it up to the actual args.
	 */
	ap++;
	subfunc = (u.u_syscall >> 8) & 0xff;
	if (subfunc >= ncxentry) {
		return (EINVAL);
	}
	callp = &cxentry[subfunc];
#ifdef	Xdebug
	Xdbprt(subfunc);
#endif	/* Xdebug */

	/* get cxenix arguments in U block */
	if (ucopyin(ap, u.u_arg, callp->sy_narg * sizeof(int), 0) != 0) {
		return (EFAULT);
	}
#ifdef	Xdebug
	for (i = 0; i < callp->sy_narg; i++) {
		if (Xdbg) {
			cmn_err(CE_CONT, "%x  ", u.u_arg[i]);
		}
	}
	if (Xdbg) {
		cmn_err(CE_CONT, "\n");
	}
#endif	/* Xdebug */

	/* do the system call */
	return ((*callp->sy_call)(u.u_ap, rvp));

}

#ifdef	Xdebug
STATIC char *Xsysnames[] = {
	"nosys",		/* 0 = obsolete (XENIX shutdown) */
	"locking",		/* 1 = XENIX file/record lock */
	"creatsem",		/* 2 = create XENIX semaphore */
	"opensem",		/* 3 = open XENIX semaphore */
	"sigsem",		/* 4 = signal XENIX semaphore */
	"waitsem",		/* 5 = wait on XENIX semaphore */
	"nbwaitsem",		/* 6 = nonblocking wait on XENIX sem */
	"rdchk",		/* 7 = read check */
	"nosys",		/* 8 = obsolete (XENIX stkgrow) */
	"nosys",		/* 9 = obsolete (XENIX ptrace) */
	"chsize",		/* 10 = change file size */
	"ftime",		/* 11 = V7 ftime*/
	"nap",			/* 12 = nap */
	"sdget",		/* 13 = create/attach XENIX shdata */
	"xsdfree",		/* 14 = free XENIX shdata */
	"sdenter",		/* 15 = enter XENIX shdata */
	"sdleave",		/* 16 = leave XENIX shdata */
	"sdgetv",		/* 17 = get XENIX shdata version */
	"sdwaitv",		/* 18 = wait for XENIX shdata version */
	"nosys",		/* 19 = obsolete (XENIX brkctl) */
	"nosys",		/* 20 = unused (reserved for XENIX) */
	"xnet_sys",		/* 21 = obsolete (XENIX nfs_sys) */
	"nosys",		/* 22 = obsolete (XENIX msgctl) */
	"nosys",		/* 23 = obsolete (XENIX msgget) */
	"nosys",		/* 24 = obsolete (XENIX msgsnd) */
	"nosys",		/* 25 = obsolete (XENIX msgrcv) */
	"nosys",		/* 26 = obsolete (XENIX semctl) */
	"nosys",		/* 27 = obsolete (XENIX semget) */
	"nosys",		/* 28 = obsolete (XENIX semop) */
	"nosys",		/* 29 = obsolete (XENIX shmctl) */
	"nosys",		/* 30 = obsolete (XENIX shmget) */
	"nosys",		/* 31 = obsolete (XENIX shmat) */
	"proctl",		/* 32 = proctl */
	"execseg",		/* 33 = execseg */
	"unexecseg",		/* 34 = unexecseg */
	"nosys",		/* 35 = obsolete (XENIX swapadd) */
	"select_sco",		/* 36 = SCO select */
	"eaccess_sco",		/* 37 = SCO eaccess */
	"nosys",		/* 38 = SCO TBD */
	"sigaction_sco",	/* 39 = SCO sigaction */
	"sigprocmask_sco",	/* 40 = SCO sigprocmask */
	"sigpending_sco",	/* 41 = SCO sigpending */
	"sigsuspend_sco",	/* 42 = SCO sigsuspend */
	"getgroups_sco",	/* 43 = SCO getgroups */
	"setgroups_sco",	/* 44 = SCO setgroups */
	"sysconf_sco",		/* 45 = SCO sysconf */
	"pathconf_sco",		/* 46 = SCO pathconf */
	"fpathconf_sco",	/* 47 = SCO fpathconf */
	"rename_sco",		/* 48 = SCO rename */
	"nosys",		/* 49 = Not Used */
	"scoinfo",		/* 50 = SCO scoinfo */
	"nosys",		/* 51 = SCO/ALTOS Reserved */
	"nosys",		/* 52 = SCO/ALTOS Reserved */
	"nosys",		/* 53 = SCO/ALTOS Reserved */
	"nosys",		/* 54 = SCO/ALTOS Reserved */
	"nosys",		/* 55 = SCO TBD */
	"nosys",		/* 56 = SCO TBD */
	"nosys",		/* 57 = SCO TBD */
	"setreuid",		/* 58 = BSD/SCO/UNIX95 setreuid */
	"setregid",		/* 59 = BSD/SCO/UNIX95 setregid */
};
/*
 * STATIC void
 * Xdbprt(int callno)
 *      Debug routine to print XENIX system call information.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
STATIC void
Xdbprt(int callno)
{
	if (Xdbg) {
		cmn_err(CE_CONT, "xenix call %d: %s, nargs = %d; ",
			callno, Xsysnames[callno], cxentry[callno].sy_narg);
	}
}
#endif	/* Xdebug */
