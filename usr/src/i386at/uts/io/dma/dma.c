#ident	"@(#)dma.c	1.16"
#ident	"$Header$"

/*
 * This is the implementation of the kernel DMA interface for the
 * AT Class machine's using an Intel 8237A DMAC.
 * 
 * The following routines in the interface are implemented:
 *         dma_init()
 *         dma_intr()
 *         _dma_alloc()
 *         _dma_relse()
 *         dma_prog()
 *         dma_cascade()
 *         dma_enable()
 *         dma_disable()
 *         dma_get_best_mode()
 *         dma_get_chan_stat()
 *         dma_swsetup()
 *         dma_swstart()
 *         dma_stop()
 * 
 * And these support routines are included for managing the DMA structures:
 *         dma_get_cb()
 *         dma_free_cb()
 *         dma_get_buf()
 *         dma_free_buf()
 */

#include <fs/buf.h>
#include <io/dma.h>		/* dma.h includes i8237A.h */
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>

#define	DMAHIER		1
#define	DMAPL		plhi

#ifdef DMA_DEBUG
#ifdef INKERNEL
#define dprintf(x)      cmn_err(CE_NOTE, x)
#else
#define dprintf(x)      printf x
#endif
#else
#define dprintf(x)
#endif

/*
 * chtorq - convert channel number to request queue index.
 *
 * Normally all channels are used simultaneously and the
 * channel number is used as the index into the request
 * queues.  Some DMA chips, however, have bugs that only
 * allow one transfer to take place at a time.  In this
 * case, dma_single should be set non-zero so that the
 * channel number is converted into a chip number (0=8 bit,
 * 1=16 bit) forcing drivers to allocate the chip instead
 * of the channel. Some systems are even worse and may
 * only allow one DMA to be operational at any time in the 
 * system. So if dma_single is 2 the whole DMA is allocated.
 *
 *	dma_single = 0		all DMAs are enabled
 *		   = 1		one DMA per chip
 *		   = 2		one DMA per system
 */
#define chtorq(ch)	((dma_single) ? \
			  (dma_single == 2 ? 0 : (ch) >> 2) : \
			  (ch))

extern int dma_single;		/* defined in dma.cf/Space.c */

STATIC uchar_t	dma_initialized = 0;	/* the initialized flag */

STATIC int	_dma_alloc(int, uchar_t);
STATIC int	_dma_relse(int);

/* the dma channel semaphore structure */
STATIC unsigned char   dma_alloc_map[D37A_MAX_CHAN];

STATIC sv_t	dma_chan_sv[D37A_MAX_CHAN];
STATIC lock_t	dma_chan_lock[D37A_MAX_CHAN];

STATIC LKINFO_DECL(dma_chan_lock_lkinfo, "IO:DMA:DMA channel spin lock", 0);

/* count of cascade-mode users per channel */
STATIC uint_t	dma_cascade_count[D37A_MAX_CHAN];

/*
 * void
 * dma_init(void)
 *	Called during system init to initialization the dma interface,
 *	the DMAC, and any dma data structures.
 *
 * caller:	main()
 * calls:	d37A_init()
 *
 * Calling/Exit State:
 *	None.
 */
void
dma_init(void)
{
	int	i;

	if (!dma_initialized) {
		++dma_initialized;
		dprintf(("dma_init: initializing dma.\n"));

                /* initialize semaphore map */
                for (i = 0; i < D37A_MAX_CHAN; i++) {
			SV_INIT(&dma_chan_sv[i]);
			LOCK_INIT(&dma_chan_lock[i], DMAHIER, DMAPL, 
					&dma_chan_lock_lkinfo, KM_NOSLEEP);
                        dma_alloc_map[i] = 0;
		}

		/* initialize the 8237A DMAC */
		d37A_init();
	}
}


