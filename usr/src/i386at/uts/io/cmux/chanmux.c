#ident	"@(#)chanmux.c	1.17"
#ident	"$Header$"

/*
 * Integrated Workstation Environment (IWE) Channel Multiplexor.
 *
 * Multiplexes N secondary input devices (lower streams) across
 * M primary input/output channels (referred to as principal streams)
 *
 * The task of the channel multiplexor is three-fold:
 *	- To route messages from the underlying input devices
 *	  (keyboard, mouse) to the currently active channel.
 *	- To forward output from the multiple channels to the 
 *	  the underlying output STREAM.
 *	- To coordinate processing of the ioctl(2) system call
 */

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	12Sep97		rodneyh@sco.com from kd@sco.com
 *	- Replaced references to lbolt with TICKS to make lbolt clean.
 */

#include <fs/buf.h>
#include <fs/file.h>
#include <io/open.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termios.h>
#include <io/ws/chan.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <svc/clock.h>				/* L000 */
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>

#include <io/cmux/chanmux.h>

#include <io/ddi.h>	/* must come last */

/* For debugging only */
#undef STATIC
#define STATIC

/*
 * TODO:
 *	Change the KM_SLEEP flag to KM_NOSLEEP or remove the check.
 */

#define INHI		4096
#define INLO		512
#define OUTHI		512
#define OUTLO		128

#define	CMUXHIER	1
#define CMUXPL		plstr	

#define IOCTL_TYPE(type)	((type==M_COPYIN) || (type==M_COPYOUT) || \
				 (type==M_IOCACK) ||(type==M_IOCNAK))

#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int cmux_debug = 0;
#define DEBUG1(a)       if (cmux_debug == 1) printf a
#define DEBUG2(a)       if (cmux_debug >= 2) printf a /* allocations */
#define DEBUG3(a)       if (cmux_debug >= 3) printf a /* M_CTL Stuff */
#define DEBUG4(a)       if (cmux_debug >= 4) printf a /* M_READ Stuff */
#define DEBUG5(a)       if (cmux_debug == 5) printf a /* cmuxclose */
#define DEBUG6(a)       if (cmux_debug == 6) printf a /* M_IOCTL Stuff */
#define DEBUG7(a)       if (cmux_debug >= 7) printf a 
#else
#define DEBUG1(a)
#define DEBUG2(a)
#define DEBUG3(a)
#define DEBUG4(a)
#define DEBUG5(a)
#define DEBUG6(a)
#define DEBUG7(a)
#endif /* DEBUG || DEBUG_TOOLS */


int		cmuxstart(void);
void		cmux_wakeup(void);

STATIC int	cmuxopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	cmuxclose(queue_t *, int, cred_t *);
STATIC int	cmux_up_rsrv(queue_t *);
STATIC int	cmux_up_wsrv(queue_t *);
STATIC int	cmux_up_rput(queue_t *, mblk_t *);
STATIC int	cmux_up_wput(queue_t *, mblk_t *);
STATIC int	cmux_mux_wsrv(queue_t *);
STATIC int	cmux_mux_rsrv(queue_t *);
STATIC int	cmux_mux_rput(queue_t *, mblk_t *);
STATIC int	cmux_mux_wput(queue_t *, mblk_t *);

STATIC int	cmux_realloc(unsigned long);
STATIC int	cmux_allocstrms(cmux_ws_t *, unsigned long);
STATIC int	cmux_allocchan(cmux_ws_t *, unsigned long);
STATIC int	cmux_initws(cmux_ws_t *, unsigned long);
STATIC int	cmux_openchan(queue_t *, cmux_ws_t *, ulong_t, dev_t, int);
STATIC int	cmux_unlink(mblk_t *, cmux_t *, struct iocblk *);
STATIC void	cmux_close_chan(cmux_ws_t *, cmux_lstrm_t *, ch_proto_t *);
STATIC clock_t	cmux_striptime(mblk_t *);
STATIC int	cmux_foundit(clock_t, clock_t, clock_t);
STATIC cmux_t	*cmux_findchan(cmux_ws_t *, clock_t);
STATIC void	cmux_clr_ioc(cmux_ws_t *);
STATIC void	cmux_switch_chan(cmux_ws_t *, ch_proto_t *);
STATIC void	cmux_iocack(queue_t *, mblk_t *, struct iocblk *, int);
STATIC void	cmux_iocnak(queue_t *, mblk_t *, struct iocblk *, int, int);
STATIC void	cmux_do_iocresp(cmux_ws_t *);

extern int	nulldev(void);
extern int	ws_getchanno(minor_t);
extern int	ws_getws(minor_t);
extern void	ws_clrcompatflgs(dev_t);
extern void	ws_initcompatflgs(void);
extern void	ws_sysv_initcompatflgs(void);
extern void	ws_initvdc_compatflgs(void);


struct module_info cmux_iinfo = {
	0, "cmux", 0, INFPSZ, INHI, INLO };

struct module_info cmux_oinfo = {
	0, "cmux", 0, CMUXPSZ, OUTHI, OUTLO };

struct qinit cmux_up_rinit = {
	cmux_up_rput, cmux_up_rsrv, cmuxopen, cmuxclose, NULL, &cmux_iinfo, NULL };

struct qinit cmux_up_winit = {
	cmux_up_wput, cmux_up_wsrv, cmuxopen, cmuxclose, NULL, &cmux_oinfo, NULL };

struct qinit cmux_mux_rinit = {
	cmux_mux_rput, cmux_mux_rsrv, nulldev, nulldev, NULL, &cmux_iinfo, NULL };

struct qinit cmux_mux_winit = {
	cmux_mux_wput, cmux_mux_wsrv, nulldev, nulldev, NULL, &cmux_oinfo, NULL };

struct streamtab cmuxinfo = {
	&cmux_up_rinit, &cmux_up_winit, &cmux_mux_rinit, &cmux_mux_winit };


/*
 * Global variables.
 */
int		cmuxdevflag = 0;
cmux_ws_t	**wsbase;
ulong_t		numwsbase = 0;	/* number of workstations allocated */

static int	openflg = 0;

lock_t		*cmux_lock;
sv_t		*cmux_openflgsv;
sv_t		*cmux_bufcallsv;
sv_t		*cmux_wclosesv;
sv_t		*cmux_wopensv;

LKINFO_DECL(cmux_lkinfo, "CMUX::cmux mutex lock", 0);


/*
 * int
 * cmuxstart(void)
 *
 * Calling/Exit State:
 *	- Return 0, if was able to allocate memory.
 */
int
cmuxstart(void)
{
	DEBUG1(("In cmuxstart!\n"));

	/*
	 * Allocate a list of workstation structure pointers.
	 */
	wsbase = (cmux_ws_t **) kmem_zalloc(
		CMUX_WSALLOC * sizeof(cmux_ws_t *), KM_NOSLEEP);
	if (wsbase == (cmux_ws_t **) NULL)
		/*
		 *+ Not enough memory available to allocate space
		 *+ for list of workstation pointers.
		 */
		cmn_err(CE_PANIC, 
			"cmuxstart: cannot allocate workstation space");

	numwsbase = CMUX_WSALLOC;

	cmux_lock = LOCK_ALLOC(CMUXHIER, CMUXPL, &cmux_lkinfo, KM_NOSLEEP);
	cmux_openflgsv = SV_ALLOC(KM_NOSLEEP);
	cmux_bufcallsv = SV_ALLOC(KM_NOSLEEP);
	cmux_wopensv   = SV_ALLOC(KM_NOSLEEP);
	cmux_wclosesv  = SV_ALLOC(KM_NOSLEEP);
	if (!cmux_lock || !cmux_openflgsv || !cmux_bufcallsv || 
	    !cmux_wopensv || !cmux_wclosesv)
		/*
		 *+ There isn't enough memory available to allocate for
		 *+ channel multiplexor locks and synch. variables.
		 */
		cmn_err(CE_PANIC, 
			"cmuxstart: not enough memory for locks");
 
	/*
	 * Clear the bit mask array indicating if a particular
	 * channel is in a XENIX compatibility mode.
	 */ 
	ws_initcompatflgs();
        ws_sysv_initcompatflgs();
        ws_initvdc_compatflgs();

	return 0;
}


/*
 * STATIC int
 * cmux_realloc(unsigned long)
 *
 * Calling/Exit State:
 *	- Return ENOMEM if kmem fails, 0 otherwise
 *
 * Description:
 *	Allocate to the power of two greater than the argument
 *	passed in and copy over old ptrs and NULL new ones out.
 */
STATIC int
cmux_realloc(unsigned long wsno)
{
	cmux_ws_t **nwsbase, **owsbase;
	int i;


	if (wsno < numwsbase) 
		return (0);

	wsno = (wsno >> 1) << 2; /* round up to next power of 2 */

	owsbase = wsbase;

	if ((nwsbase = (cmux_ws_t **)kmem_alloc(
	    wsno * sizeof(cmux_ws_t *), KM_SLEEP)) == (cmux_ws_t **) NULL)
		return (ENOMEM);

	bcopy(wsbase, nwsbase, numwsbase * sizeof(cmux_ws_t *));

	for (i = numwsbase; i < wsno; i++)
		nwsbase[i] = (cmux_ws_t *) NULL;

	wsbase = nwsbase;
	kmem_free(owsbase, numwsbase * sizeof(cmux_ws_t *));
	numwsbase = wsno;

	return (0);
}


/*
 * STATIC int
 * cmux_allocstrms(cmux_ws_t *, unsigned long)
 *
 * Calling/Exit State:
 *	- Return ENOMEM if kmem fails, 0 otherwise
 *
 * Description:
 *	cmux_allocstrms() is called to allocate additional
 *	streams for the workstations w_lstrmsp list.
 */
STATIC int
cmux_allocstrms(cmux_ws_t *wsp, unsigned long numstreams)
{
	unsigned long	size, onumlstrms;
	cmux_link_t	*olstrmsp, *nlstrmsp;


	onumlstrms = wsp->w_numlstrms;

	numstreams = max(CMUX_STRMALLOC, (numstreams>>1) << 2);
	if (numstreams <= onumlstrms)
		/* enough lower streams allocated */
		return (0);

	olstrmsp = wsp->w_lstrmsp;

	size = numstreams * sizeof(cmux_link_t);
	nlstrmsp = (cmux_link_t *)kmem_zalloc(size, KM_NOSLEEP);
	if (nlstrmsp == (cmux_link_t *)NULL)
		return(ENOMEM);

	if (olstrmsp) {
		bcopy(olstrmsp, nlstrmsp, onumlstrms * sizeof(cmux_link_t));
		wsp->w_lstrmsp = nlstrmsp;
		kmem_free(olstrmsp, onumlstrms * sizeof(cmux_link_t));
	} else
		wsp->w_lstrmsp = nlstrmsp;

	wsp->w_numlstrms = numstreams;

	return(0);
}


