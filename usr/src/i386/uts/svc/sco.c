#ident	"@(#)kern-i386:svc/sco.c	1.5.4.1"
#ident	"$Header$"

/* Enhanced Application Compatibility Support */

#include <io/poll.h>
#include <io/termios.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/procset.h>
#include <proc/regset.h>
#include <proc/unistd.h>
#include <proc/user.h>
#include <proc/wait.h>
#include <svc/errno.h>
#include <svc/limitctl.h>
#include <svc/psm.h>
#include <svc/sco.h>
#include <svc/sysconfig.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#include <util/engine.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * SCODATE should be defined in svc.mk
 * This is here to make lint happy.
 */
#ifndef SCODATE
#define SCODATE		"undefined"
#endif

char *scodate = SCODATE;
extern char *o_bustype;

extern void *umem_alloc(size_t);
extern void umem_free(void *, size_t);

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
extern int poll();
extern int access();
extern int symlink();
extern int lstat();
extern int readlink();
extern int setgroups();
extern int getgroups();

#if 0
/* Test to use a stack address below current location */
int skip_boundary = 16;
#endif

struct sco_selectarg {
	struct pollfd	pfd_arg[FD_SETSIZE];
};

struct sco_sigaction {			/* SCO version of struct sigaction */
	union {
		void (*sa__handler)();
	} sa_u;
	sco_sigset_t sa_mask;
	int sa_flags;
};

#define K_FAULT_ERR	"%s: generated bad user address (%#x)\n"

/*
 * Support for SCO implementation of sigaction(2)
*/
#define SCO_SA_NOCLDSTOP	1	/* SCO's value for SA_NOCLDSTOP */

struct sco_sigactiona {
	int			sig;
	struct sco_sigaction	*act;
	struct sco_sigaction	*oact;
};

struct sigactiona {
	int			sig;
	struct sigaction	*act;
	struct sigaction	*oact;
};


STATIC int sco_errno_xlate[] = {

0,	1,	2,	3,	4,	5,	6,	7,	8,	9,
10,	11,	12,	13,	14,	15,	16,	17,	18,	19,
20,	21,	22,	23,	24,	25,	26,	27,	28,	29,
30,	31,	32,	33,	34,	35,	36,	37,	38,	39,
40,	41,	42,	43,	44,	45,	46,	47,	48,	49,
50,	51,	52,	53,	54,	55,	56,	57,	58,	EBUSY,
60,	61,	62,	63,	64,	65,	66,	67,	68,	69,
70,	71,	72,	73,	74,	75,	76,	77,	78,	79,
80,	81,	82,	83,	84,	85,	86,	87,	88,	89,
OELOOP,	OERESTART,	OESTRPIPE,	OENOTEMPTY,	ENOMEM,	OENOTSOCK,	OEDESTADDRREQ,	OEMSGSIZE,	OEPROTOTYPE,	OENOPROTOOPT,
100,	101,	102,	103,	104,	105,	106,	107,	108,	109,
110,	111,	112,	113,	114,	115,	116,	117,	118,	119,
OEPROTONOSUPPORT,	OESOCKTNOSUPPORT,	OEOPNOTSUPP,	OEPFNOSUPPORT,	OEAFNOSUPPORT,	OEADDRINUSE,	OEADDRNOTAVAIL,	OENETDOWN,	OENETUNREACH,	OENETRESET,
OECONNABORTED,	OECONNRESET,	OENOBUFS,	OEISCONN,	OENOTCONN,	135,	136,	137,	138,	139,
140,	141,	142,	OESHUTDOWN,	OETOOMANYREFS,	OETIMEDOUT,	OECONNREFUSED,	OEHOSTDOWN,	OEHOSTUNREACH,	OEALREADY,
OEINPROGRESS,	151,	EFAULT,	EFAULT,	EFAULT,	EFAULT,	EFAULT,	EFAULT,	EINTR,	159,
EACCES,	161,	162,	163,	164,	165,	166,	167,	168,	169,
OEWOULDBLOCK,	171,	172,	173,	174,	175,	176,	177,	178,	179,
180,	181,	182,	183,	184,	185,	186,	187,	188,	189,
190,	191,	192,	193,	194,	195,	196,	197,	198,	199,
ENXIO,	ENXIO,	ENXIO,	ENXIO,	204,	205,	206,	207,	208,	209,
ENXIO,	ENXIO,	ENXIO,	ENXIO,	ENXIO,	ENXIO,	ENXIO,	ENXIO,	ENXIO,	ENXIO,
ENXIO,	ENXIO,	ENXIO,	ENXIO,	224,	225,	226,	227,	228,	229,
230,	231,	232,	233,	234,	235,	236,	237,	238,	239,
240,	241,	242,	243,	244,	245,	246,	247,	248,	249,
250,	251,	252,	253,	254,	255,	256,	257,	258,	259,
260,	261,	262,	263,	264,	265,	266,	267,	268,	269,
270,	271,	272,	273,	274,	275,	276,	277,	278,	279,
280,	281,	282,	283,	284,	285,	286,	287,	288,	289,
290,	291,	292,	293,	294,	295,	296,	297,	298,	299,
};