/*
 * uchar_t
 * dma_get_best_mode(struct dma_cb *dmacbptr)
 *	Confirm that data is aligned for efficient flyby mode.
 *
 * caller:	driver routines
 * calls:	d37A_get_best_mode()
 *
 * Calling/Exit State:
 *	None.
 */
uchar_t
dma_get_best_mode(struct dma_cb *dmacbptr)
{
	return(d37A_get_best_mode(dmacbptr));
}


/*
 * void
 * dma_intr(int lev)
 *	Service the interrupt from the DMAC.
 *
 * caller:	k_trap(), u_trap() through (*ivect[lev])()
 * calls:	d37A_intr()
 *
 * Calling/Exit State:
 *	None.
 */
void
dma_intr(int lev)
{
	dprintf(("dma_intr: calling d37A_intr(%d).\n", lev));
	/* call the d37A interrupt handler */
	d37A_intr(lev);
}


/*
 * STATIC int
 * _dma_alloc(int chan, uchar_t mode)
 *	Request the semaphore for the indicated channel. If the
 *	channel is busy and mode is DMA_SLEEP then put process
 *	to sleep waiting for the semaphore. A mode of DMA_NOSLEEP
 *	indicates to simply return on a busy channel.
 *
 * caller:	dma_prog(), dma_swsetup(), dma_stop()
 * calls:	splhi(), splx(), ASSERT()
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int	
_dma_alloc(int chan, uchar_t mode)
{
	pl_t	opl;

	/* request the semaphore for the indicated channel */

	dprintf(("_dma_alloc: attempting to allocate channel %d, mode is %s\n",
	    chan, (mode == DMA_SLEEP ? "DMA_SLEEP" : "DMA_NOSLEEP")));

	/*
	 * Serialize DMA transfers by remapping the channel
	 * number to the per-chip or per-system channel. The
	 * remapping is only done if there is a limitation or 
	 * bug in the DMA chip that does not allow concurrent 
	 * DMA transfers.
	 */
	chan = chtorq(chan);

	opl = LOCK(&dma_chan_lock[chan], DMAPL);

	/* test (and possibly set) semaphore */
	while (dma_alloc_map[chan]) {
		dprintf(("_dma_alloc: channel %d is busy.\n", chan));

		if (mode == DMA_SLEEP) {
			dprintf(("_dma_alloc: sleeping on SV %lx\n",
				 &dma_chan_sv[chan]));
			SV_WAIT(&dma_chan_sv[chan], PDMA,
				&dma_chan_lock[chan]);
			(void) LOCK(&dma_chan_lock[chan], DMAPL);
		} else {
			UNLOCK(&dma_chan_lock[chan], opl);
			return(FALSE);
		}
	}

	/* got the semaphore, now set it */
	dma_alloc_map[chan] = 1;

	UNLOCK(&dma_chan_lock[chan], opl);

	dprintf(("_dma_alloc: channel %d now allocated.\n", chan));

	return(TRUE);
}


/*
 * STATIC int
 * _dma_relse(int chan)
 *	Release the channel semaphore on chan. Assumes caller actually
 *	owns the semaphore (no check made for this). Wakeup is
 *	called to awake anyone sleeping on the semaphore.
 *
 * caller:	dma_prog(), dma_swsetup(), dma_stop()
 * calls:	none
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int	
_dma_relse(int chan)
{
	pl_t	opl;

	/* release the channel semaphore for chan */
	dprintf(("_dma_relse: attempting to release channel %d.\n", chan));

	/*
	 * Deallocate the channel that was actually allocated (by
	 * _dma_alloc) by remapping the channel number to per-chip
	 * or per-system channel. See comment in _dma_alloc also.
	 */
	chan = chtorq(chan);

	opl = LOCK(&dma_chan_lock[chan], DMAPL);

	/* is channel even allocated? */
	if (!dma_alloc_map[chan]) {
		UNLOCK(&dma_chan_lock[chan], opl);
		dprintf(("_dma_relse: channel %d not allocated.\n", chan));
		return(FALSE);
	}

	dma_alloc_map[chan] = 0;
	dprintf(("_dma_relse: channel %d released.\n", chan));

	/* wake up any sleepers */
	dprintf(("_dma_relse: waking up sleepers on address %x.\n",
                                (unsigned long) &dma_alloc_map[chan]));

	SV_SIGNAL(&dma_chan_sv[chan], 0);
	UNLOCK(&dma_chan_lock[chan], opl);

	return(TRUE);
}