/*
 * STATIC int
 * cmux_allocchan(cmux_ws_t *, unsigned long)
 *	Allocate cmux_t pointers and cmux_link_t structs for numchan channels.
 *
 * Calling/Exit State:
 *	- Return ENOMEM if kmem fails, 0 otherwise
 */
STATIC int
cmux_allocchan(cmux_ws_t *wsp, unsigned long numchan)
{
	unsigned long	cmuxsz, princsz, onumchan;
	cmux_t		**ocmuxpp, **ncmuxpp;
	cmux_link_t	*oprincp, *nprincp;


	onumchan = wsp->w_numchan;
	numchan = max(CMUX_CHANALLOC, (numchan >> 1) << 2);

	/*
	 * If the existing number of channels (onumchan) is
	 * greater than numchan, then there is already
	 * enough space allocated, simply return 0.
	 */
	if (numchan <= onumchan)
		return (0);

	ocmuxpp = wsp->w_cmuxpp;
	oprincp = wsp->w_princp;

	/*
	 * Allocate list of channel structure pointers for the
	 * workstations structure (not the channel structure
	 * themselves).
	 */
	cmuxsz = numchan * sizeof(cmux_t *);
	ncmuxpp = (cmux_t **) kmem_zalloc(cmuxsz, KM_SLEEP);
	if (ncmuxpp == (cmux_t **) NULL)
		return (ENOMEM);

	princsz = numchan * sizeof(cmux_link_t);
	nprincp = (cmux_link_t *)kmem_zalloc(princsz, KM_SLEEP);
	if (nprincp == (cmux_link_t *) NULL) {
		kmem_free(ncmuxpp, cmuxsz);
		return (ENOMEM);
	}

	DEBUG1(("ncmuxpp %x ocmuxpp %x size %x onumchan %x\n",
			ncmuxpp, ocmuxpp, cmuxsz, onumchan));

	if (ocmuxpp) {
		bcopy(ocmuxpp, ncmuxpp, onumchan*sizeof(cmux_t *));
		wsp->w_cmuxpp = ncmuxpp;
		kmem_free(ocmuxpp, onumchan*sizeof(cmux_t *));
	} else
		wsp->w_cmuxpp = ncmuxpp;

	if (oprincp) {
		bcopy(oprincp, nprincp, onumchan*sizeof(cmux_link_t));
		wsp->w_princp = nprincp;
		kmem_free(oprincp, onumchan*sizeof(cmux_link_t));
	} else
		wsp->w_princp = nprincp;

	wsp->w_numchan = numchan;

	return (0);
}

/*
 * STATIC int
 * cmux_initws(cmux_ws_t *, unsigned long)
 *	Allocate space for per-channel struct; clear flags.
 * 
 * Calling/Exit State:
 *	- Return ENOMEM if kmem fails.
 */
STATIC int
cmux_initws(cmux_ws_t *wsp, unsigned long numchan)
{
	int error = 0;
	struct cmux_swtch *switchp;


	if (error = cmux_allocchan(wsp, numchan))
		return error;

	if (error = cmux_allocstrms(wsp, CMUX_STRMALLOC))
		return error;

	wsp->w_numswitch = 1;
	switchp = &wsp->w_swtchtimes[0];
	switchp->sw_time = TICKS();			/* L000 */
	switchp->sw_chan = numchan - 1; 

	DEBUG1(("switch time is %x, switch chan is %x\n",
		wsp->w_swtchtimes[0].sw_time, wsp->w_swtchtimes[0].sw_chan));

	return (0);
}

/*
 * STATIC int
 * cmux_openchan(queue_t *, cmux_ws_t *, unsigned long, dev_t, int)
 *
 * Calling/Exit State:
 *	Return 
 *		- ENOMEM, if cmux_allocchan() fails to allocate memory.
 *		- ENXIO, if the q_ptr member of the queue is NOT set.
 *		- ENOMEM, if kmem allocation fails.
 *		- EINTR, if a signal is caught.
 *		- non-zero value, if KD fails to acknowledge the open request.
 *		- zero, othewise.
 *
 * Description:
 *	Allocate space for the new channel structure.
 *
 *	Make sure that pointers to ws struct and queues get set up.
 *	Return an error number based on the success of allocation.
 *	Send a protocol (ch_proto) message indicating that a channel
 *	is opening up.
 */
/* ARGSUSED */
STATIC int
cmux_openchan(queue_t *qp, cmux_ws_t *wsp, unsigned long chan, 
		dev_t dev, int flag)
{
	int		error;
	cmux_t		*cmuxp;
	mblk_t		*mp;
	ch_proto_t	*protop;
	proc_t		*procp;
	cmux_link_t	*linkp;


	if (error = cmux_allocchan(wsp, chan + 1))
		return error;

	if (qp->q_ptr) {
#ifdef DEBUG
		cmn_err(CE_WARN, 
			"!cmux_openchan: Invalid open state! Open fails.");
#endif
		return (ENXIO);
	}

	/*
	 * Allocate list of channel structures (cmux_t).
	 */
	cmuxp = (cmux_t *) kmem_alloc(sizeof(cmux_t), KM_SLEEP);
	if (cmuxp == (cmux_t *) NULL) 
		return ENOMEM;

	/*
	 * Initialize channel structure to point back to workstation
	 * structure (ws_stat) and STREAMS queue. It state is
	 * set to CMUX_OPEN.
	 */
	cmuxp->cmux_dev = dev;
	cmuxp->cmux_num = chan;
	cmuxp->cmux_wsp = wsp;
	cmuxp->cmux_rqp = qp;
	cmuxp->cmux_wqp = WR(qp);
	cmuxp->cmux_flg = CMUX_OPEN;

	/*
	 * Set the entry for the channel in the workstation structure's
	 * list of channel structure pointers to the channel structure.
	 */
	wsp->w_cmuxpp[chan] = cmuxp;

	qp->q_ptr = (caddr_t) cmuxp;
	WR(qp)->q_ptr = (caddr_t) cmuxp;

	/*
	 * Allocate a message of a size ch_proto_t to send downstream.
	 * Its type is CH_CTL, subtype CH_CHAN, and the command is
	 * CH_CHANOPEN. The argument is set to the process's parent
	 * process ID. This message is put on the queue to be sent
	 * to the stream linked underneath this channel. For example,
	 * the KD driver stream for /dev/kd05 is linked underneath
	 * /dev/vt05 to tell the KD driver that an open has occurred
	 * on this channel.
	 */

	while ((mp = allocb(sizeof(ch_proto_t), BPRI_HI)) == (mblk_t *) NULL) {
		(void) LOCK(cmux_lock, CMUXPL); 
		(void) bufcall(sizeof(ch_proto_t), BPRI_HI, cmux_wakeup, NULL);
		/* In SVR4 the sleep priority was STIPRI */ 
		SV_WAIT(cmux_bufcallsv, primed - 3, cmux_lock);
	}

	mp->b_wptr += sizeof(ch_proto_t);
	mp->b_datap->db_type = M_PCPROTO;

	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHAN;
	protop->chp_stype_cmd = CH_CHANOPEN;
	drv_getparm(UPROCP, &procp);
	protop->chp_stype_arg = procp->p_ppid;	
	protop->chp_chan = chan;

	/*
	 * Put it on our write queue to ship to principal 
	 * stream when it is opened.
	 */
	putq(cmuxp->cmux_wqp, mp);

	linkp = wsp->w_princp + chan;

	/*
	 * At this point in open, safe to allow another in 
	 * The counter indicating if an open is in progress
	 * is decremented and a wakeup is done for other 
	 * processes sleeping in cmuxopen().
	 */
	openflg--;

	SV_SIGNAL(cmux_openflgsv, 0);

	/*
	 * Enable put and srv routine. 
	 */
	qprocson(cmuxp->cmux_rqp);

	/*
	 * Don't sleep on open until mux is initialized
	 * The stream has not been linked underneath this
	 * channel yet. This open must be from the wsinit
	 * preparing to initialize the channel.
	 */
	if (!linkp->cmlb_flg) 
		return (0);

	/*
	 * Set the flag in the state structure for the lower
	 * stream that a wakeup is needed when KD acknowledges
	 * the CH_CHANOPEN message.
	 */
	linkp->cmlb_flg |= CMUX_PRINCSLEEP;

	/*
	 * Sleep until woken up or a signal is caught. 
	 * Release data structures and return EINTR if a
	 * signal was caught.
	 */
	(void) LOCK(cmux_lock, CMUXPL);
	/* In SVR4 the sleep priority was equal to STOPRI */
 	if (SV_WAIT_SIG(cmux_wopensv, primed - 3, cmux_lock) == B_FALSE) {
		linkp->cmlb_flg &= ~CMUX_PRINCSLEEP;
		wsp->w_cmuxpp[chan] = (cmux_t *) NULL;
		qp->q_ptr = (caddr_t) NULL;
		WR(qp)->q_ptr = (caddr_t) NULL;
		kmem_free(cmuxp, sizeof(cmux_t));
 		return (EINTR);
 	}

	/*
	 * Switch off the flag in the lower stream. 
	 */
	linkp->cmlb_flg &= ~CMUX_PRINCSLEEP;

	/*
	 * Enable the read queue of the lower stream to 
	 * start sending up enqueued data.
	 */
	qenable(RD(linkp->cmlb_lblk.l_qbot));

	/*
	 * If an acknowledgement from KD indicated an error,
	 * release the data structures and return error.
	 */
	if (linkp->cmlb_err) {
		wsp->w_cmuxpp[chan] = (cmux_t *) NULL;
		qp->q_ptr = (caddr_t) NULL;
		WR(qp)->q_ptr = (caddr_t) NULL;
		kmem_free(cmuxp, sizeof(cmux_t));
	}

	DEBUG1(("cmux_openchan: exiting ...\n"));

	return(linkp->cmlb_err);
}


/*
 * int 
 * cmuxopen(queue_t *, dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	If the q_ptr member of the queue is already set, then
 *	return
 *		- EAGAIN if the STREAM is in the process of being closed
 *		- EBUSY if FEXCL is set in the file flags
 *		- 0, otherwise
 */