/*
 * int sigaction_sco(struct sco_sigactiona *uap, rval_t *rvp)
 *
 * Description:
 *
 * Since the SCO and USL definitions of struct sigaction differ, this
 * function has to do translation before and after calling
 * scalls.c:sigaction().  The basic steps are (assuming `act' and `oact'
 * are both non-NULL):
 *	1. Copy in SCO-style structure.
 *	2. Allocate *user* memory to hold USL-style sigaction struct.
 *	3. Translate SCO format to USL.
 *	4. Call real sigaction.
 *	5. Translate old sigaction structure to SCO format.
 *	6. Copy out SCO-style structure.
 *
 * Calling/Exit State:
 */

/* ARGSUSED */
int
sigaction_sco(struct sco_sigactiona *uap, rval_t *rvp)
{
	struct sco_sigaction	sco_act;
	struct sigaction	usl_act;
	struct sigactiona	nuap;
	int			error = 0;
	int			flags;

	nuap.sig = uap->sig;
	nuap.act = NULL;
	nuap.oact = NULL;

	if (uap->act != NULL) {
		if (copyin((caddr_t)uap->act,
			   (caddr_t)&sco_act, sizeof sco_act)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.act = (struct sigaction *)umem_alloc(sizeof *nuap.act);
		if (nuap.act == NULL) {
			error = ENOMEM;
			goto cleanup;
		}

		/* Translate SCO sigaction structure to the USL format */
		flags = sco_act.sa_flags & ~(SCO_SA_NOCLDSTOP); 
		flags |= sco_act.sa_flags & SCO_SA_NOCLDSTOP ?
					SA_NOCLDSTOP : 0;
		usl_act.sa_flags = flags;

		usl_act.sa_handler = sco_act.sa_u.sa__handler;
		usl_act.sa_mask.sa_sigbits[0] = sco_act.sa_mask;
		usl_act.sa_mask.sa_sigbits[1] = 0;
		usl_act.sa_mask.sa_sigbits[2] = 0;
		usl_act.sa_mask.sa_sigbits[3] = 0;

		if (copyout((caddr_t)&usl_act,
			    (caddr_t)nuap.act, sizeof usl_act)) {
			/*
			 *+ copyout failed.
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
			nuap.oact =
			    (struct sigaction *)umem_alloc(sizeof *nuap.oact);
			if (nuap.oact == NULL) {
				error = ENOMEM;
				goto cleanup;
			}
		} else
			nuap.oact = nuap.act;

	if (error = sigaction(&nuap, rvp))
		goto cleanup;

	/*
	 * The SCO implementation of sigaction(2) expects the old method of
	 * returning from a handler, so we set it up here.
	 */
	u.u_procp->p_sigreturn = (void (*)())u.u_ar0[T_EDX];
	u.u_procp->p_sigstate[nuap.sig - 1].sst_rflags |= SST_OLDSIG;

	if (nuap.oact != NULL) {
		if (copyin((caddr_t)nuap.oact, 
			   (caddr_t)&usl_act, sizeof usl_act)) {
			/*
			 *+ copyin failed.
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "sigaction", nuap.oact);
			error = EFAULT;
			goto cleanup;
		}

		/* Translate USL sigaction structre to the SCO format */
		sco_act.sa_flags = usl_act.sa_flags & SA_NOCLDSTOP ?
					SCO_SA_NOCLDSTOP : 0;
		sco_act.sa_u.sa__handler = usl_act.sa_handler;
		sco_act.sa_mask = usl_act.sa_mask.sa_sigbits[0];

		if (copyout((caddr_t)&sco_act,
			    (caddr_t)uap->oact, sizeof sco_act))
			error = EFAULT;
	}

 cleanup:
	if (nuap.act != NULL)
		umem_free((void *)nuap.act, sizeof *nuap.act);
	else if (nuap.oact != NULL)
		umem_free((void *)nuap.oact, sizeof *nuap.oact);

	return error;
}

/*
 * Support for SCO implementation of sigprocmask(2)
*/
#define SCO_SIG_SETMASK	0

struct sco_sigprocmaska {
	int		how;
	sco_sigset_t	*set;
	sco_sigset_t	*oset;
};

struct sigprocmaska {
	int		how;
	sigset_t	*set;
	sigset_t	*oset;
};

/*
 * int sigprocmask_sco(struct sco_sigprocmaska *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
sigprocmask_sco(struct sco_sigprocmaska *uap, rval_t *rvp)
{
	sco_sigset_t		sco_set;
	sigset_t		usl_set;
	struct sigprocmaska	nuap;
	int			error = 0;

	/* USL and SCO have different definitions of SIG_SETMASK */
	nuap.how = uap->how == SCO_SIG_SETMASK ? SIG_SETMASK : uap->how;
	nuap.set = NULL;
	nuap.oset = NULL;

	if (uap->set != NULL) {
		if (copyin((caddr_t)uap->set,
			   (caddr_t)&sco_set, sizeof sco_set)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.set = (sigset_t *)umem_alloc(sizeof *nuap.set);
		if (nuap.set == NULL) {
			error = ENOMEM;
			goto cleanup;
		}

		/* Convert SCO sigset to the USL format */
		usl_set.sa_sigbits[0] = sco_set;
		usl_set.sa_sigbits[1] = 0;
		usl_set.sa_sigbits[2] = 0;
		usl_set.sa_sigbits[3] = 0;

		if (copyout((caddr_t)&usl_set,
			    (caddr_t)nuap.set, sizeof usl_set)) {
			/*
			 *+ copyout failed.
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
			nuap.oset = (sigset_t *)umem_alloc(sizeof *nuap.oset);
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
			 *+ copyin failed.
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "sigprocmask",nuap.oset);
			error = EFAULT;
			goto cleanup;
		}

		/* Convert USL sigset to the SCO format */
		sco_set = usl_set.sa_sigbits[0];

		if (copyout((caddr_t)&sco_set,
			    (caddr_t)uap->oset, sizeof sco_set))
			error = EFAULT;
	}

 cleanup:
	if (nuap.set != NULL)
		umem_free((void *)nuap.set, sizeof *nuap.set);
	else if (nuap.oset != NULL)
		umem_free((void *)nuap.oset, sizeof *nuap.oset);

	return error;
}

/*
 * Support for SCO implementation of sigpending(2)
*/
struct sco_sigpendinga {
	sco_sigset_t	*set;
};

struct sigpendinga {
	int		flag;
	sigset_t	*set;
};

/*
 * int sigpending_sco(struct sco_sigpendinga *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
sigpending_sco(struct sco_sigpendinga *uap, rval_t *rvp)
{
	sco_sigset_t		sco_set;
	sigset_t		usl_set;
	struct sigpendinga	nuap;
	int			error = 0;

	nuap.flag = 1;			/* Ask fo sigpending, not sigfillset */
	nuap.set = (sigset_t *)umem_alloc(sizeof *nuap.set);
	if (nuap.set == NULL) {
		error = ENOMEM;
		goto cleanup;
	}
		
	if (error = sigpending(&nuap, rvp))
		goto cleanup;;

	if (copyin((caddr_t)nuap.set, (caddr_t)&usl_set, sizeof usl_set)) {
		/*
		 *+ copyin failed.
		 */
		cmn_err(CE_WARN, K_FAULT_ERR, "sigpending", nuap.set);
		error = EFAULT;
		goto cleanup;
	}

	/* Convert USL sigset to the SCO format */
	sco_set = usl_set.sa_sigbits[0];

	if (copyout((caddr_t)&sco_set, (caddr_t)uap->set, sizeof sco_set))
		error = EFAULT;

 cleanup:
	if (nuap.set != NULL)
		umem_free((void *)nuap.set, sizeof *nuap.set);

	return error;
}

/*
 * Support for SCO implementation of sigsuspend(2)
*/
struct sco_sigsuspenda {
	sco_sigset_t	*set;
};

struct sigsuspenda {
	sigset_t	*set;
};

/*
 * int sigsuspend_sco(struct sco_sigsuspenda *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
sigsuspend_sco(struct sco_sigsuspenda *uap, rval_t *rvp)
{
	sco_sigset_t		sco_set;
	sigset_t		usl_set;
	struct sigsuspenda	nuap;
	int			error = 0;

	if (copyin((caddr_t)uap->set, (caddr_t)&sco_set, sizeof sco_set)) {
		error = EFAULT;
		goto cleanup;
	}

	nuap.set = (sigset_t *)umem_alloc(sizeof *nuap.set);
	if (nuap.set == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Convert SCO sigset to the USL format */
	usl_set.sa_sigbits[0] = sco_set;
	usl_set.sa_sigbits[1] = 0;
	usl_set.sa_sigbits[2] = 0;
	usl_set.sa_sigbits[3] = 0;

	if (copyout((caddr_t)&usl_set, (caddr_t)nuap.set, sizeof usl_set)) {
		/*
		 *+ copyout failed.
		 */
		cmn_err(CE_WARN, K_FAULT_ERR, "sigsuspend", nuap.set);
		error = EFAULT;
		goto cleanup;
	}

	error = sigsuspend(&nuap, rvp);

	/* NOTREACHED */
 cleanup:
	if (nuap.set != NULL)
		umem_free((void *)nuap.set, sizeof *nuap.set);

	return error;
}


/*
 * Support for SCO implementation of getgroups(2)
*/
struct sco_getgroupsa {
	int		gidsetsize;
	sco_gid_t	*gidset;
};

struct getgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

/*
 * int getgroups_sco(struct sco_getgroupsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
getgroups_sco(struct sco_getgroupsa *uap, rval_t *rvp)
{
	int			gidsetsize = uap->gidsetsize;
	size_t			sco_size = gidsetsize * sizeof(sco_gid_t);
	sco_gid_t		*sco_set = NULL;
	size_t			usl_size = gidsetsize * sizeof(gid_t);
	gid_t			*usl_set = NULL;
	struct getgroupsa	nuap;
	int			error = 0;

	if (gidsetsize > ngroups_max)
		return EINVAL;

	nuap.gidsetsize = gidsetsize;

	if (gidsetsize > 0) {
		nuap.gidset = (gid_t *)umem_alloc(usl_size);
		if ( nuap.gidset == NULL) {
			error = ENOMEM;
			goto cleanup;
		}
	}

	if (error = getgroups(&nuap, rvp))
		goto cleanup;

	if (gidsetsize > 0) {
		sco_gid_t	*sco_setp;
		gid_t		*usl_setp;
		int		i;

		usl_set = (gid_t *)kmem_alloc(usl_size, KM_SLEEP);
		usl_setp = usl_set;

		if (copyin((caddr_t)nuap.gidset, (caddr_t)usl_set, usl_size)) {
			/*
			 *+ copyin failed.
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "getgroups",nuap.gidset);
			error = EFAULT;
			goto cleanup;
		}

		sco_set = (sco_gid_t *)kmem_alloc(sco_size, KM_SLEEP);
		sco_setp = sco_set;

		for (i = 0; i < gidsetsize; ++i)
			*sco_setp++ = *usl_setp++;

		if (copyout((caddr_t)sco_set, (caddr_t)uap->gidset, sco_size))
			error = EFAULT;
	}

 cleanup:
	if (nuap.gidset != NULL)
		umem_free((void *)nuap.gidset, sizeof usl_size);

	if (sco_set != NULL)
		kmem_free((void *)sco_set, sco_size);

	if (usl_set != NULL)
		kmem_free((void *)usl_set, usl_size);

	return error;
}

/*
 * Support for SCO implementation of setgroups(2)
*/
struct sco_setgroupsa {
	int		gidsetsize;
	sco_gid_t	*gidset;
};

struct setgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

/*
 * int setgroups_sco(struct sco_setgroupsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
setgroups_sco(struct sco_setgroupsa *uap, rval_t *rvp)
{
	int			gidsetsize = uap->gidsetsize;
	size_t			sco_size = gidsetsize * sizeof(sco_gid_t);
	sco_gid_t		*sco_set = NULL;
	size_t			usl_size = gidsetsize * sizeof(gid_t);
	gid_t			*usl_set = NULL;
	struct setgroupsa	nuap;
	int			error = 0;

	if (gidsetsize > ngroups_max)
		return EINVAL;

	nuap.gidsetsize = gidsetsize;

	if (gidsetsize > 0) {
		sco_gid_t	*sco_setp;
		gid_t		*usl_setp;
		int i;

		sco_set = (sco_gid_t *)kmem_alloc(sco_size, KM_SLEEP);
		sco_setp = sco_set;

		if (copyin((caddr_t)uap->gidset, (caddr_t)sco_set, sco_size)) {
			error = EFAULT;
			goto cleanup;
		}

		nuap.gidset = (gid_t *)umem_alloc(usl_size);
		if (nuap.gidset == NULL) {
			error = ENOMEM;
			goto cleanup;
		}

		usl_set = (gid_t *)kmem_alloc(usl_size, KM_SLEEP);
		usl_setp = usl_set;

		for (i = 0; i < gidsetsize; ++i)
			*usl_setp++ = *sco_setp++;

		if (copyout((caddr_t)usl_set, (caddr_t)nuap.gidset, usl_size)){
			/*
			 *+ copyout failed.
			 */
			cmn_err(CE_WARN, K_FAULT_ERR, "setgroups",nuap.gidset);
			error = EFAULT;
			goto cleanup;
		}
	}

	error = setgroups(&nuap, rvp);

 cleanup:
	if (nuap.gidset != NULL)
		umem_free((void *)nuap.gidset, usl_size);

	if (sco_set != NULL)
		kmem_free((void *)sco_set, sco_size);

	if (usl_set != NULL)
		kmem_free((void *)usl_set, usl_size);

	return error;
}

