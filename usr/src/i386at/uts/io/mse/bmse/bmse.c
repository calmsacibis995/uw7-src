#ident	"@(#)bmse.c	1.16"
#ident	"$Header$"

/*
 * Bus Mouse Driver - STREAMS
 */


#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <io/termio.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <util/debug.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <util/cmn_err.h>
#include <io/ws/chan.h>
#include <io/mouse.h>
#include <io/mse/mse.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>

#ifdef ESMP
#include <util/ksynch.h>
#endif

#include <io/ddi.h>	/* must come last */

#include <util/mod/moddefs.h>

#define	DRVNAME	"bmse - Loadable bus mouse driver"
#define M_IN_DATA	0

int bmse_3bdly = 0;

STATIC	int	bmse_load(), bmse_unload();
int	bmse_verify();

MOD_ACDRV_WRAPPER(bmse, bmse_load, bmse_unload, NULL, bmse_verify, DRVNAME);
extern	void	mod_drvattach(), mod_drvdetach();

STATIC struct mouseconfig mse_config;

/*
 * Wrapper functions.
 */

int	bmseinit();
int	bmsedevflag = 0;
void	bmseintr(int);

STATIC	int
bmse_load(void)
{
	cmn_err(CE_NOTE, "!MOD: in bmse_load()");

	if (bmseinit()) {
		return (ENODEV);
	}
	return (0);
}

STATIC	int
bmse_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in bmse_unload()");

	cm_intr_detach(mse_config.cookie);
	return (0);
}


#ifdef ESMP
#define	BMSEHIER	1
#define	BMSEPL		plstr
#endif

#ifdef DEBUG
STATIC int bmse_debug = 0;
#define DEBUG1(a)	if (bmse_debug == 1) printf a
#define DEBUG2(a)	if (bmse_debug >= 2) printf a /* allocations */
#define DEBUG3(a)	if (bmse_debug >= 3) printf a /* M_CTL Stuff */
#else
#define DEBUG1(a)
#define DEBUG2(a)
#define DEBUG3(a)
#endif /* DEBUG */



STATIC int	bmseopen(queue_t *, dev_t *,int, int, struct cred *);
STATIC int	bmseclose(queue_t *, int, struct cred *);
STATIC int	bmse_wput(queue_t *, mblk_t *);
STATIC void	bmseInPortData();
STATIC void	bmseLogitechData();

extern void	mse_iocack(queue_t *, mblk_t *, struct iocblk *, int);
extern void	mse_iocnack(queue_t *, mblk_t *, struct iocblk *, int, int);
extern void	mse_copyout(queue_t *, register mblk_t *, register mblk_t *,
			uint, unsigned long);
extern void	mse_copyin(queue_t *, register mblk_t *, int, unsigned long);
extern void	mseproc(struct strmseinfo *);


struct module_info bmse_info = { 
	23, "bmse", 0, INFPSZ, 256, 128 
};

static struct qinit bmse_rinit = {
	NULL, NULL, bmseopen, bmseclose, NULL, &bmse_info, NULL 
};

static struct qinit bmse_winit = {
	bmse_wput, NULL, NULL, NULL, NULL, &bmse_info, NULL 
};

struct streamtab bmseinfo = { 
	&bmse_rinit, &bmse_winit, NULL, NULL 
};


static unsigned	BASE_IOA;	/* Set to base I/O addr of bus mouse */
static struct strmseinfo *bmseptr = 0;

char	bmseclosing = 0;
char	bmseInPort = 0;
void	(*bmsegetdata)();

#ifdef ESMP
lock_t	*bmse_lock;		/* bus mouse mutex spin lock */
sv_t	*bmse_closesv;		/* closing sync. variable */	

LKINFO_DECL(bmse_lkinfo, "MSE:BMSE:bmse mutex lock", 0);
#endif


/*
 * int
 * bmseinit(void)
 *
 * Calling/Exit State:
 *	None.
 */
