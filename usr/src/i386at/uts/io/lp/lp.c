#ident	"@(#)lp.c	1.4"

/*
 *      LP (Line Printer) Driver        EUC handling version
 */

/*
 *	MODIFICATIONS
 *
 *	L000	13nov97		brendank@sco.com	MR: ul97-06601
 *		- Some IEEE 1284 compliant printers don't work properly
 *		  if the SELECTIN signal from the parallel printer port is
 *		  asserted whilst printing.  A set of tunables, LP0SELECT
 *		  through to LP3SELECT have been introduced to specify
 *		  whether SELECTIN should be asserted or not whilst
 *		  printing to a given port.  These are stored in variable
 *		  lp_select in Space.c
*/

#ifdef _KERNEL_HEADERS

#include <util/param.h>
#include <util/types.h>
#include <proc/signal.h>
#include <io/dma.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <io/termio.h>
#include <io/termios.h>
#include <util/cmn_err.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/conf.h>
#include <io/strtty.h>
#include <util/debug.h>
#include <io/ldterm/eucioctl.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <util/mod/moddefs.h>
#include <io/mfpd/mfpd.h>
#include <io/mfpd/mfpdhw.h>
#include <io/lp/lp.h>

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
#include <util/ksynch.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

/* THESE MUST COME LAST */
#include <io/ddi.h>
#include <io/ddi_i386at.h>


#elif defined(_KERNEL)

#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/dma.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/termio.h>
#include <sys/termios.h>
#include <sys/cmn_err.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/conf.h>
#include <sys/strtty.h>
#include <sys/debug.h>
#include <sys/eucioctl.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/moddefs.h>
#include <sys/mfpd.h>
#include <sys/mfpdhw.h>
#include <sys/lp.h>

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
#include <sys/ksynch.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

/* THESE MUST COME LAST */
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif  /* _KERNEL_HEADERS */


#ifdef STATIC
#undef STATIC
#endif
#define STATIC


#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)

typedef	toid_t	LP_TMOUT;
typedef pl_t	LP_SPL;

#define	LP_LOCK1HEIRARCHY	1
#define	LP_LOCK1PRIORITY	plstr

#define	LP_SLEEPWAIT(lpp)	\
		( LOCK((lpp)->lp_lock, LP_LOCK1PRIORITY), \
		  !SV_WAIT_SIG((lpp)->lp_sleep, pritty, (lpp)->lp_lock) )

#define LP_SENDWAKE(lpp)	\
		SV_BROADCAST((lpp)->lp_sleep, 0)

#else

typedef	int	LP_TMOUT;
typedef int	LP_SPL;

#define	LP_SLEEPWAIT(lpp)	\
		(sleep((caddr_t) &((lpp)->lp_tty.t_oflag), TTIPRI | PCATCH))

#define LP_SENDWAKE(lpp)	\
		wakeup((caddr_t) &((lpp)->lp_tty.t_oflag))

#endif


#define DRVNAME         "lp - line printer driver"


/*
 * External functions
 */

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)

extern	toid_t	dtimeout(void (*)(), void *, long, pl_t, processorid_t);
extern	void	untimeout(toid_t);
extern	void	SV_BROADCAST(sv_t *, int);
extern	sv_t	*SV_ALLOC(int);
extern	void	SV_DEALLOC(sv_t *);

#else

extern	int	timeout(void (*)(), caddr_t, long);
extern	void	untimeout(int);
extern	int	sleep(caddr_t, int);
extern	void	wakeup(caddr_t);

/* ddicheck complains if these are not declared */
extern	void		outb(int, unsigned char);
extern	unsigned char	inb(int);

#endif



/*
 * Driver's global device information structures. One array element per
 * parallel port device.
 */
struct lpdev	lpcfg[LP_MAX_PORTS] =  {0};

/*
 * Driver global variables
 */
int	NUMLP;			/* Number of parallel ports in the system */
int	lpdevflag = D_NEW;	/* we are single-threaded */


/*
 * Driver functions
 */

int	lpstart(void);
int	lpshut(void);
void	lpintr(struct mfpd_rqst *);

STATIC	int	lpload(void);
STATIC	int	lpunload(void);
STATIC	int	lpopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC	int	lpclose(queue_t *, int, cred_t *);
STATIC	int	lpwput(queue_t *, mblk_t *);
STATIC	void	lpputioc(queue_t *, mblk_t *);
STATIC	void	lpsrvioc(queue_t *, mblk_t *);

STATIC	void	lpgetoblk(register struct lpdev *);
STATIC	void	lpflush(register struct lpdev *, int);
STATIC	void	lpdelay(register struct lpdev *);
STATIC	void	lptmout(int);
STATIC	void	lpproc(register struct lpdev *, int);
STATIC	void	lpxintr(struct lpdev *);

STATIC	void	lpcallback(struct mfpd_rqst *);
STATIC	void	lpcallback2(struct mfpd_rqst *);
STATIC	void	lpcallback3(struct mfpd_rqst *);
STATIC	void	lpwakeopen(struct mfpd_rqst *);
STATIC	void	lpsetoutmode(register struct lpdev *);
STATIC	void	lpoutputchar(register struct lpdev *, char);

STATIC int	lpchkstat(register struct lpdev *lpp);
STATIC void	lpsenderr(register struct lpdev *lpp);
STATIC void	lpsendclr(register struct lpdev *lpp);
STATIC void	lpdrivertimer(int);
STATIC LP_TMOUT lpsettimeout(void (*)(), void *, long);


/*
 * Driver's loadable-module interface structures
 */

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
MOD_ACDRV_WRAPPER(lp, lpload, lpunload, NULL, NULL, DRVNAME);
#else
MOD_DRV_WRAPPER(lp, lpload, lpunload, NULL, DRVNAME);
#endif


/*
 * Driver's STREAMS interface structures
 */

static struct module_info lp_info = {
	42,		/* module ID (also answers the ultimate question) */
	"lp",		/* module name */
	0,		/* Minimum packet size */
	INFPSZ,		/* Maximum packet size */
	256,		/* high water mark */
	128		/* low water mark */
};


static struct qinit lp_rint = {
	putq,		/* read-side put routine */
	NULL,		/* read-side service routine */
	lpopen,		/* driver open routine */
	lpclose,	/* driver close routine */
	NULL,		/* reserved */
	&lp_info,	/* module_info structure */
	NULL		/* module_stat structure */
};


static struct qinit lp_wint = {
	lpwput,		/* write-side put routine */
	NULL,		/* write-side service routine */
	lpopen,		/* driver open routine */
	lpclose,	/* driver close routine */
	NULL,		/* reserved */
	&lp_info,	/* module_info structure */
	NULL		/* module_stat structure */
};


struct streamtab lpinfo = {
	&lp_rint,
	&lp_wint,
	NULL,
	NULL
};



/*
 * lpload(void)
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC int
lpload(void)
{
	return(lpstart());
}


/*
 * lpunload(void)
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC int
lpunload(void)
{
	return(lpshut());
}


/*
 * lpstart()
 *
 * Calling/Exit State:
 *
 * Description 
 * 	Start routine for the driver.
 *	Gets the mfpd port information and fills in the lpcfg array.
 */