/*
 * Support for SCO implementation of sysconf(2)
*/
#define SCO_SC_ARG_MAX		0
#define SCO_SC_CHILD_MAX	1
#define SCO_SC_CLK_TCK		2
#define SCO_SC_NGROUPS_MAX	3
#define SCO_SC_OPEN_MAX		4
#define SCO_SC_JOB_CONTROL	5
#define SCO_SC_SAVED_IDS	6
#define SCO_SC_VERSION		7
#define SCO_SC_PASS_MAX		8
#define SCO_SC_XOPEN_VERSION	9

#define SCO_SC_PAGESIZE		34

#define SCO_POSIX_JOB_CONTROL	1
#define SCO_POSIX_SAVED_IDS	1
#define SCO_PASS_MAX		8

struct sco_sysconfa {			/* The USL and SCO argument */
	int	which;			/* structures are the same */
};

/*
 * int sysconf_sco(struct sco_sysconfa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
sysconf_sco(struct sco_sysconfa *uap, rval_t *rvp)
{
	extern int	exec_ncargs;	/* Value of ARG_MAX tunable */
	int		error = 0;

	switch(uap->which) {
		case SCO_SC_ARG_MAX:
			rvp->r_val1 = exec_ncargs;
			break;

		case SCO_SC_CHILD_MAX:
			uap->which = _CONFIG_CHILD_MAX;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_CLK_TCK:
			uap->which = _CONFIG_CLK_TCK;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_NGROUPS_MAX:
			uap->which = _CONFIG_NGROUPS;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_OPEN_MAX:
			uap->which = _CONFIG_OPEN_FILES;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_JOB_CONTROL:
			rvp->r_val1 = SCO_POSIX_JOB_CONTROL;
			break;

		case SCO_SC_SAVED_IDS:
			rvp->r_val1 = SCO_POSIX_SAVED_IDS;
			break;

		case SCO_SC_VERSION:
			uap->which = _CONFIG_POSIX_VER;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_PASS_MAX:
			rvp->r_val1 = SCO_PASS_MAX;
			break;

		case SCO_SC_XOPEN_VERSION:
			uap->which = _CONFIG_XOPEN_VER;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_PAGESIZE:
			uap->which = _CONFIG_PAGESIZE;
			error = sysconfig(uap, rvp);
			break;

		default:
			error = EINVAL;
			break;
	}

	return error;
}

