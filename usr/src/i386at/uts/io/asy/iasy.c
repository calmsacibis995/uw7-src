/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)iasy.c	1.13"
#ident 	"$Header$"

/*
 * Generic Terminal Driver (STREAMS version)
 * This forms the hardware independent part of a serial driver.
 */

#include <fs/fcntl.h>
#include <fs/file.h>
#include <io/conssw.h>
#include <io/ldterm/eucioctl.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/termiox.h>
#include <proc/cred.h>
#include <proc/signal.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <io/asy/iasy.h>

#include <io/ddi.h>		/* Must come last */

/* #define IASY_DEBUG 1	*/

#define	IASYHIER		1
#define IASYPL			plstr

/* 
 * Timeout to remove block state while closing if queue will not 
 * drain (eg. driver is flow controlled).
 */

#define	CL_TIME			(8*HZ)	

#ifdef DEBUG 
/*	#define IASY_DEBUG 1	*/
#endif

#ifdef IASY_DEBUG 

/*
 * Undefine ASSERT for building under IASY_DEBUG when not building 
 * under kernel DEBUG.
 */
	#ifdef ASSERT 
		#undef ASSERT
	#endif 

	#define ASSERT(EX) \
	( (EX) || iasy_cmnerr ( #EX " file:%s, line: %d" ,__FILE__, __LINE__ ) )

#else 
	
	#ifdef ASSERT 
		#undef ASSERT
	#endif 
		#define ASSERT(EX)

#endif 		/* IASY_DEBUG	*/

/*
 * Values for t_dstat
 */
#define IASY_EXCL_OPEN		(1 << 0)

/*
 * various macro definitions
 */
#define TP_TO_Q(tp)		((tp)->t_rdqp)
#define Q_TO_TP(q)		((struct strtty *)(q)->q_ptr)
#define TP_TO_HW(tp)		(&iasy_hw[IASY_MINOR_TO_UNIT((tp)->t_dev)])
#define TP_TO_SV(tp)		(&iasy_sv[IASY_MINOR_TO_UNIT((tp)->t_dev)])
#define TP_TO_TOID(tp)		(iasy_toid[IASY_MINOR_TO_UNIT((tp)->t_dev)])
#define HW_PROC(tp, func)	((*TP_TO_HW(tp)->proc)(tp, func))


/*
 * function prototype definitions
 */
void		iasyinit(void);
void		iasystart(void);
void		iasy_hup(struct strtty *);

STATIC int	iasyopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	iasyclose(queue_t *, int, cred_t *);
STATIC void	iasy_drain(void *); 
STATIC void	iasydelay(struct strtty *);
STATIC void	iasyputioc(queue_t *, mblk_t *);
STATIC int	iasyoput(queue_t *, mblk_t *);
STATIC mblk_t	*iasygetoblk(struct queue *);
STATIC void	iasysrvioc(queue_t *, mblk_t *);
STATIC void	iasyflush(queue_t *, int);
STATIC int	iasyisrv(queue_t *);
STATIC int	iasyosrv(queue_t *);
STATIC void	iasybufwake(struct strtty *);

STATIC dev_t	iasycnopen(minor_t, boolean_t, const char *);
STATIC void	iasycnclose(minor_t, boolean_t);
STATIC int	iasycnputc(minor_t, int);
STATIC int	iasycngetc(minor_t);
STATIC void	iasycnsuspend(minor_t);
STATIC void	iasycnresume(minor_t);

/* 
 * Defined in io/Driver.o 
 */

extern speed_t tpgetspeed(tcstype_t, const struct strtty *);
extern void tpsetspeed(tcstype_t,struct strtty *,speed_t); 

/*
 * variables defined in iasy.cf/Space.c
 */

extern struct strtty	asy_tty[];	/* strtty structs / device. */
extern struct termiox	asy_xtty[];	/* termiox structs / device. */

extern struct iasy_hw	iasy_hw[];	/* hardware info per device */
extern int				iasy_num;	/* maximum number of hardware ports */
extern struct iasy_sv	iasy_sv[];	/* sync variables per port */
extern int 				iasy_toid[];	/* timeout variables per port */


/*
 * Initialize iasy STREAMs module_info, qinits and 
 * streamtab structures
 */
struct module_info iasy_info = {
	901, "iasy", 0, INFPSZ, IASY_HIWAT, IASY_LOWAT };

static struct qinit iasy_rint = {
	putq, iasyisrv, iasyopen, iasyclose, NULL, &iasy_info, NULL};

static struct qinit iasy_wint = {
	iasyoput, iasyosrv, iasyopen, iasyclose, NULL, &iasy_info, NULL};

struct streamtab iasyinfo = {
	&iasy_rint, &iasy_wint, NULL, NULL};


/*
 * global variables
 */

int	iasydevflag = 0;	/* driver attributes */
int	iasy_cnt = 0;		/* - /etc/crash requirement
						 * - count of ports registered
				 		 */ 


conssw_t iasyconssw = { 
	iasycnopen, iasycnclose, iasycnputc, iasycngetc,
	iasycnsuspend, iasycnresume
};

STATIC lock_t *iasy_lock;		/* iasy mutex lock */

LKINFO_DECL(iasy_lkinfo, "IASY::iasy_lock", 0);

STATIC boolean_t iasy_initialized = B_FALSE;

#ifndef lint
static char iasy_copyright[] = "Copyright 1991 Intel Corporation xxxxxx";
#endif /*lint*/

extern int strhead_iasy_hiwat ; 
extern int strhead_iasy_lowat ;

#ifdef IASY_DEBUG 
extern 	int	iasy_debug; 
#endif 

/*
 * Debugging display macros.If debugging then define them to 
 * printf() etc.
 */

#ifdef IASY_DEBUG
#define IASYDBG(a)	printf a 
#else 
#define IASYDBG(a) 	
#endif	

/*
 * STATIC void
 * iasybufwake(struct strtty *)
 *
 * Calling/Exit State:
 *	None.
 * 
 * Description:
 *	Wakeup sleep function calls sleeping for a STREAMS buffer
 *	to become available
 */

STATIC void
iasybufwake(struct strtty *tp)
{
	SV_SIGNAL((TP_TO_SV(tp))->iasv_buf, 0);
}


/*
 * STATIC int
 * iasy_drain(struct strtty *)
 *
 * Calling/Exit State:
 *	Called at plstr after the timeout has expired, scheduled from iasyclose.
 * 
 * Remarks: 
 * 	Wakes the iasyclose routine, after clearing the TTIOW flag, allowing 
 *	resampling of the wrote queue state ( _NOT_ close completion ).
 */

STATIC void 
iasy_drain(void *vtp)
{
	struct strtty *tp = (struct strtty *) vtp; 

	/* 
	 * Clearing the TTIOW flag helps to distibguish between an interrupted 
	 * SV_WAIT_SIG() and a woken one, but isn't really necessary.
	 */

	HW_PROC(tp, T_RESUME);
	tp->t_state &= ~TTIOW;
	SV_SIGNAL((TP_TO_SV(tp))->iasv_drain, 0);
	TP_TO_TOID(tp) = 0;

	return; 
}


/*
 * void
 * iasyinit(void)
 *
 * Calling/Exit State:
 *	Called with interrupts off at io_init(D2DK) or init_console() time.
 *	
 * Remarks:
 *	Calls all hardware drivers initialisation functions present.
 *
 */

void
iasyinit(void)
{
	void (**funcpp)();
	extern void (*asyinit_funcs[])();

	/* Call asy initialization functions */
	for (funcpp = asyinit_funcs; *funcpp;)
		(*(*funcpp++))();

	iasy_initialized = B_TRUE;
}


/*
 * void
 * iasystart(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 * 	Attempts to allocate an MP lock.
 */

void
iasystart(void)
{

	iasy_lock = LOCK_ALLOC(IASYHIER, IASYPL, &iasy_lkinfo, KM_NOSLEEP);

	if (!iasy_lock)
		/*
		 *+ There is not enough memory available to allocate
		 *+ serial port (iasy) mutex lock.
		 */
		cmn_err(CE_PANIC, "Not enough memory available");
}


/*
 * STATIC int
 * iasyopen(queue_t *, dev_t *, int, int, cred_t *)
 *	Open an iasy line
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 * 	The routine sets up STREAMS and driver (termio[sx]) parameters for 
 * 	first open, and checks legality of further open(2) requests (must be 
 * 	of same (major/minor) device, and neither in OEXCL mode. 
 */

STATIC int
iasyopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{	
	struct strtty 		*tp;
	struct termiox		*xtp; 
	struct stroptions 	*sop;
	struct iasy_sv 		*iasv;
	minor_t			ldev;
	pl_t			oldpri;
	mblk_t			*mop;
	int			ret,unit;
	toid_t			bid;			/* bufcall id */

	ldev = getminor(*devp);
	unit = IASY_MINOR_TO_UNIT(ldev);

	/* Check for valid device Number */
	if (unit >= iasy_num)
		return (ENXIO);

	/* 
	 * Check if the HW device routine registered. Note that this 
	 * is the first indocation of open(2) on a non-existent device,
	 */

	if (iasy_hw[unit].proc == 0){
		/* No hardware for this minor number */
#ifdef IASY_DEBUG 
		if (iasy_debug & IDSP_ENXIO) 
			cmn_err(CE_NOTE, "iasyopen: no hardware for unit %d", unit);
#endif 
		return (ENXIO);
	} 

	tp = &asy_tty[unit];
	
	iasv = &iasy_sv[unit];
	if (iasv->iasv_drain == NULL) {
		iasv->iasv_drain = SV_ALLOC(KM_SLEEP);
		iasv->iasv_carrier = SV_ALLOC(KM_SLEEP);
		iasv->iasv_buf = SV_ALLOC(KM_SLEEP);
	}

	oldpri = SPL();

	/* 
	 * Do the required things on first open 
	 */

	if ((tp->t_state & (ISOPEN|WOPEN)) == 0) {

		tp->t_dev = ldev;
		tp->t_rdqp = q;
		q->q_ptr = (caddr_t) tp;
		WR(q)->q_ptr = (caddr_t) tp;

		/*
		 * set process group on first tty open 
		 */

		while ((mop = allocb(sizeof(struct stroptions), BPRI_MED)) == NULL) {
			if (flag & (FNDELAY|FNONBLOCK)) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return (EAGAIN);
			}

			bid = bufcall((uint)sizeof(struct stroptions), 
					BPRI_MED, iasybufwake, (long)tp);
			if (bid == 0) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				/*
				 *+ bufcall() was unsuccessful in scheduling
				 *+ a buffer allocation request. Check the
				 *+ memory configuration.
				 */
				cmn_err(CE_WARN, 
					"Not enough memory available");
				return (ENOMEM);
			}

			(void) LOCK(iasy_lock, plstr); 
			if (SV_WAIT_SIG((TP_TO_SV(tp))->iasv_buf, TTIPRI, 
						iasy_lock) == B_FALSE) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return (EINTR);
			}
		}

		mop->b_datap->db_type = M_SETOPTS;
		mop->b_wptr += sizeof(struct stroptions);
		/* LINTED pointer alignment */
		sop = (struct stroptions *)mop->b_rptr;
		sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
		sop->so_hiwat = strhead_iasy_hiwat;
		sop->so_lowat = strhead_iasy_lowat;
		(void) putnext(q, mop);
	
		/*
		 * Set water marks on write q 
		 */
		strqset(WR(q), QHIWAT, 0, IASY_HIWAT);
		strqset(WR(q), QLOWAT, 0, IASY_LOWAT);

		tp->t_cc[VSTART] = CSTART;
		tp->t_cc[VSTOP] = CSTOP;

		/* 
		 * Set up the termios cflag initial modes, XPG says implementation
		 * dependent. This functionality could be provided by an open(2) in 
		 * O_NONBLOCK mode and TCSETS ioctl, but using minors allows shell 
		 * level choice in port modes. We provide a SW and HW flow and MODEM 
		 * controlled ( UW ttyNNs , ttyNNh ) ans a non MODEM no flow and 
		 * MODEM no flow controlled nodes ( SCO ttyMa ( == tty0(M-1)t ) , 
		 * ttyMA ( == tty0(M-1)m )). Override other (not identity parameters)
		 * by setting space.c uart_scantbl entries. 
		 */

		xtp = &asy_xtty[unit]; 

		/* 
		 * Test for any preexisting setup flags, and, if found, assume that 
		 * all setting are done (eg. use NO defaults, apart from the identity 
		 * flag settings ( CLOCAL, IXON, IXOFF, DTSXON, RTSXOFF ). Ie. If not
		 * set, baud rate will be 0.
		 */

		if (tp->t_iflag||tp->t_oflag||tp->t_cflag||tp->t_lflag||xtp->x_hflag){ 

#ifdef IASY_DEBUG
			if (iasy_debug & IDSP_SETMODE)
				cmn_err ( CE_NOTE,"iasyopen: using tunable mode settings"); 
#endif

		} else { 	

			/* 
			 * Set the default parameters. 
			 */

			tp->t_iflag = 0;
			tp->t_oflag = 0;
			tp->t_cflag = B9600|CS8|CREAD;
			tp->t_lflag = 0;
			xtp->x_hflag = 0;
		} 

		/* 
		 * MKHW_T returns the enum value for a node type.
		 */
	
		switch ( MKHW_T ( ldev )) { 

			/* 
			 * UW S/W flow controlled, MODEM device.
			 */

			case UWSWFC : 
				tp->t_cflag &= ~CLOCAL;
				tp->t_iflag |= (IXON|IXOFF); 
				xtp->x_hflag &= ~(RTSXOFF|CTSXON);
				break ; 

			/* 
			 * UW H/W flow controlled, MODEM device.
			 */

			case UWHWFC:	
				tp->t_cflag &= ~CLOCAL;
				xtp->x_hflag |= (RTSXOFF|CTSXON);
				tp->t_iflag &= ~(IXON|IXOFF); 
				break ; 
			
			/* 
			 * SCO terminal device. No flow control and CLOCAL (ignore DCD).
			 * Allows stty on an unconnected port and 3 wire terminals. 
			 */

			case SCOTRM:
				tp->t_iflag &= ~(IXON|IXOFF);  
				tp->t_cflag |= CLOCAL; 
				xtp->x_hflag &= ~(RTSXOFF|CTSXON);
				break ; 

			/* 
			 * SCO MODEM device. No SW or HW flow control, and ~CLOCAL.
			 */

			case SCOMDM:
				tp->t_iflag &= ~(IXON|IXOFF);  
				tp->t_cflag &= ~CLOCAL; 
				xtp->x_hflag &= ~(RTSXOFF|CTSXON);
				break ; 
			
			default: 
				cmn_err(CE_WARN,"iasyopen: Bad dev %d/%d : no default mode", 
									getmajor(*devp), getminor(*devp)); 
				break; 
		} 

		/*
		 * allocate RX buffer 
		 */

		while ((tp->t_in.bu_bp = 
				allocb(IASY_BUFSZ, BPRI_MED)) == NULL) {
			if (flag & (FNDELAY|FNONBLOCK)) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return (EAGAIN);
			}

			bid = bufcall(IASY_BUFSZ, BPRI_MED, 
					iasybufwake, (long)tp);
			if (bid == 0) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				/*
				 *+ bufcall() was unsuccessful in scheduling
				 *+ a buffer allocation request. Check the
				 *+ memory configuration. 
				 */
				cmn_err(CE_WARN,
					"No STREAMS buffers available %d/%d",
					getemajor(ldev), geteminor(ldev));
				return (ENOMEM);
			}

			(void) LOCK(iasy_lock, plstr);
			if (SV_WAIT_SIG((TP_TO_SV(tp))->iasv_buf, TTIPRI, 
						iasy_lock) == B_FALSE) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return (EINTR);
			}
		}

		tp->t_in.bu_cnt = IASY_BUFSZ;
		tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
		tp->t_out.bu_bp = 0;
		tp->t_out.bu_cnt = 0;
		tp->t_out.bu_ptr = 0;

		/* 
		 * T_FIRSTOPEN sets HW driver initial state.
		 */

		if (ret = HW_PROC(tp, T_FIRSTOPEN)) {
			tp->t_rdqp = NULL;
			if (tp->t_in.bu_bp) {
				freeb(tp->t_in.bu_bp);
				tp->t_in.bu_bp = 0;
			}
			splx(oldpri);
			return (ret);
		}

		/* 
		 * Clear extended state flag 
		 */

		tp->t_dstat = 0;
		if (flag & FEXCL)
			tp->t_dstat = IASY_EXCL_OPEN;
	} else {

		if (ldev != tp->t_dev||(tp->t_dstat & IASY_EXCL_OPEN)||(flag & FEXCL)){

#ifdef ASYC_DEBUG
			cmn_err(CE_WARN,"Open %d/%d, %d/%d O_EXCL open already",
					getemajor(ldev) , geteminor(ldev) , getemajor(tp->t_dev) , 
						geteminor(tp->t_dev)); 
#endif
			splx(oldpri);
			return (EBUSY);
		}
	}
	
	/*
	 * T_CONNECT sets up the various per port data, especially 
	 * the t_state CARR_ON. May already be on if line is hardwired.
	 */

	if (ret = HW_PROC(tp, T_CONNECT)) { 
		(void) HW_PROC(tp, T_DISCONNECT);
		if (!(tp->t_state & ISOPEN))
			(void) HW_PROC(tp, T_LASTCLOSE);
		tp->t_rdqp = NULL;
		if (tp->t_in.bu_bp) {
			freeb(tp->t_in.bu_bp);
			tp->t_in.bu_bp = 0;
		}
		splx(oldpri);
		return (ret);
	}

	/*
	 * For modem devices (~CLOCAL) XPG4 et al. specify that the 
	 * driver should block until a carrier from the remote site
	 * is detected.
	 * For open(2) calls with FNDELAY/FNONBLOCK mode, the open(2)
	 * should succeed whatever the state of any [flow] control 
	 * lines.
	 */

	if (!(flag & (FNDELAY|FNONBLOCK))) {
		while ((tp->t_state & CARR_ON) == 0) {
			tp->t_state |= WOPEN;
			(void) LOCK(iasy_lock, plstr);
			if (SV_WAIT_SIG((TP_TO_SV(tp))->iasv_carrier, TTIPRI, 
						iasy_lock) == B_FALSE) {
				if (!(tp->t_state & ISOPEN)) {
					(void) HW_PROC(tp, T_LASTCLOSE);
					(void) HW_PROC(tp, T_DISCONNECT);
					q->q_ptr = NULL;
					WR(q)->q_ptr = NULL;
					tp->t_rdqp = NULL;
					if (tp->t_in.bu_bp) {
						freeb(tp->t_in.bu_bp);
						tp->t_in.bu_bp = 0;
					}
				}
				tp->t_state &= ~WOPEN;
				splx(oldpri);
				return (EINTR);
			}
		}
	}

	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;

	/*
	 * switch on put and srv. routines.
	 */
	qprocson(q);

	splx(oldpri);

	return (0);
}