/*
 * int
 * dma_prog(struct dma_cb *dmacbptr, int chan, uchar_t mode)
 *	Program chan for the to-be-initiated-by-hardware operation
 *	given in dmacbptr. _dma_alloc is called to request the channel
 *	lock and mode is passed as the sleep parameter.
 *
 * caller:	driver routines
 * calls:	_dma_alloc(), d37A_prog_chan(), _dma_relse()
 *
 * Calling/Exit State:
 *	If succeeds, holds DMA channel pseudo sleep lock on exit.
 */
int
dma_prog(struct dma_cb *dmacbptr, int chan, uchar_t mode)
{
	/* attempt to program channel chan */

	dprintf(("dma_prog: attempting to program channel %d: mode is %s",
	    chan, (mode == DMA_SLEEP ? "DMA_SLEEP" : "DMA_NOSLEEP")));
	dprintf((" dmacbptr= %x.\n", (unsigned long) dmacbptr));

	/* first step, try to get the lock */
	if (_dma_alloc(chan, mode) != TRUE) {
		dprintf(("dma_prog: channel %d not programmed, channel busy.\n",
			    chan));
		return(FALSE);
	}

	/* ok, now let d37A deal with it */
	if (!d37A_prog_chan(dmacbptr, chan)) {
		dprintf(("dma_prog: channel %d not programmed.\n", chan));
		/* problems, release lock so we can try again later */
		_dma_relse(chan);
		return(FALSE);
	}

	dprintf(("dma_prog: channel %d programmed.\n", chan));
	return(TRUE);
}


/*
 * boolean_t
 * dma_cascade(int chan, uchar_t mode)
 *	Program chan for or release chan from cascade mode operation.
 *
 * caller:	driver routines
 * calls:	_dma_alloc(), d37A_cascade(), _dma_relse()
 *
 * Calling/Exit State:
 *	mode is a combination of:
 *
 *		DMA_ENABLE to begin use of the channel
 *	     or	DMA_DISABLE to release the channel
 *
 *		DMA_SLEEP to wait for the channel to be available
 *	     or	DMA_NOSLEEP to return B_FALSE if not available
 *
 *	If succeeds, holds DMA channel pseudo sleep lock on exit,
 *	and returns B_TRUE.
 */
