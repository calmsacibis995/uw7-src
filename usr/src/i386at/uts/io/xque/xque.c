#ident	"@(#)xque.c	1.21"
#ident	"$Header$"

/*
 * X/window support -- provides support for an X-style 
 * shared memory keyboard/mouse event queue.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/mman.h>
#include <io/xque/xque.h>
#include <io/event/event.h>
#include <svc/clock.h>

#include <io/ddi.h>	/* Must come last. */


int _xq_enqueue(xqInfo *, xqEvent *);
int xq_enqueue(xqInfo *, xqEvent *);
int _xq_enqueue_scoevent(xqInfo *, xqEvent *);
int xq_enqueue_scoevent(xqInfo *, xqEvent *);

STATIC void	xq_settime(void);
STATIC clock_t	xq_get_timestamp(void);

int (*xq_enqueue_p)() = &_xq_enqueue;
int (*xq_enqueue_scoevent_p)() = &_xq_enqueue_scoevent;

/*
 * Global variables.
 */
xqInfo *xqhead;
toid_t xqtoid;


/*
 * caddr_t
 * xq_init(xqInfo *, int, int, int *)
 *
 * Calling/Exit State:
 *	Xq_init returns the user virtual address or NULL if there was an error.
 * 
 * Description:
 *	This routine is used to set up an event queue.
 *	It allocates space for the queue, maps it to a user address,
 *	and initializes the structure.
 */
caddr_t
xq_init(xqInfo *qinfo, int qsize, int signo, int *errorp)
{
	xqEventQueue	*q;
	int		npages;
	pl_t		oldpri;
	addr_t		vaddr;
	physreq_t 	*physreq;


	/*
	 * Compute size for requested # of elements; 
	 * round up to page boundary. 
	 */
	npages = btopr(sizeof(xqEventQueue) + (qsize - 1) * sizeof(xqEvent));

	/*
	 * Allocate npages of memory.
	 */
	physreq = physreq_alloc(KM_SLEEP);
	physreq->phys_align = ptob(1);
	q = NULL;
	if (physreq_prep(physreq, KM_NOSLEEP))
		q = kmem_alloc_physreq(ptob(npages), physreq, KM_NOSLEEP);
	physreq_free(physreq);
	if (q == NULL) {
		*errorp = ENOMEM;
		return NULL;
	}

	/*
	 * Compute the actual # of elements. 
	 */
	q->xq_size = (ptob(npages) - sizeof(xqEventQueue)) / sizeof(xqEvent) + 1;
	qinfo->xq_psize = q->xq_size;
	qinfo->xq_npages = npages;
	qinfo->xq_proc = proc_ref();
	qinfo->xq_signo = signo;
	qinfo->xq_ptail = q->xq_tail = q->xq_head = 0;
	qinfo->xq_queue = q;
	qinfo->xq_qtype = XQUE;
	qinfo->xq_addevent = xq_enqueue;
	qinfo->xq_devices = QUE_MOUSE|QUE_KEYBOARD;
	qinfo->xq_xlate = 0;
	qinfo->xq_qaddr = (char *) q;
	q->xq_sigenable = 0;

	/*
	 * Map the queue physical pages into the user address space.
	 */
	*errorp = drv_mmap((vaddr_t)q, 0, ptob(npages), (vaddr_t *)&vaddr, 
				PROT_READ|PROT_WRITE|PROT_USER,
				PROT_READ|PROT_WRITE|PROT_USER, 0);
	if (*errorp) {
		qinfo->xq_queue = NULL;
		kmem_free(q, ptob(npages));
		return (NULL);
	}

	qinfo->xq_uaddr = vaddr;

	oldpri = splhi();
	if (xqhead == (xqInfo *) NULL) { /* first vt to enable queue mode */
		qinfo->xq_next = (xqInfo *) NULL;
		xqtoid = itimeout(xq_settime, NULL, 
			drv_usectohz(30*1000000)|TO_PERIODIC, plhi); 
		if (xqtoid == 0) {
			drv_munmap((vaddr_t)qinfo->xq_uaddr, 
					ptob(qinfo->xq_npages));
			kmem_free(q, ptob(npages));
			*errorp = ENOMEM;
			return (NULL);
		}
	} else {
		qinfo->xq_next = xqhead;
		qinfo->xq_next->xq_prev = qinfo;
	}

	xqhead = qinfo;
	qinfo->xq_prev = (xqInfo *) NULL;
	splx(oldpri);

	return (vaddr);
}