/*
 * STATIC int
 * iasyclose(queue_t *, int, cred_t *)
 *
 * Calling/Exit State:
 *	None. 
 *
 * Remarks:
 *	Close(2) waits until unwritten data has drained, if MODEM carrier state 
 * 	is insignificant, or OK, if not open(2) in ONONBLOCK mode.
 */

/* ARGSUSED */
STATIC int
iasyclose(queue_t *q, int flag, cred_t *cred_p)
{	
	struct strtty *tp;
	pl_t	oldpri;

	tp = Q_TO_TP(q);
	oldpri = SPL();

	/*
	 * Drain queued output to the user's terminal only if FNONBLOCK
	 * or FNDELAY flag not set. 
	 */

	if (!(flag & (FNDELAY|FNONBLOCK))) {

		while (tp->t_state & CARR_ON) {

			TP_TO_TOID(tp) = 0;
				
			/* 
			 * Test for data in output queues and buffers. 
			 */

			if ((!tp->t_out.bu_bp || !tp->t_out.bu_cnt) && 
				!(WR(q)->q_first) && 
				!HW_PROC(tp, T_DATAPRESENT) && 
				!(tp->t_state & BUSY))
				break;

			/* 
			 * TTIOW is waiting for write to complete state. It is used to 
			 * generate the SV_EVENT() in iasy_output() if no data is returned 
			 * (by iasy_output()), hence don't call iasy_output() if there's 
			 * data waiting to go that isn't picked up by T_DATAPRESENT. 
			 */

			tp->t_state |= TTIOW;

			/* 
			 * Schedule a timeout period ( CL_TIME ) after which the close 
			 * resamples the write queue state (in case data drained without 
			 * sending an SV_EVENT (didn't call iasy_output). NOTE: NOT a 
			 * close(2) timeout, just a sample period. Will never timeout 
			 * since could be blocked by flow control.
			 */

			TP_TO_TOID(tp) = itimeout ( iasy_drain, (void *)tp, CL_TIME, plstr);

			(void) LOCK(iasy_lock, plstr);

			/* 
			 * Sleep on the iasv_drain event. This is generated by the 
			 * iasy_output or the iasv_drain event; if the sleep is 
			 * interrupted, then cancel timeout and quit.
			 */

			if (!SV_WAIT_SIG((TP_TO_SV(tp))->iasv_drain, TTOPRI,iasy_lock)) { 

				/* 
				 * Interrupted: abnormal WAIT termination. Clear timeout. 
				 */

				tp->t_state &= ~TTIOW; 
				if (TP_TO_TOID(tp)) { 
					untimeout(TP_TO_TOID(tp)); 
					TP_TO_TOID(tp) = 0;
				}
				break;
			}	
		}

	} 

	/* 
	 * If HUPCL mode, hangup the modem lines on close. The 
	 * T_DISCONNECT will drop DTR, RTS. 
	 */

	if (tp->t_cflag & HUPCL) 
		(void) HW_PROC (tp, T_DISCONNECT);

	/* 
	 * Calling flush of the RD(q) to clear upstream queues.
	 * Note that WR(q) should have drained, or we should flush it 
	 * anyway, since we interrupted the drain? 
	 */

	iasyflush(WR(q), FLUSHR);
	tp->t_state &= ~ISOPEN; 

	ASSERT(!(tp->t_state & (BUSY|TIMEOUT)));

	/*
	 * switch off put and srv routines.
	 */

	qprocsoff(q);
	(void) HW_PROC(tp, T_LASTCLOSE);

	/* Clear local buffer pointers	*/
	if (tp->t_in.bu_bp) {
		freeb((mblk_t *)tp->t_in.bu_bp);	
		tp->t_in.bu_bp  = 0;
		tp->t_in.bu_ptr = 0;
		tp->t_in.bu_cnt = 0;
	}

	if (tp->t_out.bu_bp) {
		freeb((mblk_t *)tp->t_out.bu_bp);	
		tp->t_out.bu_bp  = 0;
		tp->t_out.bu_ptr = 0;
		tp->t_out.bu_cnt = 0;
	}

	/* 
	 * Clear all flags and pointers, ready for next open.
	 */

	tp->t_rdqp = NULL;
	tp->t_iflag = 0;
	tp->t_oflag = 0;
	tp->t_cflag = 0;
	tp->t_lflag = 0;
	tp->t_state = 0;

	q->q_ptr = WR(q)->q_ptr = NULL;

	splx(oldpri);

	return (0);
}