int
lpstart(void)
{
	register struct lpdev *lpp;
	int	i;
	int	x;
	ulong	addr;
	static	LKINFO_DECL(lplock1, "lp driver spin lock", 0);


	if ((NUMLP = mfpd_get_port_count()) > LP_MAX_PORTS) {
		NUMLP = LP_MAX_PORTS;
	}

	/* One printer per port is assumed */

	for (lpp = lpcfg, i = 0; i < NUMLP; i++, lpp++) {

		bzero((caddr_t)lpp, sizeof(struct lpdev));

		if ((x = mfpd_get_port_type(i)) != MFPD_PORT_ABSENT) {
			lpp->lp_flag = LP_PRESENT;
		}

		(void)mfpd_query(i, MFPD_DEVICE_ADDR, MFPD_PRINTER, &addr);
		addr &= 0x01;
		if (!(addr)) {
			lpp->lp_flag = 0;
		}

		lpp->lp_porttype = x;
		lpp->lp_datareg  = mfpd_get_baseaddr(i);
		lpp->lp_portnum  = i;

		/* set the error-checking control to its defaults */
		lpp->lp_errchk = LP_OFF;
		lpp->lp_errflush = LP_OFF;

		switch (lpp->lp_porttype) {

		case MFPD_SIMPLE_PP:
		case MFPD_PS_2:
		case MFPD_PC87322:
		case MFPD_SL82360:
		case MFPD_AIP82091:
		case MFPD_SMCFDC665:
		case MFPD_COMPAQ:
			lpp->lp_statreg = lpp->lp_datareg + STD_STATUS;
			lpp->lp_cntlreg = lpp->lp_datareg + STD_CONTROL;
			break;

		case MFPD_PORT_ABSENT:
		default:
			break;
		}

		lpp->lp_ltime = LP_LONGTIME;

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
		if ((lpp->lp_sleep = SV_ALLOC(KM_NOSLEEP)) == NULL) {
			/*
			 * If anything fails, deallocate all previous ones
			 * and fail the module load.
			 */
			for (--lpp; lpp >= lpcfg; --lpp) {
				if (lpp->lp_sleep) {
					SV_DEALLOC(lpp->lp_sleep);
				}
			}
			return ENOMEM;
		}

		if ((lpp->lp_lock = LOCK_ALLOC(LP_LOCK1HEIRARCHY,
			LP_LOCK1PRIORITY, &lplock1, KM_NOSLEEP)) == NULL) {
			/*
			 * If anything fails, deallocate all previous ones
			 * and fail the module load.
			 */
			if (lpp->lp_sleep) SV_DEALLOC(lpp->lp_sleep);
			for (--lpp; lpp >= lpcfg; --lpp) {
				if (lpp->lp_sleep) SV_DEALLOC(lpp->lp_sleep);
				if (lpp->lp_lock) LOCK_DEALLOC(lpp->lp_lock);
			}
			return ENOMEM;
		}
#endif
	}
	return 0;
}


/*
 * lpshut();
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Shutdown routine for the driver.
 */
int
lpshut(void)
{
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	int	i;

	for (i = 0; i < NUMLP; i++) {
		if (lpcfg[i].lp_sleep) SV_DEALLOC(lpcfg[i].lp_sleep);
		if (lpcfg[i].lp_lock) LOCK_DEALLOC(lpcfg[i].lp_lock);
	}
#endif
	return(0);
}


/*
 * lpopen()
 *
 * Calling/Exit State:
 *
 * Description:
 */
/* ARGSUSED */
STATIC int
lpopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{
	register struct	lpdev	*lpp;
	register struct	strtty	*tp;
	mblk_t		*mop;
	minor_t		dev;
	unsigned char	status;
	LP_SPL		oldspl;
	clock_t		lp_delay = drv_usectohz(100000);	/* 100 msec */
	unsigned long	lp_cnt   = 100;				/* 10 sec max */

#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "IN LPOPEN");
#endif

	/* Check if device number is valid */
	if ((dev = getminor(*devp)) >= NUMLP) {
		return(ENXIO);
	}

	lpp = lpcfg + dev;

	oldspl = splstr();

	/* Was device detected ?? */
	if ((lpp->lp_flag & LP_PRESENT) == 0) {
		splx(oldspl);
		return(ENXIO);
	}

	/* Exclusive usage */
	if (lpp->lp_flag & LP_OPEN) {
		splx(oldspl);
		return(EBUSY);
	}

	/* Set the status as opened. (Unset below if error encountered.) */
	lpp->lp_flag |= LP_OPEN;
	splx(oldspl);

	/* 
	 * Fill in mfpd request block
	 */
	lpp->lp_req.port = lpp->lp_portnum;
	lpp->lp_req.intr_cb = NULL;
	lpp->lp_req.drv_cb = (void (*)(struct mfpd_rqst *))lpwakeopen;

	if (mfpd_request_port(&lpp->lp_req) == ACCESS_REFUSED) {

		/* Return value won't be -1 */
		lpp->lp_portreq = 1;
		lpp->lp_portcntl = 0;

		/* Sleep till allocated the port */
		if (LP_SLEEPWAIT(lpp)) {
			/* Sleep was interrupted */
#ifdef LP_DEBUG
			cmn_err(CE_NOTE, "SLEEP INTERRUPTED IN LPOPEN");
#endif
			/* Cancel the port request */
			(void)mfpd_relinquish_port(&lpp->lp_req);
			lpp->lp_portreq = 0;

			/* Open failed */
			oldspl = splstr();
			lpp->lp_flag &= ~LP_OPEN;
			splx(oldspl);

			return EINTR;
		}
#ifdef LP_DEBUG
		cmn_err(CE_NOTE, "WOKEN UP IN LPOPEN");
#endif
	}

	/* 
	 * Access granted, check printer status. Return EIO if printer
	 * status is not LP_OK (LP_OK indicates printer is connected, powered
	 * up, and ready.) A very slow printer may still be in the act of
	 * performing a reset (especially if the driver was auto-loaded), so
	 * allow a delay of up to 10 seconds to wait for this.
	 */

	lpp->lp_portreq = 0;
	lpp->lp_portcntl = 1;

	/* Put the port in the output mode */
	lpsetoutmode(lpp);

	/* Supplemental read of status port for reliability */
	inb(lpp->lp_statreg);

	while (!LP_IS_READY(status = inb(lpp->lp_statreg)) && lp_cnt) {
		delay(lp_delay);
		lp_cnt--;
	}

	/* Relinquish the port */
	(void)mfpd_relinquish_port(&lpp->lp_req);
	lpp->lp_portcntl = 0;

	/* Fail the open if the status is not LP_OK */
	if (!LP_IS_READY(status)) {
		oldspl = splstr();
		lpp->lp_flag &= ~LP_OPEN;
		splx(oldspl);
		return(EIO);
	}

	/* Save the lpp in the queue_t struct for this stream */
	WR(q)->q_ptr = q->q_ptr = (caddr_t) lpp;

	/* Save the read queue for sending M_ERROR messages */
	lpp->lp_errq = q;

	/* Set up the strtty struct for this device */
	tp = &lpp->lp_tty;
	tp->t_rdqp = q;
	tp->t_dev  = dev;

	if (mop = allocb(sizeof(struct stroptions), BPRI_MED)) {
		register struct stroptions *sop;

		mop->b_datap->db_type = M_SETOPTS;
		mop->b_wptr += sizeof(struct stroptions);
		/* LINTED pointer alignment */
		sop = (struct stroptions *)mop->b_rptr;
		sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
		sop->so_hiwat = 512;
		sop->so_lowat = 256;
		(void)putnext(q, mop);
	} else {
		return(EAGAIN);
	}

	if ((tp->t_state & (ISOPEN | WOPEN)) == 0) {
		tp->t_iflag = IGNPAR;
		tp->t_cflag = B300 | CS8 | CLOCAL;
		lpdrivertimer(LP_TIMERON);
	}

	oldspl = splstr();

	tp->t_state |=  CARR_ON;

	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;

	splx(oldspl);

	lpp->lp_count = 0;
	return(0);
}



