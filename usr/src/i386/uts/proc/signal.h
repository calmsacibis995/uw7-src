#ifndef _PROC_SIGNAL_H	/* wrapper symbol for kernel use */
#define	_PROC_SIGNAL_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/signal.h	1.34.7.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/bitmasks.h> /* REQUIRED */
#include <proc/siginfo.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/bitmasks.h> /* REQUIRED */
#include <sys/siginfo.h> /* REQUIRED */

#else

#include <sys/types.h>

#if defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1 \
	|| defined(__cplusplus) \
	|| __STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
		&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#include <sys/siginfo.h> 	/* for siginfo_t */
#endif

#if defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1
#include <sys/ucontext.h>	/* for ucontext_t */
#endif

#endif /*_KERNEL_HEADERS*/

#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt (rubout) */
#define	SIGQUIT	3	/* quit (ASCII FS) */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGABRT	6	/* used by abort, replace SIGIOT in the future */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGFPE	8	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */
#define	SIGUSR1	16	/* user defined signal 1 */
#define	SIGUSR2	17	/* user defined signal 2 */
#define	SIGCLD	18	/* child status change */
#define	SIGCHLD	18	/* child status change alias (POSIX) */
#define	SIGPWR	19	/* power-fail restart */
#define	SIGWINCH 20	/* window size change */
#define	SIGURG	21	/* urgent socket condition */
#define	SIGPOLL	22	/* pollable event occured */
#define	SIGIO	22	/* socket I/O possible (SIGPOLL alias) */
#define	SIGSTOP	23	/* stop (cannot be caught or ignored) */
#define	SIGTSTP	24	/* user stop requested from tty */
#define	SIGCONT	25	/* stopped process has been continued */
#define	SIGTTIN	26	/* background tty read attempted */
#define	SIGTTOU	27	/* background tty write attempted */
#define	SIGVTALRM 28	/* virtual timer expired */
#define	SIGPROF	29	/* profiling timer expired */
#define	SIGXCPU	30	/* exceeded cpu limit */
#define	SIGXFSZ	31	/* exceeded file size limit */
#define	SIGWAITING 32	/* all LWPs blocked interruptibly notification */
#define	SIGLWP	33	/* signal reserved for thread library implementation */
#define SIGAIO	34	/* Asynchronous I/O signal */
/*
 * SSI clustering
 * MAXSIG will increase to 36 when support is added for SIGMIGRATE, SIGCLUSTER.
 */
#define SIGMIGRATE 35	/* SSI - migrate process */
#define SIGCLUSTER 36	/* SSI - cluster reconfig */

#ifdef __cplusplus
#define	SIG_DFL	(void(*)(int))0
#else
#define	SIG_DFL	(void(*)())0
#endif

#ifdef __cplusplus
#define SIG_ERR (void(*)(int))-1
#define SIG_IGN (void (*)(int))1
#define SIG_HOLD (void(*)(int))2
#elif !#lint(on)
#define	SIG_ERR (void(*)())-1
#define	SIG_IGN (void (*)())1
#define	SIG_HOLD (void(*)())2
#else /*lint*/
#define	SIG_ERR (void(*)())0
#define	SIG_IGN (void (*)())0
#define	SIG_HOLD (void(*)())0
#endif

#define	SIG_BLOCK	1
#define	SIG_UNBLOCK	2
#define	SIG_SETMASK	3

#define	SIGNO_MASK	0xFF
#define	SIGDEFER	0x100
#define	SIGHOLD		0x200
#define	SIGRELSE	0x400
#define	SIGIGNORE	0x800
#define	SIGPAUSE	0x1000

#if __STDC__ - 0 == 0 || defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)

#ifndef _SIGSET_T
#define _SIGSET_T
typedef struct {		/* signal set type */
	unsigned int	sa_sigbits[4];
} sigset_t;
#endif

struct sigaction {
	int		sa_flags;
	union {
		void	(*sa__handler)(int);
#ifdef _SIGINFO_T
		void	(*sa__sigaction)(int, siginfo_t *, void *);
#else
		void	(*sa__sigaction)();
#endif
	} sa_u;
	sigset_t	sa_mask;
	int		sa_resv[2];
};