boolean_t
dma_cascade(int chan, uchar_t mode)
{
	int alloc_chan;
	pl_t opl;

	ASSERT((mode & ~(DMA_SLEEP|DMA_NOSLEEP|DMA_ENABLE|DMA_DISABLE)) == 0);
	ASSERT((mode & (DMA_ENABLE|DMA_DISABLE)) != 0);

	dprintf(("dma_cascade: %s cascade mode for channel %d\n",
		 (mode & DMA_ENABLE ? "DMA_ENABLE" : "DMA_DISABLE")));

	dprintf(("\tsleep flag = %s\n",
		 (mode & DMA_NOSLEEP ? "DMA_NOSLEEP" : "DMA_SLEEP")));

	/*
	 * Serialize DMA transfers by remapping the channel
	 * number to the per-chip or per-system channel. The
	 * remapping is only done if there is a limitation or 
	 * bug in the DMA chip that does not allow concurrent 
	 * DMA transfers.
	 */
	alloc_chan = chtorq(chan);

	opl = LOCK(&dma_chan_lock[alloc_chan], DMAPL);

	if (mode & DMA_DISABLE) {
		ASSERT(dma_cascade_count[alloc_chan] != 0);
		if (--dma_cascade_count[alloc_chan] == 0) {
			UNLOCK(&dma_chan_lock[alloc_chan], opl);
			_dma_relse(chan);
		} else
			UNLOCK(&dma_chan_lock[alloc_chan], opl);
		return B_TRUE;
	}

	/* test (and possibly set) semaphore */
	while (dma_alloc_map[alloc_chan] &&
	       dma_cascade_count[alloc_chan] == 0) {
		dprintf(("dma_cascade: channel %d is busy.\n", alloc_chan));

		if (mode & DMA_NOSLEEP) {
			UNLOCK(&dma_chan_lock[alloc_chan], opl);
			return B_FALSE;
		} else {
			dprintf(("dma_cascade: sleeping on SV %lx\n",
				 &dma_chan_sv[alloc_chan]));

			SV_WAIT(&dma_chan_sv[alloc_chan], PDMA,
				&dma_chan_lock[alloc_chan]);
			(void) LOCK(&dma_chan_lock[alloc_chan], DMAPL);
		}
	}

	/* got the semaphore, now set it */
	dma_alloc_map[alloc_chan] = 1;

	if (dma_cascade_count[alloc_chan]++ == 0) {
		UNLOCK(&dma_chan_lock[alloc_chan], opl);

		/* ok, now let d37A deal with it */
		if (!d37A_cascade(chan)) {
			dprintf(("dma_cascade: channel %d not programmed.\n",
				 chan));
			/* problems, release lock so we can try again later */
			opl = LOCK(&dma_chan_lock[alloc_chan], DMAPL);
			--dma_cascade_count[alloc_chan];
			UNLOCK(&dma_chan_lock[alloc_chan], opl);
			_dma_relse(chan);
			return B_FALSE;
		}
	} else
		UNLOCK(&dma_chan_lock[alloc_chan], opl);

	dprintf(("dma_cascade: channel %d programmed.\n", chan));
	return B_TRUE;
}


/*
 * int
 * dma_swsetup(struct dma_cb *dmacbptr, int chan, uchar_t mode)
 *	Setup chan for the operation given in dmacbptr.
 *	_dma_alloc is first called to request the channel lock 
 *	for chan; mode is passed to _dma_alloc().
 *
 * caller:	driver routines
 * calls:	_dma_alloc(), d37A_dma_swsetup(), _dma_relse(), proc()
 *
 * Calling/Exit State:
 *	If succeeds, holds DMA channel pseudo sleep lock on exit.
 */
int
dma_swsetup(struct dma_cb *dmacbptr, int chan, uchar_t mode)
{
	/* program and software initiate a DMA transfer */

	dprintf(("dma_swsetup: attempting to setup channel %d: mode is %s",
	    chan, (mode == DMA_SLEEP ? "DMA_SLEEP" : "DMA_NOSLEEP")));
	dprintf((" dmacbptr= %x.\n", (unsigned long) dmacbptr));

	/* first step, try and get the lock */
	if (_dma_alloc(chan, mode) != TRUE) {
		dprintf(("dma_swsetup: channel %d not set up, channel busy\n", 
		    chan));
		return(FALSE);
	}

	/* got the lock, let d37A deal with it */
	if (!d37A_dma_swsetup(dmacbptr, chan)) {
		dprintf(("dma_swsetup: channel %d not set up.\n", chan));
		/* oops, release lock for later retry */
		_dma_relse(chan);
		return(FALSE);
	}

	dprintf(("dma_swsetup: channel %d set up.\n", chan));

	/* call the caller's routine if set */
	if (dmacbptr->proc) {
		dprintf(("dma_swsetup: calling (dmacbptr->proc)(%x).\n",
		    (unsigned long) dmacbptr->procparms));
		(dmacbptr->proc)(dmacbptr->procparms);
	}

	return(TRUE);
}