/*
 * STATIC void
 * iasydelay(struct strtty *)
 *	Resume output after a delay
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
iasydelay(struct strtty *tp)
{	
	pl_t	s;

	s = SPL();

	tp->t_state &= ~TIMEOUT;
	(void) HW_PROC(tp, T_OUTPUT);

	splx(s);
}

/*
 * STATIC void
 * iasyputioc(queue_t *, mblk_t *)
 *	ioctl handler for output PUT procedure
 *
 * Calling/Exit State:
 *	None.
 */

STATIC void
iasyputioc(queue_t *q, mblk_t *bp)
{	
	struct strtty *tp;
	struct iocblk *iocbp;
	mblk_t *bp1;

	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_rptr;

	tp = Q_TO_TP(q);

	switch (iocbp->ioc_cmd) {

	/* 
	 * Queue the hardware ioctls behind the draining output, and 
	 * flush any unread input on completion. 
	 */

	case SETRTRLVL:
	case TCSETSW:
	case TCSETSF:
	case TCSETAW:
	case TCSETAF:
	case TCSETXF:
	case TCSETXW:
	case TCSBRK:	
		/* 
		 * Test for data in the output queues and buffers.
		 */
		if (q->q_first != NULL || (tp->t_state & (BUSY|TIMEOUT|TTSTOP)) ||
												HW_PROC(tp,T_DATAPRESENT)) { 
			(void) putq(q, bp);
		} else {
			iasysrvioc(q, bp);
		}
		break;
	
	/* 
	 * These ioctl calls are executed immediately: they may deliberately
	 * destroy the queue data.
	 */

	case TCSETA:
	case TCSETS:
	case TCGETA:
	case TCGETS:
		iasysrvioc(q, bp);	
		break;

	case TIOCSTI: { /* Simulate typing of a character at the terminal. */
		mblk_t *mp;

		/*
		 * The permission checking has already been done at the stream
		 * head, since it has to be done in the context of the process
		 * doing the call.
		 */
		if ((mp = allocb(1, BPRI_MED)) != NULL) {
			if (!canputnext(RD(q)))
				freemsg(mp);
			else {
				*mp->b_wptr++ = *bp->b_cont->b_rptr;
				putq(tp->t_rdqp, mp);
			}
		}

		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		putnext(RD(q), bp);
		break;
	}

	case EUC_MSAVE:
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	/*
 	 * ioctls related to control of the modem signals.
	 */
	case TCSETX:
	case TCGETX:
	case TIOCMGET:		/* Get current state of Modem status lines */
	case TIOCMSET:		/* Change the state of modem lines */
	case TIOCMBIC:		/* Reset the RTS and/or DCD lines */
	case TIOCMBIS:		/* Set the RTS and/or DCD lines */
		(*(TP_TO_HW(tp)->hwdep))(q, bp, tp);
		break;

	default:
		if ((iocbp->ioc_cmd & IOCTYPE) != LDIOC) {
			/* Handle in service routine. */
			putq(q, bp);
			return;
		}

		/* ignore LDIOC cmds */
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;
	}
}


