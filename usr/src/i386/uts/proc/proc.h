#ifndef _PROC_PROC_H	/* wrapper symbol for kernel use */
#define _PROC_PROC_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/proc.h	1.90.5.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <mem/ublock.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */
#include <util/param.h> 	/* SVR4.0COMPAT */
#include <util/ksynch.h>	/* REQUIRED */
#include <svc/time.h>		/* REQUIRED */
#include <fs/file.h>		/* REQUIRED */
#include <proc/signal.h>	/* REQUIRED */
#include <proc/tss.h>		/* REQUIRED */
#include <proc/seg.h>		/* REQUIRED */
#include <svc/fault.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/ublock.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* SVR4.0COMPAT */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <sys/file.h>		/* REQUIRED */
#include <sys/signal.h>		/* REQUIRED */
#include <sys/tss.h>		/* REQUIRED */
#include <sys/seg.h>		/* REQUIRED */
#include <sys/fault.h>		/* REQUIRED */

#else

#include <sys/immu.h>		/* SVR4.0COMPAT */
#include <sys/param.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#if defined _KERNEL || defined _KMEMUSER

typedef struct {		/* kernel syscall set type */
	uint_t word[16];
} k_sysset_t;

/* shared per-process private TSS */
struct stss {
	struct segment_desc st_tssdesc;	/* segment descriptor for st_tss */
					/* for fast loading of GDT entry */
	uchar_t		st_refcnt;	/* # shared references (1 or 2) */
	ushort_t	st_allocsz;	/* size in bytes of this struct, */
					/* including appended I/O bitmap */
	uint_t		st_gencnt;	/* generation count */
	struct tss386	st_tss;
};

/*
 * One structure allocated per active process.  It contains all
 * data needed about the process while the process may be swapped
 * out.  Other per-process data (user.h) may be swapped with the
 * process.
 */

