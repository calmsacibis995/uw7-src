#ident	"@(#)kern-i386:svc/isc.c	1.4.2.1"
#ident	"$Header$"

/* Enhanced Application Compatibility Support */

#include <util/types.h>
#include <svc/systm.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/regset.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>
#include <svc/sysconfig.h>
#include <proc/unistd.h>
#include <proc/procset.h>
#include <proc/wait.h>

#include <svc/isc.h>

/*
 * Argument prototypes are not used because we cannot get struct
 * definitions
 */
extern int getgroups();
extern int setgroups();
extern int sysconfig();
extern int pathconf();
extern int fpathconf();
extern int rename();
extern int fsync();
extern int waitsys();
extern int nosys(void);
extern void *umem_alloc(size_t);
extern void umem_free(void *, size_t);
extern int	ngroups_max;

STATIC int wstat_isc(int, int);

#define K_FAULT_ERR	"%s: generated bad user address (%#x)\n"

typedef long		isc_sigset_t;
typedef unsigned short	isc_uid_t;
typedef unsigned short	isc_gid_t;
typedef short		isc_pid_t;

int	__setostype(), rename_isc(), sigaction_isc(), sigprocmask_isc(),
	sigpending_isc(), getgroups_isc(), setgroups_isc(), pathconf_isc(),
	fpathconf_isc(), sysconf_isc(), fsync_isc(),
	waitpid_isc(), setsid_isc(), setpgid_isc();

struct sysent iscentry[] = {
        0, nosys,			/* 0 = UNUSED */
	1, __setostype,			/* 1 = __setostype */
	2, rename_isc,			/* 2 = ISC rename */
	3, sigaction_isc,		/* 3 = ISC sigaction */
	3, sigprocmask_isc,		/* 4 = ISC sigprocmask */
	1, sigpending_isc,		/* 5 = ISC sigpending */
	2, getgroups_isc,		/* 6 = ISC getgroups */
	2, setgroups_isc,		/* 7 = ISC setgroups */
	2, pathconf_isc,		/* 8 = ISC pathconf */
	2, fpathconf_isc,		/* 9 = ISC fpathconf */
	1, sysconf_isc,			/* 10 = ISC sysconf */
	3, waitpid_isc,			/* 11 = wait on PID */
	0, setsid_isc,			/* 12 = create new session */
	2, setpgid_isc,			/* 13 = set process group id */
	0, nosys,			/* 14 */
	0, nosys,			/* 15 */
	1, fsync_isc			/* 16 = ISC fsync */
};

/* number of sysisc subfunctions */	
int niscentry = sizeof iscentry/sizeof(struct sysent);

/*
 * int sysisc(char *uap, rval_t *rvp)
 *      sysisc - ISC custom system call dispatcher
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
sysisc(char *uap, rval_t *rvp)
{
	register int subfunc;
	register struct user *uptr = &u;
	register struct sysent *callp;
	register int *ap;
	register u_int i;
	int ret;

	ap = (int *)u.u_ar0[R_ESP];
	ap++;			/* ap points to the return addr on the user's
				 * stack. bump it up to the actual args.  */
	subfunc = fuword(ap++);	/* First argument is subfunction */
	if (subfunc <= 0 || subfunc >= niscentry)
		return EINVAL;
	callp = &iscentry[subfunc];

	/* get sysisc arguments in U block */
	for (i = 0; i < callp->sy_narg; i++){
		uptr->u_arg[i] = fuword(ap++);
	}

	/* do the system call */
	ret = (*callp->sy_call)(uptr->u_arg, rvp);
	return ret;
}

#define ISC_SYSV_SEMANTICS	0
#define ISC_POSIX_SEMANTICS	1

struct isc_setostypea {
	int	type;
};

/*
 * int __setostype(struct isc_setostypea *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
__setostype(struct isc_setostypea *uap, rval_t *rvp)
{
	if (uap->type == ISC_POSIX_SEMANTICS)
		RENV2 |= ISC_POSIX;
	return 0;
}


/*
** Support for ISC implementation of rename(2)
*/
struct isc_renamea {
	char	*from;
	char	*to;
};