/*
 * STATIC int
 * iasyoput(queue_t *, mblk_t *)
 *	A message has arrived for the output q
 *
 * Calling/Exit State:
 *	- Return 0 on success.
 */

STATIC int
iasyoput(queue_t *q, mblk_t *bp)
{	
	mblk_t	*bp1;
	struct strtty *tp;
	pl_t	s;


	tp = Q_TO_TP(q);

	s = SPL();

	switch (bp->b_datap->db_type) {

	/* 
	 * Breaks multiple M_DATA block messages into several M_DATA
	 * blocks (preserves oredering). This allows the queue consumer
	 * to grab a block at a time and place it on the asyc(7) data 
	 * queue, without following links etc.
	 */

	case M_DATA:
		while (bp) {		/* Normalize the messages */
			bp->b_datap->db_type = M_DATA;
			bp1 = unlinkb(bp);
			bp->b_cont = NULL;
			/* Check valid data is available.	*/
			if (!((bp->b_wptr - bp->b_rptr) > 0)) {
				freeb(bp);
			} else {
				(void) putq(q, bp);
			}
			/* bp points to new head of multisegment message	*/
			bp = bp1;
		}

		(void) HW_PROC(tp, T_OUTPUT);	/* Start output */
		break;

	/* 
	 * ioctl(2) calls can be for immediate execution or blocked until
	 * output data has drained, to avoid changing the link mode 
	 * intra message. The blocked ioctls wait until there's no data 
	 * in the output queue, or in asyc(7) output buffers.
	 */

	case M_IOCTL:
		iasyputioc(q, bp);				/* Queue it or do it */
		(void) HW_PROC(tp, T_OUTPUT);	/* just in case */
		break;

	case M_FLUSH:
#if FLUSHRW != (FLUSHR|FLUSHW)
		/*
		 *+ Incorrect M_FLUSH implementation assumption.
		 */
		cmn_err(CE_PANIC, 
			"iasyoput: implementation assumption botched\n");
#endif
		switch (*(bp->b_rptr)) {
		case FLUSHRW:
			iasyflush(q, (FLUSHR|FLUSHW));
			freemsg(bp);	/* iasyflush has done FLUSHR */
			break;
		case FLUSHR:
			iasyflush(q, FLUSHR);
			freemsg(bp);	/* iasyflush has done FLUSHR */
			break;
		case FLUSHW:
			iasyflush(q, FLUSHW);
			freemsg(bp);
			break;
		default:
			freemsg(bp);
			break;
		}
		break;

	/* 
	 * Continue transmitting data to remote.
	 */

	case M_START:
		(void) HW_PROC(tp, T_RESUME);
		freemsg(bp);
		break;

	/* 
	 * Upstream transmikt flow control messages. 
	 */

	case M_STOP:
		(void) HW_PROC(tp, T_SUSPEND);
		freemsg(bp);
		break;

	/* 
	 * The M_BREAK message must be handled in sequence, ie. need to ensure
	 * that all the preceeding data is in the ISR obuf so that the HW_PROC 
	 * can tag the BREAK position (and asyc_txsvc() can cause it). Hence 
	 * the tests for queue empty and strtty t_out STREAMS buffer empty.
	 */

	case M_BREAK:
		if (q->q_first != NULL || (tp->t_out.bu_bp && tp->t_out.bu_cnt)) {
			(void) putq(q, bp);
			break;
		}
		(void) HW_PROC(tp, T_BREAK);	/* Do break now */
		freemsg(bp);
		break;

	case M_DELAY:
		tp->t_state |= TIMEOUT;
		(void) timeout((void(*)())iasydelay, (caddr_t)tp, 
					(int)*(bp->b_rptr));
		freemsg (bp);
		break;

	/* 
	 * Send a VSTART to the remote to restart its transmit flow control.
	 */

	case M_STARTI:
#ifdef DEBUG 
		cmn_err(CE_WARN,"M_STARTI sending VSTOP"); 
#endif 
		(void) HW_PROC(tp, T_UNBLOCK);
		freemsg(bp);
		break;

	/* 
	 * Send a VSTOP to the remote to halt its transmit. 
	 */

	case M_STOPI:
#ifdef DEBUG 
		cmn_err(CE_WARN,"M_STOPI sending VSTOP"); 
#endif 
		(void) HW_PROC(tp, T_BLOCK);
		freemsg(bp);
		break;


	/* 
	 * M_CTL is part of the function separation negotiation that occurs 
	 * between the iasy and ldterm modules. The iasy MUST do all the 
	 * cflag tasks, wholst ldterm does all of the oflag and lflag tasks. 
	 * They set bits in iflag to indicate the division. However, for some
	 * tasks (eg. parity testing) there is insufficient data for ldterm
	 * to perform the processing, so iasy has no choice. Basically this is
	 * a real mess : no standards for M_CTL, and MC_CANONQUERY etc. The 
	 * only way is to look at the code in ldterm/iasy/asyc. 
	 * Related unanswered questions: what happens when a module is I_POPped
	 * as is the case for serial mouse processing : shouldn't the I_POPped
	 * module indicate to its downstream neighbour that some offloaded 
	 * processing is coming back ? What happens if several modules all do 
	 * similar processing but at different non-adjacent levels ? 
	 * Should an iasy/asyc driver support ALL possible flags, and merely 
	 * turn many off when placed as the driver in a terminal stack ? 
	 */

	case M_CTL: {
		struct termios *termp;
		struct iocblk *iocbp;

		if ((bp->b_wptr - bp->b_rptr) != sizeof(struct iocblk)) {
			freeb(bp);
			break;
		}

		/* LINTED pointer alignment */
		iocbp = (struct iocblk *)bp->b_rptr;

		if (iocbp->ioc_cmd  == MC_CANONQUERY ) {
			if ((bp1 = allocb(sizeof(struct termios), BPRI_HI)) == 
						(mblk_t *) NULL) {
				freeb(bp);
				break;
			}

			bp->b_datap->db_type = M_CTL;
			iocbp->ioc_cmd = MC_PART_CANON;
			bp->b_cont = bp1;
			bp1->b_wptr += sizeof(struct termios);
			/* LINTED pointer alignment */
			termp = (struct termios *)bp1->b_rptr;

			termp->c_iflag = ISTRIP | IXON | IXANY;
			/* c_cflag is ignored by ldterm 	*/
			termp->c_cflag = 0;	
			termp->c_oflag = 0;
			termp->c_lflag = 0;
			qreply(q, bp);
		} else
			freemsg(bp);

		break;
	}

	case M_IOCDATA:
		/*
		 * HW dep ioctl data has arrived 
		 */
		(*(TP_TO_HW(tp)->hwdep))(q, bp, tp);
		break;

	default:
		freemsg(bp);
		break;
	}

	splx(s);

	return (0);
}