/*
 * Support for SCO implementation of pathconf(2)
*/

/* Table to convert SCO pathconf args to USL values */
static int	pathconf_conv[] = {
	_PC_LINK_MAX,
	_PC_MAX_CANON,
	_PC_MAX_INPUT,
	_PC_NAME_MAX,
	_PC_PATH_MAX,
	_PC_PIPE_BUF,
	_PC_CHOWN_RESTRICTED,
	_PC_NO_TRUNC,			/* No conversion necessary */
	_PC_VDISABLE,			/* No conversion necessary */
};

static size_t	pathconf_conv_size = sizeof(pathconf_conv) / sizeof(int);

struct sco_pathconfa {			/* The USL and SCO argument */
	char*	fname;			/* structures are the same */
	int	name;
};

/*
 * int pathconf_sco(struct sco_pathconfa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
pathconf_sco(struct sco_pathconfa *uap, rval_t *rvp)
{
	if (uap->name < 0 || uap->name >= pathconf_conv_size)
		return EINVAL;

	uap->name = pathconf_conv[uap->name];
	return pathconf(uap, rvp);
}

struct sco_fpathconfa {			/* The USL and SCO argument */
	int	fdes;			/* structures are the same */
	int	name;
};

/*
 * int fpathconf_sco(struct sco_fpathconfa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
fpathconf_sco(struct sco_fpathconfa *uap, rval_t *rvp)
{
	if (uap->name < 0 || uap->name >= pathconf_conv_size)
		return EINVAL;

	uap->name = pathconf_conv[uap->name];
	return fpathconf(uap, rvp);
}

/*
 * Support for SCO implementation of rename(2)
*/
struct sco_renamea {
	char	*from;
	char	*to;
};