/* ARGSUSED */
int
cmuxopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	unsigned long	chan, wsno;
	int		dev;
	cmux_ws_t	*wsp;
	mblk_t		*mp;
	struct stroptions *sop;
	int		error = 0;
	pl_t		pl;


	if (sflag)
		return (EINVAL); /* can't be opened as module */

	/*
	 * From the device number, the workstation number 
	 * and the channel number within the workstation
	 * are obtained.
	 */
	dev = getminor(*devp);
	wsno = ws_getws(dev);
	chan = ws_getchanno(dev);

	DEBUG1(("cmuxopen: dev = %d, wsno = %d chan = %d openflg = %d\n", 
				dev, wsno, chan, openflg));

	if (qp->q_ptr) {
		cmux_t *cmuxp = (cmux_t *) qp->q_ptr;

		if (cmuxp->cmux_num != chan) {
#ifdef DEBUG
			cmn_err(CE_WARN, 
				"!cmuxopen: found q_ptr != chan in open; open fails");
#endif /* DEBUG */
			return (EINVAL);
		}

		if (cmuxp->cmux_flg & CMUX_CLOSE)
			/* prevent open during close */
			return (EAGAIN);
		else
			return (flag & FEXCL) ? EBUSY : 0;
	}

	pl = LOCK(cmux_lock, plhi);

	ASSERT(getpl() == plhi);

	/*
	 * Check to see if FNONBLOCK or FNDELAY set;
	 * return EBUSY if so.
	 */
	if (openflg && (flag & FNONBLOCK ||  flag & FNDELAY)) {
		UNLOCK(cmux_lock, pl);
		return (EAGAIN);
	}

	/*
	 * Only permit one open at a time for sanity's sake.
	 */
	while (openflg > 0) {
		/* In SVR4 the sleep priority was TTIPRI */
		SV_WAIT(cmux_openflgsv, primed+3, cmux_lock);
		pl = LOCK(cmux_lock, CMUXPL);
	}

	openflg++;

	UNLOCK(cmux_lock, pl);

	/*
	 * If the workstation number for the channel being opened
	 * exceeds the number of slots for workstation structure
	 * (cmux_ws_t) pointers allocated, call cmux_realloc()
	 * to allocate a larger set of slots.
	 */
	if (wsno >= numwsbase)
		error = cmux_realloc(wsno);

	if (error) 
		goto openexit;

	/*
	 * Obtain the pointer to the workstation structure from
	 * the array of workstation pointers. If NULL allocate one
	 * via kmem_zalloc(), initialize the workstation structure,
	 * and set the list entry for the workstation number equal
	 * to the pointer.
	 */

	wsp = wsbase[wsno];

	if (wsp == (cmux_ws_t *) NULL) {
		unsigned long size;

		size = sizeof(cmux_ws_t);
		if ((wsp = (cmux_ws_t *) kmem_zalloc(size, KM_SLEEP)) == 
				(cmux_ws_t *) NULL) {
			error = ENOMEM;
			goto openexit;
		}

		wsbase[wsno] = wsp;

		if (error = cmux_initws(wsp, chan+1))
			goto openexit;
	} /* wsp == NULL */

	/*
	 * Now open channel. Call cmux_openchan() to perform the
	 * open protocol.
	 *
	 * openflg decrement and wakeup done in cmux_openchan if
	 * we reach here.
	 */

	error = cmux_openchan(qp, wsp, chan, *devp, flag);
	if (error)
		return (error);

	/*
	 * Allocate stroptions struct and send a M_SETOPTS message
	 * upstream to indicate that stream is TTY. 
	 */

	while ((mp = allocb(sizeof(struct stroptions), BPRI_HI)) == 
			(mblk_t *) NULL) {
		pl = LOCK(cmux_lock, CMUXPL);
		(void) bufcall(sizeof(ch_proto_t), BPRI_HI, cmux_wakeup, NULL);
		/* In SVR4 the sleep priority was STIPRI */
		SV_WAIT(cmux_bufcallsv, primed - 3, cmux_lock);
	}

	mp->b_datap->db_type = M_SETOPTS;
	mp->b_wptr += sizeof(struct stroptions);
	/* LINTED pointer alignment */
	sop = (struct stroptions *) mp->b_rptr;
	sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
	sop->so_hiwat = INHI;
	sop->so_lowat = INLO;
	DEBUG1(("cmuxopen: exiting successfully...\n"));
	putnext(qp, mp);
	return (0);

openexit:
	openflg--;
	SV_SIGNAL(cmux_openflgsv, 0);
	DEBUG1(("cmuxopen: error exiting ...\n"));
	return (error);
}


/*
 * int
 * cmuxclose(queue_t *, int, cred_t *)
 *
 * Calling/Exit State:
 *	- Return 0 on success.
 */
/* ARGSUSED */
int
cmuxclose(queue_t *qp, int flag, cred_t *crp)
{
	cmux_t		*cmuxp;
	cmux_ws_t	*wsp;
	mblk_t		*mp;
	ch_proto_t	*protop;
	pl_t		oldpri;
	cmux_link_t	*linkp;
	pl_t		pl;


	cmuxp = (cmux_t *) qp->q_ptr;

	if (cmuxp == (cmux_t *) NULL) {
		DEBUG5(("cmuxclose: finding invalid q_ptr in cmuxclose"));
		return (ENXIO);	
	}

	/*
	 * Turn on the CMUX_CLOSE flag in the cmux_t structure
	 */
	oldpri = splstr();
	cmuxp->cmux_flg |= CMUX_CLOSE;
	splx(oldpri);

	/*
	 * Check to see if principal stream is linked underneath.
	 * If not, flush the queues and return.
	 */
	wsp = cmuxp->cmux_wsp;
	linkp = wsp->w_princp + cmuxp->cmux_num;
	if (!linkp->cmlb_flg) {
		flushq(qp, FLUSHALL);
		flushq(WR(qp), FLUSHALL);
		return (0);
	}

	/*
	 * Principal stream linked underneath. Allocate "channel closing"
	 * message and ship it to principal stream. It should respond
	 * with a "channel closed" message. 
	 */
	while ((mp = allocb(sizeof(ch_proto_t), BPRI_HI)) == (mblk_t *) NULL) {
		pl = LOCK(cmux_lock, CMUXPL);
		(void) bufcall(sizeof(ch_proto_t), BPRI_HI, cmux_wakeup, NULL);
		/* In SVR4 the sleep priority was STIPRI */ 
		SV_WAIT(cmux_bufcallsv, primed - 3, cmux_lock);
	}

	/*
	 * Want this to be last message received on the channel,
	 * so make it normal priority so that it doesn't get ahead of
	 * user data. 
	 */
	mp->b_wptr += sizeof(ch_proto_t);
	mp->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHAN;
	protop->chp_stype_cmd = CH_CHANCLOSE;
	protop->chp_chan = cmuxp->cmux_num;

	/*
	 * Turn on the CMUX_WCLOSE (waiting for close) before
	 * enqueing the message on the write side.
	 */
	pl = LOCK(cmux_lock, CMUXPL);
	cmuxp->cmux_flg |= CMUX_WCLOSE;
	UNLOCK(cmux_lock, pl);

	/*
	 * Put it on our write queue to ship to
	 * principal stream.
	 */
	putq(cmuxp->cmux_wqp, mp);

	/*
	 * Wait for a close acknowledgement message from
	 * the lower stream.
	 */
	pl = LOCK(cmux_lock, CMUXPL);
	ASSERT(getpl() == plstr);
	while (cmuxp->cmux_flg & CMUX_WCLOSE) {
		/* In SVR4 the sleep priority was PZERO+1 */ 
		SV_WAIT(cmux_wclosesv, primed - 2, cmux_lock);
		pl = LOCK(cmux_lock, CMUXPL);
	}
	UNLOCK(cmux_lock, pl);

	DEBUG5(("cmuxclose: woken up from close on channel %d", cmuxp->cmux_num));
	
	/*
	 * Switch off the put and service routines.
	 */
	qprocsoff(qp);

	/*
	 * Upon waking up, clear the queue pointers.
	 */
	qp->q_ptr = NULL;
	WR(qp)->q_ptr = NULL;

	/*
	 * Clear the XENIX compatibility flags for the STREAM.
	 */
	ws_clrcompatflgs(cmuxp->cmux_dev);

	if (linkp->cmlb_iocmsg)
		cmn_err(CE_WARN,
			"!cmuxclose: Closing a channel with a pending "
			"ioctl response from the KD driver");

	/* 
	 * Release the channel.
	 */
	wsp->w_cmuxpp[cmuxp->cmux_num] = (cmux_t *) NULL;
	kmem_free(cmuxp, sizeof(cmux_t));

	return (0);
}


/*
 * STATIC int
 * cmux_unlink(mblk_t *, cmux_t *, struct iocblk *)
 *
 * Calling/Exit State:
 *	Return 1, if the stream is unlinked successfully.
 *	Otherwise return 0.
 *
 * Description:
 *	cmux_unlink() is called if I_PUNLINK/I_UNLINK
 *	ioctl is received. It takes the linkblk structure
 *	argument and uses the l_index to match against
 *	the linkblk structures contained in the cmux_link_t 
 *	structures for the principal stream and the secondary
 *	streams. If a match is found, deallocate the 
 *	cmux_lstrm_t structure pointed to by the lower
 *	steams's queue, zero the cmux_link_t structure (this
 *	tells CHANMUX that the lower stream has been
 *	deallocated).
 */
/* ARGSUSED */
STATIC int
cmux_unlink(mblk_t *mp, cmux_t *cmuxp, struct iocblk *iocp)
{
	cmux_ws_t	*wsp;
	cmux_link_t	*linkp;
	struct linkblk	*ulinkbp;
	int		i;
	pl_t		oldpri;


	wsp = cmuxp->cmux_wsp;
	/* LINTED pointer alignment */
	ulinkbp = (struct linkblk *) mp->b_cont->b_rptr;

	linkp = wsp->w_princp + cmuxp->cmux_num;

	DEBUG1(("cmux_unlink: In cmux_unlink.\n"));

	if (linkp->cmlb_lblk.l_index == ulinkbp->l_index) {
		oldpri = splstr();
		kmem_free(linkp->cmlb_lblk.l_qbot->q_ptr, sizeof(cmux_lstrm_t));
		linkp->cmlb_lblk.l_qbot->q_ptr = NULL;
		RD(linkp->cmlb_lblk.l_qbot)->q_ptr = NULL;
		bzero(linkp, sizeof(cmux_link_t));
		splx(oldpri);
		DEBUG1(("cmux_unlink: unlinked principal stream."));

		return (1);
	}

	linkp = wsp->w_lstrmsp;

	for (i = 0; i < wsp->w_numlstrms; i++, linkp++) 
		if ((linkp->cmlb_flg) && 
		    (linkp->cmlb_lblk.l_index == ulinkbp->l_index)) {
			oldpri = splstr();
			kmem_free(linkp->cmlb_lblk.l_qbot->q_ptr, 
						sizeof(cmux_lstrm_t));
			linkp->cmlb_lblk.l_qbot->q_ptr = NULL;
			RD(linkp->cmlb_lblk.l_qbot)->q_ptr = NULL;
			bzero(linkp, sizeof(cmux_link_t));
			wsp->w_lstrms--;
			splx(oldpri);
			DEBUG1(("cmux_unlink: unlinked secondary stream.\n"));

			return (1);
		}

	return (0);
}