int
bmseinit(void)
{
	int			i;
	cm_args_t		cm_args;
	struct	cm_addr_rng	ioaddr;


#ifdef ESMP
	bmse_lock = LOCK_ALLOC(BMSEHIER, BMSEPL, &bmse_lkinfo, KM_SLEEP);
	bmse_closesv = SV_ALLOC(KM_SLEEP);
#endif

	if ( cm_getnbrd( "bmse" ) <= 0 )
		return ENODEV;

	/*
	 * Get key for the bmse board. 
	 */
	cm_args.cm_key = cm_getbrdkey("bmse", 0);
	cm_args.cm_n   = 0;

	/*
	 * Get interrupt vector.
	 */
	cm_args.cm_param = CM_IRQ;
	cm_args.cm_val = &(mse_config.ivect);
	cm_args.cm_vallen = sizeof(mse_config.ivect);

	if ( cm_getval(&cm_args) != 0  ||  mse_config.ivect == 0) {
		return (EINVAL);
	}

	/*
	 * Get I/O address range.
	 */
	cm_args.cm_param = CM_IOADDR;
	cm_args.cm_val = &ioaddr;
	cm_args.cm_vallen = sizeof(struct cm_addr_rng);

	if (cm_getval(&cm_args)) {
		return (EINVAL);
	} else {
		mse_config.io_addr.startaddr = ioaddr.startaddr;
		mse_config.io_addr.endaddr = ioaddr.endaddr;
	}

	/*
	 * Driver specific code to initialize board
	 */

	BASE_IOA = mse_config.io_addr.startaddr;	

	/*
	 * Determine what type of mouse board is installed.
	 */

	i = inb(IDENTREG);

	if (i == SIGN) {	/* Microsoft InPort Board */
		outb( ADDRREG, RESET );	/* reset chip */
		bmseInPort = 1;
		bmsegetdata = bmseInPortData;
		mse_config.present = 1;
	} else {		/* Logitech Board */
		/* Check if the mouse board exists */
		outb(CONFIGURATOR_PORT, 0x91);
		drv_usecwait(10);
		outb(SIGNATURE_PORT, 0xC) ;
		drv_usecwait(10);
		i = inb(SIGNATURE_PORT);
		drv_usecwait(10);
		outb(SIGNATURE_PORT, 0x50);
		drv_usecwait(10);
		if (i == 0xC && inb (SIGNATURE_PORT) == 0x50) {
			mse_config.present = 1;
			bmsegetdata = bmseLogitechData;
			control_port(INTR_DISABLE);
		}
	}
	cm_intr_attach(cm_args.cm_key, bmseintr, &bmsedevflag,
		&mse_config.cookie);

	return (0);

}


/*
 * STATIC int
 * bmseopen(queue_t *, dev_t *, int, int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
bmseopen(queue_t *q, dev_t *devp, int flag, int sflag, struct cred *cred_p)
{
	pl_t	oldpri;

	DEBUG1(("bmseopen:entered\n"));

	if (!mse_config.present)
		return EIO;

	if (q->q_ptr != NULL) {
		DEBUG1(("bmseopen:already open\n"));
		/* already attached */
		return (0);
	}

	oldpri = LOCK(bmse_lock, BMSEPL);
	ASSERT(getpl() == plstr);

	while (bmseclosing) {
		SV_WAIT(bmse_closesv, primed - 1, bmse_lock);
		oldpri = LOCK(bmse_lock, BMSEPL);
	}

	UNLOCK(bmse_lock, oldpri);

	/* allocate and initialize state structure */

	if ((bmseptr = (struct strmseinfo *) kmem_zalloc(
			 sizeof(struct strmseinfo), KM_SLEEP)) == NULL) {
		/*
		 *+ There is not enough memory available to allocate
		 *+ for strmseinfo structure. Check memory configured
		 *+ in the system.
		 */
		cmn_err(CE_WARN,
			"bmseopen: open fails, can't allocate state structure");
		return (ENOMEM);
	}

	q->q_ptr = (caddr_t) bmseptr;
	WR(q)->q_ptr = (caddr_t) bmseptr;
	bmseptr->rqp = q;
	bmseptr->wqp = WR(q);
	bmseptr->old_buttons = 0x07;		/* L999	*/

	if (bmseInPort) {
		outb(ADDRREG, MODE);	/* select mode register */
		outb(DATAREG, QUADMODE | DATAINT | HZ30);
	} else {
		control_port(INTR_ENABLE);
	}

	DEBUG1(("bmseopen:leaving\n"));
	return (0);
}


/*
 * STATIC int
 * bmseclose(queue_t *, int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
bmseclose(queue_t *q, int flag, struct cred *cred_p)
{
	pl_t	oldpri;

	DEBUG1(("bmseclose:entered\n"));

	if (bmseInPort) {
		outb(ADDRREG, MODE);	/* select mode register */
		outb(DATAREG, 0);
	} else {
		control_port(INTR_DISABLE);	/* Disable interrupts */
	}

	oldpri = LOCK(bmse_lock, BMSEPL);
	ASSERT(getpl() == plstr);

	bmseclosing = 1;
	q->q_ptr = (caddr_t) NULL;
	WR(q)->q_ptr = (caddr_t) NULL;
	kmem_free(bmseptr, sizeof(struct strmseinfo));
	bmseptr = (struct strmseinfo *) NULL;
	bmseclosing = 0;

	SV_SIGNAL(bmse_closesv, 0);
	UNLOCK(bmse_lock, oldpri);

	DEBUG1(("bmseclose:leaving\n"));
	return(0);
}