/*
 * STATIC mblk_t *
 * iasygetoblk(struct queue *)
 *
 * Calling/Exit State:
 *	Called at pl_t < plstr. Raises pl_t to plstr, preserves entry pl_t.
 *
 * Remarks:
 *	Strips out M_IOCTL and M_BREAK messages and executes them.
 * 
 */

STATIC mblk_t *
iasygetoblk(struct queue *q)
{	
	struct strtty *tp;
	pl_t	s;
	mblk_t	*bp;

	s = SPL();

	tp = Q_TO_TP(q);

	if (!tp) {	/* This can happen only if closed while no carrier */
		splx(s);
		return (0);
	}

	while ((bp = getq(q)) != NULL) {

		switch (bp->b_datap->db_type) {

			case M_DATA:
				splx(s);
				return(bp);
	
			/* 
			 * All ioctl(2)s that are executed immediately, without regard 
			 * to the state of the UART hardware or the driver have already 
			 * been processed by iasyoput() calling iasysrvioc(). The rest 
			 * require the output to have drained, so are not called until 
			 * the UART is idle and there is no ISR pending data. 
			 */

			case M_IOCTL:
				if (!(tp->t_state & (TTSTOP|BUSY|TIMEOUT)) && 
						!HW_PROC (tp, T_DATAPRESENT) ) { 
					/* 
					 * Should be no data in the strtty buffer if !BUSY.
					 */
					ASSERT(!tp->t_out.bu_cnt || !tp->t_out.bu_bp); 
					iasysrvioc(q, bp);		
				} else {
					(void)putbq(q, bp);
					splx(s);
					return (0);
				}
				break;
			

			/* 
			 * The M_BREAK is handled by placing a tag into the output
			 * buffer to maintain its use as an ordering device. Allow the 
			 * HW_PROC(T_BREAK) to set this data at any time after all the 
			 * data ahead of it has been placed into the obuf (ie. there is
			 * no data on the WR(q) or the strtty tp->tout hooks.)
			 */

			case M_BREAK:
				if ((q->q_first != NULL) || 
						(tp->t_out.bu_bp && tp->t_out.bu_cnt)) {
					(void)putbq(q, bp);
					splx(s);
					return (0);
				} else {
					(void) HW_PROC(tp, T_BREAK);
					freemsg(bp);
				}
				break;

			default:
				/* Ignore junk mail */
				freemsg(bp);
				break;

		} 

	}	

	splx(s);
	return (0);
}


/*
 * STATIC void 
 * iasysrvioc(queue_t *, mblk_t *)
 *	Routine to execute ioctl messages.
 *
 * Calling/Exit State:
 *	None. 
 *
 * Remarks.
 *	Called after the output has drained. 
 *
 */