/*
 * int rename_sco(struct sco_renamea *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
rename_sco(struct sco_renamea *uap, rval_t *rvp)
{
	return rename(uap, rvp);
}


struct sco_selecta {
		int		nfds;
		fd_set		*in0;
		fd_set		*ou0;
		fd_set		*ex0;
		struct timeval	*tv;
		
};

struct polla {
	struct pollfd	*fdp;
	unsigned long	nfds;
	long		timo;
};

/*
 * int select_sco(struct sco_selecta *uap, rval_t *rvp)
 * 	Emulation of select() system call using poll() system call.
 *
 * Assumptions:
 *	polling for input only is most common.
 *	polling for exceptional conditions is very rare.
 *
 * Note that is it not feasible to emulate all error conditions,
 * in particular conditions that would return EFAULT are far too
 * difficult to check for in a library routine.
 *
 * Calling/Exit State:
 */
int
select_sco(struct sco_selecta *uap, rval_t *rvp)
{

	long 		*in, *out, *ex;
	u_long 		m;	/* bit mask */
	u_long 		b;	/* bits to test */
	int 		j, n, ms;
	struct pollfd	*pfd;
	struct pollfd	*p;
	int 		lastj = -1;
	fd_set		zero;
	int		rval;
	int		num_set;
	int		nfds;
	int		size;
	struct pollfd	*uargs;
	int		uargsz;
	fd_set		*in0;
	fd_set		*out0;
	fd_set		*ex0;
	struct timeval	*tv;
	struct	timeval	tv_arg;
	struct	polla	nuap;

	nfds = uap->nfds;
        if (nfds <= 0)
                return(EINVAL);

	bzero((caddr_t)&zero, sizeof(fd_set));

	/* Get User Space */
	uargsz =  nfds * sizeof(struct pollfd);
	uargs = (struct pollfd *)umem_alloc(uargsz);
	if (uargs == NULL) {
		/*
		 *+ Kernel could not allocate memory in user space
		 */
		cmn_err(CE_WARN, "select_sco: Could not allocate user space\n");
		return(EFAULT);
	}

	/*
	 * If any input args are null, point them at the null array.
	 *
	 * Copyin the users Select Arguments into Kernel Space and 
	 * setup the Kernel space arguments
 	 */ 
	size = howmany(nfds, NFDBITS) * sizeof(fd_mask);
	pfd = (struct pollfd *)
	      kmem_zalloc(sizeof(struct pollfd)*nfds,KM_SLEEP);
	p = pfd;
	rval = 0;
	in0 = out0 = ex0 = NULL;
	if (uap->in0) {
		in0 = kmem_alloc(size, KM_SLEEP);
		rval = copyin((caddr_t)uap->in0, (caddr_t)in0, size);
	} else
		in0 = &zero;
		
	if (!rval && uap->ou0) {
		out0 = kmem_alloc(size, KM_SLEEP);
		rval = copyin((caddr_t)uap->ou0, (caddr_t)out0, size);
	} else
		out0 = &zero;

	if (!rval && uap->ex0) {
		ex0 = kmem_alloc(size, KM_SLEEP);
		rval = copyin((caddr_t)uap->ex0, (caddr_t)ex0, size);
	} else
		ex0 = &zero;


	if (!rval && uap->tv) {
		rval = copyin((caddr_t)uap->tv, (caddr_t)&tv_arg, 
			      sizeof(struct timeval));
		tv = &tv_arg;
	} else
		tv = (struct timeval *)NULL;


	if (rval)
		rval = EFAULT;

        /*
         * For each fd, if any bits are set convert them into
         * the appropriate pollfd struct.
         */
        in = (long *)in0->fds_bits;
        out = (long *)out0->fds_bits;
        ex = (long *)ex0->fds_bits;
        for (n = 0; n < nfds; n += NFDBITS) {
                b = (u_long)(*in | *out | *ex);
                for (j = 0, m = 1; b != 0; j++, b >>= 1, m <<= 1) {
                        if (b & 1) {
                                p->fd = n + j;
                                if (p->fd >= nfds)
                                        goto done;
                                p->events = 0;
                                if (*in & m)
                                        p->events |= POLLRDNORM;
                                if (*out & m)
                                        p->events |= POLLWRNORM;
                                if (*ex & m)
                                        p->events |= POLLRDBAND;
                                p++;
                        }
                }
                in++;
                out++;
                ex++;
        }
done:
        /*
         * Convert timeval to a number of millseconds.
         * Test for zero cases to avoid doing arithmetic.
         * XXX - this is very complex, is it worth it?
         */
        if (tv == NULL) {
                ms = -1;
        } else if (tv->tv_sec == 0) {
                if (tv->tv_usec == 0) {
                        ms = 0;
                } else if (tv->tv_usec < 0 || tv->tv_usec > 1000000) {
                        rval = EINVAL;
                } else {
                        /*
                         * lint complains about losing accuracy,
                         * but I know otherwise.  Unfortunately,
                         * I can't make lint shut up about this.
                         */
                        ms = (int)(tv->tv_usec / 1000);
                }
        } else if (tv->tv_sec > (MAXINT) / 1000) {
                if (tv->tv_sec > 100000000) {
                        rval = EINVAL;
                } else {
                        ms = MAXINT;
                }
        } else if (tv->tv_sec > 0) {
                /*
                 * lint complains about losing accuracy,
                 * but I know otherwise.  Unfortunately,
                 * I can't make lint shut up about this.
                 */
                ms = (int)((tv->tv_sec * 1000) + (tv->tv_usec / 1000));
        } else {        /* tv_sec < 0 */
                rval = EINVAL;
                return (-1);
        }

        /*
         * Now do the poll.
         */
	if (!rval) {
		n = p - pfd;	/* number of pollfd's */
		rval = copyout((caddr_t)pfd, (caddr_t)uargs, 
			       (sizeof(struct pollfd) * n));
		if (rval) {
			rval = EFAULT;
		}
		else {
			nuap.fdp = uargs;
			nuap.nfds = n;
			nuap.timo = ms;
			rval = poll(&nuap, rvp);
			if (!rval) {
				rval = copyin((caddr_t)uargs,
					      (caddr_t)pfd, 
			       		      (sizeof(struct pollfd) * n));
				if (rval)
					rval = EFAULT;
			}
		}
	}

        if (!rval && (rvp->r_val1 == 0)) {	/* no need to set bit masks */
                /*
                 * Clear out bit masks, just in case.
                 * On the assumption that usually only
                 * one bit mask is set, use three loops.
                 */
                if (in0 != &zero) {
                        in = (long *)in0->fds_bits;
                        for (n = 0; n < nfds; n += NFDBITS)
                                *in++ = 0;
                }
                if (out0 != &zero) {
                        out = (long *)out0->fds_bits;
                        for (n = 0; n < nfds; n += NFDBITS)
                                *out++ = 0;
                }
                if (ex0 != &zero) {
                        ex = (long *)ex0->fds_bits;
                        for (n = 0; n < nfds; n += NFDBITS)
                                *ex++ = 0;
                }
        }

        /*
         * Check for EINVAL error case first to avoid changing any bits
         * if we're going to return an error.
         */

        if (!rval && (rvp->r_val1 > 0)) {
	        for (p = pfd, j = n; j-- > 0; p++) {
	                /*
	                 * select will return EBADF immediately if any fd's
	                 * are bad.  poll will complete the poll on the
	                 * rest of the fd's and include the error indication
	                 * in the returned bits.  This is a rare case so we
	                 * accept this difference and return the error after
	                 * doing more work than select would've done.
	                 */
	                if (p->revents & POLLNVAL) {
	                        rval = EBADF;
	                }
	        }
	}

        /*
         * Convert results of poll back into bits
         * in the argument arrays.
         *
         * We assume POLLRDNORM, POLLWRNORM, and POLLRDBAND will only be set
         * on return from poll if they were set on input, thus we don't
         * worry about accidentally setting the corresponding bits in the
         * zero array if the input bit masks were null.
         *
         * Must return number of bits set, not number of ready descriptors
         * (as the man page says, and as poll() does).
         */
        if (!rval && (rvp->r_val1 > 0)) {
	        num_set = 0;
	        for (p = pfd; n-- > 0; p++) {
	                j = p->fd / NFDBITS;
	                /* have we moved into another word of the bit mask ? */
	                if (j != lastj) {
	                        /* clear all output bits to start with */
	                        in = (long *)&in0->fds_bits[j];
	                        out = (long *)&out0->fds_bits[j];
	                        ex = (long *)&ex0->fds_bits[j];
	                        /*
	                         * In case we made "zero" read-only (e.g., with
	                         * cc -R), avoid actually storing into it.
	                         */
	                        if (in0 != &zero)
	                                *in = 0;
	                        if (out0 != &zero)
	                                *out = 0;
	                        if (ex0 != &zero)
	                                *ex = 0;
	                        lastj = j;
	                }
	                if (p->revents) {
	                        m = 1 << (p->fd % NFDBITS);
	                        if (p->revents & POLLRDNORM) {
	                                *in |= m;
	                                num_set++;
	                        }
	                        if (p->revents & POLLWRNORM) {
	                                *out |= m;
	                                num_set++;
	                        }
	                        if (p->revents & POLLRDBAND) {
	                                *ex |= m;
	                                num_set++;
	                        }
	                        /*
	                         * Only set this bit on return if we asked 
	                         * about input conditions.
	                         */
	                        if ((p->revents & (POLLHUP|POLLERR)) &&
	                            (p->events & POLLRDNORM)) {
					/* wasn't already set */
	                                if ((*in & m) == 0)
	                                        num_set++;
	                                *in |= m;
	                        }
	                        /*
	                         * Only set this bit on return if we asked 
	                         * about output conditions.
	                         */
	                        if ((p->revents & (POLLHUP|POLLERR)) &&
	                            (p->events & POLLWRNORM)) {
					/* wasn't already set */
	                                if ((*out & m) == 0)
	                                        num_set++;
	                                *out |= m;
	                        }
	                }
	        }
		rvp->r_val1 = num_set;
	}

	/* Copyout to the user all of the select data structures */

	if (!rval) {
		if (uap->in0) 
			rval =  copyout((caddr_t)in0, (caddr_t)uap->in0, 
					size);
			
		if (!rval && uap->ou0)
			rval =  copyout((caddr_t)out0, (caddr_t)uap->ou0,
		 			size);
	
		if (!rval && uap->ex0)
			rval = copyout((caddr_t)ex0, (caddr_t)uap->ex0,
					size);
		if (rval) {
			rval = EFAULT;
		}
		
	}

	kmem_free(pfd, sizeof(struct pollfd)*nfds);
	if (in0 && (in0 != &zero))
		kmem_free(in0, size);
	if (out0 && (out0 != &zero))
		kmem_free(out0, size);
	if (ex0 && (ex0 != &zero))
		kmem_free(ex0, size);
		
	umem_free((void *)uargs, uargsz);
	return (rval);
}