#define sa_handler	sa_u.sa__handler
#define sa_sigaction	sa_u.sa__sigaction

/* these are only valid for SIGCLD */
#define	SA_NOCLDSTOP	0x00020000	/* don't send job control SIGCLD's */

#endif /*__STDC__ - 0 == 0 || ...*/

#if __STDC__ - 0 == 0 && !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
#define	MAXSIG	34		/* valid signals range from [1..MAXSIG] */
#define	NSIG	(MAXSIG+1)	/* for compatibility */
#endif

#if defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1 \
	|| __STDC__ - 0 == 0 && !defined(_POSIX_SOURCE)

/* definitions for the sa_flags field */
#define	SA_ONSTACK	0x00000001
#define	SA_RESETHAND	0x00000002
#define	SA_RESTART	0x00000004
#define	SA_SIGINFO	0x00000008
#define	SA_NODEFER	0x00000010

/* these are only valid for SIGCLD */
#define	SA_NOCLDWAIT	0x00010000	/* don't save zombie children */

/* these are only valid for SIGWAITING */
#define	SA_WAITSIG	0x00010000	/* enable SIGWAITING */

#define	SA_NSIGACT	0x80000000	/* ES/MP "new" sigaction */


#if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE_EXTENDED - 0 <1
#define	S_SIGNAL	1
#define	S_SIGSET	2
#define	S_SIGACTION	3
#define	S_NONE		4
#endif

#define	MINSIGSTKSZ	512
#define	SIGSTKSZ	8192

#define	SS_ONSTACK	0x00000001
#define	SS_DISABLE	0x00000002

#ifndef _STACK_T
#define _STACK_T
typedef struct sigaltstack {
	void	*ss_sp;		/* was char * */
	size_t	ss_size;	/* was int */
	int	ss_flags;
} stack_t;
#endif

#endif /*defined(_XOPEN_SOURCE) && ...*/

#if defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1 \
	|| __STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
		&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

/*
 * 4.3BSD structure used in sigstack call.
 */

struct  sigstack {
        void    *ss_sp;                 /* signal stack pointer */
        int     ss_onstack;             /* current status */
};

#endif /*defined(_XOPEN_SOURCE) && ...*/
       

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Stop() and issig() function return values:
 */

typedef enum stopret {STOP_DESTROY, STOP_SUCCESS, STOP_FAILED} stopret_t;
typedef enum issigret {ISSIG_NONE, ISSIG_SIGNALLED, ISSIG_STOPPED} issigret_t;

/*
 * Number of uint_t elements in the ks_sigbits array of a 
 * k_sigset_t object.
 */
#define	NSIGWORDS BITMASK_NWORDS(MAXSIG)

typedef struct {		/* kernel signal set object */
	uint_t ks_sigbits[NSIGWORDS];
} k_sigset_t;

/*
 * Signal state structure:
 * This structure completely defines the signal registration information
 * for a particular signal.  It also defines a pointer that identifies the
 * signal information associated with the most recent process instance of
 * the signal, or of the current signal.
 */
typedef struct sigstate {
	k_lwpid_t   sst_acceptlwp;	/* accepting LWP LWPID; 0 if unknown */
	uchar_t     sst_cflags;		/* common handling flags: */
					/* SA_{ONSTACK,RESETHAND,RESTART, */
					/*     SIGINFO,NODEFER} */
	uchar_t     sst_rflags;		/* registration flags: */
					/* SST_{OLDSIG,SIGWAIT} */
	k_sigset_t  sst_held;		/* signals held while in catcher */
	void      (*sst_handler)();	/* action/handler address */
	union {
		/*
		 * The sst_swcount field is defined ONLY when SST_SIGWAIT is
		 * set in sst_rflags.
		 */
		ushort_t sst_swcount;	/* #of LWPs sigwaiting for signal */
		sigqueue_t *sst_info;	/* associated signal info */
	} sst_v;
#define	sst_swcount sst_v.sst_swcount	/* alias for associated signal info */
#define	sst_info    sst_v.sst_info	/* alias for associated signal info */
} sigstate_t;