/*
 * int rename_isc(struct isc_renamea *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
rename_isc(struct isc_renamea *uap, rval_t *rvp)
{
	return rename(uap, rvp);
}


/*
** Support for ISC implementation of sigaction(2)
*/
#define ISC_SA_NOCLDSTOP	1	/* ISC's value for SA_NOCLDSTOP */

struct isc_sigaction {			/* ISC version of struct sigaction */
	union {
		void		(*sa__handler)();
	} sa_u;
	isc_sigset_t	sa_mask;
	int		sa_flags;
};

struct isc_sigactiona {
	int			sig;
	struct isc_sigaction	*act;
	struct isc_sigaction	*oact;
};

struct sigactiona {
	int			sig;
	struct sigaction	*act;
	struct sigaction	*oact;
};

/*
 * int sigaction_isc(struct isc_sigactiona *uap, rval_t *rvp)
 *
 * Since the ISC and USL definitions of struct sigaction differ, this
 * function has to do translation before and after calling
 * scalls.c:sigaction().  The basic steps are (assuming `act' and `oact'
 * are both non-NULL):
 *	1. Copy in ISC-style structure.
 *	2. Allocate *user* memory to hold USL-style sigaction struct.
 *	3. Translate ISC format to USL.
 *	4. Call real sigaction.
 *	5. Translate old sigaction structure to ISC format.
 *	6. Copy out ISC-style structure.
 *
 * Calling/Exit State:
 */