struct waita_sco {
	pid_t   pid;
	int     *stat_loc;
	int     opt;
};

/*
 * int wait_sco(struct waita_sco *uap, rval_t *rvp)
 *	Perform i386 SCO binary compatible machinations for wait().
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
wait_sco(struct waita_sco *uap, rval_t *rvp)
{
	int error;
	k_siginfo_t info;
	flags_t *eflgs = (flags_t *)&u.u_ar0[T_EFL];

	if (SCO_DOWAITPID(eflgs)) {
		int	opt;
		idtype_t idtype;
		pid_t	pid;
		id_t	id;

		if (uap->opt & ~(SCO_WNOHANG | SCO_WUNTRACED))
			return(EINVAL);
		opt = WEXITED|WTRAPPED;
		if (uap->opt & SCO_WNOHANG)
			opt |= WNOHANG;
		if (uap->opt & SCO_WUNTRACED)
			opt |= WUNTRACED;

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

		error = waitid(idtype, id, &info, opt);
	} else
		error = waitid(P_ALL, (id_t)0, &info, WEXITED|WTRAPPED);

	if (error)
		return error;
	rvp->r_val1 = info.si_pid;
	rvp->r_val2 = wstat(info.si_code, info.si_status);
	return 0;
}


struct eaccessa {
	char	*fname;
	int	fmode;
};


/*
 * int eaccess_sco(struct eaccessa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
eaccess_sco(struct eaccessa *uap, rval_t *rvp)
{
	uap->fmode |= EFF_ONLY_OK;
	return(access(uap, rvp));
}

struct scoinfoa {
	char	*cbuf;
	int	size;
};

/*
 * int scoinfo(struct scoinfoa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 */