/*
 * void
 * xq_close(xqInfo *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This routine is used to clean up when done with an event queue.
 *	It undoes the user mapping, frees the space, and clears the pointer. 
 */
void
xq_close(xqInfo *qinfo)
{
	caddr_t	qaddr = (caddr_t) qinfo->xq_queue;
	void	*procp;

	
	if (qinfo->xq_qtype == XQUE) {
		if (xqhead == qinfo) {		/* delete first vt in list */
			xqhead = qinfo->xq_next;
			if (xqhead != (xqInfo *) NULL)
				xqhead->xq_prev = (xqInfo *) NULL;
		} else {		/* delete vt elsewhere in list */
			qinfo->xq_prev->xq_next = qinfo->xq_next;
			if (qinfo->xq_next != (xqInfo *) NULL) /* last in list */
				qinfo->xq_next->xq_prev = qinfo->xq_prev;
		}

		procp = proc_ref();
		if (procp == qinfo->xq_proc)
			drv_munmap((vaddr_t)qinfo->xq_uaddr, 
				   ptob(qinfo->xq_npages));
		proc_unref(procp);

		kmem_free(qaddr, ptob(qinfo->xq_npages));
	}

	if (xqhead == (xqInfo *) NULL)
		untimeout(xqtoid);

	qinfo->xq_queue = NULL;
	if (qinfo->xq_proc)
		proc_unref(qinfo->xq_proc);
	qinfo->xq_proc = (void *) NULL;

	return;
}


/*
 * STATIC clock_t
 * xq_get_timestamp(void)
 *	This utility routine labels an event with a unique timestamp. 
 *
 * Calling/Exit State:
 *	- Returns the unique timestamp.
 */
STATIC clock_t
xq_get_timestamp(void)
{
	return (hrestime.tv_sec * 1000) + (hrestime.tv_nsec / 1000000);
}


/*
 * int
 * _xq_enqueue(xqInfo *, xqEvent *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This routine is used to add an event to a queue.
 *	It will set the xq_time field of the event; all other fields
 *	should already have been set.
 *
 *	No check is made to see if the queue is full. 
 *
 * Remarks:
 * 	All the real work is done by _xq_enqueue; xq_enqueue is the
 *	public interface. The indirection to _xq_enqueue allows us to
 *	intercept calls to xq_enqueue, which makes it possible to either
 *	passively record the events or to edit them before passing them 
 *	to _xq_enqueue. 
 */
int
_xq_enqueue(xqInfo *qinfo, xqEvent *ev)
{
	xqEventQueue *q = qinfo->xq_queue;
	boolean_t was_empty;


	was_empty = (qinfo->xq_ptail == q->xq_head);
	ev->xq_time = xq_get_timestamp();
	q->xq_curtime = ev->xq_time;
	q->xq_events[qinfo->xq_ptail] = *ev;
	q->xq_tail = qinfo->xq_ptail = (qinfo->xq_ptail + 1) % qinfo->xq_psize;

	if (q->xq_sigenable && was_empty) {
		ASSERT(proc_valid(qinfo->xq_proc));
		proc_signal(qinfo->xq_proc, qinfo->xq_signo);
	}

	return (0);
}


/*
 * int 
 * xq_enqueue(xqInfo *, xqEvent *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	See the description and remarks for _xq_enqueue().
 */