STATIC void
iasysrvioc(queue_t *q, mblk_t *bp)
{	
	struct strtty *tp;
	struct iocblk *iocbp;
	int	arg;
	pl_t	s;
	mblk_t	*bpr;
	mblk_t	*bp1;
	int	return_val;

#ifdef IASY_DEBUG 
	if (iasy_debug & IDSP_IOCTL)
		cmn_err(CE_CONT,"Rcvd M_IOCTL");  
#endif 

	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_rptr;
	tp = Q_TO_TP(q);

#ifdef IASY_DEBUG 
	if (iasy_debug & IDSP_IOCTL) { 
		cmn_err(CE_CONT,"cmd 0x%x id 0x%x ",
				iocbp->ioc_cmd, iocbp->ioc_id);  
		if ( iocbp->ioc_count == TRANSPARENT ) {  
			cmn_err(CE_CONT, " TRANSPARENT ");
			if (bp->b_cont)
				cmn_err(CE_CONT, "uaddr 0x%x\n", bp->b_cont->b_rptr);
		} else { 
			cmn_err(CE_CONT, " I_STR ");
			if (bp->b_cont)
				cmn_err(CE_CONT, "%dB at 0x%x\n", iocbp->ioc_count, 
														bp->b_cont->b_rptr);
		} 
	} 
#endif 

	switch (iocbp->ioc_cmd) {

	case TCSETAF:
		iasyflush(q, FLUSHR);
		/* FALLTHROUGH */

	case TCSETA:
	case TCSETAW: {
		struct termio *cb;

		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		/* LINTED pointer alignment */
		cb = (struct termio *)bp->b_cont->b_rptr;
		tp->t_cflag = (tp->t_cflag & 0xffff0000 | cb->c_cflag);
		tp->t_iflag = (tp->t_iflag & 0xffff0000 | cb->c_iflag);
		bcopy((caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCC);

		s = SPL();

		if (HW_PROC(tp, T_PARM)) {
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		splx(s);

		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	}

	// TCSETSF flushes RD(q) data ?? Is that right ? Whats the point ? 

	case TCSETSF:
			iasyflush(q, FLUSHR); 
			/* FALLTHROUGH */
	case TCSETS:
	case TCSETSW: {
		struct termios *cb;

		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		/* LINTED pointer alignment */
		cb = (struct termios *)bp->b_cont->b_rptr;

		tp->t_cflag = cb->c_cflag;
		tp->t_iflag = cb->c_iflag;
		bcopy((caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCCS);

		s = SPL();

		if (HW_PROC(tp, T_PARM)) {
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		splx(s);

		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;
	}

	case TCGETA: {	/* immediate parm retrieve */
		struct termio *cb;

		if (bp->b_cont)	/* Bad user supplied parameter */
			freemsg(bp->b_cont);

		if ((bpr = allocb(sizeof(struct termio), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void) putbq(q, bp);
			(void) bufcall((ushort)sizeof(struct termio),
						BPRI_MED, iasydelay, (long)tp);
			return;
		}

		bp->b_cont = bpr;
		/* LINTED pointer alignment */
		cb = (struct termio *)bp->b_cont->b_rptr;
		cb->c_iflag = (ushort)tp->t_iflag;
		cb->c_cflag = (ushort)tp->t_cflag;
		cb->c_cflag &= ~CIBAUD;
		cb->c_cflag |= (tp->t_cflag&CBAUD)<<IBSHIFT;
		bcopy ((caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCC);
		bp->b_cont->b_wptr += sizeof(struct termio);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termio);
		(void) putnext(RD(q), bp);
		break;

	}

	case TCGETS: {	/* immediate parm retrieve */
		struct termios *cb;

		if (bp->b_cont)	/* Bad user supplied parameter */
			freemsg(bp->b_cont);

		if ((bpr = allocb(sizeof(struct termios), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void) putbq(q, bp);
			(void) bufcall((ushort)sizeof(struct termios),
						BPRI_MED, iasydelay, (long)tp);
			return;
		}

		bp->b_cont = bpr;
		/* LINTED pointer alignment */
		cb = (struct termios *)bp->b_cont->b_rptr;
		cb->c_iflag = tp->t_iflag;
		cb->c_cflag = tp->t_cflag;
		bcopy((caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCCS);
		bp->b_cont->b_wptr += sizeof(struct termios);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termios);
		(void) putnext(RD(q), bp);
		break;
	}

	/*
	 * TCSBRK with any non zero data is a null ioctl, used 
	 * to achieve an indication of output data drain. 
	 */

	case TCSBRK:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		
		arg = *(int *)bp->b_cont->b_rptr;
		if (arg == 0) {
			s = SPL();
			(void) HW_PROC(tp, T_BREAK);
			splx(s);
		}

		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	case SETRTRLVL:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		s = SPL();

		/* LINTED pointer alignment */
		switch (*(int *)bp->b_cont->b_rptr) {
		case T_TRLVL1: 
			return_val = HW_PROC(tp, T_TRLVL1);
			break;
		case T_TRLVL2: 
			return_val = HW_PROC(tp, T_TRLVL2);
			break;
		case T_TRLVL3: 
			return_val = HW_PROC(tp, T_TRLVL3);
			break;
		case T_TRLVL4: 
			return_val = HW_PROC(tp, T_TRLVL4);
			break;
		default:
			return_val = 1;
			break;
		}

		if (return_val) {
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		splx(s);

		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	case EUC_MSAVE:	/* put these here just in case... */
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

		/*
		 * This ioctl is treated as unknown to iasy module along
		 * with the other termiox ioctls (TCSETX, TCGETX & TCSETXW).
		 */
	case TCSETXF:
		iasyflush(q, FLUSHR);
		/* FALLTHROUGH */

	default:
		/*
		 * Unknown ioctls are either intended for the hardware 
		 * dependent code or an upstream module that is not 
		 * present. Pass the request to the HW dependent code
		 * to handle it.
		 */
		(*(TP_TO_HW(tp)->hwdep))(q, bp, tp);
		break;
	}
}


/*
 * STATIC void 
 * iasyflush(queue_t *, int) 
 *	Flush input and/or output queues
 *
 * Calling/Exit State:
 *	None.
 */

STATIC void
iasyflush(queue_t *q, int cmd)
{	
	struct strtty *tp;
	pl_t	s;


	s = SPL();

	tp = Q_TO_TP(q);

	if (cmd & FLUSHW) {
		(void) HW_PROC(tp, T_WFLUSH);
		flushq(q, FLUSHDATA);
	}

	if (cmd & FLUSHR) {
		q = RD(q);
		(void) HW_PROC(tp, T_RFLUSH);
		flushq(q, FLUSHDATA);
		(void) putnextctl1(q, M_FLUSH, FLUSHR);
	}

	splx(s);
}


/*
 * STATIC int
 * iasyisrv(queue_t *)
 *	New service procedure. Pass everything upstream.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
iasyisrv(queue_t *q)
{	
	mblk_t	*mp;
	struct strtty *tp;
	pl_t	s;


	tp = Q_TO_TP(q);

	s = SPL();

	while ((mp = getq(q)) != NULL) {
		/*
		 * If we can't put, then put it back if it's not
		 * a priority message.  Priority messages go up
		 * whether the queue is "full" or not.  This should
		 * allow an interrupt in, even if the queue is hopelessly
		 * backed up.
		 */
		if (!canputnext(q)) {
			(void) putbq(q, mp);
			splx(s);
			return (0);
		}

		(void) putnext(q, mp);

		if (tp->t_state & TBLOCK) {
			(void) HW_PROC(tp, T_UNBLOCK);
		}
	}

	splx(s);

	return (0);
}


/*
 * STATIC int
 * iasyosrv(queue_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
iasyosrv(queue_t *q)
{
	return (0);
}


/*
 * int
 * iasy_input(struct strtty *, unsigned int)
 *	Modify your interrupt thread to use this routine instead of l_input. It
 *	takes the data from tp->t_in, ships it upstream to the line discipline,
 *	and allocates another buffer for tp->t_in.
 *
 * Calling/Exit State:
 *	- Return 0, if there are no character in the strtty input buffers.
 *	- Return 1, if there is no upstream queue or no memory.
 *	- Return cnt, if there are cnt character in the strtty input buffer. 
 *	- Call at pl < plstr, lest we interrupt STREAMS.
 *
 * Description:
 *	Device with input to report 
 *	BUF or L_BREAK 
 */
int
iasy_input(struct strtty *tp, unsigned int cmd)
{	
	queue_t	*q;
	mblk_t	*bp;
	int	cnt;


	q = TP_TO_Q(tp);
	if (!q)
		return(1);

	switch (cmd) {

	case L_BUF:
		cnt = IASY_BUFSZ - tp->t_in.bu_cnt;
		if (cnt && canputnext(q)) {
			bp = allocb(IASY_BUFSZ, BPRI_MED);
			if (bp) {	/* pass up old bp contents */
				tp->t_in.bu_bp->b_wptr += cnt;
				tp->t_in.bu_bp->b_datap->db_type = M_DATA;
				(void) putnext(q, tp->t_in.bu_bp);
				tp->t_in.bu_bp = bp;
				/* Reset to go again */
				tp->t_in.bu_cnt = IASY_BUFSZ;
				tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
			} else
				return (1);
		} else
			return (cnt);
		return (0);

	/* 
	 * Upstream, ldterm handles the M_BREAK message as specified 
	 * taing account of the IGNBRK, BRKINT, PARMRK, and ISIG flags,  
	 * ie. it sends a SIGINT, \0, \0377 \0 \0, or nothing upstream.
	 * Note that no explicit flag setting in MC_CANONQUERY would 
	 * seem to describe this behaviour. 
	 */

	case L_BREAK:
		/* signal "break detected" */
		(void) putnextctl(q, M_BREAK);
		break;

	default:
		/*
		 *+ An unknown command is received.
		 */
		cmn_err(CE_WARN, 
			"iasy_input: unknown command\n");
		break ; 	
	}
	
	return (0);
}


/*
 * int
 * iasy_output(struct strtty *)
 *	Modify your interrupt thread to use this routine instead of l_output.
 *	It retrieves the next output block from the stream and hooks it into
 *	tp->t_out.
 *
 * Calling/Exit State:
 *	Call at pl < plstr, lest we interrupt STREAMS
 */

int
iasy_output(struct strtty *tp)
{	
	queue_t	*q;
	mblk_t	*bp;
	pl_t	ipl; 

	if (tp->t_out.bu_bp) {			/* Previous message block	*/
		freeb((mblk_t *)tp->t_out.bu_bp);
		tp->t_out.bu_bp = 0;
		tp->t_out.bu_ptr = 0;
		tp->t_out.bu_cnt = 0;
	}

	q = TP_TO_Q(tp);

	if (!q)
		return(0);
	q = WR(q);

	bp = iasygetoblk(q);
	

	if (bp) {
		/* Debug 	*/
		ASSERT( bp->b_rptr );
		ASSERT( bp->b_wptr );
		ASSERT((bp->b_wptr - bp->b_rptr) > 0); 
		/* Messages are single M_DATA blocks */
		tp->t_out.bu_ptr = bp->b_rptr;
		tp->t_out.bu_cnt = bp->b_wptr - bp->b_rptr;
		tp->t_out.bu_bp = bp;
		return(CPRES);
	}

	/* 
	 * If iasyclose is waiting for the iasv_drain signal (TTIOW) 
	 * ensure the data has all gone, else the driver state found 
	 * by the last character timeout (asyc_idle() ) can be fairly 
	 * dead!
	 */ 

	if (tp->t_state & TTIOW) { 
		if ((q->q_first == NULL) && !(tp->t_state & (BUSY|TIMEOUT))) {
			if (TP_TO_TOID(tp)) {
				untimeout(TP_TO_TOID(tp));
				TP_TO_TOID(tp) = 0;
			}
		}
	
		/* 
	 	 * Send iasv_drain signal, unblock iasyclose(). 
	 	 */

		tp->t_state &= ~TTIOW;
		SV_SIGNAL((TP_TO_SV(tp))->iasv_drain, 0);
	}

	return(0);
}

/*
 * int
 * iasy_register(minor_t, int, int (*)(), void (*)(), int (*)(), void (*)(),
 *		 conssw_t *);
 *
 * 
 * Calling/Exit State:
 *	- fmin		Starting minor number 
 *	- count		Number of units(ports).
 *	- (*proc)()	proc routine 
 *	- (*hwdep)()	Hardware dependant ioctl routine 
 *	- conssw_t *	System console routines structure.
 *
 * Remarks:
 *	iasy_register allows various hardware drivers to register themselves
 *	for use with the iasy supervisory module. Each driver defines both 
 *	proc & hwdep functions to co-process serial driver actions, and to 
 *	perform hardware component of ioctl(2)s. Also defines a structure of
 *	conssw containing all system console support functions: allows the 
 *	driver to acts as a system console (/dev/syscon).
 * 
 * 	If we are initialising as a side effect of being a system console, 
 *	then cmn_err(D3) wont work.
 *
 */

int
iasy_register(minor_t reqmin, int units, int (*proc)(), void (*hwdep)(), 
	      conssw_t *cnswp)
{	
	int		i, req_unit, max_req_unit;

	/* 
	 * Verify input arguments from the hardware driver. Drivers have 
	 * tuneable minor numbers to obviate conflicts. Maximum number is 
	 * configured in IASY_UNITS / iasy_num (set in Space.c).
	 */

	req_unit = IASY_MINOR_TO_UNIT(reqmin);

	if ((req_unit < 0) || (req_unit >= iasy_num)) { 
		/*
		 *+ The serial hardware driver has attempted to register 
		 *+ a serial port at a bad minor number.
		 */
		cmn_err( CE_WARN, "!iasy_register: bad registration, minor %d", 
							reqmin);
		return (-1);
	} 
	
	if (units <= 0)	{ 
		/*
		 *+ The serial hardware driver has attempted to register
		 *+ a zero/negative number of ports.
		 */
		cmn_err(CE_WARN, "!iasy_register: bad device count  %d", units);
		return (-1);
	} 

	/*
	 * proc/hwdep used in processing. cnswp indicates that the device is
	 * console capable - this is not necessarily so for all serial devices.
	 * Relax conditions to allow NULL conssw_t ? 
	 */

	if ((proc == 0) || (hwdep == 0) || (cnswp == 0)){ 
		/*
		 *+ The serial hardware driver has passed bad routine addresses
		 *+ for handling console IO operations.
		 */
		cmn_err(CE_WARN, "!iasy_register: NULL routine addresses" ); 
		return (-1);
	} 

	/* 
	 * Calculate requested iasy_hw[] field range.
	 */

	max_req_unit = req_unit + units - 1; 

	/* 
	 * Check table size not exceeded. 
	 */

	if (max_req_unit >= iasy_num) {
		/*
		 *+ The serial hardware driver has attempted to register 
		 *+ more ports than the kernel is configured for. All fail.
		 */
		cmn_err(CE_WARN, "!iasy_register: minor %d is out of range\n", 
					IASY_UNIT_TO_MINOR(max_req_unit));
		return (-1);
	}

	/* 
	 * Check all slots in range are free, and set them if so. If there
	 * is a single collision then fail the request. Callers can register
	 * devices singly if required. Callers must deregister prior to 
	 * reregistering at a given port slot.
	 */
	
	for (i = req_unit; i <= max_req_unit; i++ ){

		if ( iasy_hw[i].proc ) { 

			/*
			 *+ The minor number range for the serial port device
			 *+ conflicts with a previously registered device.
			 */

			cmn_err(CE_WARN, 
			"!iasy_register: minor %d conflict  0x%x vs 0x%x\n",
				IASY_UNIT_TO_MINOR(i), iasy_hw[i].proc, proc);
			return (-1);
		}
	}

	for (i = req_unit; i <= max_req_unit; i++ ){
		iasy_hw[i].proc = proc;
		iasy_hw[i].hwdep = hwdep;
		iasy_hw[i].consswp = cnswp;
	}

	/* 
	 * Bump count of iasy devices for use by crash(1M). We registered 
	 * all or none. iasy_cnt used hereafter as a device total count.
	 */

	iasy_cnt += units;

	return(req_unit);
}

/*
 * int
 * iasy_deregister(minor_t, int, int (*)())
 *
 * Calling/Exit State:
 *	- fmin		Starting minor number 
 *	- count		Number of minor devices requested for 
 *			deregister.
 *
 * Description:
 *	Deregister a terminal server. If the port is originally initialised 
 * 	in console xxinit() (very early), then it may have got some data 
 * 	out of line with config manager: allow deregister and reregister.
 */

int
iasy_deregister(minor_t minor, int units)
{	
	int	base, max, i;

	/* 
	 * Get unit to be deregistered.
	 */

	base = IASY_MINOR_TO_UNIT(minor);

	/* 
	 * Check parameters. 
	 */

	if ((base < 0) || (units <= 0) || ((max = (base+units-1)) > iasy_num)){
		/* 
		 *+ Invalid attempt to deregister a serial hardware device.
		 *+ Indicates a serial hardware driver software error.
		 */ 
		cmn_err(CE_WARN,
			"iasy_deregister: minor %d is out of range\n", 
					IASY_UNIT_TO_MINOR(max));

		return (-1);
	} 

	/*
	 * De-allocate the range of minor numbers and clear hw-dependent 
	 * structures. Flag empty entries.
	 */

	for (i = base ; i <= max ; i++) {

		if (!iasy_hw[i].proc) { 
#ifdef IASY_DEBUG 
			if (iasy_debug & IDSP_REG) 
				cmn_err(CE_WARN, "iasy_deregister: minor %d unregistered",
											IASY_UNIT_TO_MINOR(i));
#endif 
		} 

		iasy_hw[i].proc = 0;
		iasy_hw[i].hwdep = 0;
		iasy_hw[i].consswp = 0;

	}

	iasy_cnt -= units;
	return(0);
}


/*
 * void
 * iasyhwdep(queue_t *, mblk_t *, struct strtty *)
 *	Default Hardware dependent ioctl support (i.e. none).
 *	Use this routine as your hwdep() routine if you don't have any
 *	special ioctls to implement.
 *
 * Calling/Exit State:
 *	- q points to the write queue pointer.
 *	- bp contains the ioctl or ioctl data not understood by the DI code.
 */

/* ARGSUSED */

void
iasyhwdep(queue_t *q, mblk_t *bp, struct strtty *tp)
{	
	struct iocblk *ioc;


	/* LINTED pointer alignment */
	ioc = (struct iocblk *)bp->b_rptr;

	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		ioc->ioc_error = EINVAL;	/* NAK unknown ioctls */
		ioc->ioc_rval = -1;
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return;

	default:
		/*
		 *+ An unknown message type.
		 */
		cmn_err(CE_PANIC, 
			"iasyhwdep: illegal message type");
	}
}


/*
 * void
 * iasy_hup(struct strtty *)
 *	Send a hangup upstream to indicate loss of the connection.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Called when we lose CARR_ON if !CLOCAL. 
 */

void
iasy_hup(struct strtty *tp)
{	
	queue_t *q;
	pl_t	s;


	q = TP_TO_Q(tp);
	if (!q)
		return;

	s = SPL();

	iasyflush(WR(q), FLUSHR|FLUSHW);
	(void) putnextctl(q, M_HANGUP);

	splx(s);
}

/*
 * void
 * iasy_carrier(struct strtty *)
 *	wakeup/signal a process to indicate the establishment of connection.
 *
 * Calling/Exit State:
 *	None.
 */

void
iasy_carrier(struct strtty *tp)
{	
	SV_SIGNAL((TP_TO_SV(tp))->iasv_carrier, 0);
}

/* 
 * Note: iasycn*() routines to implement console device are defined 
 * only if the asyc(7) driver is not acting as a console directly. 
 * These routines assume that all hardware will be capable of acting 
 * as a console (the lack of system services may cause problems for 
 * some implementations, and the routines do not perform any common 
 * processing. 
 */

#ifndef ASYC_CONSOLE_DIRECT

/*
 * STATIC dev_t
 * iasycnopen(minor_t, boolean_t, const char *)
 *
 * Calling/Exit State:
 *	Called at init_console() in sysinit(), pre io_init(D2DK) 
 *	No cmn_err, ASSERTs, display output
 *	If device can display then return dev_t to address it by (or NODEV).
 *	The console devices (/dev/systty/syscon/sysmsg/console) set to dev_t
 *	returned.
 *
 * Remarks:
 *	Pass the request on to the hardware driver console routine specified 
 *	by minor. NB. iasyinit() ensures hardware drivers registered.
 *
 */

STATIC dev_t
iasycnopen(minor_t minor, boolean_t syscon, const char *params)
{
	struct iasy_hw *hp;

	if (!iasy_initialized)
		iasyinit();

	hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	if (IASY_MINOR_TO_UNIT(minor) > iasy_cnt || hp->consswp == NULL)
		return(NODEV);

	return((*hp->consswp->cn_open)(minor, syscon, params));
}


/*
 * STATIC void
 * iasycnclose(minor_t, boolean_t)
 *
 * Calling/Exit State:
 */

STATIC void
iasycnclose(minor_t minor, boolean_t syscon)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	(*hp->consswp->cn_close)(minor, syscon);

	return;
}

		
/*
 * STATIC int
 * iasycngetc(minor_t)
 * 
 * Calling/Exit State:
 *	- Return -1, if its an illegal minor number, otherwise
 *	  return the character.
 */

STATIC int
iasycngetc(minor_t minor)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

#ifdef IASY_DEBUG 
	/* Testing console handling routines	*/
	static int off = 0; 
	if (iasy_debug & IDSP_DOGETC) { 
		off = (off + 1)% 26;
		if (off == 0) 
			return (-1);
		else 
			return ('A' + off);
	}
#endif

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	return((*hp->consswp->cn_getc)(minor));
}


/*
 * STATIC int 
 * iasycnputc(minor_t, int)
 *
 * Calling/Exit State:
 *	- Returns 1 if OK, else 0.
 */

STATIC int
iasycnputc(minor_t minor, int c)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	return((*hp->consswp->cn_putc)(minor, c));
}


/*
 * STATIC void
 * iasycnsuspend(minor_t)
 *
 * Calling/Exit State:
 *	Called at plhi, all interrupts disabled.
 *
 * Remarks
 *	Called by basic console handling code, prior ro any call to the 
 *	console_getc() function. Allow any previous pending actions to 
 *	complete, then return.
 */

STATIC void
iasycnsuspend(minor_t minor)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	(*hp->consswp->cn_suspend)(minor);
}