/*
 * STATIC void
 * cmux_close_chan(cmux_ws_t *, cmux_lstrm_t *, ch_proto_t *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	cmux_close_chan() is called when CH_CLOSE_ACK message
 *	is returned by the principal stream in response to
 *	CH_CHANCLOSE message.
 */
/* ARGSUSED */
STATIC void
cmux_close_chan(cmux_ws_t *wsp, cmux_lstrm_t *lstrmp, ch_proto_t *protop)
{
	unsigned long	chan;
	cmux_t		*cmuxp;
	pl_t		oldpri;


	chan = lstrmp->lstrm_id;
	cmuxp = wsp->w_cmuxpp[chan];

	if (!cmuxp) {
		DEBUG5(("cmux_close_chan: Found null cmuxp; do not wakeup"));
		return;
	}

	DEBUG5(("cmux_close_chan: Close on channel %d\n", cmuxp->cmux_num));

	/*
	 * Turn off the CMUX_CLOSE flag in the cmux_t channel
	 * state structure and signal the waiting cmuxclose()
	 * routine.
	 */
	oldpri = LOCK(cmux_lock, CMUXPL);
	cmuxp->cmux_flg &= ~CMUX_WCLOSE;
	SV_SIGNAL(cmux_wclosesv, 0);
	UNLOCK(cmux_lock, oldpri);

	DEBUG5(("cmux_close_chan: Called wakeup channel %d\n", cmuxp->cmux_num));

	return;
}


/*
 * STATIC clock_t 
 * cmux_striptime(mblk_t *)
 *
 * Calling/Exit State:
 *	It returns the timestamp of the last channel switch.
 */
STATIC clock_t
cmux_striptime(mblk_t *mp)
{
	ch_proto_t *protop;


	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;

	return (protop->chp_tstmp);
}


/*
 * STATIC int
 * cmux_foundit(clock_t, clock_t, clock_t)
 *
 * Calling/Exit State:
 *	It returns non-zero value if the <timeval> is within the 
 *	<mintime> and <maxtime> range, otherwise it returns zero.
 */
STATIC int
cmux_foundit(clock_t mintime, clock_t maxtime, clock_t timeval)
{
	return !(TICKS_LATER(mintime, timeval) || TICKS_LATER(timeval, maxtime));	/* L000 */
}


/*
 * STATIC cmux_t *
 * cmux_findchan(cmux_ws_t *, clock_t);
 * 
 * Calling/Exit State:
 *	Return the pointer to the channel structure, if
 *	their exists a channel that was active at the
 *	given timestamp, otherwise return NULL.
 *
 * Description:
 *	cmux_findchan is called with the timestamp argument
 *	to find the channel that was active for the timestamp
 *	value. This is accomplished by looking at the 
 *	w_switchtimes array of the workstation of ws_stat
 *	structure. This array contains the timestamps of
 *	the last w_numswitch (maximum value is 10) channel
 *	switches and the channel numbers made active at
 *	each timestamp. Using the timestamp argument,
 *	cmux_findchan() finds the channel that was active
 *	at the given timestamp. 
 *
 *	100  <-- most recent switch 
 *	 30
 *	930
 *	700  <-- last switch
 */
STATIC cmux_t *
cmux_findchan(cmux_ws_t *wsp, clock_t timestamp)
{
	struct cmux_swtch *switchp;
	clock_t curtime, mintime;
	int found, cnt;
	

	curtime = TICKS();			/* L000 */

	switchp = &wsp->w_swtchtimes[wsp->w_numswitch - 1];
	if (!cmux_foundit(switchp->sw_time, curtime, timestamp))
		/* chanmux will drop the message */
		return (cmux_t *) NULL;

	switchp = &wsp->w_swtchtimes[0];
	cnt = 0;
	mintime = switchp->sw_time;
	found = cmux_foundit(mintime, curtime, timestamp);

	while ((cnt < wsp->w_numswitch - 1) && !found) {
		switchp++;
		cnt++;
		curtime = mintime;
		mintime = switchp->sw_time;
		found = cmux_foundit(mintime, curtime, timestamp);
	}

	if (!found)
		return (cmux_t *) NULL;

	return (wsp->w_cmuxpp[switchp->sw_chan]);
}


/*
 * STATIC void
 * cmux_clr_ioc(cmux_ws_t *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	cmux_clr_ioc() is responsible for freeing all
 *	M_IOCNAK messages from secondary streams.
 */
STATIC void
cmux_clr_ioc(cmux_ws_t *wsp)
{
	cmux_link_t	*linkp;
	cmux_t		*cmuxp;
	int		i;
	pl_t		oldpri;
	

	DEBUG6(("cmux_clr_ioc: Entering\n"));

	if (wsp->w_iocmsg) 
		freemsg(wsp->w_iocmsg);

	wsp->w_iocmsg = (mblk_t *) NULL;

	linkp = wsp->w_princp + wsp->w_ioctlchan;
	linkp->cmlb_iocresp = 0;
	if (linkp->cmlb_iocmsg) 
		freemsg(linkp->cmlb_iocmsg);

	for (i = 0, linkp = wsp->w_lstrmsp; i < wsp->w_numlstrms; i++, linkp++) {
		linkp->cmlb_iocresp = 0;
		if (linkp->cmlb_iocmsg) 
			freemsg(linkp->cmlb_iocmsg);
	}

	oldpri = splstr();
	wsp->w_ioctlcnt = wsp->w_ioctllstrm = 0;
	wsp->w_ioctlchan = 0;
	wsp->w_state &= ~CMUX_IOCTL;
	splx(oldpri);

	for (i = 0; i < wsp->w_numchan; i++) {
		cmuxp = wsp->w_cmuxpp[i];
		if (cmuxp && cmuxp->cmux_wqp)
			qenable(cmuxp->cmux_wqp);
	}
}


/*
 * STATIC void
 * cmux_switch_chan(cmux_ws_t *, ch_proto_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cmux_switch_chan(cmux_ws_t *wsp, ch_proto_t *protop)
{
	int		i;
	pl_t		oldpri;
	unsigned long	chan;
	struct cmux_swtch *switchp;


	wsp->w_numswitch = min(wsp->w_numswitch + 1, CMUX_NUMSWTCH);
	chan = protop->chp_chan;

	if (!wsp->w_cmuxpp[chan]) {
		/*
		 *+ An invalid channel switch request is sent
		 *+ by the principal lower stream.
		 */
		cmn_err(CE_PANIC, 
			"invalid channel switch request %x", chan);
	}

	DEBUG1(("cmux_switch_chan: switching to channel %x", chan));

	/*
	 * Must be a good channel. Update switchtime list. 
	 * The elemets of the w_switcchtimes array are updated
	 * to reflect the addition of the latest channel switch.
	 * The timestamp in the ch_proto_t structure becomes the
	 * head of the w_switchtimes list, so that any secondary
	 * stream inputs with timestamps greater than this new one
	 * are sent to the newly active channel.
	 */
	oldpri = splstr();
	for (i = CMUX_NUMSWTCH - 1; i > 0; i--) {
		DEBUG1(("cmux_switch_chan: wsp %x, i %x, i-1 %x",
			wsp, &wsp->w_swtchtimes[i], &wsp->w_swtchtimes[i-1]));
		bcopy(&wsp->w_swtchtimes[i-1], &wsp->w_swtchtimes[i],
				sizeof(struct cmux_swtch));
	}

	switchp = &wsp->w_swtchtimes[0];
	DEBUG1(("cmux_switch_chan: wsp %x, switchp %x", wsp, switchp));
	switchp->sw_chan = protop->chp_chan;
	switchp->sw_time = protop->chp_tstmp;
	splx(oldpri);

	return;
}


/*
 * STATIC void
 * cmux_iocack(queue_t *, mblk_t *, struct iocblk *, int)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cmux_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mblk_t	*tmp;


	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;

	if ((tmp = unlinkb(mp)) != (mblk_t *)NULL)
		freeb(tmp);

	qreply(qp, mp);
}


/*
 * STATIC void
 * cmux_iocnak(queue_t *, mblk_t *, struct iocblk *, int, int)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cmux_iocnak(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error, int rval)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp, mp);
}


/*
 * STATIC void
 * cmux_do_iocresp(cmux_ws_t *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	The cmux_do_iocresp() is responsible for tallying the M_IOCTL
 *	responses. It counts the number of positive and negative
 *	acknowledgements.
 *
 *	If all are negative, the M_IOCNAK from the principal stream
 *	is sent up unless a secondary stream set ioc_err in its
 *	M_IOCNAK message to non-zero (typically, a M_IOCNAK message
 *	with ioc_err == 0 means the driver/module did not recognize
 *	the ioctl command). cmux_clr_ioc() is called to free all
 *	other M_IOCNAK messages.
 *
 *	If only one response to the M_IOCTL was positive, then we are
 *	in a sane state for ioctl processing. Send the positive response
 *	upstream, and set w_ioctllstrm to 0 if it was the principal
 *	stream that ACK'd or the (index + 1) of the secondary stream
 *	that ACK'd. If the response was M_IOCACK, call cmux_clr_ioc()
 *	since we are done, otherwise there is still more ioctl processing
 *	to do (M_COPYIN/M_COPYOUT).
 *
 *	Otherwise there were multiple positive responses. The M_IOCTL
 *	message with an errno set to EACCES. For each lower stream that
 *	responded, we check the message type. If it is M_COPYIN/M_COPYOUT
 *	we turn the message into a M_IOCDATA message and indicate in the
 *	copyresp structure that an error (EACCES) occurred, and then ship
 *	it down the lower stream. cmux_clr_ioc() is called to free any
 *	M_IOCACK/M_IOCNAK responses.
 */