/*
 * void
 * dma_swstart(struct dma_cb *dmacbptr, int chan, uchar_t mode)
 *	Start the operation setup by dma_swsetup(). Go to sleep
 *	if mode is DMA_SLEEP after starting operation.
 *
 * caller:	driver routines
 * calls:	d37A_dma_swstart(), sleep()
 *
 * Calling/Exit State:
 *	DMA channel pseudo sleep lock on entry & exit.
 */
void
dma_swstart(struct dma_cb *dmacbptr, int chan, uchar_t mode)
{
	extern int sleep();

	dprintf(("dma_swstart: starting channel %d.\n", chan));

	/* start operation previously set up on chan */
	d37A_dma_swstart(chan);

	/*
	 * Go to sleep waiting for transfer to complete. It is
	 * the responsibility of the driver calling dma_swstart()
	 * to issue wakeup after it gets xfer-complete interrupt.
	 */
	if (mode == DMA_SLEEP) {
		/*
		 *+ The driver must be responsible for sleeping
		 *+ since it is doing the wakeup operation. The
		 *+ sleep/wakeup must be at the same level.
		 */
		cmn_err(CE_WARN,
			"dma_swstart: Asynchronous bcopy. "
			"Must not use this option");
		dprintf(("dma_swstart: sleeping on address %x.\n",
			(unsigned long) dmacbptr));
		sleep((caddr_t)dmacbptr, PDMA);
	}
}


/*
 * void
 * dma_stop(int chan)
 * 	Stop DMA activity on chan and release the channel lock.
 *
 * caller:	driver routines
 * calls:	splhi(), _dma_alloc(), _dma_relse(), splx(), d37A_dma_stop()
 *
 * Calling/Exit State:
 *	DMA channel pseudo sleep lock held on entry, released on exit.
 */
void
dma_stop(int chan)
{
	/* stop activity on DMA channel and release lock */

	dprintf(("dma_stop: stopping channel %d.\n", chan));

	/* call d37A the stop the channel */
	d37A_dma_stop(chan);

	/* release the lock */
	_dma_relse(chan);
}


/*
 * void
 * dma_enable(int chan)
 *	Allow the hardware tied to channel chan to request service
 *	from the DMAC. dma_prog() should have been called prior to this.
 *
 * caller:	driver routines
 * calls:	d37A_dma_enable()
 *
 * Calling/Exit State:
 *	DMA channel pseudo sleep lock held on entry & exit.
 */
void
dma_enable(int chan)
{
	dprintf(("dma_enable: enable channel %d.\n", chan));

	/* allow preprogrammed hardware transfer to occur */
	d37A_dma_enable(chan);
}


/*
 * void
 * dma_disable(int chan)
 *	Called to mask off hardware requests on channel chan. Assumes
 *	the caller owns the channel.
 *
 * caller:	driver routines
 * calls:	d37A_dma_disable()
 *
 * Calling/Exit State:
 *	DMA channel pseudo sleep lock held on entry, released on exit.
 */
void
dma_disable(int chan)
{
	/* dprintf(("dma_disable: disable channel %d.\n", chan)); */

	/* disallow subsequent hardware requests on channel chan */
	d37A_dma_disable(chan);

	/* free the lock */
	_dma_relse(chan);
}


/*
 * void
 * dma_get_chan_stat(struct dma_stat *dmastat, int can)
 *	Obtain the current channel status from the DMAC.
 *
 * caller:	driver routines
 * calls:	d37A_get_chan_stat()
 *
 * Calling/Exit State:
 *	DMA channel pseudo sleep lock held on entry & exit.
 */
void
dma_get_chan_stat(struct dma_stat *dmastat, int chan)
{
	dprintf(("dma_get_chan_stat: obtaining channel %d's status\n",
	    chan));
	dprintf((" dmastat= %x.\n", dmastat));

	/* obtain current channel status from the DMAC */
	d37A_get_chan_stat(dmastat, chan);
}