int
scoinfo(struct scoinfoa *uap, rval_t *rvp)
{
	char name[XSYS_NMLN];
	struct scoutsname sconame;
	int len;
	int userlim;
	char *str;

	if (uap->size > sizeof(struct scoutsname))
		return(EINVAL);

	bzero((char *)&sconame, sizeof(sconame));
	bcopy(xutsname.sysname, sconame.sysname, XSYS_NMLN);

	getutsname(xutsname.nodename, name);
	bcopy(name, sconame.nodename, XSYS_NMLN);

	bcopy(xutsname.release, sconame.release, XSYS_NMLN);

	len = strlen(scodate);
	bcopy(scodate, sconame.kernelid,
		MIN(sizeof sconame.kernelid - 1, len));

	bcopy(xutsname.machine, sconame.machine, XSYS_NMLN);

	len = strlen(o_bustype);
	bcopy(o_bustype, sconame.bustype, MIN(XSYS_NMLN - 1, len));

	len = strlen(os_hw_serial);
	bcopy(os_hw_serial, sconame.sysserial,
		MIN(sizeof sconame.sysserial - 1, len));

	sconame.sysorigin = xutsname.sysorigin;
	sconame.sysoem = xutsname.sysoem;

	limit(L_GETUSERLIMIT, &userlim);
	/* 8-digit upper bound prevents overflow of sconame.numuser (ugh) */
	if (userlim <= 0 || userlim > 99999999)
		strcpy(sconame.numuser, "unlim");
	else
		numtos(userlim, sconame.numuser);

	sconame.numcpu = Nengine;

	if (copyout((char *)&sconame, uap->cbuf, uap->size))
		return(EFAULT);

	return(0);
}