/*
 * STATIC void
 * iasycnresume(minor_t)
 *
 * Calling/Exit State:
 *	Called at plhi, all interrupts disabled.
 *
 * Remarks
 *	Called by basic console handling code, after any call to the 
 *	console_getc() function. Allow any previous pending actions to 
 *	complete, then return.
 */

STATIC void
iasycnresume(minor_t minor)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	(*hp->consswp->cn_resume)(minor);
}

#endif 		/* ASYC_CONSOLE_DIRECT */

#ifdef IASY_DEBUG 

/*
 * void
 * print_iasy(int unit)
 *	Formatted print of strtty structure. Can be invoked from
 *	kernel debugger.
 *
 * Calling/Exit State:
 *	None.
 */

void
print_iasy(struct strtty *tp)
{
        cmn_err(CE_CONT, "\n STREAM strtty struct: size=0x%x(%d)\n",
                sizeof(struct strtty), sizeof(struct strtty));
        cmn_err(CE_CONT, "\tt_in.bu_bp=0x%x, \tt_in.bu_ptr=0x%x\n",
                tp->t_in.bu_bp, tp->t_in.bu_ptr);
        cmn_err(CE_CONT, "\tt_in.bu_cnt=0x%x, \tt_out.bu_bp=0x%x\n",
                tp->t_in.bu_cnt, tp->t_out.bu_bp);
        cmn_err(CE_CONT, "\tt_out.bu_ptr=0x%x, \tt_out.bu_cnt=0x%x\n",
                tp->t_out.bu_ptr, tp->t_out.bu_cnt);
        cmn_err(CE_CONT, "\tt_rdqp=0x%x, \tt_ioctlp=0x%x\n",
                tp->t_rdqp, tp->t_ioctlp);
        cmn_err(CE_CONT, "\tt_lbuf=0x%x, \tt_dev=0x%x\n",
                tp->t_lbuf, tp->t_dev);
        cmn_err(CE_CONT, "\tt_iflag=0x%x, \tt_oflag=0x%x\n",
                tp->t_iflag, tp->t_oflag);
        cmn_err(CE_CONT, "\tt_cflag=0x%x, \tt_lflag=0x%x\n",
                tp->t_cflag, tp->t_lflag);
        cmn_err(CE_CONT, "\tt_state=0x%x, \tt_line=0x%x\n",
                tp->t_state, tp->t_line);
        cmn_err(CE_CONT, "\tt_dstat=0x%x, \tt_cc=%19x\n",
                tp->t_dstat, tp->t_cc);
}

/* 
 * int 
 * iasy_cmnerr(const char *, char * filename, int line); 
 *
 * Calling/Exit State:
 * 
 * Remarks:
 * 	Used in the ASSERT macro.
 */ 

int
iasy_cmnerr(const char *string, char * filename, int line)
{
	cmn_err(CE_PANIC,string,filename,line);
	return (1);
}


#endif 	/* IASY_DEBUG	*/