#endif /*_KERNEL || _KMEMUSER*/

#ifdef _KERNEL

/*
 * CAN_SEND(int)
 *	Checks to see if the signal specified in the argument can be
 *	immediately delivered to the calling context.  This means the
 *	signal is not held and its disposition is such that a handler
 *	will be called or the process will be terminated.
 *
 * Calling/Exit State:
 *	The caller understands that the information returned may be stale.
 */
#define CAN_SEND(sig) \
	(!sigismember(&u.u_procp->p_sigignore, (sig)) && \
	 !sigismember(&u.u_lwpp->l_sigheld, (sig)))

/*
 *
 * FORCERUN_FROMSTOP(lwp_t *, proc_t *)
 *	Place the designated LWP residing in the specified process into
 *	the SRUN state from the SSTOP state (a debugger's control of the
 *	LWP is being voluntarily or forcibly relinquished).  This is
 *	only used when the process is to be terminated.
 *
 * Locking requirements:
 *	The p_mutex lock of the process, and the l_mutex lock of the
 *	LWP must be held upon entry.  Both locks remain held upon
 *	return.
 *
 */
#define	FORCERUN_FROMSTOP(lwp, p) \
{ \
	if (!((lwp)->l_flag & L_SUSPENDED))				\
		(p)->p_nstopped--;					\
	if ((lwp)->l_flag & L_PRSTOPPED) {				\
		(p)->p_nprstopped--;					\
		if ((lwp)->l_whystop == PR_REQUESTED)			\
			(p)->p_nreqstopped--;				\
	}								\
	(lwp)->l_trapevf &= ~(EVF_PL_JOBSTOP|EVF_PL_PRSTOP);		\
	(lwp)->l_flag &= ~(L_JOBSTOPPED|L_PRSTOPPED);			\
	setrun(lwp);							\
}

/*
 * Signal sets exported from sig.c:
 */
extern k_sigset_t	
	sig_fillset,		/* valid signals, guaranteed contiguous */
	sig_cantmask,		/* cannot be caught or ignored */
	sig_ignoredefault,	/* ignored by default */
	sig_stopdefault,	/* stop by default */
	sig_jobcontrol;		/* job control signals */

extern event_t pause_event;

/*
 * Signal operation macros:
 */
#define	sigfillset(s)	    (*(s) = sig_fillset)

#if MAXSIG <= NBITPW		/* All valid signals fit into a single word */

#define signext(s)	    (BITMASK1_FLS((s)->ks_sigbits) + 1)
#define	sigaddset(s,sig)    BITMASK1_SET1((s)->ks_sigbits, (sig) - 1)
#define	sigdelset(s,sig)    BITMASK1_CLR1((s)->ks_sigbits, (sig) - 1)
#define	sigdelnext(s)	    (BITMASK1_FLSCLR((s)->ks_sigbits) + 1)
#define	sigismember(s,sig)  BITMASK1_TEST1((s)->ks_sigbits, (sig) - 1)
#define	sigemptyset(s)	    BITMASK1_CLRALL((s)->ks_sigbits)
#define	sigisempty(s)	    (!BITMASK1_TESTALL((s)->ks_sigbits))
#define	sigorset(s1,s2)	    BITMASK1_SETN((s1)->ks_sigbits, (s2)->ks_sigbits)
#define	sigmembers(s1,s2)   BITMASK1_TESTN((s1)->ks_sigbits, (s2)->ks_sigbits)
#define	sigdiffset(s1,s2)   BITMASK1_CLRN((s1)->ks_sigbits, (s2)->ks_sigbits)
#define	sigandset(s1,s2)    BITMASK1_ANDN((s1)->ks_sigbits, (s2)->ks_sigbits)

#define	sigutok(us,ks)	(void) ((ks)->ks_sigbits[0] = (us)->sa_sigbits[0], \
				sigandset((ks), &sig_fillset))