typedef struct proc {
	lock_t	p_mutex;	/* spin lock for most process state, and */
				/* some LWP state */
	lock_t	p_sess_mutex;	/* spin lock for p_[sessp,sid] */
	lock_t	p_seize_mutex;	/* spin lock for p_nseized */
	lock_t	p_dir_mutex;	/* spin lock for p_cdir, p_rdir, and l_rdir */
	lock_t	p_squpdate;	/* spin lock for process_private sync queue list */
	fspin_t p_niblk_mutex;	/* mutex for p_niblked */
	volatile uint_t p_flag;	/* process flags */
	sysid_t	p_sysid;	/* normally same as sysid; for servers, the */
				/* system that sent the message */
	pid_t	p_epid;		/* effective pid; normally same as p_pid; */
				/* for servers, the system that sent the msg */
	uint_t	p_slot;		/* procdir[] slot referencing this process */
	int	p_holdcnt;	/* proc struct can't go away when nonzero */
	struct pid *p_pidp;	/* process ID info */
	volatile pid_t p_pgid;	/* process group-ID */
	struct pid *p_pgidp;	/* process group ID info */
	list_t	*p_sqhashp;	/* pointer to hash bucket array */
	struct proc *p_pglinkf;	/* process group hash chain link (forwards) */
	struct proc *p_pglinkb;	/* process group hash chain link (backwards) */
	volatile dev_t p_cttydev;/* process controlling tty dev */
	volatile pid_t p_sid;	/* process session-ID */
	struct sess *p_sessp;	/* session information */
	volatile pid_t p_ppid;	/* process-ID of parent */
	struct proc *p_parent;	/* ptr to parent process */
	struct proc *p_child;	/* ptr to first child process */
	struct proc *p_prevsib;	/* ptr to prev sibling process on child chain */
	struct proc *p_nextsib;	/* ptr to next sibling process on child chain */
	struct proc *p_zombies;	/* ptr to first ignored zombie process */
	struct proc *p_nextzomb;/* ptr to next ignored zombie process */
	struct proc *p_nextofkin;/* gets accounting info at exit */
	struct proc *p_orphan;	/* list of processes orphaned to us */
	struct proc *p_prevorph;/* previous orphan process */
	struct proc *p_nextorph;/* next orphaned process */
	struct proc *p_prev;	/* active process chain link */
	struct proc *p_next;	/* active process chain link */

	struct uidquotas *p_uidquotap;	/* per-real-user-ID limits */

	struct cred *p_cred;	/* process credentials */

	struct vnode *p_cdir;	/* current directory */
	struct vnode *p_rdir;	/* root directory */
	struct rlimits *p_rlimits;
				/* process resource limits */
	struct lwp *p_lwpp;	/* pointer to the first LWP in the process */
	u_short	p_nlwp;		/* number of LWPs in process */
	u_short p_ntotallwp;	/* total # of LWPs including those that have */
				/* exited but have not been waited for */  
	u_short	p_nstopped;	/* number of stopped LWPs in process */
	u_short	p_nprstopped;	/* number of LWPs stopped on /proc events */
	u_short	p_nreqstopped;	/* number of LWPs in PR_REQUESTED stops */
	u_short p_niblked;	/* number of LWPs blocked interruptibly */

	u_short	p_nlwpdir;	/* number of lwpdir "directory slots" */
	struct lwp **p_lwpdir;	/* ptr to LWP directory for process and /proc */
	union {			/* bitmap of free LWP-IDs (and LWP dir slots) */
		uint_t idmap_small;	/* this variant used when few LWPs */
		uint_t *idmap_large;	/* this variant used when many LWPs */
	} p_lwpidmap;
                                /* shorthand for p_lwpidmap */
#define p_small_lwpidmap p_lwpidmap.idmap_small
#define p_large_lwpidmap p_lwpidmap.idmap_large

	k_sigset_t p_sigs;	/* signals: signals pending to this process */
	k_sigset_t p_sigaccept;	/* signals: signals accepted by an LWP */
	k_sigset_t p_sigtrmask;	/* signals: tracing signal mask for /proc */
	k_sigset_t p_sigignore;	/* signals: ignore when generated */
	void	(*p_sigreturn)();	/* signals: old-style trampoline fn */
	void	(*p_sigactret)();	/* signals: new-style trampoline fn */
	sigstate_t p_sigstate[MAXSIG];	/* signals: signal handling state */
	struct sigqueue *p_sigcldinfo;	/* signals: to send SIGCHLD to parent */
	uchar_t	p_sigjobstop;	/* signals: current job control stop signal */
	uchar_t	p_parentinsess;	/* non-zero iff parent in same session */
	u_short	p_maxlwppri;	/* maximum of the lwp scheduling priorities */
				/* for a swapped out process */

        /*
         * These offsets must be reflected in ttrap.s and misc.s.
         */
        struct prof {                   /* profiling state */
		sleep_t  pr_lock;       /* profiling buffer update lock */
                short   *pr_base;       /* buffer base */
                unsigned pr_size;       /* buffer size */
                unsigned pr_off;        /* pc offset */
                unsigned pr_scale;      /* pc scaling */
        } *p_profp;
	vaddr_t	p_brkbase;	/* base address of heap */
	uint_t	p_brksize;	/* heap size in bytes */
	uint_t	p_stkgapsize;	/* size in bytes of gap starting at p_stkbase */
				/* that map_addr will use first */
	vaddr_t	p_stkbase;	/* base address of autogrow stack */
	uint_t	p_stksize;	/* autogrow stack size in bytes */
	sv_t	p_vfork;	/* child exit/exec vfork interlock */
	sv_t	p_wait2wait;	/* waiting to wait for child proc interlock */
	sv_t	p_waitsv;	/* child process wait event interlock */
	sv_t	p_lwpwaitsv;	/* sibling LWP wait event interlock */
	sv_t	p_wait2seize;	/* LWPs waiting to seize process wait here */
	event_t	p_seized;	/* LWP seizing process waits here */
	u_short	p_nseized;	/* #of LWPs seized from execution */
	u_short	p_nrendezvoused;/* number of LWPs that have rendezvoused */
	sv_t	p_rendezvous;	/* LWPs requesting rendezvous wait here */
	sv_t	p_rendezvoused;	/* Rendezvoused LWPs wait here */
	sv_t	p_stopsv;	/* wait here for all LWPs not to be stopped */
	sv_t	p_destroy;	/* LWP awaiting destroy op waits here */
	sv_t	p_suspsv;	/* sibling LWP suspend event interlock */ 
	rwsleep_t p_rdwrlock;	/* process reader/writer lock */
	int	p_wdata;	/* current wait return value */
	char	p_wcode;	/* current wait code: CLD_EXITED, etc. */
	char	p_nshmseg;	/* #of shared-memory segs currently attached */
	uchar_t	p_plock;	/* process/text locking flags */
	char	p_acflag;	/* accounting flag use (updated on LWP exit) */
	boolean_t p_sigwait;	/* If set SIGWAITING may be sent */
	struct as *p_as;	/* process address space pointer */
	struct segacct  *p_segacct;	/* SystemV IPC: shared memory */
	struct sem_undo *p_semundo;	/* SystemV IPC: semaphores */
	struct sd *p_sdp;	/* pointer to XENIX shared data info */
	timestruc_t p_start;	/* time of process creation */
	long	p_mem;		/* memory stat */
	dl_t	p_ior;		/* accumulator for deceased LWP ior  */
	dl_t	p_iow;		/*	"	"	"       iow  */
	dl_t	p_ioch;		/*	"	"	"	ioch */
	mode_t	p_cmask;	/* process file creation mask */
	dl_t	p_utime;	/* total user time, this process */
	dl_t	p_stime;	/* total system time, this process */
	dl_t	p_cutime;	/* sum of children's user time */
	dl_t	p_cstime;	/* sum of children's system time */
	struct vnode *p_trace;  /*/proc: pointer to traced vnode */
	k_fltset_t p_fltmask;	/* /proc: traced fault set */
	union {
		struct {
			k_sysset_t *entrymsk;	/* /proc: stop-on-entry set */
			k_sysset_t *exitmsk;	/* /proc: stop-on-exit set */
		} prsct;	/* /proc: system call tracing masks */
		struct {
			int wdata;	/* current wait return value */
			char wcode;	/* current wait code: CLD_EXITED, etc */
		} twinfo;	/* temporary holding location for wait info */
	} p_mw;
#define	p_entrymask p_mw.prsct.entrymsk	/* /proc: syscall stop-on-entry set */
#define	p_exitmask  p_mw.prsct.exitmsk	/* /proc: syscall stop-on-exit set */
#define	p_tmp_wdata p_mw.twinfo.wdata	/* temporary wait data holding place */
#define	p_tmp_wcode p_mw.twinfo.wcode	/* temporary wait code holding place */
	fd_table_t p_fdtab;		/* process file descriptors */
	struct execinfo *p_execinfo;	/* a.out vnode, ps(1) args, cmd name */
	struct aproc	*p_auditp;	/* security add-on: audit information */
	uint_t	p_swrss;		/* resident set size before last swap */
					/* (in PAGESIZE units) */
	union {
		clock_t	active;		/* last instance of process entering */
					/* the clock handler or having one */
					/* of its LWPs being dispatched. */
		clock_t	swouttime;	/* time when process was swapped out */
	} p_sw1;			/* mutex protection: see note below */
#define p_active p_sw1.active 		/* if loaded: when last active */
#define p_swouttime p_sw1.swouttime	/* if unloaded: when swapped out */
	union {
		clock_t	incoretime;	/* when process was loaded/started*/
		struct proc *swap_next; /* link to next swapped out process */
	} p_sw2; 			/* mutex protection: see note below */ 
#define	p_incoretime p_sw2.incoretime	/* time process was started/loaded */
#define p_swnext p_sw2.swap_next	/* next process on swapped-out list */
	union {
		size_t	nonlockedrss;	/* number of unlocked (PAGESIZE) */
					/* pages in the address space. */
					/* maintained here in order to let */
					/* the swapper read it without */
					/* locking the process or its */
					/* address space. */
					/* updated periodically */
	} p_sw3;
#define	p_nonlockedrss	p_sw3.nonlockedrss

	short	p_notblocked;		/* flag to inform swapper that least */
					/* one LWP in a swapped out process */
					/* has become runnable */
	short	p_slptime;		/* number of seconds for which the */
					/* process registered no activity */
					/* before being swapped out. See note*/
					/* below re: mutex protection */
	sv_t 	p_swdefer_sv;		/* Rendezvous SV before swapout */
	struct stss *p_tssp;		/* if non-NULL, private TSS */
	sleep_t	p_tsslock;		/* lock protecting p_tssp, all LWPs' */
					/* l_tssp, and the structures they */
					/* point to. */
	proc_ubinfo_t	*p_ubinfop;	/* extended ublock support */
	boolean_t	p_perusercnt;	/* per user count flag */
	sv_t	p_pwsv;			/* post-wait LWPs wait here */
	uchar_t	p_pwflag;		/* post-wait flags */
	struct aio_proc	*p_aioprocp;	/* Async IO private info */
	struct engine *p_bind;		/* Future lwp's bound to p_bind */
	int p_bindnum;			/* Number of times process is bound */
	int p_iobcount;			/* Number of open ports */
} proc_t;