STATIC void
cmux_do_iocresp(cmux_ws_t *wsp)
{
	cmux_link_t	*linkp;
	ulong_t		ackcnt;		/* number of ACKs */
	ulong_t		nakcnt;		/* number of NAKs */
	ulong_t		acknum;		/* channel number that sent an ACK */
	ulong_t		naknum;		/* channel number that sent an NAK */
	ulong_t		i;
	struct iocblk	*iocp;
	struct copyresp *resp;
	cmux_t		*cmuxp;
	mblk_t		*mp;


	ackcnt = nakcnt = 0;
	acknum = naknum = 0;

	cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
	linkp = wsp->w_princp + wsp->w_ioctlchan;
	/* LINTED pointer alignment */
	iocp = (struct iocblk *) linkp->cmlb_iocmsg->b_rptr;

	if (linkp->cmlb_iocresp == M_IOCACK) 
		ackcnt++;
	else if (linkp->cmlb_iocresp == M_IOCNAK) 
		nakcnt++;

	linkp = wsp->w_lstrmsp;
	for (i = 1; i <= wsp->w_numlstrms; i++, linkp++) {
		if (linkp->cmlb_iocresp == M_IOCACK) {
			ackcnt++;
			acknum = i;
		} else if (linkp->cmlb_iocresp == M_IOCNAK) {
			nakcnt++;
			/* LINTED pointer alignment */
			iocp = (struct iocblk *) linkp->cmlb_iocmsg->b_rptr;
			if (iocp->ioc_error != 0)
				naknum = i;
		}
	}

	if (ackcnt == 0) {		/* failure all around */

		DEBUG6(("cmux_do_iocresp: everyone nackd\n"));

		if (naknum == 0) {
			linkp = wsp->w_princp + wsp->w_ioctlchan;
			putnext(cmuxp->cmux_rqp, linkp->cmlb_iocmsg);
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
		} else {
			linkp = wsp->w_lstrmsp + naknum - 1;
			putnext(cmuxp->cmux_rqp, linkp->cmlb_iocmsg);
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
		}

		cmux_clr_ioc(wsp);

	} else if (ackcnt == 1) {	/* success! */

		DEBUG6(("cmux_do_iocresp: only one ack\n"));

		if (acknum == 0)
			linkp = wsp->w_princp + wsp->w_ioctlchan;
		else
			linkp = wsp->w_lstrmsp + acknum - 1;

		DEBUG6(("cmux_do_iocresp: about to call putnext\n"));
		putnext(cmuxp->cmux_rqp, linkp->cmlb_iocmsg);

		if (linkp->cmlb_iocmsg->b_datap->db_type != M_IOCACK) {
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
			wsp->w_ioctllstrm = acknum;
		} else {
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
			cmux_clr_ioc(wsp);
		}

	} else {			/* multiple acks */

		/*
		 * Multiple acks. Send up an M_IOCNAK with
		 * errno set to EACCES, and fail each
		 * M_COPYIN/M_COPYOUT that was passed up
		 * by sending down an M_IOCDATA message
		 * with ioc_rval set to EACCES.
		 */

		DEBUG6(("cmux_do_iocresp: multiple acks\n"));

		linkp = wsp->w_princp + wsp->w_ioctlchan;
		mp = linkp->cmlb_iocmsg;

		if (mp->b_datap->db_type == M_COPYIN ||
		    mp->b_datap->db_type == M_COPYOUT) {
			/* LINTED pointer alignment */
			resp = (struct copyresp *) mp->b_rptr;
			resp->cp_rval = (caddr_t) EACCES;
			mp->b_datap->db_type = M_IOCDATA;
			putnext(linkp->cmlb_lblk.l_qbot, mp);
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
		}

		linkp = wsp->w_lstrmsp;
		for (i = 1; i <= wsp->w_numlstrms; i++, linkp++) {
			if (!linkp->cmlb_flg) 
				continue;

			mp = linkp->cmlb_iocmsg;
			if (mp->b_datap->db_type == M_COPYIN ||
			    mp->b_datap->db_type == M_COPYOUT) {
				/* LINTED pointer alignment */
				resp = (struct copyresp *) mp->b_rptr;
				resp->cp_rval = (caddr_t) EACCES;
				mp->b_datap->db_type = M_IOCDATA;
				putnext(linkp->cmlb_lblk.l_qbot, mp);
				linkp->cmlb_iocmsg = (mblk_t *) NULL;
      			}
		} /* for */

		mp = wsp->w_iocmsg;
		mp->b_datap->db_type = M_IOCNAK;
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;
		iocp->ioc_error = EACCES;
		putnext(cmuxp->cmux_rqp, mp);
		wsp->w_iocmsg = (mblk_t *) NULL;
		cmux_clr_ioc(wsp);

	} /* multiple acks */
}


/*
 * STATIC int 
 * cmux_up_rsrv(queue_t *)
 * 
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This routine will only get called when a canputnext from a lower
 *	stream of this queue failed.
 *
 *	Since the data coming from below the multiplexor is enqueued
 *	in the lower's stream read queues, this service procedure
 *	is not used for message processing. However, it may be called
 *	if a flow control condition blocked cmux_mux_rsrv() (which
 *	is feeding messages to the module above the upper stream
 *	of the mux) and STREAMS flow control is trying to back
 *	enable the queue. Therefore, this routine calls qenable() for the 
 *	principal lower stream of the channel and all secondary streams 
 *	linked to the workstation.
 */
STATIC int 
cmux_up_rsrv(queue_t *qp)
{
	cmux_t		*cmuxp;
	cmux_ws_t	*wsp;
	unsigned long	i;
	cmux_link_t	*linkp;


	cmuxp = (cmux_t *) qp->q_ptr;
	if (cmuxp == (cmux_t *) NULL) {
#ifdef DEBUG
		cmn_err(CE_WARN, "chanmux: Invalid q_ptr in cmux_up_rsrv");
#endif
		return (0);
	}

	wsp = cmuxp->cmux_wsp;

	/*
	 * qenable all secondary lower streams and the principal
	 * stream associated with channel
	 */

	/*
	 * Do secondary streams first. 
	 */
	linkp = wsp->w_lstrmsp; 

#if defined(DEBUG) || defined(DEBUG_TOOLS)
	if (linkp == (cmux_link_t *) NULL) {
		cmn_err(CE_WARN, "chanmux: NULL lstrms ptr in cmux_up_rsrv");
		return (0);
	}
#endif /* DEBUG || DEBUG_TOOLS */

	for (i = 0; i < wsp->w_numlstrms; i++, linkp++) {
		if (!linkp->cmlb_flg) 
			continue;
		if (linkp->cmlb_lblk.l_qbot)
			qenable(RD(linkp->cmlb_lblk.l_qbot));
	}
	
	/*
	 * Now enable principal stream.
	 */
#if defined(DEBUG) || defined(DEBUG_TOOLS)
	if (linkp == (cmux_link_t *) NULL) {
		cmn_err(CE_WARN, "chanmux: NULL princstrms ptr in up_rsrv");
		return (0);
	}
#endif /* DEBUG || DEBUG_TOOLS */

	linkp = wsp->w_princp + cmuxp->cmux_num;
	if (linkp->cmlb_lblk.l_qbot)
		qenable(RD(linkp->cmlb_lblk.l_qbot));

	return (0);
}


/*
 * STATIC int
 * cmux_up_wsrv(queue_t *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Service routine for all messages going downstream. Do not service
 *	any messages until a principal stream is linked below the channel,
 *	nor while an ioctl is being processed on a channel different from
 *	ours.
 *
 *	When the service routine is activated and we find an M_IOCTL
 *	message, mark that this channel is servicing an ioctl.
 *	Perform a copymsg of the message for each secondary stream
 *	and ship the messages to the principal stream for the channel
 *	as well as all secondary streams. 
 *
 *	For M_FLUSH handling, only flush the principal stream.
 */
STATIC int
cmux_up_wsrv(queue_t *qp)
{
	mblk_t		*mp;
	cmux_t		*cmuxp;
	cmux_ws_t	*wsp;
	cmux_link_t	*linkp;
	unsigned long	chan;
	int		i;
	pl_t		oldpri;


	cmuxp = (cmux_t *) qp->q_ptr;
	if (cmuxp == (cmux_t *) NULL) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
		cmn_err(CE_WARN, 
			"!cmux_up_wsrv: Invalid q_ptr in cmux_up_wsrv");
#endif /* DEBUG || DEBUG_TOOLS */
		return (0);
	}

	wsp = cmuxp->cmux_wsp;
	chan = cmuxp->cmux_num;

	linkp = wsp->w_princp + chan;
	if (!linkp->cmlb_flg) 
		return (0);

	if ((wsp->w_state & CMUX_IOCTL) && (wsp->w_ioctlchan != chan)) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
		cmn_err(CE_WARN, 
			"!cmux_up_wsrv: blocked on ioctl");