/*
 * lpclose()
 *
 * Calling/Exit State:
 *
 * Description:
 */
/* ARGSUSED */
STATIC int
lpclose(queue_t *q, int flag, cred_t *crp)
{
	register struct lpdev	*lpp = q->q_ptr;
	register struct strtty	*tp  = &(lpp->lp_tty);
	LP_SPL	oldspl;

#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "IN LPCLOSE");
#endif

	if ((lpp < lpcfg) || (lpp >= lpcfg + NUMLP)) {
		return(0);
	}

	if (!(tp->t_state & ISOPEN)) {
		return(0);
	}

	if (!(flag & (FNDELAY | FNONBLOCK))) {
		/* Drain queued output to the printer. */
		oldspl = spltty();
		while ((tp->t_state & CARR_ON)) {
			if ((tp->t_out.bu_bp == 0) && (WR(q)->q_first == NULL))
				break;
			tp->t_state |= TTIOW;
			if (LP_SLEEPWAIT(lpp)) {
				tp->t_state &= ~(TTIOW | CARR_ON);
				break;
			}
		}
		splx(oldspl);
	}

	/*
	 *  do not >>> outb(lpp->lp_cntlreg, 0) -- because 
	 * close() gets called before all characters are sent, therefore, 
	 * the last chars do not get output with the interrupt turned off
	 */

	lpdrivertimer(LP_TIMEROFF);

	(void)mfpd_relinquish_port(&lpp->lp_req);
	lpp->lp_portcntl = 0;

	oldspl = spltty();

	lpp->lp_flag &= ~LP_OPEN;
	tp->t_state  &= ~(ISOPEN | CARR_ON);

	tp->t_rdqp = NULL;
	q->q_ptr   = WR(q)->q_ptr = NULL;

	/* error detection gets shut off automatically on close */
	lpp->lp_errchk   = LP_OFF;
	lpp->lp_errflush = LP_OFF;

	splx(oldspl);

	return (1);
}


/*
 * lpwput()
 *
 * Calling/Exit State:
 *
 * Description: lp driver write-side put routine
 */

STATIC int
lpwput(queue_t *q, mblk_t *bp)
{
	register struct lpdev	*lpp = q->q_ptr;
	register struct strtty	*tp  = &(lpp->lp_tty);
	register struct msgb	*bp1;
	register struct msgb	*bp2;
	int	retval;
	LP_SPL	oldspl;

#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "IN lpwput");
#endif

	switch (bp->b_datap->db_type) {

	case M_DATA:
		oldspl = spltty();
		/*
		 * Go thru all msg blocks on this message and queue up
		 * all those that have some data on them.
		 */
		while (bp) {
			bp->b_datap->db_type = M_DATA;
			bp1 = unlinkb(bp);
			bp->b_cont = NULL;
			if (bp->b_wptr <= bp->b_rptr) {
				freeb(bp);
			} else {
				(void)putq(q, bp);
			}
			bp = bp1;
		}
		splx(oldspl);

		/* If anything on the queue, call lpgetoblk to process it. */
		if (q->q_first != NULL) {
			lpgetoblk(lpp);
		}
		break;

	case M_IOCTL:
		lpputioc(q, bp);	/* only entry point for lpputioc() */
		if (q->q_first != NULL) {
			lpgetoblk(lpp);
		}
		break;

	case M_DELAY:
		oldspl = spltty();
		if ((tp->t_state & TIMEOUT) || (q->q_first != NULL)
		     || (tp->t_out.bu_bp != NULL)) {
			(void)putq(q, bp);
			splx(oldspl);
			break;
		}
		tp->t_state |= TIMEOUT;
		lpsettimeout(lpdelay, lpp, ((long)*(bp->b_rptr)) * HZ / 60);
		splx (oldspl);
		freemsg (bp);
		if (lpp->lp_portcntl) {
			(void)mfpd_relinquish_port(&lpp->lp_req);
			lpp->lp_portcntl = 0;
			lpp->lp_count = 0;
		}
		break;

	case M_FLUSH:
		oldspl = splstr();

#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_CONT,"lpwput: got M_FLUSH ");
#endif

		switch (*(bp->b_rptr)) {
		case FLUSHRW:
#ifdef LP_DEBUG2
			/* 
			 *+ debug2
			 */
			cmn_err(CE_CONT,"with FLUSHRW\n");
#endif

			lpflush(lpp, (FREAD | FWRITE));
			*(bp->b_rptr) = FLUSHR;
			qreply(q, bp);
			break;

		case FLUSHR:
#ifdef LP_DEBUG2
			/* 
			 *+ debug2
			 */
			cmn_err(CE_CONT,"with FLUSHR\n");
#endif

			lpflush(lpp, FREAD);
			qreply(q, bp);
			break;

		case FLUSHW:
#ifdef LP_DEBUG2
			/* 
			 *+ debug2
			 */
			cmn_err(CE_CONT,"with FLUSHW\n");
#endif

			lpflush(lpp, FWRITE);
			freemsg(bp);
			break;

		default:
#ifdef LP_DEBUG2
			/* 
			 *+ debug2
			 */
			cmn_err(CE_CONT,"with INVALID flags\n");
#endif

			break;
		}
		splx(oldspl);
		break;

	case M_START:
		freemsg(bp);
		if (!(tp->t_state & TTSTOP)) {
			break;
		}

		oldspl = splstr();
		tp->t_state &= ~TTSTOP;
		splx(oldspl);

		if ((q->q_first == NULL) && (tp->t_out.bu_bp == NULL)) {
			break;
		}

		if (lpp->lp_portreq) {
			break;
		}

		if (lpp->lp_portcntl) {
			bp2 = tp->t_out.bu_bp;
			lpp->lp_count += (bp2->b_wptr - bp2->b_rptr);
			lpsetoutmode(lpp);
			oldspl = splstr();
			lpproc(lpp, T_OUTPUT);
			splx(oldspl);
			break;
		}

		lpp->lp_req.port = lpp->lp_portnum;
		lpp->lp_req.intr_cb = (void (*)(struct mfpd_rqst *))lpintr;
		lpp->lp_req.drv_cb = (void (*)(struct mfpd_rqst *))lpcallback2;
		retval = mfpd_request_port(&lpp->lp_req);
		/* retval won't be -1 */
		if (retval == ACCESS_REFUSED) {
			lpp->lp_portreq = 1;
			bp2 = tp->t_out.bu_bp;
			lpp->lp_count += (bp2->b_wptr - bp2->b_rptr);
			break;
		} else {
			lpp->lp_portcntl = 1;
			bp2 = tp->t_out.bu_bp;
			lpp->lp_count += (bp2->b_wptr - bp2->b_rptr);
			lpsetoutmode(lpp);
			oldspl = splstr();
			lpproc(lpp, T_OUTPUT);
			splx(oldspl);
		}
		break;

	case M_STOP:
		freemsg(bp);
		if (tp->t_state & TTSTOP)
			break;
		oldspl = splstr();
		tp->t_state |= TTSTOP;
		lpp->lp_count = 0;
		splx(oldspl);
		break;

	default:
		freemsg(bp);
		break;
	}
	return (0) ;
}