/*
 * STATIC int
 * bmse_wput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
bmse_wput(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocbp;
	mblk_t	*bp;
	struct copyresp *csp;
#ifdef ESMP
	pl_t	oldpri;
#else
	int	oldpri;
#endif


	DEBUG1(("bmse_wput:entered\n"));

	if (bmseptr == 0) {
		freemsg(mp);
		DEBUG1(("bmse_wput:bmseptr == NULL\n"));
		return(0);
	}

	/* LINTED pointer alignment */
	iocbp = (struct iocblk *) mp->b_rptr;

	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		DEBUG1(("bmse_wput:M_FLUSH\n"));

		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);

		qreply(q, mp);
		break;

	case M_IOCTL:
		DEBUG1(("bmse_wput:M_IOCTL\n"));

		switch(iocbp->ioc_cmd) {
		case MOUSEIOCREAD:
			DEBUG1(("bmse_wput:M_IOCTL-MOUSEIOCREAD\n"));

			if ((bp = allocb(sizeof(struct mouseinfo), BPRI_MED)) == NULL) {
				mse_iocnack(q, mp, iocbp, EAGAIN, 0);
				break;
			}

			oldpri = splstr();
			bcopy(&bmseptr->mseinfo, bp->b_rptr, 
						sizeof(struct mouseinfo));
			bmseptr->mseinfo.xmotion = bmseptr->mseinfo.ymotion = 0;
			bmseptr->mseinfo.status &= BUTSTATMASK;
			splx(oldpri);

			bp->b_wptr += sizeof(struct mouseinfo);

			if (iocbp->ioc_count == TRANSPARENT) 
				mse_copyout(q, mp, bp, 
						sizeof(struct mouseinfo), 0);
			else {
				DEBUG1(("bmse_wput:M_IOCTL not transparent\n"));
				mp->b_datap->db_type = M_IOCACK;
				iocbp->ioc_count = sizeof(struct mouseinfo);
				if (mp->b_cont)
					freemsg(mp->b_cont);

				mp->b_cont = bp;
				qreply(q, mp);
			}

			break;

		case MOUSEIOC3BE:
			if (iocbp->ioc_count != TRANSPARENT){
				mse_iocnack(q, mp, iocbp, EINVAL, 0);
				break;
			}

			mse_copyin(q,mp,sizeof(int),M_IN_DATA);
			break;

		default:
			DEBUG1(("bmse_wput:M_IOCTL-DEFAULT\n"));

			mse_iocnack(q, mp, iocbp, EINVAL, 0);
			break;
		}

		break;

	case M_IOCDATA:
		DEBUG1(("bmse_wput:M_IOCDATA\n"));

		/* LINTED pointer alignment */
		csp = (struct copyresp *)mp->b_rptr;


		if (csp->cp_rval) {
			DEBUG1(("bmse_wput:M_IOCDATA - freemsging\n"));
			freemsg(mp);
			break;
		}

		switch (csp->cp_cmd) {

			case MOUSEIOCREAD:
				DEBUG1(("bmse_wput:M_IOCDATA - ACKing\n"));
				mse_iocack(q, mp, iocbp, 0);
				break;

			case MOUSEIOC3BE:
				if (!mp->b_cont)
					cmn_err(CE_WARN,"bmse: NO MOUSEIOC3BE M_IOCDATA");
				else { 
					bmse_3bdly = (*(int *)mp->b_cont->b_rptr);
					((struct strmseinfo *)q->q_ptr)->fsm_timeout = bmse_3bdly;
					freemsg(mp->b_cont);
					mp->b_cont = 0;
					mse_iocack(q, mp, iocbp, 0);
				}
				break;

			default:
				DEBUG1(("bmse_wput:M_IOCDATA - NACKing\n"));
				mse_iocnack(q, mp, iocbp, EINVAL, 0);
				break;
		}
		break;

	default:
		freemsg(mp);
		break;
	}

	DEBUG1(("bmse_wput:leaving\n"));
	return(0);
}


/*
 * void
 * bmseintr(int)
 *
 * Calling/Exit State:
 *	None.
 */