/* ARGSUSED */
int
sigaction_isc(struct isc_sigactiona *uap, rval_t *rvp)
{
	struct isc_sigaction	isc_act;
	struct sigaction	usl_act;
	struct sigactiona	nuap;
	int			error = 0;

	switch (uap->sig) {
	case ISC_SIGCONT:
		nuap.sig = SIGCONT;
		break;
	case ISC_SIGSTOP:
		nuap.sig = SIGSTOP;
		break;
	case ISC_SIGTSTP:
		nuap.sig = SIGTSTP;
		break;
	default:
		nuap.sig = uap->sig;
		break;
	}

	nuap.act = NULL;
	nuap.oact = NULL;

	if (uap->act != NULL) {
		if (copyin((caddr_t)uap->act,
			   (caddr_t)&isc_act, sizeof isc_act)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.act = umem_alloc(sizeof *nuap.act);
		if (nuap.act == NULL) {
			error = ENOMEM;
			goto cleanup;
		}

		/* Translate ISC sigaction structure to the USL format */
		usl_act.sa_flags = isc_act.sa_flags & ISC_SA_NOCLDSTOP ?
					SA_NOCLDSTOP : 0;
		usl_act.sa_handler = isc_act.sa_handler;
		usl_act.sa_mask.sa_sigbits[0] = isc_act.sa_mask;
		usl_act.sa_mask.sa_sigbits[1] = 0;
		usl_act.sa_mask.sa_sigbits[2] = 0;
		usl_act.sa_mask.sa_sigbits[3] = 0;

		if (copyout((caddr_t)&usl_act,
			    (caddr_t)nuap.act, sizeof usl_act)) {
			/*
			 *+ sigaction_isc copyout failed
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "sigaction", nuap.act);
			error = EFAULT;
			goto cleanup;
		}
	}

	/*
	 * Only allocate space for one sigaction structure.  If we didn't
	 * allocate one above and we need one for translation of `oact',
	 * then allocate one now.  If we did allocate one above, just use
	 * it, since sigaction(2) handles the case where `act' and `oact'
	 * point to the same address.
	 */
	if (uap->oact != NULL)
		if (nuap.act == NULL) {
			nuap.oact = umem_alloc(sizeof *nuap.oact);
			if (nuap.oact == NULL) {
				error = ENOMEM;
				goto cleanup;
			}
		} else
			nuap.oact = nuap.act;

	if (error = sigaction(&nuap, rvp))
		goto cleanup;

	/*
	 * The ISC implementation of sigaction(2) expects the old method of
	 * returning from a handler, so we set it up here.
	 */
	u.u_procp->p_sigreturn = (void (*)())u.u_ar0[EDX];
	u.u_procp->p_sigstate[nuap.sig - 1].sst_rflags |= SST_OLDSIG;

	if (nuap.oact != NULL) {
		if (copyin((caddr_t)nuap.oact,
			   (caddr_t)&usl_act, sizeof usl_act)) {
			/*
			 *+ sigaction_isc copyin failed
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "sigaction", nuap.oact);
			error = EFAULT;
			goto cleanup;
		}

		/* Translate USL sigaction structre to the ISC format */
		isc_act.sa_flags = usl_act.sa_flags & SA_NOCLDSTOP ?
					ISC_SA_NOCLDSTOP : 0;
		isc_act.sa_handler = usl_act.sa_handler;
		isc_act.sa_mask = usl_act.sa_mask.sa_sigbits[0];

		if (copyout((caddr_t)&isc_act,
			    (caddr_t)uap->oact, sizeof isc_act))
			error = EFAULT;
	}

 cleanup:
	if (nuap.act != NULL)
		umem_free(nuap.act, sizeof *nuap.act);
	else if (nuap.oact != NULL)
		umem_free(nuap.oact, sizeof *nuap.oact);

	return error;
}

/*
 * Support for ISC implementation of sigprocmask(2)
 */
/* Converts ISC sigprocmask `how' arg. to USL value */
static int procmask_conv[] = {SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK};

struct isc_sigprocmaska {
	int		how;
	isc_sigset_t	*set;
	isc_sigset_t	*oset;
};

struct sigprocmaska {
	int		how;
	sigset_t	*set;
	sigset_t	*oset;
};

/*
 * int sigprocmask_isc(truct isc_sigprocmaska *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
sigprocmask_isc(truct isc_sigprocmaska *uap, rval_t *rvp)
{
	isc_sigset_t		isc_set;
	isc_sigset_t		temp_isc_set;
	sigset_t		usl_set;
	struct sigprocmaska	nuap;
	int			error = 0;

	/* USL and ISC have different definitions of SIG_* */
	if (uap->how < 0 || uap->how > 2)
		return EINVAL;
	nuap.how = procmask_conv[uap->how];
	nuap.set = NULL;
	nuap.oset = NULL;

	if (uap->set != NULL) {
		if (copyin((caddr_t)uap->set,
			   (caddr_t)&isc_set, sizeof isc_set)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.set = umem_alloc(sizeof *nuap.set);
		if (nuap.set == NULL) {
			error = ENOMEM;
			goto cleanup;
		}

		/* Structure Copy */
		temp_isc_set = isc_set;

		/* Convert 
		 * ISC_SIGCONT to SIGCONT, 
		 * ISC_SIGSTOP to SIGSTOP 
		 * ISC_SIGTSTP to SIGTSTP
		 */

		sigdelset(&isc_set, ISC_SIGCONT);
		sigdelset(&isc_set, ISC_SIGSTOP);
		sigdelset(&isc_set, ISC_SIGTSTP);

		if (sigismember(&temp_isc_set, ISC_SIGCONT))
			sigaddset(&isc_set, SIGCONT);
		if (sigismember(&temp_isc_set, ISC_SIGSTOP))
			sigaddset(&isc_set, SIGSTOP);
		if (sigismember(&temp_isc_set, ISC_SIGTSTP))
			sigaddset(&isc_set, SIGTSTP);

		/* Convert ISC sigset to the USL format */
		usl_set.sa_sigbits[0] = isc_set;
		usl_set.sa_sigbits[1] = 0;
		usl_set.sa_sigbits[2] = 0;
		usl_set.sa_sigbits[3] = 0;

		if (copyout((caddr_t)&usl_set,
			    (caddr_t)nuap.set, sizeof usl_set)) {
			/*
			 *+ sigprocmask_isc copyout failed
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "sigprocmask", nuap.set);
			error = EFAULT;
			goto cleanup;
		}
	}

	/*
	 * Only allocate space for one sigset.  If we didn't
	 * allocate one above and we need one for conversion of `oset',
	 * then allocate one now.  If we did allocate one above, just use
	 * it, since sigprocmask(2) handles the case where `set' and `oset'
	 * point to the same address.
	 */
	if (uap->oset != NULL)
		if (nuap.set == NULL) {
			nuap.oset = umem_alloc(sizeof *nuap.oset);
			if (nuap.oset == NULL) {
				error = ENOMEM;
				goto cleanup;
			}
		} else
			nuap.oset = nuap.set;

	if (error = sigprocmask(&nuap, rvp))
		goto cleanup;

	if (nuap.oset != NULL) {
		if (copyin((caddr_t)nuap.oset,
			   (caddr_t)&usl_set, sizeof usl_set)) {
			/*
			 *+ sigprocmask_isc copyin failed
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "sigprocmask",nuap.oset);
			error = EFAULT;
			goto cleanup;
		}

		/* Convert USL sigset to the ISC format */
		isc_set = usl_set.sa_sigbits[0];
		temp_isc_set = usl_set.sa_sigbits[0];

		/* Convert 
		 * SIGCONT to ISC_SIGCONT, 
		 * SIGSTOP to ISC_SIGSTOP 
		 * SIGTSTP to ISC_SIGTSTP
		 */

		sigdelset(&isc_set, SIGCONT);
		sigdelset(&isc_set, SIGSTOP);
		sigdelset(&isc_set, SIGTSTP);

		if (sigismember(&temp_isc_set, SIGCONT))
			sigaddset(&isc_set, ISC_SIGCONT);
		if (sigismember(&temp_isc_set, SIGSTOP))
			sigaddset(&isc_set, ISC_SIGSTOP);
		if (sigismember(&temp_isc_set, SIGTSTP))
			sigaddset(&isc_set, ISC_SIGTSTP);

		if (copyout((caddr_t)&isc_set,
			    (caddr_t)uap->oset, sizeof isc_set))
			error = EFAULT;
	}

 cleanup:
	if (nuap.set != NULL)
		umem_free(nuap.set, sizeof *nuap.set);
	else if (nuap.oset != NULL)
		umem_free(nuap.oset, sizeof *nuap.oset);

	return error;
}

/*
 * Support for ISC implementation of sigpending(2)
 */
struct isc_sigpendinga {
	isc_sigset_t	*set;
};

struct sigpendinga {
	int		flag;
	sigset_t	*set;
};

/*
 * int sigpending_isc(struct isc_sigpendinga *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
sigpending_isc(struct isc_sigpendinga *uap, rval_t *rvp)
{
	isc_sigset_t		isc_set;
	isc_sigset_t		temp_isc_set;
	sigset_t		usl_set;
	struct sigpendinga	nuap;
	int			error = 0;

	nuap.flag = 1;			/* Ask for sigpending, not sigfillset*/
	nuap.set = umem_alloc(sizeof *nuap.set);
	if (nuap.set == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	if (error = sigpending(&nuap, rvp))
		goto cleanup;;

	if (copyin((caddr_t)nuap.set, (caddr_t)&usl_set, sizeof usl_set)) {
		/*
		 *+ sigpending_isc copyin failed
		 */
		cmn_err(CE_WARN, K_FAULT_ERR, "sigpending", nuap.set);
		error = EFAULT;
		goto cleanup;
	}

	/* Convert USL sigset to the ISC format */
	isc_set = usl_set.sa_sigbits[0];
	temp_isc_set = usl_set.sa_sigbits[0];

	/* Convert 
	 * SIGCONT to ISC_SIGCONT, 
	 * SIGSTOP to ISC_SIGSTOP 
	 * SIGTSTP to ISC_SIGTSTP
	 */

	sigdelset(&isc_set, SIGCONT);
	sigdelset(&isc_set, SIGSTOP);
	sigdelset(&isc_set, SIGTSTP);

	if (sigismember(&temp_isc_set, SIGCONT))
		sigaddset(&isc_set, ISC_SIGCONT);
	if (sigismember(&temp_isc_set, SIGSTOP))
		sigaddset(&isc_set, ISC_SIGSTOP);
	if (sigismember(&temp_isc_set, SIGTSTP))
		sigaddset(&isc_set, ISC_SIGTSTP);

	if (copyout((caddr_t)&isc_set, (caddr_t)uap->set, sizeof isc_set))
		error = EFAULT;

 cleanup:
	if (nuap.set != NULL)
		umem_free(nuap.set, sizeof *nuap.set);

	return error;
}


/*
 * Support for ISC implementation of getgroups(2)
 */
struct isc_getgroupsa {
	int		gidsetsize;
	isc_gid_t	*gidset;
};

struct getgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

/*
 * int getgroups_isc(struct isc_getgroupsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
getgroups_isc(struct isc_getgroupsa *uap, rval_t *rvp)
{
	int			gidsetsize = uap->gidsetsize;
	size_t			isc_size = gidsetsize * sizeof(isc_gid_t);
	isc_gid_t		*isc_set = NULL;
	size_t			usl_size = gidsetsize * sizeof(gid_t);
	gid_t			*usl_set = NULL;
	struct getgroupsa	nuap;
	int			error = 0;

	nuap.gidsetsize = gidsetsize;

	if (gidsetsize > 0) {
		nuap.gidset = umem_alloc(usl_size);
		if ( nuap.gidset == NULL) {
			error = ENOMEM;
			goto cleanup;
		}
	}

	if (error = getgroups(&nuap, rvp))
		goto cleanup;

	if (gidsetsize > 0) {
		register isc_gid_t	*isc_setp;
		register gid_t		*usl_setp;
		register int		i;

		usl_set = (gid_t *)kmem_alloc(usl_size, KM_SLEEP);
		usl_setp = usl_set;

		if (copyin((caddr_t)nuap.gidset, (caddr_t)usl_set, usl_size)) {
			/*
			 *+ getgroups_isc copyin failed
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "getgroups",nuap.gidset);
			error = EFAULT;
			goto cleanup;
		}

		isc_set = (isc_gid_t *)kmem_alloc(isc_size, KM_SLEEP);
		isc_setp = isc_set;

		for (i = 0; i < gidsetsize; ++i)
			*isc_setp++ = *usl_setp++;

		if (copyout((caddr_t)isc_set, (caddr_t)uap->gidset, isc_size))
			error = EFAULT;
	}

 cleanup:
	if (nuap.gidset != NULL)
		umem_free(nuap.gidset, usl_size);

	if (isc_set != NULL)
		kmem_free(isc_set, isc_size);

	if (usl_set != NULL)
		kmem_free(usl_set, usl_size);

	return error;
}

/*
 * Support for ISC implementation of setgroups(2)
 */
struct isc_setgroupsa {
	int		gidsetsize;
	isc_gid_t	*gidset;
};

struct setgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

/*
 * int setgroups_isc(struct isc_setgroupsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
setgroups_isc(struct isc_setgroupsa *uap, rval_t *rvp)
{
	int			gidsetsize = uap->gidsetsize;
	size_t			isc_size = gidsetsize * sizeof(isc_gid_t);
	isc_gid_t		*isc_set = NULL;
	size_t			usl_size = gidsetsize * sizeof(gid_t);
	gid_t			*usl_set = NULL;
	struct setgroupsa	nuap;
	int			error = 0;

	if (gidsetsize > ngroups_max)
		return EINVAL;

	nuap.gidsetsize = gidsetsize;

	if (gidsetsize > 0) {
		register isc_gid_t	*isc_setp;
		register gid_t		*usl_setp;
		register int i;

		isc_set = (isc_gid_t *)kmem_alloc(isc_size, KM_SLEEP);
		isc_setp = isc_set;

		if (copyin((caddr_t)uap->gidset, (caddr_t)isc_set, isc_size)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.gidset = umem_alloc(usl_size);
		if (nuap.gidset == NULL) {
			error = ENOMEM;
			goto cleanup;
		}

		usl_set = (gid_t *)kmem_alloc(usl_size, KM_SLEEP);
		usl_setp = usl_set;

		for (i = 0; i < gidsetsize; ++i)
			*usl_setp++ = *isc_setp++;

		if (copyout((caddr_t)usl_set, (caddr_t)nuap.gidset, usl_size)){
			/*
			 *+ setgroups_isc copyout failed
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "setgroups",nuap.gidset);
			error = EFAULT;
			goto cleanup;
		}
	}

	error = setgroups(&nuap, rvp);

 cleanup:
	if (nuap.gidset != NULL)
		umem_free(nuap.gidset, usl_size);

	if (isc_set != NULL)
		kmem_free(isc_set, isc_size);

	if (usl_set != NULL)
		kmem_free(usl_set, usl_size);

	return error;
}

/*
 * Support for ISC implementation of sysconf(2)
 */
#define ISC_SC_ARG_MAX		1
#define ISC_SC_CHILD_MAX	2
#define ISC_SC_CLK_TCK		3
#define ISC_SC_NGROUPS_MAX	4
#define ISC_SC_OPEN_MAX		5
#define ISC_SC_JOB_CONTROL	6
#define ISC_SC_SAVED_IDS	7
#define ISC_SC_VERSION		8

#define ISC_POSIX_JOB_CONTROL	1
#define ISC_POSIX_SAVED_IDS	1

struct isc_sysconfa {			/* The USL and ISC argument */
	int	which;			/* structures are the same */
};

/*
 * int sysconf_isc(struct isc_sysconfa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
sysconf_isc(struct isc_sysconfa *uap, rval_t *rvp)
{
	extern int	exec_ncargs;	/* Value of ARG_MAX tunable */
	int		error = 0;

	switch(uap->which) {
		case ISC_SC_ARG_MAX:
			rvp->r_val1 = exec_ncargs;
			break;

		case ISC_SC_CHILD_MAX:
			uap->which = _CONFIG_CHILD_MAX;
			error = sysconfig(uap, rvp);
			break;

		case ISC_SC_CLK_TCK:
			uap->which = _CONFIG_CLK_TCK;
			error = sysconfig(uap, rvp);
			break;

		case ISC_SC_NGROUPS_MAX:
			uap->which = _CONFIG_NGROUPS;
			error = sysconfig(uap, rvp);
			break;

		case ISC_SC_OPEN_MAX:
			uap->which = _CONFIG_OPEN_FILES;
			error = sysconfig(uap, rvp);
			break;

		case ISC_SC_JOB_CONTROL:
			rvp->r_val1 = ISC_POSIX_JOB_CONTROL;
			break;

		case ISC_SC_SAVED_IDS:
			rvp->r_val1 = ISC_POSIX_SAVED_IDS;
			break;

		case ISC_SC_VERSION:
			uap->which = _CONFIG_POSIX_VER;
			error = sysconfig(uap, rvp);
			break;

		default:
			error = EINVAL;
			break;
	}

	return error;
}

/*
 * Support for ISC implementation of pathconf(2)
 */

/* Table to convert ISC pathconf args to USL values */
static int	pathconf_conv[] = {
	-1,				/* 0 - Illegal value */
	_PC_LINK_MAX,			/* 1 - No conversion */
	_PC_MAX_CANON,			/* 2 - No conversion */
	_PC_MAX_INPUT,			/* 3 - No conversion */
	_PC_NAME_MAX,			/* 4 - No conversion */
	_PC_PATH_MAX,			/* 5 - No conversion */
	_PC_PIPE_BUF,			/* 6 - No conversion */
	_PC_CHOWN_RESTRICTED,		/* 7 */
	_PC_NO_TRUNC,			/* 8 */
	_PC_VDISABLE			/* 9 */
};

static size_t	pathconf_conv_size = sizeof pathconf_conv / sizeof(int);

struct isc_pathconfa {			/* The USL and ISC argument */
	char*	fname;			/* structures are the same */
	int	name;
};

/*
 * int pathconf_isc(struct isc_pathconfa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
pathconf_isc(struct isc_pathconfa *uap, rval_t *rvp)
{
	if (uap->name < 0 || uap->name >= pathconf_conv_size)
		return EINVAL;

	uap->name = pathconf_conv[uap->name];
	return pathconf(uap, rvp);
}

struct isc_fpathconfa {			/* The USL and ISC argument */
	int	fdes;			/* structures are the same */
	int	name;
};

/*
 * int fpathconf_isc(struct isc_fpathconfa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
fpathconf_isc(struct isc_fpathconfa *uap, rval_t *rvp)
{
	if (uap->name < 0 || uap->name >= pathconf_conv_size)
		return EINVAL;

	uap->name = pathconf_conv[uap->name];
	return fpathconf(uap, rvp);
}

struct isc_fsynca {
	int	fd;
};

/*
 * int fsync_isc(struct isc_fsynca *uap, rval_t *rvp)
 * 	Support for ISC implementation of fsync(2)
 *
 * Calling/Exit State:
 */
int
fsync_isc(struct isc_fsynca *uap, rval_t *rvp)
{
	return fsync(uap, rvp);
}

struct	isc_waitpida {
	int     pid;
	int    *stat_loc;
	int     options;
};

struct svr4_waitsysa {
	idtype_t idtype;
	id_t	 id;
	siginfo_t *infop;
	int	 options;
};

/*
 * int waitpid_isc(struct isc_waitpida *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
waitpid_isc(struct isc_waitpida *uap, rval_t *rvp)
{
	int			options;
	int			pid;

	struct svr4_waitsysa	nuap;
	idtype_t		idtype;
	id_t			id;
	siginfo_t		*infop;
	int			rval;
	

	/* Validate options: only these bits may be set */
	if (uap->options & ~(ISC_WNOHANG | ISC_WUNTRACED))
		return(EINVAL);

	nuap.options = (WEXITED|WTRAPPED);
	options = uap->options;
	if (options & ISC_WNOHANG)
		nuap.options |= WNOHANG;
	if (options & ISC_WUNTRACED)
		nuap.options |= WUNTRACED;

	pid = uap->pid;
	if (pid > 0) {
		idtype = P_PID;
		id = pid;
	} else if (pid < -1) {
		idtype = P_PGID;
		id = -pid;
	} else if (pid == -1) {
		idtype = P_ALL;
		id = 0;
	} else {
		idtype = P_PGID;
		id = u.u_procp->p_pgid;
	}
	
	infop = umem_alloc(sizeof *infop);
	if (infop == NULL)
		return(ENOMEM);

	nuap.idtype = idtype;
	nuap.id = id;
	nuap.infop = infop;
	rval = waitsys(&nuap, rvp);

	if (!rval) {
		rvp->r_val1 = infop->si_pid;
		rvp->r_val2 = wstat_isc(infop->si_code, infop->si_status);
	
		if (uap->stat_loc) {
			if (suword(uap->stat_loc, rvp->r_val2) == -1)
				rval = EFAULT;
		
		}
	}

	umem_free(infop, sizeof *infop);
	return(rval);
}

/*
 * STATIC int wstat_isc(int code, int data)
 * 	convert code/data pair into old style wait status
 *
 * Calling/Exit State:
 */
STATIC int
wstat_isc(int code, int data)
{
	int stat = (data & 0377);

	switch (code) {
		case CLD_EXITED:
			stat <<= 8;
			break;
		case CLD_DUMPED:
			stat |= WCOREFLG;
			break;
		case CLD_KILLED:
			break;
		case CLD_TRAPPED:
		case CLD_STOPPED:
			stat <<= 8;
			stat |= WSTOPFLG;
			break;
		case CLD_CONTINUED:
			stat = WCONTFLG;
			break;
	}
	return stat;
}

struct svr4_setsida {
	int	flag;
	int	pid;
	int	pgid;
};

/*
 * int setsid_isc(int *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
setsid_isc(int *uap, rval_t *rvp)
{
	struct svr4_setsida	nuap;

	/* Want the Base Code (setpgrp()) to be able to detect
	 * that call is not a SCO request. 
	 */

	if (!ISC_USES_POSIX)
		return(EINVAL);

	nuap.flag = 3;	/* SVR4 setsid() */

	return (setpgrp(&nuap, rvp));
}

struct isc_setpgida {
	int	pid;
	int	pgid;
};

struct svr4_setpgida {
	int	flag;
	int	pid;
	int	pgid;
};

/*
 * int setpgid_isc(struct isc_setpgida *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
setpgid_isc(struct isc_setpgida *uap, rval_t *rvp)
{
	struct svr4_setpgida	nuap;

	/* Want the Base Code (setpgrp()) to be able to detect
	 * that call is not a SCO request. 
	 */

	if (!ISC_USES_POSIX)
		return(EINVAL);

	nuap.flag = 5;	/* SVR4 setpgid() */
	nuap.pid = uap->pid;
	nuap.pgid = uap->pgid;

	return (setpgrp(&nuap, rvp));
}

/* End Enhanced Application Compatibility Support */