int
xq_enqueue(xqInfo *qinfo, xqEvent *ev)
{
	return ((*xq_enqueue_p)(qinfo, ev));
}


/*
 * STATIC void
 * xq_settime(void)
 * 
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Called every 30 seconds (approximately) that there is
 *	atleast one VT in X/queue mode. Calculate the current 
 *	time since 1/1/70 GMT in milliseconds, and run down
 *	the list of VTs updating the xq_curtime field of all
 *	VTs in X/queue mode with this value.
 */
STATIC void
xq_settime(void)
{
	clock_t milliseconds;
	xqInfo *tqinfo;


	milliseconds = xq_get_timestamp();
	tqinfo = xqhead;

	while (tqinfo != (xqInfo *) NULL) {
		tqinfo->xq_queue->xq_curtime = milliseconds;
		tqinfo = tqinfo->xq_next;
	}
}


/*
 * addr_t
 * xq_allocate_scoq(struct evchan *, int *)
 *
 * Calling/Exit State:
 *	Returns the user virtual address or 
 *	NULL if there was an error.
 *
 * Description:
 *	This routine is used to set up an event queue.
 *	It allocates space for the queue, maps it to a user address,
 *	and initializes the structure.
 */
addr_t
xq_allocate_scoq(struct evchan *evchanp, int *errorp)
{
	QUEUE	*q;
	int	npages;
	pl_t	oldpri;
	addr_t	vaddr;
	xqInfo  *qinfo = &evchanp->eq_xqinfo;
	physreq_t *physreq;


	*errorp = 0;
	npages = btopr(sizeof(QUEUE));

	/* Allocate the pages */
	physreq = physreq_alloc(KM_SLEEP);
	physreq->phys_align = ptob(1);
	q = NULL;
	if (physreq_prep(physreq, KM_NOSLEEP))
		q = kmem_alloc_physreq(ptob(npages), physreq, KM_NOSLEEP);
	physreq_free(physreq);
	if (q == NULL) {
		*errorp = ENOMEM;
		return NULL;
	}

	qinfo->xq_psize = QSIZE;
	qinfo->xq_npages = npages;
	qinfo->xq_proc = proc_ref();
	qinfo->xq_signo = 0;
	qinfo->xq_ptail = q->tail = q->head = 0;
	qinfo->xq_qtype = SCOQ;
	qinfo->xq_addevent = xq_enqueue_scoevent;
	qinfo->xq_devices  = 0;
	qinfo->xq_xlate    = 1;
	qinfo->xq_qaddr = (char *) q;

	spl0();
	*errorp = drv_mmap((vaddr_t)q, 0, ptob(npages), (vaddr_t *)&vaddr,
				PROT_READ|PROT_WRITE|PROT_USER,
				PROT_READ|PROT_WRITE|PROT_USER, 0);
	splx(plstr);
	if (*errorp) {
		qinfo->xq_queue = NULL;
		kmem_free(q, ptob(npages));
		return (NULL);
        }

	qinfo->xq_uaddr = vaddr;

#ifdef DEBUG
	cmn_err(CE_NOTE, 
		"user address for SCO Q is %x", qinfo->xq_uaddr);
#endif
	return vaddr;
}


/*
 * int
 * xq_close_scoq(xqInfo *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This routine is used to clean up when done with an event queue.
 *	It undoes the user mapping, frees the space, and clears the pointer. 
 */
int
xq_close_scoq(xqInfo *qinfo)
{
	caddr_t qaddr = (caddr_t)qinfo->xq_qaddr;
	void	*procp;


#ifdef DEBUG
	cmn_err(CE_NOTE, 
		"calling xq_close_scoq");	
#endif

	procp = proc_ref();
	if (procp == qinfo->xq_proc)
		drv_munmap((vaddr_t)qinfo->xq_uaddr, ptob(qinfo->xq_npages));
	proc_unref(procp);

	qinfo->xq_qaddr = NULL;
	if (qinfo->xq_proc)
		proc_unref(qinfo->xq_proc);
	qinfo->xq_proc = (void *)NULL;
	
	kmem_free(qaddr, ptob(qinfo->xq_npages));

	return (0);
}