void
bmseintr(int vect)
{
	if (mse_config.ivect != vect) {
		/*
		 *+ Mouse interrupt is received on an unconfigured vector.
		 */
		cmn_err(CE_WARN,
			"Mouse interrupt on un-configured vector: %d", vect);
		return;
	}

	if (!bmseptr) {
#ifdef DEBUG
		/*
		 *+ An interrupt is received before the mouse driver
		 *+ is opened.
		 */
		cmn_err(CE_NOTE, 
			"received interrupt before opened");
#endif
		if (bmseInPort) {
			outb(ADDRREG, MODE);	/* select mode register */
			outb(DATAREG, QUADMODE | DATAINT | HZ30);
		} else {
			control_port(INTR_ENABLE);
		}

		return;
	}

	(*bmsegetdata)();
}


/*
 * STATIC void
 * bmseInPortData(void)
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
bmseInPortData(void)
{
	unsigned  BASE_IOA = mse_config.io_addr.startaddr;
	unchar	d;


	outb(ADDRREG, MODE);
	outb(DATAREG, QUADMODE | HZ30 | HOLD);

	outb(ADDRREG, MSTATUS);
	d = inb(DATAREG);	/* read mouse status byte */
	bmseptr->button = ~d & BUTMASK;

	if (d & MSEMOTION) {
		outb(ADDRREG, XMOTION);
		bmseptr->x = inb(DATAREG);
		outb(ADDRREG, YMOTION);
		bmseptr->y = inb(DATAREG);
	} else {
		bmseptr->x = bmseptr->y = 0;
	}

	mseproc(bmseptr);

	/* Re-enable interrupts on the mouse and return */

	outb(ADDRREG, MODE);	/* select mode register */
	outb(DATAREG, QUADMODE | DATAINT | HZ30);
}


/*
 * STATIC void
 * bmseLogitechData(void)
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
bmseLogitechData(void)
{
	unsigned BASE_IOA = mse_config.io_addr.startaddr;
	unchar	d;


	/*
	 * Get the mouse's status and put it into the
	 * appropriate virtual structure
	 */

	control_port(INTR_DISABLE | HC | HIGH_NIBBLE | X_COUNTER);
	bmseptr->x = (data_port & 0x0f) << 4;
	control_port(INTR_DISABLE | HC | LOW_NIBBLE | X_COUNTER);
	bmseptr->x |= (data_port & 0x0f);
	control_port(INTR_DISABLE | HC | HIGH_NIBBLE | Y_COUNTER);
	bmseptr->y = (data_port & 0x0f) << 4;
	control_port(INTR_DISABLE | HC | LOW_NIBBLE | Y_COUNTER);
	bmseptr->y |= ((d = data_port) & 0x0f);
	bmseptr->button = (d >> 5) & 0x07;

	mseproc(bmseptr);

	/* Re-enable interrupts on the mouse and return */

	control_port(INTR_ENABLE);
}


/*
 * int
 * bmse_verify(rm_key_t key)
 *
 * Calling/Exit State:
 *	None
 */
int
bmse_verify(rm_key_t key)
{
	int			i;
	cm_args_t		cm_args;
	struct	mouseconfig	inst;

	cm_args.cm_key = key;
	cm_args.cm_n = 0;

	/*
	 * Get interrupt vector.
	 */
	cm_args.cm_param = CM_IRQ;
	cm_args.cm_val = &inst.ivect;
	cm_args.cm_vallen = sizeof(inst.ivect);
	if (cm_getval(&cm_args)) {
		return (EINVAL);
	}

	/*
	 * Get I/O address range.
	 */
	cm_args.cm_param = CM_IOADDR;
	cm_args.cm_val = &inst.io_addr;
	cm_args.cm_vallen = sizeof(struct cm_addr_rng);
	if (cm_getval(&cm_args)) {
		return (EINVAL);
	}

	/*
	 * Driver specific code to verify the existence
	 * of hardware using the parameters obtained above,
	 * return 0 if the hardware is found, ENODEV otherwise.
	 */

	/*
	 * Determine what type of mouse board is installed.
	 */
	i = inb(inst.io_addr.startaddr + 2);

	if (i == SIGN) {	/* Microsoft InPort Board */
		outb( inst.io_addr.startaddr, RESET );	/* reset chip */
		return (0);
	} else {		/* Logitech Board */
		/* Check if the mouse board exists */
		outb((inst.io_addr.startaddr + 3), 0x91);
		drv_usecwait(10);
		outb((inst.io_addr.startaddr + 1), 0xC) ;
		drv_usecwait(10);
		i = inb((inst.io_addr.startaddr + 1));
		drv_usecwait(10);
		outb((inst.io_addr.startaddr + 1), 0x50);
		drv_usecwait(10);
		if (i == 0xC && inb ((inst.io_addr.startaddr + 1)) == 0x50) {
			return (0);
		}
	}
	return (ENODEV);
}