#endif /* DEBUG || DEBUG_TOOLS */
		return (0);
	}

	/*
	 * Keep getting messages until none left or we honor flow
	 * control and see that the stream above us is blocked or 
	 * are set to enqueue messages while an ioctl is processed.
	 */
	while ((mp = getq(qp)) != NULL) {

		switch (mp->b_datap->db_type) {
		case M_FLUSH:
			/*
	 		 * Flush everything we haven't looked at yet.
			 * Turn the message around if FLUSHR was set
	 		 */
			if (*mp->b_rptr & FLUSHW) {
				flushq(qp, FLUSHDATA);
				*mp->b_rptr &= ~FLUSHW;
			}

			if (*mp->b_rptr & FLUSHR) 
				putnext(RD(qp), mp);
			else 
				freemsg(mp);

			continue;

		case M_IOCDATA:
			/*
			 * Route the message downsteam if the workstation 
			 * state is set to CMUX_IOCTL or processing is being
			 * done on this channel, otherwise free the message.
			 */
			if (!wsp->w_state & CMUX_IOCTL || 
					wsp->w_ioctlchan != chan) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
				cmn_err(CE_WARN,
					"!cmux_up_wsrv: Unexpected M_IOCDATA "
					"msg; freeing it");
#endif /* DEBUG || DEBUG_TOOLS */
				freemsg(mp);
				continue;
			}

			/*
			 * The w_ioctllstrm indicates whether the ioctl
			 * is being processed on the principal lower stream
			 * or a secondary stream. If the values is 0, send
			 * the M_IOCDATA down to the principal stream.
			 * Otherwise, subtract 1 and use the value as
			 * an index to find the secondary stream dealing
			 * with the ioctl and send M_IOCDATA down its stream.
			 */
			if (wsp->w_ioctllstrm == 0) 
				putnext(linkp->cmlb_lblk.l_qbot, mp);
			else {
				cmux_link_t *nlinkp;

				nlinkp = wsp->w_lstrmsp + wsp->w_ioctllstrm - 1;
				putnext(nlinkp->cmlb_lblk.l_qbot, mp);
			}

			continue;

		case M_IOCTL: {
			/*
	   		 * We could not have gotten in here if an ioctl was
			 * in process, on this stream or any other. STREAMS 
			 * protect against multiple ioctls on the same stream,
			 * and we protect against multiple ioctls on different
			 * streams.
			 */

			struct iocblk	*iocp;
			cmux_link_t	*nlinkp;

			/* LINTED pointer alignment */
			iocp = (struct iocblk *) mp->b_rptr;

			if (iocp->ioc_cmd == I_PUNLINK || iocp->ioc_cmd == I_UNLINK) {
				if (cmux_unlink(mp, cmuxp, iocp)) {
					DEBUG6(("\ncmux_up_wsrv: unlinking cmux\n"));
					cmux_iocack(qp, mp, iocp, 0);

					/*
					 * Explicitly enable queue before 
					 * returning so message processing
					 * can continue. We return rather
					 * than continue because we need to
					 * reset state.
					 */
					qenable(qp);
					return (0);
				}
			}

			DEBUG6(("\ncmux_up_wsrv: ioctl 0x%x starting in chan 0x%x\n", iocp->ioc_cmd, chan));

			/*
			 * Put the message in front of the queue if an
			 * ioctl is already in-transit. Note that since
			 * M_IOCTL is not a priority message, the queue
			 * would not be enabled. An explicit enable is
			 * done in cmux_clr_ioc() after a reply to an
			 * in-transit ioctl is received.
			 */
			if (wsp->w_state & CMUX_IOCTL) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
				cmn_err(CE_WARN,
					"!cmux_up_wsrv: An ioctl already in transit");
#endif /* DEBUG || DEBUG_TOOLS */
				putbq(qp, mp);
				return (0);
			}

			/*
			 * The CMUX_IOCTL flag is turned on in and the
			 * w_ioctlchan value is set to the chan number.
			 * The w_ioctlcnt value is set to 1 (for the
			 * principal stream) plus the number of secondary
			 * streams.
			 */

			oldpri = splstr();
			wsp->w_ioctlcnt = 1 + wsp->w_lstrms;
			splx(oldpri);

			/*
			 * Ship copies of message to secondary streams.
			 * Adjust ioctlcnt so that if message copy
			 * fails, we aren't waiting for a response
			 * that will never come.
			 */
			nlinkp = wsp->w_lstrmsp;
			for (i = 0; i < wsp->w_numlstrms; i++, nlinkp++) {
				if (!nlinkp->cmlb_flg) 
					continue;

				nlinkp->cmlb_iocresp = 0;
				nlinkp->cmlb_iocmsg = copymsg(mp);
				if (nlinkp->cmlb_iocmsg) {
					oldpri = splstr();
					wsp->w_state |= CMUX_IOCTL;
					wsp->w_ioctlchan = chan;
					splx(oldpri);
			 		putnext(nlinkp->cmlb_lblk.l_qbot,
						nlinkp->cmlb_iocmsg);
				} else {
	                                wsp->w_ioctlcnt -= 1;
				}
			}

			/*
			 * Ship message to principal stream.
			 */
			nlinkp = wsp->w_princp + chan;
			nlinkp->cmlb_iocresp = 0;
			nlinkp->cmlb_iocmsg = copymsg(mp);
			if (nlinkp->cmlb_iocmsg) {
				oldpri = splstr();
				wsp->w_state |= CMUX_IOCTL;
				wsp->w_ioctlchan = chan;
				splx(oldpri);
				putnext(nlinkp->cmlb_lblk.l_qbot,
						nlinkp->cmlb_iocmsg);
			} else {
				wsp->w_ioctlcnt -= 1;
			}

			if (!(wsp->w_state & CMUX_IOCTL)) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
				cmn_err(CE_NOTE, 
					"!cmux_up_wsrv: Could not send "
					"an M_IOCTL message downstream");
#endif /* DEBUG || DEBUG_TOOLS */
				cmux_iocnak(qp, mp, iocp, EAGAIN, -1);
				continue;
			}

			wsp->w_iocmsg = mp;
			continue;

		} /* M_IOCTL */			
			
		default:
			if (mp->b_datap->db_type <= QPCTL && 
					!canputnext(linkp->cmlb_lblk.l_qbot)) {
				putbq(qp, mp);
				/* read side is blocked */
				return (0);
			}

			putnext(linkp->cmlb_lblk.l_qbot, mp);
			continue;

		} /* switch */

	} /* while */

	return (0);
}


/*
 * STATIC int 
 * cmux_up_rput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int 
cmux_up_rput(queue_t *qp, mblk_t *mp)
{
	/* should not be called */

	freemsg(mp);
#ifdef DEBUG
	cmn_err(CE_WARN,"cmux_up_rput: In cmux_up_rput");
#endif

	return (0);
}


/*
 * STATIC int 
 * cmux_up_wput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This routine is the upper stream's write put procedure. Its
 *	principal function is to process the I_LINK/I_PLINK M_IOCTL
 *	messages that are used to set up the multiplexor.
 *
 *	If non-priority messages are put before a principal stream has 
 *	been linked under, free them. For non-ioctl priority messages,
 *	enqueue them, and for non-I_LINK/I_PLINK ioctls, NACK them.
 */
STATIC int
cmux_up_wput(queue_t *qp, mblk_t *mp)
{
	cmux_t		*cmuxp;
	struct iocblk	*iocp;
	cmux_ws_t	*wsp;
	cmux_link_t	*linkp;
	cmux_lstrm_t	*lstrmp;
	int		error, i;


	cmuxp = (cmux_t *) qp->q_ptr;
	if (cmuxp == (cmux_t *) NULL) {
#ifdef DEBUG
		cmn_err(CE_WARN, "cmux_up_wput: Invalid q_ptr in cmux_up_wput");
#endif
		freemsg(mp);
		return (0);
	}

	wsp = cmuxp->cmux_wsp;
	linkp = wsp->w_princp + cmuxp->cmux_num;

	if (mp->b_datap->db_type < QPCTL && mp->b_datap->db_type != M_IOCTL) {
		/*
		 * If the principal stream has been linked underneath
		 * then enqueue the message, otherwise free the message
		 * since its a low priorty message.
		 */
		if (!linkp->cmlb_flg)
			freemsg(mp);
		else
			putq(qp, mp);
		return (0);
	}

	/*
	 * The message is enqueued since its a high priority message.
	 */
	if (mp->b_datap->db_type != M_IOCTL) {
		putq(qp, mp);
		return (0);
	}

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;

	if (iocp->ioc_cmd != I_LINK && iocp->ioc_cmd != I_PLINK) {
		/*
		 * If the principal stream has been linked underneath
		 * then enqueue the message, otherwise send up an nak 
		 * message, since its a high priority M_IOCTL message.
		 */
		if (!linkp->cmlb_flg) {
			cmux_iocnak(qp, mp, iocp, EAGAIN, -1);
		} else {
			putq(qp, mp);
		}
		return (0);
	}

	/*
	 * The message is a high priority I_PLINK/I_LINK M_IOCTL message.
	 */

	lstrmp = (cmux_lstrm_t *) kmem_alloc(sizeof(cmux_lstrm_t), KM_NOSLEEP);
	if (lstrmp == (cmux_lstrm_t *) NULL) {
		cmux_iocnak(qp, mp, iocp, EAGAIN, -1);
		return (0);
	}

	/*
	 * The workstation structure's lower stream structure for
	 * the channel (the cmux_link_t structure in the w_princp
	 * field of the cmux_ws_t structure) is examined to see if
	 * the stream has been linked underneath. If not, then we
	 * are setting up the principal stream for the channel, 
	 * otherwise, we are linking an auxillary input device 
	 * (mouse, etc.) to the workstation.
	 */
	if (linkp->cmlb_flg) {
		/*
		 * Add secondary stream to set of lower streams. 
		 */

		/*
		 * Call cmux_allocstrms() to allocate additional
		 * cmxu_link_t structures for the workstation's
		 * w_lstrmsp list.
		 */
		if (error = cmux_allocstrms(wsp, ++wsp->w_lstrms)) {
			wsp->w_lstrms--;
			kmem_free(lstrmp, sizeof(cmux_lstrm_t));
			cmux_iocnak(qp, mp, iocp, error, -1);
			return (0);
		}

		/*
		 * Find the first free cmux_link_t structure in
		 * the list and save its index.
		 */
		linkp = wsp->w_lstrmsp;
		for (i = 0; i < wsp->w_numlstrms; i++, linkp++) 
			if (!linkp->cmlb_flg) 
				break;

		/*
		 * Now i is the first free link_t struct found.
		 * Save the index, set the lstrm_flg to CMUX_SECSTRM.
		 */
		lstrmp->lstrm_wsp = wsp;
		lstrmp->lstrm_flg = CMUX_SECSTRM;
		lstrmp->lstrm_id = i; 

		/*
		 * Copy the linkblk structure argument to the ioctl
		 * into the cmux_link_t structure for the lower stream
		 * and set the q_ptr field of the queues given by the
		 * linkblk structure to point to the cmux_lstrm_t 
		 * structure.
		 */
		bcopy(mp->b_cont->b_rptr, &linkp->cmlb_lblk, 
					sizeof(struct linkblk));
		linkp->cmlb_flg = CMUX_SECSTRM;
		linkp->cmlb_lblk.l_qbot->q_ptr = (caddr_t) lstrmp;
		RD(linkp->cmlb_lblk.l_qbot)->q_ptr = (caddr_t) lstrmp;
		
		/*
		 * Send up an M_IOCACK message to pass the ioctl.
		 */
		cmux_iocack(qp, mp, iocp, 0);

		return (0);
	}

	/*
	 * The stream is going to be the principal stream.
	 */

	/*
	 * Set the cmux_lstrm_t flag to CMUX_PRINCSTRM, the
	 * id number to the channel (VT) number.
	 */
	lstrmp->lstrm_wsp = wsp;
	lstrmp->lstrm_flg = CMUX_PRINCSTRM;
	lstrmp->lstrm_id = cmuxp->cmux_num;
	
	/*
	 * Copy the linkblk structure to the ioctl into the 
	 * cmux_link_t structure for the lower stream.
	 */
	bcopy(mp->b_cont->b_rptr, &linkp->cmlb_lblk, sizeof(struct linkblk));

	/*
	 * Set the flag in cmux_link_t to be CMUX_PRINCSTRM and set
	 * the q_ptr field of the queues given by the linkblk structure
	 * to point to the cmux_lstrm_t structure.
	 */
	linkp->cmlb_flg = CMUX_PRINCSTRM;
	linkp->cmlb_lblk.l_qbot->q_ptr = (caddr_t ) lstrmp;
	RD(linkp->cmlb_lblk.l_qbot)->q_ptr = (caddr_t) lstrmp;

	/*
	 * Send up an M_IOCACK to pass the ioctl.
	 */
	cmux_iocack(qp, mp, iocp, 0);

	/*
	 * Enable message processing on queue now that there is 
	 * an active channel 
	 */
	qenable(qp); 

	return (0);
}


/*
 * STATIC int 
 * cmux_mux_wsrv(queue_t *)
 *
 * Calling/Exit State:
 *
 * Description:
 *	This routine will be invoked by flow control when the queue below
 *	it is enabled because a canputnext() from cmux_up_wsrv() on the 
 *	queue failed.
 *
 *	This routine, if invoked on a principal stream linked below,
 *	will enable only the upper write queue associated with the
 *	principal stream. If invoked on a secondary stream, any of the
 *	upper streams above could be the culprit, so enable them all.
 */