#define	sigktou(ks,us)	(void) (sigandset((ks), &sig_fillset), \
				(us)->sa_sigbits[0] = (ks)->ks_sigbits[0], \
				(us)->sa_sigbits[1] = 0, \
				(us)->sa_sigbits[2] = 0, \
				(us)->sa_sigbits[3] = 0)

#elif MAXSIG <= 2 * NBITPW	/* All valid signals fit into two words */

#define signext(s)	    (BITMASK2_FLS((s)->ks_sigbits) + 1)
#define	sigaddset(s,sig)    BITMASK2_SET1((s)->ks_sigbits, (sig) - 1)
#define	sigdelset(s,sig)    BITMASK2_CLR1((s)->ks_sigbits, (sig) - 1)
#define	sigdelnext(s)	    (BITMASK2_FLSCLR((s)->ks_sigbits) + 1)
#define	sigismember(s,sig)  BITMASK2_TEST1((s)->ks_sigbits, (sig) - 1)
#define	sigemptyset(s)	    BITMASK2_CLRALL((s)->ks_sigbits)
#define	sigisempty(s)	    (!BITMASK2_TESTALL((s)->ks_sigbits))
#define	sigorset(s1,s2)	    BITMASK2_SETN((s1)->ks_sigbits, (s2)->ks_sigbits)
#define	sigmembers(s1,s2)   BITMASK2_TESTN((s1)->ks_sigbits, (s2)->ks_sigbits)
#define	sigdiffset(s1,s2)   BITMASK2_CLRN((s1)->ks_sigbits, (s2)->ks_sigbits)
#define	sigandset(s1,s2)    BITMASK2_ANDN((s1)->ks_sigbits, (s2)->ks_sigbits)
#define	sigutok(us,ks)	(void) ((ks)->ks_sigbits[0] = (us)->sa_sigbits[0], \
			  	(ks)->ks_sigbits[1] = (us)->sa_sigbits[1], \
				sigandset((ks), &sig_fillset))
#define	sigktou(ks,us)	(void) (sigandset((ks), &sig_fillset), \
				(us)->sa_sigbits[0] = (ks)->ks_sigbits[0], \
				(us)->sa_sigbits[1] = (ks)->ks_sigbits[1], \
				(us)->sa_sigbits[2] = 0, \
				(us)->sa_sigbits[3] = 0)

#else				/* At least three words required */

#define signext(s)	    (BITMASKN_FLS((s)->ks_sigbits, NSIGWORDS) + 1)
#define	sigaddset(s,sig)    BITMASKN_SET1((s)->ks_sigbits, (sig) - 1)
#define	sigdelset(s,sig)    BITMASKN_CLR1((s)->ks_sigbits, (sig) - 1)
#define	sigdelnext(s)	    (BITMASKN_FLSCLR((s)->ks_sigbits, NSIGWORDS) + 1)
#define	sigismember(s,sig)  BITMASKN_TEST1((s)->ks_sigbits, (sig) - 1)
#define	sigemptyset(s)	    BITMASKN_CLRALL((s)->ks_sigbits, NSIGWORDS)
#define	sigisempty(s)	    (!BITMASKN_TESTALL((s)->ks_sigbits, NSIGWORDS))
#define	sigorset(s1,s2)	    \
		BITMASKN_SETN((s1)->ks_sigbits, (s2)->ks_sigbits, NSIGWORDS)
#define	sigmembers(s1,s2)   \
		BITMASKN_TESTN((s1)->ks_sigbits, (s2)->ks_sigbits, NSIGWORDS)
#define	sigdiffset(s1,s2)   \
		BITMASKN_CLRN((s1)->ks_sigbits, (s2)->ks_sigbits, NSIGWORDS)
#define	sigandset(s1,s2)    \
		BITMASKN_ANDN((s1)->ks_sigbits, (s2)->ks_sigbits, NSIGWORDS)

#if MAXSIG <= 3 * NBITPW	/* All valid signals fit into three words */