/*
 * Updates to p_sw1, p_sw2, or p_sw3 are not protected, except that they occur
 * with the guarantee that the process structure exists. The races 
 * for updating these fields are either benign or non-existent.
 * Specifically:
 *	p_active: multiple encompassed LWPs can race for updating p_active,
 *		  while being added to runqueues, but it does not matter which 
 *		  one succeeds; they do not race while updating p_active from
 *		  the clock handler because of incidental p_mutex coverage. 
 *
 *	p_swouttime: set only when process is swapped out, and of interest
 *		only after process is swapped out,
 *	p_incoretime: set either when process is created or is swapped in,	
 *		and of interest only to the swapper when considering a process
 *		for swap out,
 *	p_swnext: set only when process is swapped out, and of interest only
 *		after the process is swapped out,
 * 	p_slptime and p_notblocked are not protected as these fields are
 *	updated in mutually exclusive contexts: during swapout and after
 *	swapout. Furthermore, p_notblocked does not need protection from
 *	multiple concurrent updates due to load_lwp invocations, since the
 *	intent of all such updates is to make the field non-zero.
 * 	p_nonlockedrss is updated in the local clock handler, under the
 *	protection of p_mutex, which assures that the as cannot disapper.
 *	It is also updated at swapin time, when as is guaranteed to exist.
 */