/*
 * lpgetoblk()
 *
 * Calling/Exit State:
 *
 * Description: Retrieves msgs from the driver's queue & processes them
 */
STATIC void
lpgetoblk(register struct lpdev *lpp)
{
	register struct strtty *tp = &(lpp->lp_tty);
	register struct queue *q;
	register struct msgb *bp;
	int	retval;
	LP_SPL	oldspl;


#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "in lpgetoblk");
#endif
	if (tp->t_rdqp == NULL) {
		return;
	}
	q = WR(tp->t_rdqp);

	oldspl = splstr();

	while (!(tp->t_state & BUSY) && (bp = getq(q)) != NULL) {

		switch (bp->b_datap->db_type) {

		case M_DATA:
			if (tp->t_state & (TTSTOP | TIMEOUT)) {
				(void)putbq(q, bp);
				splx(oldspl);
				if (lpp->lp_portcntl) {
					(void)mfpd_relinquish_port(
					    &lpp->lp_req);
					lpp->lp_portcntl = 0;
					lpp->lp_count = 0;
				}
				return;
			}

			if (lpp->lp_portreq) {
				(void)putbq(q, bp);
				splx(oldspl);
				return;
			}

			if (!lpp->lp_portcntl) {
				lpp->lp_req.port = lpp->lp_portnum;
				lpp->lp_req.intr_cb = 
				    (void (*)(struct mfpd_rqst *))lpintr;
				lpp->lp_req.drv_cb = 
				    (void (*)(struct mfpd_rqst *))lpcallback;
				retval = mfpd_request_port(&lpp->lp_req);
				/* retval won't be -1 */
				if (retval == ACCESS_REFUSED) {
					lpp->lp_portreq = 1;
					(void)putbq(q, bp);
					splx(oldspl);
					return;
				} else {
					lpp->lp_portcntl = 1;
					tp->t_out.bu_bp = bp;
					lpp->lp_count += 
					    bp->b_wptr - bp->b_rptr;
					lpsetoutmode(lpp);
					lpproc(lpp, T_OUTPUT);
				}
			} else {
				tp->t_out.bu_bp = bp;
				lpp->lp_count += 
				    (bp->b_wptr - bp->b_rptr);
				lpsetoutmode(lpp);
				lpproc(lpp, T_OUTPUT);
			}
			break;

		case M_DELAY:
			if (tp->t_state & TIMEOUT) {
				(void)putbq(q, bp);
				splx(oldspl);
				if (lpp->lp_portcntl) {
					(void)mfpd_relinquish_port(
					    &lpp->lp_req);
					lpp->lp_portcntl = 0;
					lpp->lp_count = 0;
				}
				return;
			}
			tp->t_state |= TIMEOUT;

			lpsettimeout(lpdelay, lpp,
				((long)*(bp->b_rptr)) * HZ / 60);

			freemsg (bp);
			if (lpp->lp_portcntl) {
				(void)mfpd_relinquish_port(&lpp->lp_req);
				lpp->lp_portcntl = 0;
				lpp->lp_count = 0;
			}
			break;

		case M_IOCTL:
			lpsrvioc(q, bp);
			break;

		default:
			freemsg(bp);
			break;
		}
	}
	/* Wakeup any process sleeping waiting for drain to complete */
	if ((tp->t_out.bu_bp == 0) && (tp->t_state & TTIOW)) {
		tp->t_state &= ~(TTIOW);
		LP_SENDWAKE(lpp);
	}
	splx(oldspl);
}


/*
 * lpputioc()
 *
 * Calling/Exit State:
 *
 * Description: processes M_IOCTL messages from driver's wput routine (lpwput)
 */
STATIC void
lpputioc(queue_t *q, mblk_t *bp)
{
	register struct lpdev	*lpp = q->q_ptr;
	register struct strtty	*tp  = &(lpp->lp_tty);
	struct	iocblk	*iocbp;
	mblk_t	*bp1;
	LP_SPL	oldspl;

	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_rptr;

#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "!IN LPPUTIOC ioctl=%x", iocbp->ioc_cmd);
#endif

	switch (iocbp->ioc_cmd) {

	case TCSBRK:
	case TCSETAW:
	case TCSETSW:
	case TCSETSF:
	case TCSETAF:/* run these now, if possible */
		if (q->q_first != NULL || (tp->t_state & (BUSY | TIMEOUT))
		     || (tp->t_out.bu_bp != NULL)) {
			(void)putq(q, bp);
			break;
		}
		lpsrvioc(q, bp);
		break;

	case TCSETS:
	case TCSETA:    /* immediate parm set   */

		if (tp->t_state & BUSY) {
			(void)putbq(q, bp); /* queue these for later */
			break;
		}

		lpsrvioc(q, bp);
		break;

	case TCGETS:
	case TCGETA:    /* immediate parm retrieve */

		lpsrvioc(q, bp);
		break;

	case EUC_MSAVE:
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case LP_ERRCHK_ON:	/* enable driver's error checking */
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_CONT,"lpputioc: LP_ERRCHK_ON\n");
#endif

		if (bp->b_cont)
			freemsg(bp->b_cont);
		bp->b_cont = NULL;

		oldspl = splstr();
		lpp->lp_errchk = LP_ON;
		splx(oldspl);

		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case LP_ERRCHK_OFF:	/* disable driver's error checking */
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_CONT,"lpputioc: LP_ERRCHK_OFF\n");
#endif

		if (bp->b_cont)
			freemsg(bp->b_cont);
		bp->b_cont = NULL;

		oldspl = splstr();
		lpp->lp_errchk = LP_OFF;
		splx(oldspl);

		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case LP_ERRFLUSH_ON:	/* enable flushing while device has error */
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_CONT,"lpputioc: LP_ERRFLUSH_ON\n");
#endif

		if (bp->b_cont)
			freemsg(bp->b_cont);
		bp->b_cont = NULL;

		oldspl = splstr();
		lpp->lp_errflush = LP_ON;
		splx(oldspl);

		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case LP_ERRFLUSH_OFF:	/* disable flushing while device has error */
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_CONT,"lpputioc: LP_ERRFLUSH_OFF\n");
#endif

		if (bp->b_cont)
			freemsg(bp->b_cont);
		bp->b_cont = NULL;

		oldspl = splstr();
		lpp->lp_errflush = LP_OFF;
		splx(oldspl);

		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	default:
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_WARN,"lpputioc: UNKNOWN ioctl!\n");
#endif

		if ((iocbp->ioc_cmd & IOCTYPE) == LDIOC) {
			bp->b_datap->db_type = M_IOCACK; /* ignore LDIOC cmds */
			bp1 = unlinkb(bp);
			if (bp1) {
				freeb(bp1);
			}
			iocbp->ioc_count = 0;
		} else {
			/*
			 * Unknown IOCTLs aren't errors, they just may
			 * have been intended for an
			 * upper module that isn't present.  NAK them...
			 */
			iocbp->ioc_error = EINVAL;
			iocbp->ioc_rval = (-1);
			bp->b_datap->db_type = M_IOCNAK;
		}
		qreply(q, bp);
		break;
	}
}