#define	sigutok(us,ks)	(void) ((ks)->ks_sigbits[0] = (us)->sa_sigbits[0], \
			  	(ks)->ks_sigbits[1] = (us)->sa_sigbits[1], \
			  	(ks)->ks_sigbits[2] = (us)->sa_sigbits[2], \
				sigandset((ks), &sig_fillset))

#define	sigktou(ks,us)	(void) (sigandset((ks), &sig_fillset), \
				(us)->sa_sigbits[0] = (ks)->ks_sigbits[0], \
				(us)->sa_sigbits[1] = (ks)->ks_sigbits[1], \
				(us)->sa_sigbits[2] = (ks)->ks_sigbits[2], \
				(us)->sa_sigbits[3] = 0)

#elif MAXSIG <= 4 * NBITPW	/* All valid signals fit into four words */

#define	sigutok(us,ks)	(void) ((ks)->ks_sigbits = (us)->sa_sigbits, \
				sigandset((ks), &sig_fillset))
#define	sigktou(ks,us)	(void) (sigandset((ks), &sig_fillset), \
				(us)->sa_sigbits = (ks)->ks_sigbits)

#else

 # error	"More than four words of signal bits not supported."

#endif

#endif

typedef struct {
	int	ss_sig;			/* signal to send */
	sigqueue_t *ss_sqp;		/* assoc. signal info (NULL if none) */
	boolean_t ss_checkperm;		/* set B_TRUE to check permissions */
	int ss_count;			/* #of objects which accepted siginfo */
	int *ss_pidlistp;		/* list of pids for audit */
} sigsend_t;


/* sst_rflags flags */
#define	SST_OLDSIG  0x01	/* signal registered by signal(2)/sigset(2) */
#define	SST_SIGWAIT 0x02	/* signal is being sigwait(2)ed for */
#define SST_NSIGACT 0x04	/* ES/MP "new" sigaction (4th parm supplied) */

struct proc;			/* allow use for extern declarations below */
struct lwp;			/* " */
struct procset;			/* " */

#ifdef __cplusplus
extern "C" {
#endif

extern sigqueue_t *siginfo_get(int, u_long);
extern void siginfo_free(sigqueue_t *);
extern void sigcld_l(struct proc *);
extern void sigcld(struct proc *);
extern int sigtolwp_l(struct lwp *, int, sigqueue_t *);
extern int sigtolwp(struct lwp *, int, sigqueue_t *);
extern void sigsynchronous(struct lwp *, int, sigqueue_t *);
extern int sigtoproc_l(struct proc *, int, sigqueue_t *);
extern int sigtoproc(struct proc *, int, sigqueue_t *);
extern void dbg_sigheld(struct lwp *, k_sigset_t);
extern void dbg_sigtrmask(struct proc *, k_sigset_t);
extern void dbg_restop(struct lwp *);
extern void dbg_setrun(struct lwp *);
extern void dbg_clearlwpsig(struct lwp *);
extern int dbg_setlwpsig(struct lwp *, int, sigqueue_t *);
extern int dbg_setprocsig(struct proc *, int, sigqueue_t *);
extern void dbg_unkilllwp(struct lwp *, int);
extern void dbg_unkillproc(struct proc *, int);
extern void discard_lwpsigs(struct lwp *);
extern void discard_procsigs(void);
extern void mask_signals(struct lwp *, k_sigset_t);
extern void unmask_signals(struct lwp *, k_sigset_t);
extern void lwpsigmask_l(k_sigset_t);
extern void lwpsigmask(k_sigset_t);
extern stopret_t stop(int, int);
extern issigret_t issig(lock_t *);
extern void sigsendinit(sigsend_t *, ulong_t, int, boolean_t);
extern int sigsendproc(struct proc *, void *);
extern void sigsenddone(sigsend_t *, ulong_t);
extern int sigsendset(struct procset *, int);
extern void sigfork(struct proc *, int);
extern void sigexec(void);
extern void setsigact(int, void (*)(), k_sigset_t, u_int);
extern void post_destroy(boolean_t, u_int);
extern void abort_rendezvous(void);

#ifdef __cplusplus
}
#endif

#endif /*_KERNEL*/

#endif /*_PROC_SIGNAL_H*/