/* p_flag codes: */

#define P_SYS         0x0000001 /* process is a system process */
#define P_LOAD        0x0000002 /* process is resident in memory
				 *
				 * It is set after all U-areas for a
				 * process have been locked in-core,
				 * and cleared after all U-areas have been
				 * committed to be swapped out (but have
				 * not yet been unlocked).
				 */
#define P_NOSWAP      0x0000004 /* process is not swappable */
#define P_NFSLKS      0x0000008 /* one or more LWPs in proc have NFS locks */
#define P_VFORK       0x0000010 /* process is vfork(2)ed child */
#define P_VFDONE      0x0000020 /* vfork child releasing parent's addr space */
#define P_EXECED      0x0000040 /* process has exec(2)ed */
#define P_NOWAIT      0x0000080 /* exiting children are ignored */
#define P_JCTL        0x0000100 /* receive SIGCHLD if child stops for jobctrl */
#define P_PROCTR      0x0000200 /* process is being traced by /proc */
#define P_PRFORK      0x0000400 /* inherit /proc tracing flags on fork */
#define P_PROPEN      0x0000800 /* process open by /proc */
#define P_TRC         0x0001000 /* ptrace simulation enabled */
#define P_CLDWAIT     0x0002000 /* walking list of child processes */
#define P_CLDEVT      0x0004000 /* child entered a waitable state */
#define P_DESTROY     0x0008000 /* process is being destroyed */
#define P_PGORPH      0x0010000 /* process in orphaned process group */
#define P_SEIZE       0x0020000 /* VM seize operation active against process */
#define P_NOSEIZE     0x0040000 /* process cannot be VM seized */
#define	P_LOCAL_AGE   0x0080000 /* process is memory-aging itself */
#define P_GONE        0x0100000 /* process has been freeproc'd */
#define	P_PRASYNC     0x0200000	/* /proc "asynchronous stop" mode */
#define	P_PRRLC       0x0400000	/* /proc: setrun process on last close */
#define	P_PRKLC	      0x0800000	/* /proc: kill process on last close */
#define	P_PRSTOP      0x1000000	/* /proc: stop all LWPs in process */
#define	P_AS_ONLOAN   0x2000000	/* VM has given p_as to vfork child */
#define	P_HAS_DSHM    0x4000000	/* DSHM segments attached	*/
#define P_DEFER_SWAP  0x8000000 /* swapout has been deferred */
/* Flags for spawn_proc(): */

#define NP_FAILOK	0x001	/* don't panic if cannot create process */
#define NP_NOLAST	0x002	/* don't use last process slot */
#define NP_SYSPROC	0x004	/* system (resident) process */
#define NP_INIT		0x008	/* this is init process */
#define NP_VFORK	0x010	/* share address space - vfork */
#define NP_SHARE	0x020	/* share address space - asyncio */
#define NP_FORK		0x040	/* the normal fork */
#define NP_FORKALL	0x080	/* the forkall */
#define NP_CGBIND	0x100	/* create process bound to its CG */

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/* Active process chain */
extern proc_t *practive;

/* Spin lock protecting the active chain */
extern rwlock_t proc_list_mutex;
#define PL_PROCLIST	PLHI

extern struct pid pid0;			/* PID structure for process 0 */
extern struct pid *pid0p;		/* Pointer to pid0 */