/*
 * lpsrvioc()
 *
 * Calling/Exit State:
 *
 * Description: processes enqueued M_IOCTL messages
 */
STATIC void
lpsrvioc(queue_t *q, mblk_t *bp)
{
	register struct lpdev	*lpp = q->q_ptr;
	register struct strtty	*tp  = &(lpp->lp_tty);
	struct	iocblk	*iocbp;
	struct	termio	*cb;
	struct	termios	*scb;
	mblk_t	*bpr;
	mblk_t	*bp1;

	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_rptr;

#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "!IN LPSRVIOC ioctl=%x",  iocbp->ioc_cmd);
#endif

	switch (iocbp->ioc_cmd) {

	case TCSETSF: /* The output has drained now. */
		lpflush(lpp, FREAD); /* fall thru .. */

		/* (couldn't get block before...) */

		/* FALLTHRU */
	case TCSETS:
	case TCSETSW:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			qreply(q, bp);
			break;
		}

		/* LINTED pointer alignment */
		scb = (struct termios *)bp->b_cont->b_rptr;
		tp->t_cflag = scb->c_cflag;
		tp->t_iflag = scb->c_iflag;
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1) {
			freeb(bp1);
		}
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case TCSETAF: /* The output has drained now. */
		lpflush(lpp, FREAD); /* fall thru .. */

		/* FALLTHRU */
	case TCSETA:
	case TCSETAW:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			qreply(q, bp);
			break;
		}

		/* LINTED pointer alignment */
		cb = (struct termio *)bp->b_cont->b_rptr;
		tp->t_cflag = cb->c_cflag;
		tp->t_iflag = cb->c_iflag;
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1) {
			freeb(bp1);
		}
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case TCGETS:    /* immediate parm retrieve */
		if (bp->b_cont)
			freemsg(bp->b_cont);
		bp->b_cont = NULL;

		if ((bpr = allocb(sizeof(struct termios), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void)putbq(q, bp);
			(void)bufcall((ushort)sizeof(struct termios), BPRI_MED,
			    lpgetoblk, (long)tp);
			break;
		}
		bp->b_cont = bpr;

		/* LINTED pointer alignment */
		scb = (struct termios *)bp->b_cont->b_rptr;

		scb->c_iflag = tp->t_iflag;
		scb->c_cflag = tp->t_cflag;

		bp->b_cont->b_wptr += sizeof(struct termios);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termios);
		qreply(q, bp);
		break;

	case TCGETA:    /* immediate parm retrieve */
		if (bp->b_cont) {
			/* freemsg(bp); this was a bug in the previous ver. */
			freemsg(bp->b_cont); /* bad user supplied parameter */
		}
		bp->b_cont = NULL;

		if ((bpr = allocb(sizeof(struct termio), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void)putbq(q, bp);
			(void)bufcall((ushort)sizeof(struct termio), BPRI_MED,
			    lpgetoblk, (long)tp);
			break;
		}
		bp->b_cont = bpr;
		/* LINTED pointer alignment */
		cb = (struct termio *)bp->b_cont->b_rptr;

		cb->c_iflag = tp->t_iflag;
		cb->c_cflag = tp->t_cflag;

		bp->b_cont->b_wptr += sizeof(struct termio);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termio);
		qreply(q, bp);
		break;

	case TCSBRK:
		/* Skip the break since it's a parallel port. */
		/*
		   arg = *(int *)bp->b_cont->b_rptr;
		*/
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1) {
			freeb(bp1);
		}
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case EUC_MSAVE: /* put these here just in case... */
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	default: /* unexpected ioctl type */
		if (canput(RD(q)->q_next) == 1) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			qreply(q, bp);
		} else {
			(void)putbq(q, bp);
		}
		break;
	}
}


/*
 * lpflush()
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC void
lpflush(register struct lpdev *lpp, int cmd)
{
	register struct strtty	*tp = &(lpp->lp_tty);
	queue_t *q;
	LP_SPL	oldspl;


	oldspl = splstr();

#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "IN LPFLUSH");
#endif

	/*
	 * Skips flush if error exists on the device and if flush handling
	 * during errors is disabled.
	 */

	if ((cmd & FWRITE) && ((lpp->lp_errflush == LP_ON) || !lpp->lp_wrerr)) {
		q = WR(tp->t_rdqp);
		/* Discard all messages on the output queue. */
		flushq(q, FLUSHDATA);
		tp->t_state &= ~(BUSY);
		tp->t_state &= ~(TBLOCK);
		if (tp->t_state & TTIOW) {
			tp->t_state &= ~(TTIOW);
			LP_SENDWAKE(lpp);
		}
	}

	if ((cmd & FREAD) && ((lpp->lp_errflush == LP_ON) || !lpp->lp_rderr)) {
		tp->t_state &= ~(BUSY);
	}

	splx(oldspl);

	lpgetoblk(lpp);
}


/*
 * lpintr()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	lpintr is the entry point for all interrupts. 
 */

void	
lpintr(struct mfpd_rqst *prqst)
{
	register struct lpdev *lpp = lpcfg + prqst->port;

	ASSERT(prqst->port < NUMLP);

	if ((lpp->lp_porttype == MFPD_PORT_ABSENT) || !(lpp->lp_wantintr)) {
		return;
	}

	lpp->lp_wantintr = 0;

	if (lpchkstat(lpp) == 0) {
		lpxintr(lpp);
	}
}


/*
 * lpxintr()
 *
 * Calling/Exit State:
 *
 * Description: This is logically a part of lpintr.  This code handles
 * transmit buffer empty interrupts and works in conjunction with
 * lptmout() to insure that lost interrupts don't hang  the driver.
 * If a char is xmitted and we go more than 2s (LP_MAXTIME) without an
 * interrupt, lptmout will supply the 'interrupt'.
 */
