#ident	"@(#)kern-i386at:io/async/async.c	1.23.7.4"
#ident	"$Header$"

#include <fs/buf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/specfs/snode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/poll.h>
#include <io/uio.h>
#include <io/elog.h>
#include <io/mkdev.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_kmem.h>
#include <mem/hat.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/mod/mod_k.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_comm.h>
#include <io/async/aiosys.h>

/*
 * A process doing aio first opens the aio device, then does an aio_memlock
 * on a virtual address range, to provide pages that can always be found
 * at interrupt level.  A process may have at most one aio_memlocked buffer
 * at a time.  If an address space operation destroys the aio_memlocked
 * range, hooks in as routines call in here to force aio to go quiescent
 * before the operation completes.  Subsequent aio operations will return
 * EINVAL.
 *
 * The aio_proc structure is the per-aio_memlock, per-process data structure.
 * It is reachable from the p_aioprocp proc structure member, which is
 * protected by the process's address space lock.  The existence of the
 * structure pointer is equivalent to the existence of an aio_memlocked
 * range.
 */
typedef struct aio_range {
	vaddr_t		ar_vaddr;	/* Ptr to locked user addr space */
	size_t		ar_size;	/* Size of locked memory space */
	struct aio_range *ar_next;	/* next aio_proc in list */
	struct aio_range *ar_prev;	/* previous aio_proc in list */
	boolean_t	ar_needs_prep;	/* TRUE if must call as_aio_prep() */
} aio_range_t;

typedef struct aio_proc {
	aio_range_t	*ap_rangelist;	/* list of ranges for locked area */
	vaddr_t		ap_polladdr;	/* Ptr to locked user addr for poll */
	caddr_t		ap_kpolladdr;	/* Ptr to kernel addr for poll */
	aio_queue_t	ap_waitq;	/* Queue of jobs to be completed */
	aio_queue_t	ap_compq;	/* Queue of completed jobs */
	u_int		ap_flags;
	lock_t		ap_lock;	/* Spin lock at pldisk. */
	sv_t		ap_cleansv;	/* Sync on end of all I/O, protected
					 * by ap_lock. */
	aiolistio_t *	ap_listio;	/* Ptr to list I/O args buffer */
} aio_proc_t;

/* ap_flags values. */
#define AP_STOP		0x1	/* Process is sleeping for I/O to stop. */

/*
 *+ Per-aio_proc structure lock.  Protects all members of the structure, and aio
 *+ structures on ap_waitq and ap_compq.
 */
STATIC LKINFO_DECL(aio_proc_lkinfo, "IO:aio_:aio_proc_lkinfo", 0);

#define AIO_PROC_LOCK(aio_procp)	LOCK(&(aio_procp)->ap_lock, pldisk)
#define AIO_PROC_UNLOCK(aio_procp, ipl)	UNLOCK(&(aio_procp)->ap_lock, (ipl))
#define AIO_PROC(procp)			((procp)->p_aioprocp)
#define AIO_PROC_COMPQP(aio_procp)	(&(aio_procp)->ap_compq)
#define AIO_PROC_WAITQP(aio_procp)	(&(aio_procp)->ap_waitq)
#define	AIO_RANGE_BASE(aio_rangep)	((aio_rangep)->ar_vaddr)
#define	AIO_RANGE_END(aio_rangep)	((aio_rangep)->ar_vaddr + \
						(aio_rangep)->ar_size)

STATIC aio_proc_t *	aio_proc_alloc(void);
STATIC void		aio_proc_free(aio_proc_t *);

/*
 * The free list of per-request data structures, its counter and lock.
 */
struct {
	lock_t		lock;
	u_int 		count;
	aio_queue_t	queue;
} aio_free;

/*
 *+ aio structure free list lock.  
 */
STATIC LKINFO_DECL(aio_free_lkinfo, "IO:aio_:aio_free_lkinfo", 0);

#define	AIO_FREE_LOCK()		LOCK(&aio_free.lock, pldisk)
#define AIO_FREE_UNLOCK(ipl)	UNLOCK(&aio_free.lock, (ipl))

STATIC aio_t *	aio_aio_alloc(void);
STATIC void	aio_aio_free(aio_t *);
STATIC int	aio_list_alloc(int, aio_t **, aio_t **);
STATIC void	aio_list_free(int, aio_t *, aio_t *);
STATIC void	aio_aio_init(struct aio *);
STATIC void	aio_list_del(aio_t *, aio_t *);
STATIC void	aio_list_add(aio_queue_t *, aio_t *, aio_t *);

#ifdef DEBUG
u_int aiodebug;
#endif

/*
 * Other than the load and unload routines, external entry points are exported
 * through aiosys.h.
 */
STATIC int	aio_load(void);
STATIC int	aio_unload(void);

/*
 * These operations are switches out of the driver ioctl, and their
 * subroutines.
 */
STATIC int	aio_memlock(caddr_t);
STATIC int	aio_rw(caddr_t);
STATIC int	aio_listio(caddr_t);
STATIC int	aio_poll(caddr_t);
STATIC int	aio_gettune(caddr_t);
STATIC caddr_t	aio_getpoll(vaddr_t);
STATIC void	aio_freepoll(vaddr_t, caddr_t, struct as *);
STATIC int	aio_format_req(aiojob_t *, aio_t *);
STATIC void	aio_issue_req(aio_t *);
STATIC void	aio_listio_done(aio_proc_t *, aio_t *, aio_t *);
STATIC int	aio_listio_post_proc(aiolistio_t *, aio_t *, aio_t *, u_int);
STATIC aio_range_t *aio_findrange(proc_t *, vaddr_t, u_int);
STATIC boolean_t aio_range_overlaps(proc_t *, vaddr_t, u_int);

/* Interrupt handler. */
STATIC void 	aio_done(struct buf *);

/* Subroutines for as operation hooks. */
STATIC void	aio_cleanall(struct proc *, struct as *);
STATIC void	aio_waitforio(proc_t *);

int aio_devflag = D_MP;

#define DRVNAME "async - asynchronous I/O driver"

MOD_DRV_WRAPPER(aio_, aio_load, aio_unload, NULL, DRVNAME);

/*
 * STATIC int
 * aio_load(void)
 *	Load the aio module, calling the initialization routine.
 *
 * Calling/Exit State:
 *	No locks held on entry and exit.  Non-idempotent.  Returns an errno.
 */
STATIC int
aio_load(void)
{
	int error;

	if ((error = aio_init()) == 0)
		mod_drvattach(&aio__attach_info);
	return error;
}

/*
 * STATIC int
 * aio_unload(void)
 *	Unload the aio module.
 *
 * Calling/Exit State:
 *	No locks held on entry and exit. Must have been preceeded by a
 *	succesful call of aio_load.  Returns an errno.
 */
STATIC int
aio_unload(void)
{
	mod_drvdetach(&aio__attach_info);
	LOCK_DEINIT(&aio_free.lock);
	return 0;
}

/*
 * int
 * aio_init(void)
 *	aio driver initialization routine.
 *
 * Calling/Exit State:
 *	No locks held on entry and exit.  Returns an errno.
 *
 * Description:
 *	Initializes the aio free list and its lock; takes and releases the
 *	lock as it populates the free list.
 */
int
aio_init(void)
{
	aio_t *aiop;

	INIT_QUEUE(&aio_free.queue);
	LOCK_INIT(&aio_free.lock, AIO_HIER_AIO_FREE_LIST, pldisk,
		  &aio_free_lkinfo, KM_NOSLEEP);
	aio_free.count = 0;
	for (aiop = aio_list; aiop < &aio_list[numaio]; aiop++) {
		aio_aio_free(aiop);
		SV_INIT(&aiop->aio_sv);
	}
	return 0;
}

/*
 * int
 * aio_open(dev_t *dev_p, int flags, int otype, struct cred *cred_p)
 *	aio driver open routine.
 *
 * Calling/Exit State:
 *	No locks held on entry and exit.  Returns an errno.
 *
 * Description:
 *	Currently a stub that trivially returns success, because there
 *	is no state change required in aio for an open.
 */