/*
 * int coff_errno(int errno)
 *	Map a modern errno into one suitable for a SCO binary.
 *
 * Calling/Exit State:
 */
int
coff_errno(int errno)
{

	return(sco_errno_xlate[errno]);

}

/*
 * int sco_tbd(void)
 *	for unsupported calls, instead of killing the process, return EINVAL.
 *
 * Calling/Exit State:
 */
int
sco_tbd(void)
{
	return(EINVAL);
}

/*
 * int syscall_89(char *, rval_t)
 *	front end for system call #89. if !SCO, it is symlink, if SCO, then
 *	security
 *
 * Calling/Exit State:
 */
int
syscall_89(char *uap, rval_t *rvp)
{
	if(isSCO) {
		return(nopkg());
	}
	else {
		return(symlink(uap, rvp));
	}
}

/*
 * int syscall_90(char *, rval_t)
 *	front end for system call #90. if !SCO, it is readlink, if SCO, then
 *	symlink
 *
 * Calling/Exit State:
 */
int
syscall_90(char *uap, rval_t *rvp)
{
	if(isSCO) {
		return(symlink(uap, rvp));
	}
	else {
		return(readlink(uap, rvp));
	}
}

/*
 * int syscall_91(char *, rval_t)
 *	front end for system call #91. if !SCO, it is setgroups, if SCO, then
 *	lstat
 *
 * Calling/Exit State:
 */
int
syscall_91(char *uap, rval_t *rvp)
{
	if(isSCO) {
		return(lstat(uap, rvp));
	}
	else {
		return(setgroups(uap, rvp));
	}
}

/*
 * int syscall_92(char *, rval_t)
 *	front end for system call #92. if !SCO, it is getgroups, if SCO, then
 *	readlink
 *
 * Calling/Exit State:
 */
int
syscall_92(char *uap, rval_t *rvp)
{
	if(isSCO) {
		return(readlink(uap, rvp));
	}
	else {
		return(getgroups(uap, rvp));
	}
}


/* End Enhanced Application Compatibility Support */