STATIC int
cmux_mux_wsrv(queue_t *qp)
{
	cmux_lstrm_t	*lstrmp;
	cmux_ws_t	*wsp;
	cmux_t		*cmuxp;
	unsigned long	i;


	lstrmp = (cmux_lstrm_t *) qp->q_ptr;
	if (lstrmp == (cmux_lstrm_t *) NULL) {
#ifdef DEBUG
		cmn_err(CE_WARN, 
			"cmux_mux_wsrv: Invalid q_ptr in cmux_mux_wsrv");
#endif
		return (0);
	}

	wsp = (cmux_ws_t *) lstrmp->lstrm_wsp;
	if (lstrmp->lstrm_flg & CMUX_PRINCSTRM) {
		cmuxp = wsp->w_cmuxpp[lstrmp->lstrm_id];
		if (cmuxp == NULL) {
#ifdef DEBUG
			cmn_err(CE_WARN, 
				"cmux_mux_wsrv: Invalid cmuxp in mux_wsrv");
#endif
		   return (0);
		}
		if (cmuxp->cmux_flg & CMUX_OPEN)
			qenable(cmuxp->cmux_wqp);
	} else {
		for (i = 0; i <= wsp->w_numchan; i++) {
		   cmuxp = wsp->w_cmuxpp[i];
		   if ((cmuxp == NULL) || !(cmuxp->cmux_flg & CMUX_OPEN))
			continue;
		   qenable(cmuxp->cmux_wqp);
		}
	}

	return (0);
}


/* 
 * STATIC int
 * cmux_mux_rsrv(queue_t *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	cmux_mux_rsrv() is the service routine of all input messages from the 
 *	lower streams. For normal messages from principal streams, forward
 *	directly to the associated upper stream, if it exists, otherwise
 *	discard the message. Normal messages from the lower streams should
 *	be timestamped with an M_PROTO header for non-priority messages,
 *	M_PCPROTO for priority messages. Send the message to the channel
 *	that was active in the range given by the timestamp. If the
 *	channel was closed, drop the message on the floor.
 *
 *	If the message is ioctl-related, make note of the
 *	response in the cmux_link_t structure for the lower stream
 *	and update the count of waiting responses. When zero, check
 *	all STREAMS. If exactly 1 ack (M_IOCACK, M_COPYIN, M_COPYOUT) was
 *	sent up, we are in good shape. If more than one ack was sent up,
 *	NACK the ioctl, and send M_IOCDATA messages to all lower streams
 *	requesting M_COPYINs/M_COPYOUTs.
 *
 *	For the switch channel command message from the principal stream, 
 *	update the list of most recently active channels and its count.
 *	Upon receipt of the "channel close acknowledge" message, 
 *	wakeup the process sleeping in the close.
 */
STATIC int
cmux_mux_rsrv(queue_t *qp)
{
	cmux_lstrm_t	*lstrmp;
	mblk_t		*mp;
	unsigned long	princflg;
	time_t		timestamp;
	cmux_t		*cmuxp;
	ch_proto_t	*protop;
	cmux_link_t	*linkp;
	cmux_ws_t	*wsp;


	lstrmp = (cmux_lstrm_t *) qp->q_ptr;
	if (lstrmp == (cmux_lstrm_t *) NULL) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
		cmn_err(CE_WARN, 
			"!cmux_mux_rsrv: Invalid q_ptr in cmux_mux_rsrv");
#endif /* DEBUG || DEBUG_TOOLS */
		return (0);
	}

#if defined(DEBUG) || defined(DEBUG_TOOLS)
	if (!lstrmp->lstrm_flg) {
		cmn_err(CE_WARN, 
			"!cmux_mux_rsrv: Invalid q_ptr in cmux_mux_rsrv");
		return (0);
	}
#endif /* DEBUG || DEBUG_TOOLS */

	wsp = lstrmp->lstrm_wsp;

	if (lstrmp->lstrm_flg & CMUX_PRINCSTRM) {
		princflg = 1;
		linkp = wsp->w_princp + lstrmp->lstrm_id;
	} else {
		princflg = 0;
		linkp = wsp->w_lstrmsp + lstrmp->lstrm_id;
	}
	
	while ((mp = getq(qp)) != NULL) {

		if (IOCTL_TYPE(mp->b_datap->db_type))
			goto msgproc;

		if (!princflg) {
			/*
			 * Message is from lower stream and should have
			 * header indicating timestamp.
			 */
			if ((mp->b_wptr - mp->b_rptr) != sizeof(ch_proto_t)) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
				cmn_err(CE_WARN,
					"!cmux_mux_rsrv: illegal lower "
					"stream protocol in mux_rsrv");
#endif /* DEBUG || DEBUG_TOOLS */
				freemsg(mp);
				continue;
			}

			timestamp = cmux_striptime(mp);
			cmuxp = cmux_findchan(wsp, timestamp);
			if (cmuxp == (cmux_t *)NULL) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
				cmn_err(CE_WARN,
					"!cmux_mux_rsrv: illegal cmuxp found "
					"in cmux_mux_rsrv");
#endif /* DEBUG || DEBUG_TOOLS */
				freemsg(mp);
				continue;
			}

			if (mp->b_datap->db_type < QPCTL && 
					!canputnext(cmuxp->cmux_rqp)) {
				putbq(qp, mp);
				return (0);
			}
		} else {
			cmuxp = wsp->w_cmuxpp[lstrmp->lstrm_id];
			if (cmuxp == (cmux_t *)NULL) {
#if defined(DEBUG) || defined(DEBUG_TOOLS)
				cmn_err(CE_NOTE,
					"!cmux_mux_rsrv: did not find cmuxp; id %d %x",
					lstrmp->lstrm_id, wsp->w_cmuxpp);
#endif /* DEBUG || DEBUG_TOOLS */
				freemsg(mp);
				continue;
			}

			if (mp->b_datap->db_type < QPCTL && 
					!canputnext(cmuxp->cmux_rqp)) {
				putbq(qp, mp);
				return (0);
			}
		}

msgproc:   
		switch (mp->b_datap->db_type) {
		case M_FLUSH:
			if (*mp->b_rptr & FLUSHR) {
				flushq(qp, FLUSHDATA);
				*mp->b_rptr &= ~FLUSHR;
			}

			if (*mp->b_rptr & FLUSHW) {
				/*
				 * Nothing to flush on the lower write side.
				 */
				qreply(qp, mp);
			} else
				freemsg(mp);

			continue;

		/*
		 * M_IOCTL related message handling: 
		 *
		 * M_COPYIN, M_COPYOUT, M_IOCACK and M_IOCNAK are the
		 * the four valid responses lower streams may have in
		 * acknowledging an M_IOCTL or M_IOCDATA message.
		 *
		 * ioctl handling differentiate between waiting for ack and
		 * received ack. This treats M_COPYIN/M_COPYOUT messages 
		 * differently
		 */
		case M_IOCACK:
			DEBUG6(("cmux_mux_rsrv: Found M_IOCACK on queue.\n"));

			/*
			 * If w_ioctlcnt is non-zero, then CHANMUX is still
			 * gathering responses from the lower streams to the
			 * ioctl. Set the cmlb_iocmsg to the message block
			 * pointer and the cmlb_iocresp to M_IOCACK to indicate
			 * positive acknowledgement of this ioctl by the 
			 * lower stream. 
			 */
			if (wsp->w_ioctlcnt) {
				linkp->cmlb_iocresp = M_IOCACK;
				linkp->cmlb_iocmsg = mp;

				DEBUG6(("cmux_mux_rsrv: ioctlcnt (0x%x) > 0 on chan 0x%x\n", wsp->w_ioctlcnt, lstrmp->lstrm_id));
				
				/*
				 * Decrement w_ioctlcnt and if the value is 0,
				 * then all lower streams have responded to
				 * this ioctl. Call cmux_io_iocresp() to
				 * tally the responses.
				 */
				if (--wsp->w_ioctlcnt == 0)
					cmux_do_iocresp(wsp);
				continue;
			}

			/*
			 * ioctlcnt == 0 means that this message
			 * comes after a M_COPYIN/M_COPYOUT. Send
			 * the message up to the next read queue.
			 */
			cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
			putnext(cmuxp->cmux_rqp, mp);
			cmux_clr_ioc(wsp);
			continue;
	   
		case M_IOCNAK:
			DEBUG6(("cmux_mux_rsrv: Found M_IOCNAK on queue.\n"));

			/*
			 * If w_ioctlcnt is non-zero, then CHANMUX is still
			 * gathering responses from the lower streams to the
			 * ioctl. Set the cmlb_iocmsg to the message block
			 * pointer and the cmlb_iocresp to M_IOCNAK to indicate
			 * negative acknowledgement of this ioctl by the 
			 * lower stream. 
			 */
			if (wsp->w_ioctlcnt) {
				linkp->cmlb_iocresp = M_IOCNAK;
				linkp->cmlb_iocmsg = mp;

				DEBUG6(("cmux_mux_rsrv: ioctlcnt (0x%x) > 0 on chan 0x%x\n", wsp->w_ioctlcnt, lstrmp->lstrm_id));
				/*
				 * Decrement w_ioctlcnt and if the value is 0,
				 * then all lower streams have responded to
				 * this ioctl. Call cmux_io_iocresp() to
				 * tally the responses.
				 */
				if (--wsp->w_ioctlcnt == 0)
					cmux_do_iocresp(wsp);
				continue;
			}

			/*
			 * ioctlcnt == 0 means that this message
			 * comes after a M_COPYIN/M_COPYOUT. Send
			 * the message up to the next read queue.
			 */
			cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
			putnext(cmuxp->cmux_rqp, mp);
			cmux_clr_ioc(wsp);
			continue;


		case M_COPYIN:
			DEBUG6(("cmux_mux_rsrv: Found M_COPYIN on queue.\n"));

			/*
			 * If w_ioctlcnt is non-zero, then CHANMUX is still
			 * gathering responses from the lower streams to the
			 * ioctl. Set the cmlb_iocmsg to the message block
			 * pointer and the cmlb_iocresp to M_IOCACK to indicate
			 * positive acknowledgement of this ioctl by the 
			 * lower stream. 
			 */
			if (wsp->w_ioctlcnt) {
				linkp->cmlb_iocresp = M_IOCACK;
				linkp->cmlb_iocmsg = mp;

				DEBUG6(("cmux_mux_rsrv: ioctlcnt (0x%x) > 0 on chan 0x%x\n", wsp->w_ioctlcnt, lstrmp->lstrm_id));
				/*
				 * Decrement w_ioctlcnt and if the value is 0,
				 * then all lower streams have responded to
				 * this ioctl. Call cmux_io_iocresp() to
				 * tally the responses.
				 */
				if (--wsp->w_ioctlcnt == 0)
					cmux_do_iocresp(wsp);
				continue;
			}

			/*
			 * ioctlcnt == 0 means that this message
			 * comes after a M_COPYIN/M_COPYOUT. Send
			 * the message up to the next read queue.
			 */
			cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
			putnext(cmuxp->cmux_rqp, mp);
			continue;

		case M_COPYOUT:
			DEBUG6(("cmux_mux_rsrv: Found M_COPYOUT on queue.\n"));

			/*
			 * If w_ioctlcnt is non-zero, then CHANMUX is still
			 * gathering responses from the lower streams to the
			 * ioctl. Set the cmlb_iocmsg to the message block
			 * pointer and the cmlb_iocresp to M_IOCACK to indicate
			 * positive acknowledgement of this ioctl by the 
			 * lower stream. 
			 */
			if (wsp->w_ioctlcnt) {
				linkp->cmlb_iocresp = M_IOCACK;
				linkp->cmlb_iocmsg = mp;

				DEBUG6(("cmux_mux_rsrv: ioctlcnt (0x%x) > 0 on chan 0x%x\n", wsp->w_ioctlcnt, lstrmp->lstrm_id));
				/*
				 * Decrement w_ioctlcnt and if the value is 0,
				 * then all lower streams have responded to
				 * this ioctl. Call cmux_io_iocresp() to
				 * tally the responses.
				 */
				if (--wsp->w_ioctlcnt == 0)
					cmux_do_iocresp(wsp);
				continue;
			}

			/*
			 * ioctlcnt == 0 means that this message
			 * comes after a M_COPYIN/M_COPYOUT. Send
			 * the message up to the next read queue.
			 */
			cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
			putnext(cmuxp->cmux_rqp, mp);
			continue;

		case M_PCPROTO:
		case M_PROTO:
			/*
			 * Close acknowledgement and switch channel processing. 
			 */
			if ((mp->b_wptr - mp->b_rptr) != sizeof(ch_proto_t)) {
				putnext(cmuxp->cmux_rqp, mp);
				continue;
			}

			/* LINTED pointer alignment */
			protop = (ch_proto_t *) mp->b_rptr;

			if (princflg && (protop->chp_type != CH_CTL ||
			    protop->chp_stype != CH_PRINC_STRM))
				putnext(cmuxp->cmux_rqp, mp);
			else if (princflg) {
				/*
				 * Potentially a command for us. 
				 */
				switch (protop->chp_stype_cmd) {
				case CH_CHANGE_CHAN:
					/*
					 * This message is sent by the principal
					 * stream to change the active channel
					 * to the number given by the 
					 * chp_stype_arg field of the ch_proto_t
					 * structure that is part of the 
					 * M_PROTO message. cmux_switch_chan()
					 * is called to perform the necessary
					 * updates of the workstation state
					 * structure.
					 */
					cmux_switch_chan(wsp, protop);
					freemsg(mp); 
					continue;

				case CH_OPEN_RESP:
					/*
					 * This message is sent by the 
					 * principal lower stream in response
					 * to the CH_CHANOPEN message. The
					 * cmlb_err of the principal steams
					 * cmux_link_t structure is set to the
					 * argument of the ch_proto_t structure.
					 */ 
					linkp->cmlb_err = protop->chp_stype_arg;
					freemsg(mp);

					/*
					 * If CMUX_PRINCSLEEP is set in the
					 * cmux_link_t structure flag, that
					 * means that open of this channel
					 * is waiting for the response, in
					 * which case wakeup is done.
					 */
					if (linkp->cmlb_flg & CMUX_PRINCSLEEP) {
						SV_SIGNAL(cmux_wopensv, 0);
						/* q will be enabled */
						return (0);
					}

					/*
					 * Only if we were not sleeping in open.
					 */
					continue;

				case CH_CLOSE_ACK:
					/*
					 * The CH_CLOSE_ACK is returned by the 
					 * principal stream in response to a
					 * CH_CHANCLOSE message. 
					 *
					 * cmux_close_chan() is called to reset
					 * the CMUX_WCLOSE flag in the cmux_t
					 * channel state structure and the 
					 * wakeup of the sleeping cmuxclose()
					 * routine is done.
					 */
					DEBUG5(("cmux_mux_rsrv: Found close_ack\n"));
					cmux_close_chan(wsp, lstrmp, protop);
					freemsg(mp);
					continue;

				default:
					putnext(cmuxp->cmux_rqp, mp);
					continue;
				} /* switch */
			} else {
				/*
				 * No CH_* protocol with lower streams.
				 */
				putnext(cmuxp->cmux_rqp, mp);
			}
			continue;	

		default: 
			putnext(cmuxp->cmux_rqp, mp);
			continue;

		} /* switch */

	} /* while */

	return (0);
}