STATIC void
lpxintr(register struct lpdev *lpp)
{
	register struct	strtty	*tp = &(lpp->lp_tty);

#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "in lpxintr");
#endif

	lpp->lp_ltime = LP_LONGTIME;  /* don't time out */

	if (tp->t_state & BUSY) {
		tp->t_state &= ~BUSY;
		lpproc(lpp, T_OUTPUT);

		/* if output didn't start get a new message */
		if (!(tp->t_state & BUSY)) {
			if (tp->t_out.bu_bp) {
				freemsg(tp->t_out.bu_bp);
				/* 
				 * if there are no more blocks in the Q,
				 * then relinquish the port control.
				 */
				if (tp->t_rdqp != NULL) {

					register struct queue *q;

					q = WR(tp->t_rdqp);
					if ((q->q_first == NULL) && 
					    (lpp->lp_portcntl)) {
						/*
						 * Q empty but lp driver
						 * has the control of the port.
						 */
						(void)mfpd_relinquish_port(
						    &lpp->lp_req);
						lpp->lp_portcntl = 0;
						lpp->lp_count = 0;
					}
				}

				if ((lpp->lp_count >= LP_BOUND) && 
				    (lpp->lp_portcntl)) {
					(void)mfpd_relinquish_port(
					    &lpp->lp_req);
					lpp->lp_portcntl = 0;
					lpp->lp_count = 0;
				}
			}
			tp->t_out.bu_bp = 0;
			lpgetoblk(lpp);
		}
	}
}


/*
 * lpproc()
 *
 * Calling/Exit State:
 *
 * Description:
 *      General command routine that performs device specific operations for
 * generic i/o commands.  All commands are performed with tty level interrupts
 * disabled.
 */

STATIC void
lpproc(register struct lpdev *lpp, int cmd)
{
	register struct	strtty	*tp = &(lpp->lp_tty);
	register struct	msgb	*bp;
	LP_SPL	oldspl;


	oldspl = splstr();

#ifdef LP_DEBUG
	/* 
	 *+ debug
	 */
	cmn_err(CE_NOTE, "in lpproc cmd=%x",  cmd);
#endif

	switch (cmd) {

	case T_TIME:            /* stop sending a break -- disabled for LP */
		goto start;

	case T_OUTPUT:          /* do some output */
start:
		/* If we are busy, do nothing */
		if (tp->t_state & (TTSTOP | TIMEOUT)) {
			if (lpp->lp_portcntl) {
				(void)mfpd_relinquish_port(&lpp->lp_req);
				lpp->lp_portcntl = 0;
				lpp->lp_count = 0;
			}
			break;
		}
		if (tp->t_state &  BUSY) 
			break;

		/*
		 * Check for characters ready to be output.
		 * If there are any, ship one out.
                 */
		bp = tp->t_out.bu_bp;
		if (bp == NULL || bp->b_wptr <= bp->b_rptr) {
			if (tp->t_out.bu_bp) {
				freemsg(tp->t_out.bu_bp);
				/* 
				 * if there are no more blocks in the Q,
				 * then relinquish the port control.
				 */
				if (tp->t_rdqp != NULL) {

					register struct queue *q;

					q = WR(tp->t_rdqp);
					if ((q->q_first == NULL) && 
					    (lpp->lp_portcntl)) {
						/*
						 * Q empty but lp driver
						 * has the control of the port.
						 */
						(void)mfpd_relinquish_port(
						    &lpp->lp_req);
						lpp->lp_portcntl = 0;
						lpp->lp_count = 0;
					}
				}

				if ((lpp->lp_count >= LP_BOUND) && 
				    (lpp->lp_portcntl)) {
					(void)mfpd_relinquish_port(
					    &lpp->lp_req);
					lpp->lp_portcntl = 0;
					lpp->lp_count = 0;
				}
			}
			tp->t_out.bu_bp = 0;
			lpgetoblk(lpp);
			break;
		}

		/* specify busy, and output a char */
		tp->t_state |= BUSY;
		/* reset the time so we can catch a missed interrupt */
		lpoutputchar(lpp, *bp->b_rptr++);
		(void)drv_getparm(TIME, (ulong *) & lpp->lp_ltime);
		break;

	case T_BREAK:           /* send a break -- disabled for LP    */
		break;
	}
	splx(oldspl);
}


int dbg_lptmout = 0;

/*
 * lptmout()
 *
 * Calling/Exit State:
 *
 * Description:
 * Watchdog timer handler.
 */
/* ARGSUSED */
STATIC void
lptmout(int notused)
{
	register struct	lpdev	*lpp;
	struct	lpdev	*elpp;
	struct	msgb	*mp;
	time_t	lptime;
	time_t	diff;
	int	retval;
	LP_SPL	oldspl;


	for (lpp = lpcfg, elpp = lpcfg + NUMLP; lpp < elpp; lpp++) {

		(void)drv_getparm(TIME, (ulong *) &lptime);

		if (!((diff = lptime - lpp->lp_ltime) > LP_MAXTIME && 
		    diff <= LP_MAXTIME + 2))  {
			continue;
		}

		if (lpp->lp_porttype == MFPD_PORT_ABSENT) {
			continue;
		}

		if (lpchkstat(lpp) == 0) {
			oldspl = splstr();
			lpp->lp_wantintr = 0;
			lpxintr(lpp);
			splx(oldspl);
			continue;
		}

		if (lpp->lp_portcntl == 0) {
			(void)drv_getparm(TIME, &lpp->lp_ltime);
			continue;
		}

		(void)mfpd_relinquish_port(&lpp->lp_req);
		lpp->lp_portcntl = 0;
		lpp->lp_count = 0;

		lpp->lp_req.port    = lpp->lp_portnum;
		lpp->lp_req.intr_cb = (void (*)(struct mfpd_rqst *))lpintr;
		lpp->lp_req.drv_cb  = (void (*)(struct mfpd_rqst *))lpcallback3;
		retval = mfpd_request_port(&lpp->lp_req);

		/* retval won't be -1 */
		if (retval == ACCESS_REFUSED) {
			lpp->lp_portreq = 1;
			mp = lpp->lp_tty.t_out.bu_bp;
			lpp->lp_count += (mp->b_wptr - mp->b_rptr);
			lpp->lp_ltime = LP_LONGTIME;
		} else {
			lpp->lp_portcntl = 1;
			mp = lpp->lp_tty.t_out.bu_bp;
			lpp->lp_count += (mp->b_wptr - mp->b_rptr);
			(void)drv_getparm(TIME, (ulong *) &lpp->lp_ltime);
		}
	}

#if (MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20)
	/*
	 * Reschedule the timeout routine so that it gets called again at
	 * the next timing interval. Note that this is not needed for the
	 * later release since that version uses TO_PERIODIC with dtimeout
	 * to cause this routine to be recheduled automatically.
	 */
	timeflag = timeout(lptmout, (caddr_t)NULL, drv_usectohz(1000000));
#endif
}


/*
 * lpdelay()
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC void
lpdelay(register struct lpdev *lpp)
{
	LP_SPL	oldspl = spltty();

	lpp->lp_tty.t_state &= ~TIMEOUT;
	lpgetoblk(lpp);

	splx(oldspl);
}


/*
 * lpsetoutmode()
 *
 * Calling/Exit State:
 *
 * Description:
 *	Initializes the parallel port and puts it in output mode.
 */

STATIC void 
lpsetoutmode(register struct lpdev *lpp)
{
	if (lpp->lp_porttype == MFPD_PORT_ABSENT) {
		return;
	}

	(void)(mfpd_mhr_init(lpp->lp_porttype))(lpp->lp_portnum);

	return;
}


/*
 * lpoutputchar()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Output a character to the printer.
 */