/*
 * int
 * _xq_enqueue_scoevent(xqInfo *qinfo, xqEvent *ev)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This routine is used to add an event to a queue.
 *	It is similar to the xq_enqueue() function described above.
 *	It will set the xq_time field of the event; all other fields
 *	should already have been set.
 *
 *	No check is made to see if the queue is full. 
 *
 * Remarks:
 * 	All the real work is done by _xq_enqueue_scoevent; 
 * 	xq_enqueue_scoevent is the public interface. The indirection 
 *	to _xq_enqueue_scoevent allows us to intercept calls to 
 *	xq_enqueue_scoevent, which makes it possible to either
 *	passively record the events or to edit them before passing 
 *	them to _xq_enqueue_scoevent. 
 */
int
_xq_enqueue_scoevent(xqInfo *qinfo, xqEvent *ev)
{
	QUEUE	*q = (QUEUE *) qinfo->xq_qaddr;
	EVENT	scoev;
	struct evchan *evchanp = (struct  evchan *) qinfo->xq_private;
	extern void event_wakeup(struct evchan *);


#ifdef DEBUG
	cmn_err(CE_NOTE, "calling xq_enqueue_scoevent %x ACTIVE? %s",
		evchanp, evchanp->eq_state != EVCH_ACTIVE ? "NO" : "YES");
#endif

	if (evchanp->eq_state != EVCH_ACTIVE) {
		if (ev->xq_type == XQ_KEY)
			return 0;	
		return 1;
	}

	EV_TIME(scoev) = xq_get_timestamp();
	EV_DX(scoev)  = ev->xq_x;
	EV_DY(scoev)  = -ev->xq_y;
	EV_BUFCNT(scoev) = 1;

	switch(ev->xq_type) {
	case XQ_BUTTON: {
		char	buttons = 0;

		if ((ev->xq_code & 01) == 0)
			buttons |= RT_BUTTON;
		if ((ev->xq_code & 02) == 0)
			buttons |= MD_BUTTON;
		if ((ev->xq_code & 04) == 0)
			buttons |= LT_BUTTON;
		if (buttons == qinfo->xq_buttons)
			return 1;

		EV_TAG(scoev) = T_BUTTON;
		EV_BUTTONS(scoev) = buttons;
		qinfo->xq_buttons = buttons;
		break;
	}

	case XQ_MOTION: 
		EV_TAG(scoev) = T_REL_LOCATOR;
		break;

	case XQ_KEY: 
		EV_TAG(scoev) = T_STRING;
		EV_BUF(scoev)[0] = ev->xq_code;
		break;

	default:
#ifdef DEBUG
		cmn_err(CE_NOTE, "EVENT of unknown type %d", ev->xq_type);
#endif
		return 0;
	}

	if ((EV_TAG(scoev)&evchanp->eq_emask) == 0) {
#ifdef DEBUG
		cmn_err(CE_NOTE, 
			"EVENT of type %d is rejected", EV_TAG(scoev));
#endif
		return 0;
	}

	if (((q->head + 1) % QSIZE) == q->tail)
		q->tail = ((q->tail + 1) % QSIZE);
	q->queue[q->head] = scoev;
	q->head = (q->head + 1) % QSIZE;

	event_wakeup(evchanp);
	return 1;
}

/*
 * int
 * xq_enqueue_scoevent(xqInfo *, xqEvent *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	See description and remarks for _xq_enqueue_scoevent().
 */
int
xq_enqueue_scoevent(xqInfo *qinfo, xqEvent *ev)
{
	return((*xq_enqueue_scoevent_p)(qinfo, ev));
}