/*
 * STATIC int
 * cmux_mux_rput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
cmux_mux_rput(queue_t *qp, mblk_t *mp)
{
	putq(qp, mp);
	return (0);
}


/*
 * STATIC int
 * cmux_mux_wput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
cmux_mux_wput(queue_t *qp, mblk_t *mp)
{
	/* should not be called */

	freemsg(mp);
#ifdef DEBUG
	cmn_err(CE_WARN, 
		"cmux_mux_wput: cmux_mux_wput called");
#endif
	return (0);
}


/*
 * void
 * cmux_wakeup(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
cmux_wakeup(void)
{
	SV_SIGNAL(cmux_bufcallsv, 0);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_cmux(void *, int)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Generic dump function that can be called (from kdb also)
 *	to print the values of various data structures that are
 *	part of the channel multiplexor. It takes two arguments:
 *	a data structure pointer whose fields are to be displayed
 *	and the data structure type. Following are the values of
 *	data structure type:
 *		1 - cmux_ws_t
 *		2 - cmux_link_t
 *		3 - cmux_t
 *		4 - cmux_lstrm_t
 *		5 - w_cmuxpp (list of cmux_t)
 */
void
print_cmux(void *structp, int type)
{
	switch (type) {
	case 1: {
		cmux_ws_t *wsp;

		/*
		 * cmux_ws_t structure
		 */

		wsp = (cmux_ws_t *) structp;
		debug_printf("\n cmux_ws_t struct: size=0x%x(%d)\n",
			sizeof(cmux_ws_t), sizeof(cmux_ws_t));
		debug_printf("\tw_ioctlchan=0x%x, \tw_ioctllstrm=0x%x\n",
			wsp->w_ioctlchan, wsp->w_ioctllstrm);
		debug_printf("\tw_ioctlcnt=0x%x,\tw_iocmsg=0x%x\n",
			wsp->w_ioctlcnt, wsp->w_iocmsg);
		debug_printf("\tw_state=0x%x, \tw_cmuxpp=0x%x\n",
			wsp->w_state, wsp->w_cmuxpp);
		debug_printf("\tw_numchan=0x%x, \tw_princp=0x%x\n",
			wsp->w_numchan, wsp->w_princp);
		debug_printf("\tw_lstrmsp=0x%x, \tw_numlstrms=0x%x\n",
			wsp->w_lstrmsp, wsp->w_numlstrms);
		debug_printf("\tw_lstrms=0x%x\n", wsp->w_lstrms);
		debug_printf("\tw_numswitch=0x%x, \tw_swtchtimes=0x%x\n",
			wsp->w_numswitch, wsp->w_swtchtimes);
		break;
	}

	case 2: {
		cmux_link_t *linkp;
		
		/*
		 * cmux_link_t structure
		 */

		linkp = (cmux_link_t *) structp;
		debug_printf("\n cmux_link_t struct: size=0x%x(%d)\n",
			sizeof(cmux_link_t), sizeof(cmux_link_t));
		debug_printf("\tcmlb_iocresp=0x%x, \tcmlb_flg=0x%x\n",
			linkp->cmlb_iocresp, linkp->cmlb_flg);
		debug_printf("\tcmlb_iocmsg=0x%x, \tcmlb_err=0x%x\n",
			linkp->cmlb_iocmsg, linkp->cmlb_err);
		debug_printf("\tcmlb_lblkp=0x%x\n",
			&linkp->cmlb_lblk);
		break;
	}

	case 3: {
		cmux_t *cmuxp;
		
		/*
		 * cmux_t structure
		 */

		cmuxp = (cmux_t *) structp;
		debug_printf("\n cmux_t struct: size=0x%x(%d)\n",
			sizeof(cmux_t), sizeof(cmux_t));
		debug_printf("\tcmux_num=0x%x, \tcmux_flg=0x%x\n",
			cmuxp->cmux_num, cmuxp->cmux_flg);
		debug_printf("\tcmux_rqp=0x%x, \tcmux_wqp=0x%x\n",
			cmuxp->cmux_rqp, cmuxp->cmux_wqp);
		debug_printf("\tcmux_wsp=0x%x,\n",
			cmuxp->cmux_wsp);
		debug_printf("\tcmux_dev=0x%x\n",
			cmuxp->cmux_dev);
		break;
	}

	case 4: {
		cmux_lstrm_t *lstrmp;

		/*
		 * cmux_lstrm_t structure
		 */

		lstrmp = (cmux_lstrm_t *) structp;
		debug_printf("\n cmux_lstrm_t struct: size=0x%x(%d)\n",
			sizeof(cmux_lstrm_t), sizeof(cmux_lstrm_t));
		debug_printf("\tlstrm_wsp=0x%x, \tlstrm_flg=0x%x\n",
			lstrmp->lstrm_wsp, lstrmp->lstrm_flg);
		debug_printf("\tlstrm_id=0x%x\n",
			lstrmp->lstrm_id);
		break;
	}

	case 5: {
		cmux_t **cmuxpp;
		cmux_t *cmuxp;
		
		/*
		 * list of cmux_t structure (w_cmuxpp)
		 */

		cmuxpp = (cmux_t **) structp;
		for (; cmuxp = *cmuxpp; cmuxpp++) {
			debug_printf("\n cmux_t struct: size=0x%x(%d)\n",
				sizeof(cmux_t), sizeof(cmux_t));
			debug_printf("\tcmux_num=0x%x, \tcmux_flg=0x%x\n",
				cmuxp->cmux_num, cmuxp->cmux_flg);
			debug_printf("\tcmux_rqp=0x%x, \tcmux_wqp=0x%x\n",
				cmuxp->cmux_rqp, cmuxp->cmux_wqp);
			debug_printf("\tcmux_wsp=0x%x,\n",
				cmuxp->cmux_wsp);
			debug_printf("\tcmux_dev=0x%x\n",
				cmuxp->cmux_dev);
		}
		break;
	}

        default:
		debug_printf("\nUsage (from kdb):\n");
		debug_printf("\t<struct ptr> <type (1-5)> print_cmux 2 call\n");
		break;

        } /* end switch */

}

#endif /* DEBUG || DEBUG_TOOLS */