/* ARGSUSED */
int
aio_open(dev_t *dev_p, int flags, int otype, struct cred *cred_p)
{
	return 0;
}


/*
 * int
 * aio_close(dev_t dev, int flag, int otyp, cred_t *cred_p)
 *	aio driver close routine.
 *
 * Calling/Exit State:
 *	No locks held on entry and exit.  Returns an errno.
 *
 * Description:
 *	Currently a stub that trivially returns success, because there
 *	is no state change required in aio for a close.
 */
/* ARGSUSED */
int
aio_close(dev_t dev, int flag, int otyp, cred_t *cred_p)
{
	return 0;
}

/*
 * int
 * aio_ioctl(dev_t dev, int cmd, int arg, int mode, struct cred *cred_p,
 *	     int *rval_p)
 *	aio driver ioctl routine.
 *
 * Calling/Exit State:
 *	No locks held on entry and exit.  Returns an errno.  Call only
 *	at base level.
 *
 * Description:
 *	aio_ioctl is overloaded as the focus of all aio user requests
 *	of substance.  Sub-operations include aio_memlock, aio_rw, aio_listio,
 *	aio_poll, and aio_gettune.  To protect u.u_procp->p_aioprocp, takes
 *	the address space lock of the current process for some operations.
 *	Can also take the aio_proc lock and aio free list lock.
 */
/* ARGSUSED */
int
aio_ioctl(dev_t dev, int cmd, int arg, int mode, struct cred *cred_p,
	  int *rval_p)
{

	switch(cmd) {
	case AIOMEMLOCK:
		return aio_memlock((caddr_t) arg);
	case AIORW:
		return aio_rw((caddr_t) arg);
	case AIOLISTIO:
		return aio_listio((caddr_t) arg);
	case AIOPOLL:
		return aio_poll((caddr_t) arg);
	case AIOGETTUNE:
		return aio_gettune((caddr_t) arg);
	default:
		return EINVAL;
	}
}

/*
 * STATIC int
 * aio_memlock(caddr_t arg)
 *	aio driver operation to lock down a virtual address range for
 *	an aio buffer.
 *
 * Calling/Exit State:
 *	'arg' is the address of the argument lock request.
 *	Must be called with the current process's address space unlocked;
 *	Takes and releases the lock. In non-error cases, updates the
 *	process's p_aioprocp.  Returns an errno.  Call only at base level.
 *
 * Description:
 *	Gets and releases the process's address space write lock.
 *	It is an error for a process holding an aio_memlocked range to try
 *	to acquire another.  In the success case, the aio_proc structure
 *	is updated with the page-aligned virtual address and size of the range.
 *	Note that the lock for the allocated aio_proc structure is taken and
 *	dropped here.
 *	We also lock down a page in the user address space.
 *	We use a byte in this page as a flag to tell the library
 *	that it needs to do an AIOPOLL to get results.  The library
 * 	passes in the address of this byte with the lock request.
 */