/*
 * System daemons and special processes which need to be identified.
 */
extern proc_t *proc_sys;		/* general system daemon process */
extern proc_t *proc_init;		/* init user process */
extern struct lwp *lwp_pageout;		/* pageout daemon */

#define IS_PAGEOUT()	(u.u_lwpp == lwp_pageout)

/*
 * Macros to get and release the mutex which mediates changes
 * to the current and root directories.
 */
#define	CUR_ROOT_DIR_LOCK(p)		LOCK(&(p)->p_dir_mutex, PLHI)
#define	CUR_ROOT_DIR_UNLOCK(p, pl)	UNLOCK(&(p)->p_dir_mutex, (pl))

/*
 * Pointer to current process slot
 */
extern proc_t *uprocp;

/*
 * Macro to test if the calling context is single threaded.
 */
#define SINGLE_THREADED()	(uprocp->p_nlwp == 1)

/*
 * Macro to test if the calling context is processor bound
 * (user, exclusive, or kernel). if so, don't remote fork
 * because all threads within process are bound, checking
 * first thread is OK.
 */
#define PROCESS_BOUND()         (uprocp->p_lwpp->l_flag & L_BOUND)


/*
 * Macro to test if the calling context was ever multi-threaded.
 * Depends on the extended LWP directory never being compressed back
 * to a single entry during the process's lifetime
 */
#define WAS_MT()	(uprocp->p_nlwpdir != 1)



/*
 * CDIR_HOLD() acquires a reference to the current directory vnode of
 * the calling process, and sets 'vp' to point to that vnode upon
 * completion.
 */
#define CDIR_HOLD(vp) \
{ \
        pl_t pl; \
        pl = CUR_ROOT_DIR_LOCK(uprocp); \
        (vp) = uprocp->p_cdir; \
        VN_HOLD((vp)); \
        CPATH_HOLD(); \
	CUR_ROOT_DIR_UNLOCK(uprocp, pl); \
}

/*
 *
 * PROC_NOWAIT(proc_t *)
 *	The given process should no longer be in any non-zombie waitable
 *	state (i.e., CLD_STOPPED, CLD_TRAPPED, or CLD_CONTINUED).  Enforce
 *	this assertion by clearing the necessary process state variables
 *	to prevent the process from being visible to the various wait(2)
 *	system calls.
 *
 * Locking requirements:
 *	The p_mutex lock of the process must be held by the caller
 *	if the p_wcode field of the process is one of: CLD_STOPPED,
 *	CLD_CONTINUED, or CLD_TRAPPED.  It remains held upon return.
 *
 *	If the p_wcode field of the process is one of: CLD_EXITED,
 *	CLD_DUMPED, or CLD_KILLED, then the p_mutex lock of the
 *	parent process must be held instead.  It remains held upon
 *	return.
 *
 */
#define	PROC_NOWAIT(p)  (void) ((p)->p_wdata = 0, (p)->p_wcode = 0)	


/*
 * For i386, some special treatment is needed to recognize SCO
 * binaries doing waitid.  Other families should not define this.
 */
#define WAIT_F(uap, rvp) \
	{ if (IS_SCOEXEC) return wait_sco(uap, rvp); }


/* 	
 *	Miscellaneous macros for swapping/aging uses.  
 */

/*
 * boolean_t
 * CAN_SKIP_AGING(proc_t *procp)
 *	True, if the given process is
 *		- being seized,
 *		- or, cannot be seized (may be exiting),
 *		- or, is a system process,
 *		- or, is already being aged by one of its LWPs.
 */

#define	SKIP_AGESWAP_FLAGS	(P_SEIZE | P_NOSEIZE | P_SYS)

#define	SKIP_AGING_FLAGS	(SKIP_AGESWAP_FLAGS | P_LOCAL_AGE)

#define	CAN_SKIP_AGING(procp)	((procp)->p_flag & SKIP_AGING_FLAGS)

/*
 * boolean_t
 * NOT_LOADED(proc_t *procp)
 *	TRUE if process swapped out, FALSE otherwise.
 */
#define NOT_LOADED(procp)	(!((procp)->p_flag & P_LOAD))

/*
 * boolean_t
 * DONTSWAP(proc_t *procp):
 *	The process (which is assumed to be in core) is an undesirable
 *	candidate for swapout, because:
 *		- not a good candidate for aging, or,
 *		- it is unswappable.
 */