STATIC void 
lpoutputchar(register struct lpdev *lpp, char ch)
{
	switch (lpp->lp_porttype) {

	case MFPD_SIMPLE_PP:
	case MFPD_PS_2:
	case MFPD_PC87322:
	case MFPD_SL82360:
	case MFPD_AIP82091:
	case MFPD_SMCFDC665:
	case MFPD_COMPAQ:
	     {
							/* L000 vvv */
		extern int lp_select;			/* from Space.c	*/
		int std_sel;

		/* Should the SELECTIN signal from the port be asserted
		   whilst printing ?  Check lp_select to find out.
		*/
		std_sel = ((lp_select >> lpp->lp_portnum) & 0x01) ? STD_SEL : 0;
							/* L000 ^^^ */

		outb(lpp->lp_cntlreg, std_sel | STD_RESET);
		outb(lpp->lp_datareg, ch);
		outb(lpp->lp_cntlreg,
			std_sel | STD_RESET | STD_STROBE | STD_INTR_ON);
		drv_usecwait(10);
		outb(lpp->lp_cntlreg, std_sel | STD_RESET | STD_INTR_ON);
		lpp->lp_wantintr = 1;
		break;
	    }
	default:
		break;
	}
	return;
}


/*
 * lpcallback()
 *
 * Calling/Exit state:
 *
 * Description:
 * 	Callback routine.
 */
STATIC void 
lpcallback(struct mfpd_rqst *prqst)
{
	register struct lpdev *lpp = lpcfg + prqst->port;

	lpp->lp_portreq = 0;
	lpp->lp_portcntl = 1;
	lpgetoblk(lpp);
}


/*
 * lpcallback2()
 *
 * Calling/Exit state:
 *
 * Description:
 * 	Callback routine.
 */
STATIC void 
lpcallback2(struct mfpd_rqst *prqst)
{
	register struct lpdev *lpp = lpcfg + prqst->port;

	lpp->lp_portreq = 0;
	lpp->lp_portcntl = 1;
	lpsetoutmode(lpp);
	lpproc(lpp, T_OUTPUT);
}


/*
 * lpcallback3()
 *
 * Calling/Exit state:
 *
 * Description:
 * 	Callback routine.
 */
STATIC void 
lpcallback3(struct mfpd_rqst *prqst)
{
	register struct lpdev *lpp = lpcfg + prqst->port;
	LP_SPL	oldspl;

	lpp->lp_portreq = 0;
	lpp->lp_portcntl = 1;

	/* If the device is not ready, then re-set the timeout interval */
	if (lpchkstat(lpp) != 0) {
		(void)drv_getparm(TIME, (ulong *) & lpp->lp_ltime);
		return;
	}

	/* Device status is OK; send the next character via lpxintr() */
	lpsetoutmode(lpp);
	oldspl = splstr();
	lpp->lp_wantintr = 0;
	lpxintr(lpp);
	splx(oldspl);
}


/*
 * lpwakeopen();
 *
 * Calling/Exit State:
 *
 * Description:
 *	It is called by mfpd when the request for a port is
 *	granted to a process sleeping in lpopen()
 */
STATIC void
lpwakeopen(struct mfpd_rqst *prqst)
{
	LP_SENDWAKE(lpcfg + prqst->port);
}


/*
 * lpchkstat()
 *
 * Calling/Exit State: Returns the following codes:
 *
 *	1 = something is wrong, do not send next character
 *	0 = all OK, send the next character
 *
 * Description:
 * 
 *	Examines the status register of the given parallel port and 
 *	determines if an error condition exists or if it is ok to send the
 *	next byte to the device. If an error condition exists, then an
 *	error message is sent upstream.
 */
STATIC int
lpchkstat(register struct lpdev *lpp)
{
	unsigned char status;
	int	spincount = 10;
	int	busycount = 10;

	/* Don't service it if device is no longer open */
	if ((lpp->lp_tty.t_state & ISOPEN) == 0) {
		return (1);
	}

read_status:

	/* Get the status from the hardware port */
	switch (lpp->lp_porttype) {
	case MFPD_SIMPLE_PP:
	case MFPD_PS_2:
	case MFPD_PC87322:
	case MFPD_SL82360:
	case MFPD_AIP82091:
	case MFPD_SMCFDC665:
	case MFPD_COMPAQ:

		inb(lpp->lp_statreg);	/* extra read for reliability */

		/*
		 * Spin for a very short time if we caught an ACK. Minimum
		 * duration of an ACK is 0.5 usec.
		 */
		while (LP_IS_ACK(status = inb(lpp->lp_statreg)) && spincount--){
			drv_usecwait(1);
		}

		if (spincount == 0) {
			/* HW still shows busy. Re-try later (after timeout) */
			return (1);	
		}

		break;

	default:
		return (1);	/* unable to read the HW */
	}

	/*
	 * If already in an error condition, check if status has returned
	 * to normal. Exit & wait for next watchdog timeout if it has not. 
	 */
	if (lpp->lp_wrerr || lpp->lp_rderr) {
		if (LP_IS_READY(status)) {
			/* HW has returned to ready state */
			lpp->lp_rderr = lpp->lp_wrerr = LP_NOERROR;
			if (lpp->lp_errchk == LP_ON) {
				lpsendclr(lpp);
			}
			return (0);
		} else {
			return (1);
		}
	}

#ifdef LP_DEBUG2
	/* 
	 *+ debug2
	 */
	if ((status & LP_STATUSMASK) != LP_OK) {
		cmn_err(CE_CONT,"lpchkstat: error status = 0x%x\n",
			status & LP_STATUSMASK);
	}
#endif

	/*
	 * Get the error state from the status byte & set the write-side
	 * error status accordingly.  NOTE: if the printer is in an error
	 * state, then it might be holding the busy line low.
	 */
	lpp->lp_rderr = LP_NOERROR;
	lpp->lp_curstat = status;

	if (LP_IS_READY(status)) {
		lpp->lp_wrerr = LP_NOERROR;
		return (0);
	} else if (LP_IS_NOPAPER(status)) {
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_CONT,"STATUS: NOPAPER\n");
#endif
		lpp->lp_wrerr = LP_PAPEROUT;
	} else if (LP_IS_OFFLINE(status)) {
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_CONT,"STATUS: OFFLINE\n");
#endif
		lpp->lp_wrerr = LP_OFFLINE;
	} else if (LP_IS_DEVERROR(status)) {
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_CONT,"STATUS: DEVERROR2\n");
#endif
		lpp->lp_wrerr = LP_DEVERROR;
	} else {
		lpp->lp_wrerr = LP_NOERROR;
		if (LP_IS_BUSY(status)) {
			/*
			 * if it got here it's busy but has no error so
			 * delay & re-try. (Should be the only way that
			 * gets here.)
			 */
			if (--busycount) {
				drv_usecwait(5);
				goto read_status;
			}
		}

		/* Busy line may be stuck. Re-try later, after timeout */
		return (1);
	}

	/* If enabled, send the error notification upstream. */
	if (lpp->lp_errchk == LP_ON) {
		lpsenderr(lpp);
	}

	return (1);		/* don't service the interrupt */
}