/*
 * struct dma_cb *
 * dma_get_cb(uchar_t mode)
 *	Get a DMA Command Block for a future DMA operation.
 *	Zero it out as well. Return a pointer to the block.
 *
 * caller:	driver routines
 * calls:	kmem_zalloc()
 *
 * Calling/Exit State:
 *	None.
 */
struct dma_cb *
dma_get_cb(uchar_t mode)
{
	struct dma_cb *dmacbptr;

	dprintf(("dma_get_cb: get new dma_cb, mode is %s.\n",
	    (mode == DMA_SLEEP ? "DMA_SLEEP" : "DMA_NOSLEEP")));

	/* get a new dma_cb structure and zero it out */

	/* anything available? */
	dmacbptr = (struct dma_cb *) kmem_zalloc(sizeof(struct dma_cb),
			mode == DMA_NOSLEEP ? KM_NOSLEEP : KM_SLEEP);
#ifdef DMA_DEBUG
	if (dmacbptr != NULL)
		dprintf(("dma_get_cb: new dmacbptr= %x.\n",
			(unsigned long) dmacbptr));
#endif /* DMA_DEBUG */
	return(dmacbptr);
}


/*
 * void
 * dma_free_cb(struct dma_cb *dmacbptr)
 *	Return a DMA Command Block to the free list
 *
 * caller:	driver routines
 * calls:	kmem_free()
 *
 * Calling/Exit State:
 *	None.
 */
void
dma_free_cb(struct dma_cb *dmacbptr)
{
	dprintf(("dma_free_cb: freeing dmacbptr= %x.\n",
	    (unsigned long) dmacbptr));

	/* return the dma_cb to the kernel pool */
	kmem_free((void *) dmacbptr, sizeof(struct dma_cb));
}


/*
 * struct dma_buf *
 * dma_get_buf(uchar_t mode)
 *	Get a DMA Buffer Descriptor for a future DMA operation.
 *
 * caller:	driver routines
 * calls:	kmem_zalloc()
 *
 * Calling/Exit State:
 *	Return a pointer to the block.
 */
struct dma_buf *
dma_get_buf(uchar_t mode)
{
	struct dma_buf *dmabufptr;

	/* get a new dma_buf structure */

	dprintf(("dma_get_buf: get new dma_buf, mode is %s.\n",
	    (mode == DMA_SLEEP ? "DMA_SLEEP" : "DMA_NOSLEEP")));

	/* anything available? */
	dmabufptr = (struct dma_buf *) kmem_zalloc(sizeof(struct dma_buf),
			mode == DMA_NOSLEEP ? KM_NOSLEEP : KM_SLEEP);
#ifdef DMA_DEBUG
	if (dmabufptr != NULL)
		dprintf(("dma_get_buf: new dmabufptr= %x.\n",
				(unsigned long) dmabufptr));
#endif /* DMA_DEBUG */
	return(dmabufptr);
}


/*
 * void
 * dma_free_buf(struct dma_buf *dmabufptr)
 *	Return a DMA Buffer Descriptor to the free list.
 *
 * caller:	driver routines
 * calls:	kmem_free()
 *
 * Calling/Exit State:
 *	None.
 */
void
dma_free_buf(struct dma_buf *dmabufptr)
{
	dprintf(("dma_free_buf: freeing dmabufptr= %x.\n",
	    (unsigned long) dmabufptr));

	kmem_free((void *) dmabufptr, sizeof(struct dma_buf));
}


/*
 * void
 * dma_physreq(int chan, int datapath, physreq_t *preqp)
 *	Apply constraints appropriate for the given channel and path size
 *	to the physical requirements structure at (*preqp).
 *
 * Calling/Exit State:
 *	None.
 */
void
dma_physreq(int chan, int datapath, physreq_t *preqp)
{
	d37A_physreq(chan, datapath, preqp);
}