#define DONTSWAP(procp)	((procp)->p_flag & (P_NOSWAP | SKIP_AGING_FLAGS))

/*
 * boolean_t
 * DONT_ET_AGE(proc_t *procp):
 *	The process (which is assumed to be in core) cannot or should not
 *	be aged externally, for the same reasons that it is not a
 *	suitable or desirable candidate for self-aging.
 */

#define DONT_ET_AGE(procp)	CAN_SKIP_AGING(procp)

/*
 * boolean_t
 * CANTSHRED(proc_t *procp):
 *	The process (which is assumed to be in core) cannot be "shredded",
 *	i.e., its memory holding cannot be returned to the system. This may
 *	be because the process cannot be seized, or it is a system process,  
 *	or, it is already seized. 
 *	
 *	To prevent a potential livelock resulting from the swapper constantly
 *	discovering the same process in a local aging induced seize, we
 *	should ideally test that the P_LOCAL_AGE bit is turned off, though
 *	P_SEIZE bit is not. See PERF comment below.
 *
 * PERF:
 *	Should we also check if the process is aging itself? There is a 
 *	possibility, not highly probable, that the swapper constantly
 *	finds the process in local aging, and therefore a misbehaved
 *	process may hold down a lot of memory without ever getting
 *	shredded. 
 */
#define CANTSHRED(procp)	((procp)->p_flag & SKIP_AGESWAP_FLAGS)

/*
 * boolean_t
 * NOTRUNNABLE(proc_t *procp):
 *	TRUE if all of its LWPs are blocked. Used solely as an aid 
 *	in keeping processes swapped out if they have not been made 
 *	runnable since swapout.
 */
#define NOTRUNNABLE(procp)	((procp)->p_notblocked == 0)

/*
 * boolean_t
 * DEFERRED_SWAP(proc_t *procp)
 *	Returns TRUE iff process proc_p has a deferred swapout pending.
 */
#define DEFERRED_SWAP(procp)	((procp)->p_flag & P_DEFER_SWAP)

/* Function prototypes: */

#ifdef __STDC__

extern  int	spawn_proc(int, pid_t *, void (*)(void *), void *);
extern  pid_t 	proc_setup(int, proc_t **, int *);
extern	void	relvm(proc_t *);
extern	void	vfwait(pid_t);
extern	void	exit(int, int);
extern	void	freeproc(proc_t *);
extern	void	freeprocs(void);
extern	void	proc_hold(proc_t *);
extern	void	proc_rele(proc_t *);
extern  void	pid_alloc(struct pid *, proc_t *);
extern  void 	pid_hold(struct pid *);
extern  void 	pid_rele(struct pid *);
extern  void 	pid_exit(proc_t *);
extern	proc_t	*pid_next_entry(int *);
extern  proc_t *prfind(pid_t);
extern  proc_t *prfind_sess(pid_t);
extern  boolean_t pgorphaned(struct pid *, boolean_t);
extern  void 	pgjoin(proc_t *, struct pid *);
extern  void 	pgexit(proc_t *, boolean_t);
extern  void 	pgzmove(proc_t *, struct pid *);
extern  void 	pgdetach(void);
extern	struct pid *pgfind(pid_t);
extern	struct pid *pgfind_sess(pid_t);
extern  void   *proc_ref(void);
extern  void   *proc_ref_pp(struct pid *);
extern  boolean_t  proc_valid(void *);
extern  boolean_t  proc_valid_pp(struct pid *);
extern  boolean_t  proc_traced(void *);
extern  int     proc_signal(void *, int);
extern  void    proc_unref(void *);
extern  void    prsignal(struct pid *, int);
extern	void	pid_init(proc_t *);
extern  void	setrun(struct lwp *);
extern	boolean_t rendezvous(void);
extern	void	release_rendezvous(void);
extern	boolean_t destroy_proc(boolean_t);
extern  void 	trapevproc(proc_t *, uint_t, boolean_t);
extern  void 	trapevunproc(proc_t *, uint_t, boolean_t);
extern  void 	trapevnudge(struct lwp *, boolean_t);
extern  void 	process_trapret(void);
extern  void 	initproc_return(void);
extern  void 	complete_fork(void);
extern  void 	syscontext_returned(void);
extern	int	set_special_sp(ushort_t, ulong_t);

#else

/* Only functions used by DDI/DKI drivers need to be declared non-ANSI */

extern  void 	*proc_ref();
extern  int     proc_signal();
extern  void    proc_unref();
extern  boolean_t  proc_valid();

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_PROC_H */