/*
 * lpsenderr()
 *
 * DESCRIPTION: Sends an M_ERROR message upstream to set the write-side
 * error condition on the stream to EIO. Then sends a M_PCPROTO  message
 * upstream with the driver-defined error codes for the error condition.
 *
 * Sets up a bufcall() or a timeout() call for itself if it cannot allocate
 * enough message blocks.
 *
 * May be called from interrupt context or from timeout context.
 *
 * CALLING/EXIT STATE:
 *
 */
STATIC void
lpsenderr(register struct lpdev *lpp)
{
	register mblk_t *mp0;
	register mblk_t *mp1;
	register struct lperrmsg *lpcp;

#ifdef LP_DEBUG2
	/* 
	 *+ debug2
	 */
	cmn_err(CE_CONT,"lpsenderr: entry\n");
#endif

	/* First cancel the timeout if that's what got us here */
	if (lpp->lp_timerstat & LP_BUFCALL) {
		unbufcall(lpp->lp_timer);
	} else if (lpp->lp_timerstat & LP_TIMEOUT) {
		untimeout(lpp->lp_timer);
	}
	lpp->lp_timerstat = 0;
	lpp->lp_timer = 0;

	/* We need two messages; set the timer if either allocation fails */
	if ((mp0 = allocb(2,BPRI_HI)) == NULL) {
		goto settimer;
	}

	if ((mp1 = allocb(sizeof(struct lperrmsg),BPRI_HI)) == NULL) {
		freemsg(mp0);
		goto settimer;
	}

	mp0->b_datap->db_type = M_ERROR;
	*mp0->b_wptr++ = 0;		/* read-side error code */
	*mp0->b_wptr++ = EIO;		/* write-side error code */

	mp1->b_datap->db_type = M_PCPROTO;
	lpcp = (struct lperrmsg *)mp1->b_wptr;
	lpcp->lp_cmd    = LP_ERRINFO;
	lpcp->lp_rderr  = lpp->lp_rderr;
	lpcp->lp_wrerr  = lpp->lp_wrerr;
	lpcp->lp_hwstat = lpp->lp_curstat;
	mp1->b_wptr += sizeof(struct lperrmsg);

#ifdef LP_DEBUG2
	/* 
	 *+ debug2
	 */
	cmn_err(CE_CONT,"SENDING: M_ERROR(0) with read:%d write:%d\n",
		*mp0->b_rptr, *(mp0->b_rptr + 1));
#endif

	putnext(lpp->lp_errq, mp0);

#ifdef LP_DEBUG2
	/* 
	 *+ debug2
	 */
	cmn_err(CE_CONT,"SENDING: M_PCPROTO");
	cmn_err(CE_CONT, "  lp_cmd=0x%x", lpcp->lp_cmd);
	cmn_err(CE_CONT, "  lp_rderr=0x%x", lpcp->lp_rderr);
	cmn_err(CE_CONT, "  lp_wrerr=0x%x", lpcp->lp_wrerr);
	cmn_err(CE_CONT, "  lp_hwstat=0x%x\n", lpcp->lp_hwstat);
#endif

	putnext(lpp->lp_errq, mp1);

	return;

settimer:

	lpp->lp_timer =
		bufcall(sizeof(struct lperrmsg), BPRI_HI, lpsenderr, (long)lpp);

	if (lpp->lp_timer != 0) {
		lpp->lp_timerstat = LP_BUFCALL;
		return;
	}
	/* bufcall failed so try again later */
	lpp->lp_timer = timeout(lpsenderr, (caddr_t)lpp, LP_TIMETICK);
	lpp->lp_timerstat = LP_TIMEOUT;
}


/*
 * lpsendclr()
 *
 * DESCRIPTION: Sends an M_ERROR message upstream with both read and
 * write side error codes set to zero, to clear the error state at the
 * Stream head that was set with an earlier M_ERROR message.
 *
 * Sets up a bufcall() or a timeout() call for itself if it cannot allocate
 * enough message blocks.
 *
 * May be called from interrupt context or from timeout context.
 *
 * CALLING/EXIT STATE:
 *
 */
STATIC void
lpsendclr(struct lpdev *lpp)
{
	mblk_t *mp0;

#ifdef LP_DEBUG2
	/* 
	 *+ debug2
	 */
	cmn_err(CE_CONT,"lpsendclr: entry\n");
#endif


	/* First cancel the timeout if that's what got us here */
	if (lpp->lp_timerstat & LP_BUFCALL) {
		unbufcall(lpp->lp_timer);
	} else if (lpp->lp_timerstat & LP_TIMEOUT) {
		untimeout(lpp->lp_timer);
	}
	lpp->lp_timerstat = 0;
	lpp->lp_timer = 0;

	if ((mp0 = allocb(2,BPRI_HI)) == NULL) {
		goto settimer;
	}

	mp0->b_datap->db_type = M_ERROR;
	*mp0->b_wptr++ = 0;
	*mp0->b_wptr++ = 0;

#ifdef LP_DEBUG2
	/* 
	 *+ debug2
	 */
	cmn_err(CE_CONT,"SENDING: M_ERROR with CLEAR\n");
#endif

	putnext(lpp->lp_errq, mp0);

	return;

settimer:

	lpp->lp_timer = bufcall(2, BPRI_HI, lpsendclr, (long)lpp);
	if (lpp->lp_timer != 0) {
		lpp->lp_timerstat = LP_BUFCALL;
		return;
	}
	/* bufcall failed so try again later */
	lpp->lp_timer = timeout(lpsendclr, (caddr_t)lpp, LP_TIMETICK);
	lpp->lp_timerstat = LP_TIMEOUT;
}


STATIC LP_TMOUT
lpsettimeout(void (*fnp)(), void *argp, long ticks)
{
	LP_TMOUT tf;
	LP_SPL   oldspl = splstr();

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)

try_timer:

	tf = dtimeout(fnp, argp, ticks, plhi, MFPD_PROCESSOR);
	if (tf == 0) {
		/*
		 * Could not schedule timeout routine. Notify user and
		 * try again.
		 */
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_WARN, "Cannot schedule timeout, retrying ...");
#endif

		drv_usecwait(2000);	/* 2 msec busy-wait */
		goto try_timer;
	}
#else
	tf = timeout(fnp, (caddr_t)argp, ticks);
#endif 
	splx(oldspl);
	return (tf);
}


STATIC void
lpdrivertimer(int flag)
{
	static	LP_TMOUT timer_id = 0;
	static	int      timer_on = 0;
	long	ticks  = drv_usectohz(1000000);
	LP_SPL	oldspl = splstr();

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	ticks |= TO_PERIODIC;
#endif

	switch(flag) {
	case LP_TIMERON:
		if (timer_on == 0) {
			timer_id = lpsettimeout(lptmout, NULL, ticks);
		}
		timer_on++;
		break;

	case LP_TIMEROFF:
		if (timer_on == 1) {
			untimeout(timer_id);
		}
		timer_on--;
		break;

	default:
#ifdef LP_DEBUG2
		/* 
		 *+ debug2
		 */
		cmn_err(CE_WARN,"lpdrivertimer: invalid flag\n");
#endif
		break;
	}
	splx(oldspl);
}