STATIC int
aio_memlock(caddr_t arg)
{
	asyncmlock_t lock;
	struct proc *procp;
	aio_proc_t *aio_procp;
	aio_range_t *aio_rangep, *aio_nextrp;
	struct as *asp;
	vaddr_t raddr;                  /* rounded down addr */
	u_int rsize;                    /* rounded up size */
	vaddr_t polladdr;               /* user address for poll info */
	caddr_t kpolladdr;              /* kernel address for poll info */
	int	error = 0;

	/*
	 * Do the copyin before locking the address space, to avoid
	 * potential deadlocks on the as lock during fault handling.
	 */
	if (copyin(arg, (caddr_t)&lock, sizeof(lock)))
		return EFAULT;

	raddr = (vaddr_t)((u_int)lock.am_vaddr & PAGEMASK);
	rsize = ((vaddr_t)lock.am_vaddr + lock.am_size);
	rsize = ((rsize + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	procp = u.u_procp;
	asp = procp->p_as;
	as_wrlock(asp);

	if (aio_range_overlaps(procp, raddr, rsize)) {
		error = EINVAL;
		goto unlock;
	}
	else if (as_aio_able(asp, raddr, rsize) != B_TRUE) {
		error = EFAULT;
		goto unlock;
	}
	else if (error = as_ctl(asp, raddr, rsize, MC_LOCK, NULL, NULL))
		goto unlock;

	aio_rangep = kmem_alloc(sizeof(aio_range_t), KM_SLEEP);
	ASSERT(aio_rangep != NULL);

	if (aio_procp = AIO_PROC(procp)) {
		if ((vaddr_t)lock.am_polladdr == aio_procp->ap_polladdr) {
			AIO_PROC_LOCK(aio_procp);
		} else {
			error = ENOMEM;
			kmem_free(aio_rangep, sizeof(aio_range_t));
			(void)as_ctl(asp, raddr, rsize, MC_UNLOCK, NULL, NULL);
			goto unlock;
		}
	} else {
		polladdr = (vaddr_t)lock.am_polladdr;
		if (kpolladdr = aio_getpoll(polladdr)) {
			*kpolladdr = 0;	/* no jobs waiting */

			aio_procp = aio_proc_alloc();
			ASSERT(aio_procp != NULL);
			AIO_PROC(procp) = aio_procp;
			aio_procp->ap_polladdr = polladdr;
			aio_procp->ap_kpolladdr = kpolladdr;
		} else {
			error = ENOMEM;
			kmem_free(aio_rangep, sizeof(aio_range_t));
			(void)as_ctl(asp, raddr, rsize, MC_UNLOCK, NULL, NULL);
			goto unlock;
		}
	}

	aio_rangep->ar_vaddr = raddr;
	aio_rangep->ar_size = rsize;
	aio_rangep->ar_next = aio_rangep->ar_prev = aio_rangep;
	aio_rangep->ar_needs_prep = as_aio_needs_prep(procp->p_as, raddr,
		rsize);

	aio_nextrp = aio_procp->ap_rangelist;
	if ((aio_nextrp == NULL) ||
			(AIO_RANGE_BASE(aio_nextrp) >=
				AIO_RANGE_END(aio_rangep)))
		aio_procp->ap_rangelist = aio_rangep;
	else {
		do
			aio_nextrp = aio_nextrp->ar_next;
		while ((aio_nextrp != aio_procp->ap_rangelist) &&
				(AIO_RANGE_END(aio_nextrp) <=
					AIO_RANGE_BASE(aio_rangep)));
	}

	if (aio_nextrp != NULL) {
		aio_rangep->ar_next = aio_nextrp;
		aio_rangep->ar_prev = aio_nextrp->ar_prev;
		aio_nextrp->ar_prev = aio_rangep;
		aio_rangep->ar_prev->ar_next = aio_rangep;
	}
	AIO_PROC_UNLOCK(aio_procp, plbase);
unlock:
	as_unlock(procp->p_as);
	return error;
}

/*
 * STATIC int
 * aio_rw(caddr_t arg)
 *	aio driver operation to request a single async read or write.
 *
 * Calling/Exit State:
 *	'arg' is the address of the argument job request.
 *	Must be called with the current process's address space unlocked;
 *	Takes and releases the lock. In non-error cases, schedules the
 *	requested I/O.  Returns an errno.  Call only at base level.
 *
 * Description:
 *	The address space read lock is needed to protect the aio_proc
 *	structure until setup is finished.  An aio structure is allocated  
 *	to describe the requested I/O, which means that the free list lock
 *	is taken and released.  Also, the aio_proc lock is taken, so the
 *	aio structure can be added to the waitq, and is then released.
 */
STATIC int
aio_rw(caddr_t arg)
{
	aiojob_t aiojob;
	aio_t *aiop;
	aio_proc_t *aio_procp;
	int error = 0;
	proc_t *procp;

	/*
	 * Do the copyin before locking the address space, to avoid
	 * potential deadlocks on the as lock during fault handling.
	 */
	if (copyin(arg, (caddr_t)&aiojob, sizeof(aiojob)))
		return EFAULT;

	procp = u.u_procp;
	as_rdlock(procp->p_as);

	if ((aio_procp = AIO_PROC(procp)) == NULL) {
		error = EINVAL;
		goto unlock;
	}

	if ((aiop = aio_aio_alloc()) == NULL) {
		error = ENOMEM;
		goto unlock;
	}

	if ((error = aio_format_req(&aiojob, aiop)) != 0) {
		aio_aio_free(aiop);
		goto unlock;
	}

	/*
	 * Input parameters have been validated.  Add the control block to the
	 * wait queue and issue the request.
	 */

	(void)AIO_PROC_LOCK(aio_procp);
	ADD_QUEUE((AIO_PROC_WAITQP(aio_procp)), &aiop->aio_queue);
	AIO_PROC_UNLOCK(aio_procp, plbase);
	aio_issue_req(aiop);
unlock:
	as_unlock(procp->p_as);
	return error;
}

/*
 * STATIC int
 * aio_listio(caddr_t arg)
 *	aio driver operation to request a listio read or write, for which
 *	the caller may optionally wait.
 *
 * Calling/Exit State:
 *	'arg' is the address of the user's listio request.
 *	Must be called with the current process's address space unlocked;
 *	takes and releases the lock.  In non-error cases, schedules the
 *	requested I/Os.  Returns an errno.  Non-zero returns imply global
 *	errors, except that EIO is returned even for detected partial errors.
 *	Call only at base level.
 *
 * Description:
 *	The address space read lock is needed to protect the aio_proc
 *	structure until setup is finished.  It is managed locally so that
 *	it may be dropped around copyouts.  Otherwise, we will try to take
 *	the lock recursively in begin_user_write, if running on a 386.  A
 *	list of aio structures is allocated to describe the requested I/O,
 *	which means that the free list lock is taken and released.
 *	Also, the aio_proc lock is taken, so the aio list can be added to
 *	the waitq, and is then released.  Because partial failures that
 *	are detected locally contribute to completion of the list request,
 *	this routine is prepared to move a request from the wait queue to
 *	the completion queue.  Because of resource shortages, may try
 *	fewer than the number of jobs requested.  If all jobs attempted
 *	fail synchronously, places the job list on the aio_proc completion
 *	queue, where a poll may find it.
 */
STATIC int
aio_listio(caddr_t arg)
{
	aio_t	*head, *tail, *aiop;
	int	error = 0, format_error = 0;
	int	i, listsize;
	proc_t	*procp;
	aio_proc_t *aio_procp;
	aio_queue_t *queuep;
	aiolistio_t *aiolistp;
	aiojob_t *aiojobp;

	procp = u.u_procp;
	as_rdlock(procp->p_as);
	if ((aio_procp = AIO_PROC(procp)) == NULL) {
		error = EINVAL;
		goto unlock;
	}
	if (aio_procp->ap_listio == NULL) {
		if ((aio_procp->ap_listio =
		   kmem_alloc(listio_size, KM_NOSLEEP)) == NULL) {
			error = EAGAIN;
			goto unlock;
		}
	}
	aiolistp = aio_procp->ap_listio;
	aiojobp = aiolistp->al_jobs;
	if (copyin(arg, (caddr_t)aiolistp, AIOLIST_HDRSIZE)) {
		error = EFAULT;
		goto unlock;
	}
	if (aiolistp->al_nent > aio_listio_max) {
		error = EINVAL;
		goto unlock;
	}
	if (copyin(arg + AIOLIST_HDRSIZE,
	    (caddr_t)aiojobp, aiolistp->al_nent * sizeof(aiojob_t))) {
		error = EFAULT;
		goto unlock;
	}

#ifdef DEBUG
	if (aiodebug) {
		cmn_err(CE_CONT, "listio_size:%d\n", listio_size);
		cmn_err(CE_CONT, "nent:%d mode:%x\n", aiolistp->al_nent,
			aiolistp->al_mode);
	}
#endif

	if ((listsize = aio_list_alloc(aiolistp->al_nent, &head, &tail)) == 0) {
		error = EAGAIN;
		goto unlock;
	}

	/*
	 * For each element in the req list format the appropriate disk driver
	 * request.  Flag errors if I/O reqs are incorrect.
	 */

	for (i = 0, queuep = &head->aio_queue, aiojobp = aiolistp->al_jobs;
	     i < listsize;
	     i++, queuep = NEXT_ELEM(queuep), aiojobp++) {
		/* LINTED pointer alignment */
		aiop = QPTOAIOP(queuep);
		if ((error = aio_format_req(aiojobp, aiop)) != 0) {
			aiojobp->aj_errno = aiop->aio_errno = error;
			aiojobp->aj_cnt = aiop->aio_nbytes = -1;
			format_error++;
		} else
			aiop->aio_errno = 0;
		aiop->aio_lhead = head;
	}

	/*
	 * When ctrl blocks could not be allocated for some requests, mark the
	 * rest of the job list as EAGAIN.
	 */

	for( ; i < aiolistp->al_nent; i++) {
		aiojobp->aj_errno = EAGAIN;
		aiojobp++;
	}
	if (aiolistp->al_mode == LIOWAIT)
		head->aio_kflags &= ~A_LISTNOTIFY;
	else
		head->aio_kflags |= A_LISTNOTIFY;

	error = 0;
	head->aio_lcount = listsize - format_error;

	(void)AIO_PROC_LOCK(aio_procp);

	if (head->aio_lcount) {
		/*
		 * I/O yet to do.  Add the sub-list to the wait queue and set
		 * the counters.  We have to do this now so aio_done can find
		 * things.
		 */
		aio_list_add(AIO_PROC_WAITQP(aio_procp), head, tail);

		/*
		 * It is necessary to drop the aio_proc lock now because the
		 * strategy routine called by aio_issue_req can block.  It is
		 * desirable, also, because we want to let completion run even
		 * while we're processing the list.  It is safe because (1)
		 * the data we access here and that accessed by completion are
		 * disjoint, and (2) the requests are on the wait list, so no
		 * other lwp will mess with them.
		 */

		AIO_PROC_UNLOCK(aio_procp, plbase);
		for (i = 0, queuep = &head->aio_queue,
		      aiojobp = aiolistp->al_jobs;
		     i < listsize;
		     i++, queuep = NEXT_ELEM(queuep), aiojobp++) {
			/* LINTED pointer alignment */
			aiop = QPTOAIOP(queuep);
			ASSERT(aiop->aio_errno ||
			       aiop->aio_cbp == aiojobp->aj_cbp);
			ASSERT(aiop->aio_lhead == head);
			if (!(aiop->aio_errno))
				aio_issue_req(aiop);
		}
		(void)AIO_PROC_LOCK(aio_procp);
	} else {
		/*
		 * We got a format error on every list element.  Complete the
		 * I/O here, and fall through to the general case.
		 */
		aio_list_add(AIO_PROC_COMPQP(aio_procp), head, tail);
		if (aiolistp->al_mode == LIONOWAIT)
			sigtoproc(procp, SIGEMT, (sigqueue_t *)NULL);
	}

	/*
	 * In the LIOWAIT case, wait for I/O to complete, remove the aio list
	 * from the completion queue, collect errors, free the list, and return
	 * the results.  In the LIONOWAIT case, the user will poll for results,
	 * possibly after a signal from aio_done (or here, if we got format
	 * errors on everything).
	 */

	if (aiolistp->al_mode == LIOWAIT) {
		if (head->aio_lcount) {
			SV_WAIT(&head->aio_sv, PRIBUF, &aio_procp->ap_lock);
			(void)AIO_PROC_LOCK(aio_procp);
			ASSERT(head->aio_lcount == 0);
		}
		aio_list_del(head, tail);
		AIO_PROC_UNLOCK(aio_procp, plbase);
		if (aio_listio_post_proc(aiolistp, head, tail, listsize))
			error = EIO;

		/* Copyout the status to reflect the error codes */

		as_unlock(procp->p_as);
		if (copyout((caddr_t)aiolistp->al_jobs,
		    arg + AIOLIST_HDRSIZE,
	    	    aiolistp->al_nent * sizeof(aiojob_t)))
			error = EFAULT;
	} else {
		AIO_PROC_UNLOCK(aio_procp, plbase);
		as_unlock(procp->p_as);
	}
	return error;

unlock:
	as_unlock(procp->p_as);
	return error;
}

int aio_pollstats[NSTATUS+1];

/*
 * STATIC int
 * aio_poll(caddr_t arg)
 *	aio driver operation to retrieve completed aio jobs.
 *
 * Calling/Exit State:
 *	'arg' is the address of the user's result buffer.
 *	Must be called with the current process's address space unlocked;
 *	takes and releases the lock.  Returns an errno.  Call only at base
 *	level. Copies completed jobs out to user.
 *
 * Description:
 *	The address space read lock is needed to protect the aio_proc
 *	structure until setup is finished.  It is managed locally so that
 *	it may be dropped around copyouts.  Otherwise, we will try to take
 *	the lock recursively in begin_user_write, if running on a 386.
 *	listio jobs for which a process is waiting synchronously are not
 *	returned.  Under protection of the aioproc lock, moves aio structures
 *	for retrieved jobs to a local list.  Then releases the lock, and gives
 *	up file references from the aio structures, and frees the structures,
 *	which takes and drops the aio free list lock.
 */
STATIC int
aio_poll(caddr_t arg)
{
	struct proc *procp;
	aio_proc_t *aio_procp;
	aio_queue_t *ap_compq, *queuep, *nextqp;
	aio_t	*aiop;
	int	nfound = 0, nwanted, error = 0;
	aioresult_t aioresult;
	aiostatus_t *statusp;
	aio_queue_t holdqueue;

	/*
	 * Do the copyin before locking the address space, to avoid
	 * potential deadlocks on the as lock during fault handling.
	 */
	if (copyin(arg, (caddr_t)&aioresult,
	     sizeof(aioresult) - sizeof(aioresult.ar_stat)))
		return EFAULT;

	procp = u.u_procp;
	as_rdlock(procp->p_as);
	if ((aio_procp = AIO_PROC(procp)) == NULL) {
		error = EINVAL;
		goto unlock;
	}
	statusp = aioresult.ar_stat;
	nwanted = MIN(NSTATUS, aioresult.ar_total);
	INIT_QUEUE(&holdqueue);

	(void)AIO_PROC_LOCK(aio_procp);
	ap_compq = AIO_PROC_COMPQP(aio_procp);
	/*
	 * Retrieve as many elements as possible from the comp list.  Based on
	 * the results of I/O completion, update the various fields of the stat
	 * structure.  Add the elements or control blocks to the free list and
	 * update the counters.  An exception is the case when control blocks
	 * with error information are available that has already been copied
	 * back to the user space.  Ignore the status info for these special
	 * cases.  Retrieved elements are placed on the holding queue, so that
	 * we can drop the aio_proc lock before the FTE_RELE.
	 */

	for (queuep = FIRST_ELEM(ap_compq), nextqp = NEXT_ELEM(queuep);
	     nfound < nwanted && queuep != END_QUEUE(ap_compq);
	     queuep = nextqp, nextqp = NEXT_ELEM(nextqp)) {

		/*
		 * Check aio_lhead and A_LISTNOTIFY to avoid ripping away jobs
		 * for which an lwp is waiting.
		 */
		/* LINTED pointer alignment */
		aiop = QPTOAIOP(queuep);

		if (!aiop->aio_lhead ||
		    !(aiop->aio_lhead->aio_kflags & A_LISTNOTIFY)) {
			statusp->ast_count = aiop->aio_nbytes;
			statusp->ast_errno = aiop->aio_errno;
			statusp->ast_cbp = aiop->aio_cbp;
			statusp->ast_apflag = aiop->aio_apflag;
			nfound++;
			statusp++;
			aiop->aio_kflags = 0;
			DEL_QUEUE(&aiop->aio_queue);
			ADD_QUEUE(&holdqueue, &aiop->aio_queue);
		}
	}
	if (nfound < nwanted)
		*aio_procp->ap_kpolladdr = 0;
	AIO_PROC_UNLOCK(aio_procp, plbase);

	/*
	 * Now that the lock is dropped, and the elements are on the
	 * local list, get rid of file references.  Also remember the
	 * number of elements for the call to aio_list_free.
	 */
	for (queuep = FIRST_ELEM(&holdqueue); queuep != END_QUEUE(&holdqueue);
	     queuep = NEXT_ELEM(queuep)) {
		/* LINTED pointer alignment */
		aiop = QPTOAIOP(queuep);
		if (aiop->aio_filep)
			FTE_RELE(aiop->aio_filep);
	}
	if (nfound)
		/* LINTED pointer alignment */
		aio_list_free(nfound, QPTOAIOP(FIRST_ELEM(&holdqueue)),
			      /* LINTED pointer alignment */
			      QPTOAIOP(LAST_ELEM(&holdqueue)));

	aioresult.ar_total = nfound;
	as_unlock(procp->p_as);
	aio_pollstats[nfound]++;
	/*
	 * Do the copyout after unlocking the address space, to avoid
	 * potential deadlocks on the as lock during fault handling.
	 */
	if (copyout((caddr_t)&aioresult, arg,
	     sizeof(aioresult) - (NSTATUS-nfound)*sizeof(struct aiostatus)))
		error = EFAULT;

	return error;

unlock:
	as_unlock(procp->p_as);
	return error;
}


/*
 * STATIC int
 * aio_gettune(caddr_t arg)
 *	aio driver operation to retrieve driver tuneable values.
 *
 * Calling/Exit State:
 *	'arg' is the address of the user's result buffer.
 *	No locks assumed.  Takes locks only if copyout does.  Returns
 *	an errno.
 *
 * Description:
 */
STATIC int
aio_gettune(caddr_t arg)
{
	aio_tune_t aio_tune;

	aio_tune.at_listio_max = aio_listio_max;
	aio_tune.at_num_ctlblks = numaio;
	if (copyout((caddr_t)&aio_tune, arg, sizeof(aio_tune)))
		return EFAULT;
	return 0;
}

/*
 * STATIC caddr_t
 * aio_getpoll(vaddr_t addr)
 *	aio driver operation to lock down the user poll address.
 *
 * Calling/Exit State:
 *	addr is the user virtual address of the poll location.
 *	Must be called with the current process's address space write
 *	locked; preserves the lock.  In non-error cases, returns the
 *	kernel poll address.  Call only at base level.
 *
 * Description:
 *	Return the address that the kernel will use to tell the library
 *	that results are ready to be picked up with AIOPOLL.
 */
STATIC caddr_t
aio_getpoll(vaddr_t addr)
{
	struct proc *procp;
	struct as *asp;
	caddr_t kaddr;                  /* kernel address for poll info */
	vaddr_t raddr;                  /* rounded down addr */
	u_int rsize;                    /* rounded up size */
	int	error;

	procp = u.u_procp;
	asp = procp->p_as;

	raddr = (vaddr_t)(addr & PAGEMASK);
	rsize = ((vaddr_t)addr + sizeof(char));
	rsize = ((rsize + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	if (as_aio_able(asp, raddr, rsize) != B_TRUE)
		return NULL;

	error = as_ctl(asp, addr, sizeof(char), MC_LOCK, NULL, NULL);
	if (error) {
		return NULL;
	}

	kaddr = physmap64(vtop64((caddr_t)addr, procp), sizeof(char), KM_NOSLEEP);
	if (kaddr == NULL) {
		(void)as_ctl(asp, addr, sizeof(char), MC_UNLOCK, NULL, NULL);
		return NULL;
	}
	return kaddr;
}

/*
 * STATIC void
 * aio_freepoll(vaddr_t addr, caddr_t kaddr, struct as *asp)
 *	aio driver operation to unlock user poll address.
 *
 * Calling/Exit State:
 *	addr is the user virtual address of the poll location.
 *	kaddr is the kernel virtual address of the poll location.
 *	Must be called with the current process's address space write
 *	locked (or single threaded during exit); preserves the lock. 
 *	Call only at base level.
 *	We need to pass in the address space because by the time
 *	we get here during exit/relvm, p_as has been nulled out.
 *
 * Description:
 *	Free the kernel physical mapping.  Unlock the user address.
 */
STATIC void
aio_freepoll(vaddr_t addr, caddr_t kaddr, struct as *asp)
{

	physmap_free(kaddr, sizeof(char), 0);
	(void)as_ctl(asp, addr, sizeof(char), MC_UNLOCK, NULL, NULL);
}

struct aio_invals {
	int aio_badcnt;
	int aio_badalign;
	int aio_badgetf;
	int aio_badtype;
	int aio_badrange;
	int aio_badsize;
} aio_invals;

/*
 * STATIC int
 * aio_format_req(aiojob_t *aiojobp, aio_t *aiop)
 *	Format *aiop to reflect the job request in *aiojobp.
 *
 * Calling/Exit State:
 *	Assumes that the address space lock is held, protecting aioprocp.
 *
 * Description:
 *	Validates the arguments in *aiojobp, and initializes *aiop.  In
 *	the success case, (*aiop)->aio_filep holds a reference to the file
 *	structure for the raw slice.
 */
STATIC int
aio_format_req(aiojob_t *aiojobp, aio_t *aiop)
{
	vnode_t	*vp;
	file_t	*fp;
	dev_t	dev;
	int	rw;
	int	error;
	struct buf *bp;
	u_int	major;
	u_int	bmajor;
	static u_int	last_bmajor;
	aio_range_t *aiorangep;
	extern void do_strategy(buf_t *);

	/* Size of I/O cannot be negative. */

	if (aiojobp->aj_cnt < 0) {
		aio_invals.aio_badcnt++;
		return EINVAL;
	}

	/* I/O sizes and counts have to be in multiples of sector sizes. */

	if ((aiojobp->aj_cnt & (NBPSCTR -1)) ||
	    (aiojobp->aj_offset & (NBPSCTR -1))) {
		aio_invals.aio_badalign++;
		return EINVAL;
	}

	if (error = getf(aiojobp->aj_fd, &fp)) {
		aio_invals.aio_badgetf++;
		return error;
	}

	if (aiojobp->aj_cmd == AIOREAD && !(fp->f_flag & FREAD) ||
	    aiojobp->aj_cmd == AIOWRITE && !(fp->f_flag & FWRITE)) {
		FTE_RELE(fp);
		return EBADF;
	}

	vp = fp->f_vnode;
	dev = vp->v_rdev;

	if (vp->v_type != VCHR) {
		FTE_RELE(fp);
		aio_invals.aio_badtype++;
		return EINVAL;
	}

	/* Verify that the user buffer is in the locked memory range. */
	if ((aiorangep = aio_findrange(u.u_procp, aiojobp->aj_buf,
			aiojobp->aj_cnt)) == NULL) {
		FTE_RELE(fp);
		aio_invals.aio_badrange++;
		return EINVAL;
	}


	/*
	 * If we need to call as_aio_prep() before the I/O, do so now.
	 */
	if (aiorangep->ar_needs_prep) {
		error = as_aio_prep(u.u_procp->p_as, aiojobp->aj_buf,
					aiojobp->aj_cnt, &aiop->aio_saved_segp);
		if (error != 0) {
			FTE_RELE(fp);
			return error;
		}
	}

	aiop->aio_strat = do_strategy;
	aiop->aio_cbp = aiojobp->aj_cbp;
	aiop->aio_apflag = aiojobp->aj_apflag;
	aiop->aio_kflags = aiojobp->aj_flag & A_USERFLAGS;
	aiop->aio_filep = fp;
	aiop->aio_nbytes = aiojobp->aj_cnt;

	rw = (aiojobp->aj_cmd == AIOREAD) ? B_READ : B_WRITE;

	/*
	 * Initialize the info in the buf struct based on the info provided in
	 * the aiojob structure.
	 */

	bp = &aiop->aio_buf;
	bp->b_error = 0;
	bp->b_proc = u.u_procp;
	bp->b_flags = B_ASYNC | B_KERNBUF | B_BUSY | B_PHYS | rw;
	bp->b_edev = dev;
	spec_tagbp_vp(bp,vp);
	bp->b_blkno = btodt(aiojobp->aj_offset);
	bp->b_bufsize = bp->b_bcount = aiojobp->aj_cnt;
	bp->b_un.b_addr = (caddr_t)aiojobp->aj_buf;
	bp->b_iodone = aio_done;
	bp->b_misc = (char *)aiop;

#ifdef DEBUG
	if (aiodebug)
		cmn_err(CE_CONT, "flags:%x  off:%Lx buf:%x cnt:%x\n",
			bp->b_flags, aiojobp->aj_offset, bp->b_un.b_addr,
			bp->b_bcount);
#endif
	return 0;
}

/*
 * STATIC void
 * aio_issue_req(aio_t *aiop)
 *	Submit the job described by *aiop to the strategy routine,
 *	which is responsible for attending to dma restrictions and other
 *	constraints.  Errors are reported through b_error to aio_done.
 *
 * Calling/Exit State:
 *	Call holding no locks for which aio_done might contend, i.e., the
 *	aio_proc lock.
 *
 * Description:
 *	Earlier versions of the async driver concerned themselves with
 *	I/O constraints such as buffer misalignment, non-DMAable memory,
 *	page-crossing and so on.  These were handled in aio_issue_req
 *	and its subsidiaries so as to minimize the likelihood of blocking.
 *
 *	In modifying the driver to allow transfers greater than 4K in size,
 *	we now rely on the strategy routine to handle such constraints
 *	(by resort to buf_breakup).  This introduces the possibility
 *	that underlying code may block, a reasonable tradeoff for
 *	advantages including support for large transfers, much simpler
 *	code in the async driver, and central management of I/O constraints.
 */
STATIC void
aio_issue_req(aio_t *aiop)
{
	struct buf *bp = &aiop->aio_buf;

	/*
	 * Zero-length requests are short-circuited here;
	 * buf_breakup does not want to see them.
	 */
	if (bp->b_bcount > 0)
		(*aiop->aio_strat)(bp);
	else
		aio_done(bp);
}

/*
 * STATIC void
 * aio_listio_done(aio_proc_t *aio_procp, aio_t *head, aio_t *tail)
 *	Destroy the identity of the list delimited by head and tail, and
 *	place the elements on the completion queue for aio_procp.
 *
 * Calling/Exit State:
 *	Assumes caller protects aio_procp.  Takes no locks.
 *
 * Description:
 */
STATIC void
aio_listio_done(aio_proc_t *aio_procp, aio_t *head, aio_t *tail)
{
	head->aio_lhead = NULL;
	head->aio_ltail = NULL;
	head->aio_lcount = 0;
	aio_list_del(head, tail);
	aio_list_add(AIO_PROC_COMPQP(aio_procp), head, tail);
}

/*
 * STATIC int
 * aio_listio_post_proc(aiolistio_t *aiolistp, aio_t *head, aio_t *tail,
 *			u_int listsize)
 *	Clean up the list of listsize elements delimited by head and tail,
 *	deleting it from queuep, and returning it to the global aio free list.
 *
 * Calling/Exit State:
 *	Caller must hold no spin locks.  Takes and releases the aio free list
 *	lock.  Call only at base level, because it may block.  Returns the
 *	number of errors detected in the aio structures.
 *
 * Description:
 *	*aiolistp is normally in the stack of the caller, to protect it
 *	without locks.   Gives up file references from aio structures on
 *	the list, so it may block.
 */
STATIC int
aio_listio_post_proc(aiolistio_t *aiolistp, aio_t *head, aio_t *tail,
		     u_int listsize)
{
	aiojob_t *aiojobp;
	aio_t	*aiop;
	aio_queue_t aio_queue, *queuep;
	u_int	i;
	int	nerrors = 0;

	ASSERT(KS_HOLD0LOCKS());
	INIT_QUEUE(&aio_queue);
	aio_list_add(&aio_queue, head, tail);
	for (i = 0, queuep = FIRST_ELEM(&aio_queue),
	      aiojobp = aiolistp->al_jobs;
	     i < listsize;
	     i++, queuep = NEXT_ELEM(queuep), aiojobp++) {
		/* LINTED pointer alignment */
		aiop = QPTOAIOP(queuep);
		if ((aiojobp->aj_errno = aiop->aio_errno) != 0)
			nerrors++;
		ASSERT(aiojobp->aj_errno != EINPROGRESS);
		ASSERT(aiop->aio_errno ||
		       aiop->aio_cbp == aiojobp->aj_cbp && aiop->aio_filep);
		if (aiop->aio_filep)
			FTE_RELE(aiop->aio_filep);
		aiojobp->aj_cnt = aiop->aio_nbytes;
		aiop->aio_kflags = 0;
	}

	/*
	 * Delete the list from the aio_queue and add it back to the free
	 * list.
	 */

	if (listsize) {
		aio_list_del(head, tail);
		aio_list_free(listsize, head, tail);
	}
	return nerrors;
}

/*
 * STATIC void
 * aio_done(struct buf *bp)
 *	aio completion routine, called from biodone.
 *
 * Calling/Exit State:
 *	Takes and releases the aio_proc lock for the process that initiated
 *	the I/O.
 *
 * Description:
 *	Transfers error numbers and byte counts from bp to the associated aio
 *	structure.  Moves aio structure to the completion queue, where a poll
 *	can find it.  (For listio, does so only when all I/Os on the list are
 *	done.)
 */
STATIC void
aio_done(struct buf *bp)
{
	struct proc *procp;
	aio_proc_t *aio_procp;
	aio_t *aiop, *lheadp, *head, *tail;
	int	error;
	pl_t	pl;

	if (bp->b_scgth.sg_format) {
		if (!biocanblock(bp))
			return;
		scgth_free(&bp->b_scgth);
	}

	error = geterror(bp);
#ifdef DEBUG
	if (error)
		cmn_err(CE_CONT, "aio_done error %d\n", error);
#endif

	ASSERT(bp->b_misc != NULL);
	aiop = (aio_t *)bp->b_misc;

	bp_mapout(bp);
	bp->b_flags |= B_DONE;
	bp->b_resid = 0;

	/*
	 * Only at this point does b_proc become valid.  The above code SHOULD
	 * be safe unlocked, because nothing will touch the aio structure and
	 * buffer header.  (The I/O is done, so the driver won't, and no
	 * other lwp will touch it until we move it to the completed list.)
	 */

	procp = bp->b_proc;
	aio_procp = AIO_PROC(procp);

	/*
	 * Do any special postprocessing here.
	 *
	 * aio_nbytes is used here to make sure that the range unprep()ed
	 * matches the range prep()ed.  bp->b_bcount could also be used
	 * here.
	 */
	if (aiop->aio_saved_segp != NULL) {
		as_aio_unprep(aiop->aio_saved_segp, (vaddr_t) bp->b_un.b_addr,
							aiop->aio_nbytes);
		aiop->aio_saved_segp = NULL;
	}

	pl = AIO_PROC_LOCK(aio_procp);
	aiop->aio_nbytes = ((error == 0) ? bp->b_bcount : -1);
	aiop->aio_errno = error;

	if (ISPART_OF_SUBLIST(aiop)) {
		lheadp = LIST_HEAD(aiop); /* Get head of sub list */
		ASSERT(lheadp->aio_lcount > 0);
		lheadp->aio_lcount--;	/* Dec cnt of I/O done for sublist */

		/* If not head of sublist reset your pointers */

		if (NOTHEAD_OF_SUBLIST(aiop))
			aiop->aio_lhead = aiop->aio_ltail = NULL;

		/* Check whether the entire list of I/O reqs is completed */
		if (!lheadp->aio_lcount) {
			head = lheadp->aio_lhead; /* Head of new sub list */
			tail = lheadp->aio_ltail; /* Tail of new sub list */

			aio_listio_done(aio_procp, head, tail);

			/*
			 * Either send a signal to the process or wakeup the
			 * process.
			 */

			if (head->aio_kflags & A_LISTNOTIFY)
				sigtoproc(procp, SIGEMT, (sigqueue_t *)NULL);
			else
				SV_BROADCAST(&head->aio_sv, 0);
		}
	} else {
		DEL_QUEUE(&aiop->aio_queue);	/* Del elem from waitq */
		ADD_QUEUE(AIO_PROC_COMPQP(aio_procp), &aiop->aio_queue);

		if (aiop->aio_kflags & A_ASYNCNOTIFY)
			 sigtoproc(procp, SIGEMT, (sigqueue_t *)NULL);
		else
			*aio_procp->ap_kpolladdr = 1;
	}

	if (aio_procp->ap_flags & AP_STOP) {
		aio_procp->ap_flags &= ~AP_STOP;
		SV_BROADCAST(&aio_procp->ap_cleansv, 0);
	}
	bp->b_flags &= ~B_PHYS;
	bp->b_iodone = NULL;
	bp->b_misc = NULL;
	AIO_PROC_UNLOCK(aio_procp, pl);
}

/*
 * STATIC aio_proc_t *aio_proc_alloc(void)
 *	Return an initialized, locked aio_proc structure.  Can block.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Currently a wrapper around kmem_zalloc.  Passes the KM_SLEEP flag.
 */
STATIC aio_proc_t *
aio_proc_alloc(void)
{
	aio_proc_t *aio_procp;

	aio_procp = kmem_zalloc(sizeof(*aio_procp), KM_SLEEP);
	INIT_QUEUE(AIO_PROC_WAITQP(aio_procp));
	INIT_QUEUE(AIO_PROC_COMPQP(aio_procp));
	LOCK_INIT(&aio_procp->ap_lock, AIO_HIER_PROC, pldisk, &aio_proc_lkinfo,
		  KM_SLEEP);
	SV_INIT(&aio_procp->ap_cleansv);
	AIO_PROC_LOCK(aio_procp);
	return aio_procp;
}

/*
 * STATIC void aio_proc_free(aio_proc_t *aio_procp)
 *	Free an aio_proc structure.
 *
 * Calling/Exit State:
 *	aio_proc is assumed held.
 *
 * Description:
 *	Currently a wrapper around kmem_free.
 */
STATIC void
aio_proc_free(aio_proc_t *aio_procp)
{
	AIO_PROC_UNLOCK(aio_procp, plbase);
	LOCK_DEINIT(&aio_procp->ap_lock);
	if (aio_procp->ap_listio != NULL)
		kmem_free(aio_procp->ap_listio, listio_size);
	kmem_free(aio_procp, sizeof(*aio_procp));
}

int aio_noaios;

/*
 * STATIC aio_t *aio_aio_alloc(void)
 *	Return a pointer to an initialized aio structure, or NULL for failure.
 *
 * Calling/Exit State:
 *	Takes and releases the aio free list lock.
 *
 * Description:
 *	Updates the free list and its counter.  Initializes all structure
 *	members.
 *	
 */
STATIC aio_t *
aio_aio_alloc(void)
{
	aio_t	*aiop;
	pl_t	pl;

	pl = AIO_FREE_LOCK();
	if (aio_free.count) {
		aio_free.count--;
		/* LINTED pointer alignment */
		aiop = QPTOAIOP(aio_free.queue.aq_fowd);
		aiop->aio_queue.aq_fowd->aq_back = aiop->aio_queue.aq_back;
		aiop->aio_queue.aq_back->aq_fowd = aiop->aio_queue.aq_fowd;
		aiop->aio_queue.aq_fowd = NULL;
		aiop->aio_queue.aq_back = NULL;
		aio_aio_init(aiop);
	} else {
		ASSERT(aio_free.queue.aq_fowd == &aio_free.queue);
		ASSERT(aio_free.queue.aq_back == &aio_free.queue);
		aio_noaios++;
		aiop = NULL;
	}
	AIO_FREE_UNLOCK(pl);
	return aiop;
}

/*
 * STATIC void aio_aio_free(aio_t *)
 *	Free an aio structure.
 *
 * Calling/Exit State:
 *	Takes and releases the aio free list lock.
 *
 * Description:
 *	Updates the free list and its counter.
 */
STATIC void
aio_aio_free(aio_t *aiop)
{
	pl_t	pl;

	pl = AIO_FREE_LOCK();
	aiop->aio_queue.aq_fowd = aio_free.queue.aq_fowd;
	aiop->aio_queue.aq_back = aio_free.queue.aq_fowd->aq_back;
	aio_free.queue.aq_fowd->aq_back = &aiop->aio_queue;
	aio_free.queue.aq_fowd = &aiop->aio_queue;
	aio_free.count++;
	AIO_FREE_UNLOCK(pl);
}

/*
 * STATIC int aio_list_alloc(int nelem, aio_t **head, aio_t **tail)
 *	Allocate a list of up to nelem aio structures, suitably initialized
 *	for listio.  Returns the number of elements actually allocated.
 *
 * Calling/Exit State:
 *	Takes and releases the aio free list lock.
 *
 * Description:
 *	Updates *head and *tail with pointers delimiting the list.
 *	Updates the free list and its counter.  Initializes all elements
 *	in the returned structures.  In particular, they are threaded
 *	through aio_queue, each points to **head, and (*head)->aio_ltail
 *	points to *tail.
 */
STATIC int
aio_list_alloc(int nelem, aio_t **head, aio_t **tail)
{
	int i;
	aio_t *aiop;
	int count;
	pl_t	pl;

	pl = AIO_FREE_LOCK();
	count = MIN(aio_free.count, nelem);
	if (count) {
		aio_free.count -= count;
		/* LINTED pointer alignment */
		*head = aiop = QPTOAIOP(FIRST_ELEM(&aio_free.queue));
		for (i = 0; i < count; i++) {
			aio_aio_init(aiop);
			/* LINTED pointer alignment */
			aiop = QPTOAIOP(NEXT_ELEM(&aiop->aio_queue));
		}
		/* LINTED pointer alignment */
		*tail = QPTOAIOP(aiop->aio_queue.aq_back);
		(*head)->aio_ltail = (*tail);
		aio_free.queue.aq_fowd = (aio_queue_t *)aiop;
		aiop->aio_queue.aq_back = &aio_free.queue;
		aiop->aio_filep = NULL;
		(*head)->aio_queue.aq_back = &(*tail)->aio_queue;
		(*tail)->aio_queue.aq_fowd = &(*head)->aio_queue;
	}
	AIO_FREE_UNLOCK(pl);
	return count;
}

/*
 * STATIC void aio_list_free(int nelem, aio_t *head, aio_t *tail)
 *	Free the list of nelem aio structures delimited by head and tail.
 *
 * Calling/Exit State:
 *	Takes and releases the aio free list lock.  Assumes that the list
 *	is threaded through aio_queue.
 *
 * Description:
 */
STATIC void
aio_list_free(int nelem, aio_t *head, aio_t *tail)
{
	pl_t	pl;

	if (nelem) {
		pl = AIO_FREE_LOCK();
		aio_list_add(&aio_free.queue, head, tail);
		aio_free.count += nelem;
		AIO_FREE_UNLOCK(pl);
	}
}

/*
 * STATIC void aio_aio_init(struct aio *aiop)
 *	Initialize the non-queue, non-sv elements of *aiop.
 *
 * Calling/Exit State:
 *	Assumes caller protects *aiop.
 *
 * Description:
 */
STATIC void
aio_aio_init(struct aio *aiop)
{
	aiop->aio_filep = NULL;
	aiop->aio_nbytes = 0;
	aiop->aio_errno = 0;
	aiop->aio_kflags = 0;
	aiop->aio_cbp = NULL;
	aiop->aio_apflag = 0;
	aiop->aio_lhead = NULL;
	aiop->aio_ltail = NULL;
	aiop->aio_lcount = 0;
	aiop->aio_saved_segp = NULL;
	bzero(&aiop->aio_buf, sizeof(aiop->aio_buf));
	aiop->aio_buf.b_flags |= B_KERNBUF | B_BUSY;
}

/*
 * STATIC void aio_list_del(aio_t *head, aio_t *tail)
 *	Delete a sub list delimited by head and tail from their list.
 *
 * Calling/Exit State:
 *	Assumes caller protects the list.
 *
 * Description:
 */
STATIC void
aio_list_del(aio_t *head, aio_t *tail)
{
	ASSERT(head != NULL);
	ASSERT(tail != NULL);
#ifdef DEBUG
	if (aiodebug)
		cmn_err(CE_CONT, "del_list: head:%x tail:%x\n", head, tail);
#endif
	head->aio_queue.aq_back->aq_fowd = tail->aio_queue.aq_fowd;
	tail->aio_queue.aq_fowd->aq_back = head->aio_queue.aq_back;
	head->aio_queue.aq_back = &tail->aio_queue;
	tail->aio_queue.aq_fowd = &head->aio_queue;
}

/*
 * STATIC void aio_list_add(aio_t *head, aio_t *tail)
 *	Add a sub list represented by head and tail to the list provided.
 *
 * Calling/Exit State:
 *	Assumes caller protects the list.
 *
 * Description:
 */
STATIC void
aio_list_add(aio_queue_t *list, aio_t *head, aio_t *tail)
{
	ASSERT(head != NULL);
	ASSERT(tail != NULL);
#ifdef DEBUG
	if (aiodebug)
		cmn_err(CE_CONT, "add_list: head:%x tail:%x\n", head, tail);
#endif
	head->aio_queue.aq_back = list;
	tail->aio_queue.aq_fowd = list->aq_fowd;
	list->aq_fowd->aq_back = &tail->aio_queue;
	list->aq_fowd = &head->aio_queue;
}

/*
 * STATIC void aio_intersect(struct as *as, vaddr_t base, size_t size)
 *	If as is the current process's address space, and the range intersects
 *	the process's aio buffer, wait for aio to complete, and tear down
 *	the aio_proc structure.
 *
 * Calling/Exit State:
 *	Assumes the caller holds the address space write-locked.  Takes the
 *	aio_proc lock.  If I/O is not yet completed and reaped, will also
 *	take the aio free list lock, and give up file references.  Call
 *	only at base level.
 *
 * Description:
 *	Buggy programs that do things like shrinking breaks while continuing
 *	aio on an intersecting range will block on the as lock.  Therefore
 *	this routine will not delay indefinitely.  After the aio_proc is gone,
 *	an attempt at aio will get an error return.
 */
void
aio_intersect(struct as *as, vaddr_t base, size_t size)
{
	proc_t *pp = u.u_procp;

	/*
	 * If this this isn't our as, we can't do any checking.
	 */
	if (pp->p_as != as)
		return;

	if (!aio_range_overlaps(pp, base, size))
		return; /* no intersection */

	/*
	 * We do intersect.  Wait for all IO to finish.
	 * We are holding off new IO because we hold the
	 * write lock on the address space.  After all
	 * IO is finished, remove the AIO_MEMLOCK.
	 */
	aio_waitforio(pp);
	aio_cleanall(pp, as);
}

/*
 * STATIC void aio_as_free(struct as *as)
 *	The argument should be the current process's address space.
 *	Wait for aio to complete, and deallocate the aio_proc structure.
 *
 * Calling/Exit State:
 *	Assumes the caller holds the address space write-locked.  Takes the
 *	aio_proc lock.  If I/O is not yet completed and reaped, will also
 *	take the aio free list lock, and give up file references.  Call
 *	only at base level.
 *
 * Description:
 *	Formerly called from as_free(), where the argument might not have
 *	been the current process's address space; now called from relvm(),
 *	where this should always be true.
 *	A process calling here is being torn down, and is single threaded,
 *	therefore will not be here indefinitely.
 */
void
aio_as_free(struct as *as)
{
	proc_t *pp = u.u_procp;

	ASSERT(AIO_PROC(pp));
	ASSERT(as == pp->p_as);

	aio_waitforio(pp);
	aio_cleanall(pp, as);
}

/*
 * STATIC void aio_cleanall(struct proc *procp, struct as *asp)
 *	Destroy all aio state in the process.
 *
 * Calling/Exit State:
 *	Assumes the caller holds the address space write-locked, and that there
 *	is no pending aio.  Takes the aio_proc lock.  If aio is not yet reaped,
 *	will also take the aio free list lock, and give up file references.
 *	Call only at base level.
 *
 * Description:
 *	After the aio_state is gone, an attempt at aio will get an error
 *	return.
 */
STATIC void
aio_cleanall(struct proc *procp, struct as *asp)
{
	aio_t	*aiop, *aiop_prev;
	aio_proc_t *aio_procp;
	aio_range_t *aio_rangelist, *aio_rangep;
	aio_queue_t aio_queue, *queuep;
	u_int nelem;
	vaddr_t polladdr;
	caddr_t kpolladdr;

	ASSERT(KS_HOLD0LOCKS());
	aio_procp = AIO_PROC(procp);
	ASSERT(aio_procp);

	(void)AIO_PROC_LOCK(aio_procp);

	aio_rangelist = aio_procp->ap_rangelist;
	polladdr = aio_procp->ap_polladdr;
	kpolladdr = aio_procp->ap_kpolladdr;

	ASSERT(ISNULL_QUEUE(AIO_PROC_WAITQP(aio_procp)));

	/*
	 * If there are I/O requests on the completed queue, delete them, and
	 * put them on the local list, so we can drop the aioproc lock before
	 * calling FTE_RELE.  When the completed queue is clean, free the
	 * aio_proc structure.
	 */

	if (!ISNULL_QUEUE(AIO_PROC_COMPQP(aio_procp))) {

		/* Pick up list head and tail. */
		/* LINTED pointer alignment */
		aiop = QPTOAIOP(FIRST_ELEM(AIO_PROC_COMPQP(aio_procp)));
		/* LINTED pointer alignment */
		aiop_prev = QPTOAIOP(LAST_ELEM(AIO_PROC_COMPQP(aio_procp)));

		aio_list_del(aiop, aiop_prev);
		aio_proc_free(aio_procp);	/* Drops aio_proc lock. */
		INIT_QUEUE(&aio_queue);
		aio_list_add(&aio_queue, aiop, aiop_prev);

		/*
		 * Now that the lock is dropped, and the elements are on the
		 * local list, get rid of file references.  Also remember the
		 * number of elements for the call to aio_list_free.
		 */
		for (nelem = 0, queuep = FIRST_ELEM(&aio_queue);
		     queuep != END_QUEUE(&aio_queue);
		     nelem++, queuep = NEXT_ELEM(queuep)) {
			/* LINTED pointer alignment */
			aiop = QPTOAIOP(queuep);
			if (aiop->aio_filep)
				FTE_RELE(aiop->aio_filep);

			ASSERT(aiop->aio_saved_segp == NULL);
		}
		/* LINTED pointer alignment */
		aio_list_free(nelem, QPTOAIOP(FIRST_ELEM(&aio_queue)),
			      aiop_prev);
	} else
		aio_proc_free(aio_procp);	/* Drops aio_proc lock. */

	AIO_PROC(procp) = NULL;

	/* Unlock the pages that were aio_memlocked. */

	while ((aio_rangep = aio_rangelist) != NULL) {
		(void)as_ctl(asp, aio_rangep->ar_vaddr, aio_rangep->ar_size,
			MC_UNLOCK, NULL, NULL);
		aio_rangelist = aio_rangep->ar_next;
		if (aio_rangelist == aio_rangep)
			aio_rangelist = NULL;
		else {
			aio_rangelist->ar_prev = aio_rangep->ar_prev;
			aio_rangelist->ar_prev->ar_next = aio_rangelist;
		}
		kmem_free(aio_rangep, sizeof(*aio_rangep));
	}
	aio_freepoll(polladdr, kpolladdr, asp);
}


/*
 * STATIC void aio_waitforio(proc_t *pp)
 *	Wait for all pending aio in the process to complete.
 *
 * Calling/Exit State:
 *	Assumes the caller holds the address space write-locked.  Takes the
 *	aio_proc lock.  Call only at base level.
 *
 * Description:
 *	While I/O is pending, waits on aio_procp->ap_cleansv.  Buggy programs
 *	that do things like shrinking breaks while continuing aio on an
 *	intersecting range will block on the as lock.  Therefore this routine
 *	will not delay indefinitely.
 */
STATIC void
aio_waitforio(proc_t *pp)
{
	aio_proc_t *aio_procp = AIO_PROC(pp);
	pl_t	pl = AIO_PROC_LOCK(aio_procp);

	while (!ISNULL_QUEUE(AIO_PROC_WAITQP(aio_procp))) {
		aio_procp->ap_flags |= AP_STOP;
		SV_WAIT(&aio_procp->ap_cleansv, PRIBUF, &aio_procp->ap_lock);
		(void)AIO_PROC_LOCK(aio_procp);
	}
	AIO_PROC_UNLOCK(aio_procp, pl);
}

/*
 * STATIC aio_range_t *aio_findrange(proc_t *procp, vaddr_t rbase,
 *		u_int rsize)
 *	Find the aio_range_t which contains the specified address range.
 *
 * Calling/Exit State:
 *	Assumes the caller holds the address space write-locked on entry
 *	and leaves it write-locked.
 *
 *	Returns pointer to the aio_range_t containing the specified
 *	address range, if it exists, otherwise returns NULL.
 */
STATIC aio_range_t *
aio_findrange(proc_t *procp, vaddr_t rbase, u_int rsize)
{
	aio_proc_t *aioprocp;
	aio_range_t *aiorangep;
	vaddr_t rend = rbase + rsize;

	if ((aioprocp = AIO_PROC(procp)) != NULL) {
		aiorangep = aioprocp->ap_rangelist;
		do {
			if ((AIO_RANGE_BASE(aiorangep) <= rbase) &&
					(rend <= AIO_RANGE_END(aiorangep)))
				return aiorangep;
			aiorangep = aiorangep->ar_next;
		} while ((aiorangep != aioprocp->ap_rangelist) &&
				(AIO_RANGE_END(aiorangep) >= rend));
	}
	return NULL;
}

/*
 * STATIC boolean_t aio_range_overlaps(proc_t *procp, vaddr_t rbase,
 *		u_int rsize)
 *	See if any aio_proc_t structure overlaps the specified range.
 *
 * Calling/Exit State:
 *	Assumes the caller holds the address space write-locked on entry
 *	and leaves it write-locked.
 *
 *	Returns B_TRUE if there is overlap and B_FALSE if not.
 */
STATIC boolean_t
aio_range_overlaps(proc_t *procp, vaddr_t rbase, u_int rsize)
{
	aio_proc_t *aioprocp;
	aio_range_t *aiorangep;
	vaddr_t rend = rbase + rsize;

	if ((aioprocp = AIO_PROC(procp)) != NULL) {
		aiorangep = aioprocp->ap_rangelist;
		do {
			if (((AIO_RANGE_BASE(aiorangep) <= rbase) &&
					(rbase < AIO_RANGE_END(aiorangep))) ||
				((rbase <= AIO_RANGE_BASE(aiorangep)) &&
					(AIO_RANGE_BASE(aiorangep) < rend)))
				return B_TRUE;
			aiorangep = aiorangep->ar_next;
		} while ((aiorangep != aioprocp->ap_rangelist) &&
				(AIO_RANGE_BASE(aiorangep) < rend));
	}
	return B_FALSE;
}
