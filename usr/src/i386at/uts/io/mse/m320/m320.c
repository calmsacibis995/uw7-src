#ident	"@(#)m320.c	1.19"
#ident	"$Header$"

/*
 * AT&T 320 Mouse Driver - Streams Version
 */


/*
 *	MODIFICATIONS
 *
 *	L000	12nov97		brendank@sco.com	MR: ul97-00907
 *		- Modified test that checks validity of first byte of 
 *		  mouse data packet.
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
#include <util/cmn_err.h>
#include <io/ws/chan.h>
#include <io/ws/8042.h>
#include <io/mouse.h>
#include <io/mse/mse.h>
#include <util/ksynch.h>

#include <io/ddi.h>	/* must come last */


#include <util/mod/moddefs.h>

#define	DRVNAME	"mse3 - Loadable 320 mouse driver"

int mse3_3bdly = 0;

STATIC int mse3_load(void); 
STATIC int mse3_unload(void);

MOD_DRV_WRAPPER(mse3, mse3_load, mse3_unload, NULL, DRVNAME);
extern void mod_drvattach(), mod_drvdetach();

/*
 * Wrapper functions.
 */

void	mse3start(void);

STATIC	int
mse3_load(void)
{
	cmn_err(CE_NOTE, "!MOD: in mse3_load()");

	mod_drvattach(&mse3_attach_info);
	mse3start();
	return (0);
}

STATIC	int
mse3_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in mse3_unload()");

	mod_drvdetach(&mse3_attach_info);
	return (0);
}


#define	MSE3HIER	1
#define	MSE3PL		plstr

/*
 * MSE_ANY used in 320 mouse command processing for case where we don't
 * care what return byte val is
 */
#define MSE_ANY		0xFE

#define	M_IN_DATA	0
#define	M_OUT_DATA	1
#define	SNDERR2		0xfc

#define DEBUG 1
#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int mse3_debug = 0;
#define DEBUG1(a)	if (mse3_debug == 1) printf a
#define DEBUG2(a)       if (mse3_debug >= 2) printf a /* allocations */
#define DEBUG3(a)       if (mse3_debug >= 3) printf a /* M_CTL Stuff */
#else
#define DEBUG1(a)
#define DEBUG2(a)
#define DEBUG3(a)
#endif /* DEBUG || DEBUG_TOOLS */


STATIC int	mse3open(queue_t *, dev_t *, int, int, struct cred *);
STATIC int	mse3close(queue_t *, int, struct cred *);
STATIC void	mse3ioc(queue_t *, mblk_t *);
STATIC int	mse3_wput(queue_t *, mblk_t *);
STATIC void	ps2parse(uchar_t);
STATIC int	snd_320_cmd(unsigned char, unsigned char, int, unchar *);
STATIC int	mcafunc(struct cmd_320 *);
STATIC void	clear_sickness(void);

extern void	mse_iocack(queue_t *, mblk_t *, struct iocblk *, int);
extern void	mse_iocnack(queue_t *, mblk_t *, struct iocblk *, int, int);
extern void	mse_copyout(queue_t *, register mblk_t *, register mblk_t *,
			uint, unsigned long);
extern void	mse_copyin(queue_t *, register mblk_t *, int, unsigned long);
extern void	mseproc(struct strmseinfo *);


struct module_info mse3_info = { 
	22, "mse3", 0, INFPSZ, 256, 128
};

STATIC struct qinit mse3_winit = {
	mse3_wput, NULL, NULL, NULL, NULL, &mse3_info, NULL
};

STATIC struct qinit mse3_rinit = {
	NULL, NULL, mse3open, mse3close, NULL, &mse3_info, NULL
};

struct streamtab mse3info = {
	&mse3_rinit, &mse3_winit, NULL, NULL
};


STATIC struct mcastat mcastat;
STATIC struct strmseinfo *mse3ptr;

int	mse3devflag = 0;
int	mse3closing = 0;

lock_t	*mse3_lock;		/* basic spin mutex lock */
sv_t	*mse3_closesv;		/* mse3_closing sync. variable */

LKINFO_DECL(mse3_lkinfo, "MSE:MSE3:mse3 mutex lock", 0);

extern int	i8042_has_aux_port;
extern int	i8042_aux_state;


/*
 * void
 * mse3start(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
mse3start(void)
{
	mse3_lock = LOCK_ALLOC(MSE3HIER, MSE3PL, &mse3_lkinfo, KM_SLEEP);
	mse3_closesv = SV_ALLOC(KM_SLEEP);

	mcastat.present = 0;

	/* check if 8042 has aux device */
	if (i8042_aux_port()) {
		DEBUG1(("msestart: thinks there's a 320\n"));
		mcastat.present = 1;
		mcastat.mode = MSESTREAM;

		i8042_program(P8042_AUXDISAB); /* we'll enable in open */
	}
}


/*
 * STATIC int
 * mse3open(queue_t *, dev_t *, int, int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
mse3open(queue_t *q, dev_t *devp, int flag, int sflag, struct cred *cred_p)
{
	struct cmd_320 mca;
	pl_t oldpri;


	if (q->q_ptr != NULL)
		return (0);

	if (mcastat.present != 1)
		return (ENXIO);

	oldpri = LOCK(mse3_lock, MSE3PL);
	while (mse3closing) {
		/* In SVR4.2 the sleep priority was PZERO+1 */
		if (!SV_WAIT_SIG(mse3_closesv, primed - 1, mse3_lock))
			return (EINTR);
		oldpri = LOCK(mse3_lock, MSE3PL);
	}
	UNLOCK(mse3_lock, oldpri);
	
	oldpri = splstr();

	i8042_program(P8042_AUXDISAB);	/* disable mouse interrupts */

	mca.cmd = MSEON;		/* turn on 320 mouse */
	if (mcafunc(&mca) == FAILED) {
		splx(oldpri); 
		return (ENXIO);
	}

	mca.cmd = MSESETRES;		/* set resloution */
	mca.arg1 = 0x03;	
	mcafunc(&mca);

	mca.cmd = MSECHGMOD;		/* set sample rate */
	mca.arg1 = 0x28;	
	mcafunc(&mca);

	splx(oldpri);

	/* allocate and initialize state structure */
	if ((mse3ptr = (struct strmseinfo *) kmem_zalloc(
			sizeof(struct strmseinfo), KM_SLEEP)) == NULL) {
		cmn_err(CE_WARN, 
			"MSE320: open fails, can't allocate state structure");
		return (ENOMEM);
	}

	q->q_ptr = (caddr_t) mse3ptr;
	WR(q)->q_ptr = (caddr_t) mse3ptr;
	mse3ptr->rqp = q;
	mse3ptr->wqp = WR(q);
	/* Initialize of all buttons up */
	mse3ptr->old_buttons = 0x07;

	i8042_aux_state = 0;
	i8042_program(P8042_AUXENAB);	/* enable both mouse interface and 
					 * mouse interrupts
					 */
	return (0);
}


/*
 * STATIC int
 * ms3close(queue_t *, int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
mse3close(queue_t *q, int flag, struct cred *cred_p)
{
	pl_t oldpri;
	struct cmd_320 mca;
	struct strmseinfo *tmpmse3ptr;


	oldpri = splstr();

	mse3closing = 1;
	mcastat.map_unit = -1;

#if defined(DEBUG) || defined(DEBUG_TOOLS)
	if (i8042_aux_state != 0)
		cmn_err(CE_NOTE, 
			"!Incorrect aux state on close");
#endif /* DEBUG || DEBUG_TOOLS */

	/*
	 * leave aux interface disabled after mcafunc() runs 
	 * and also disable the irq line.
	 */
	i8042_program(P8042_AUXDISAB);

	/* disable the auxiliary device */
	mca.cmd = MSEOFF;
	mcafunc(&mca);

	q->q_ptr = (caddr_t) NULL;
	WR(q)->q_ptr = (caddr_t) NULL;

	mse3closing = 0;
	SV_SIGNAL(mse3_closesv, 0);
	tmpmse3ptr = mse3ptr;
	mse3ptr = (struct strmseinfo *) NULL;

	splx(oldpri);

	kmem_free(tmpmse3ptr, sizeof(struct strmseinfo));
	
	return (0);
}

/*
 * STATIC int
 * mse3_wput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */

STATIC int
mse3_wput(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocbp;

	switch (mp->b_datap->db_type) {
	case M_IOCTL:
		/* LINTED pointer alignment */
		iocbp = (struct iocblk *) mp->b_rptr;
		if (iocbp->ioc_count != TRANSPARENT) {
			if (mp->b_cont) {
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}
			mse_iocnack(q, mp, iocbp, EINVAL, 0);
			return (0);
		}

		switch (iocbp->ioc_cmd) {
			case MOUSE320: 
				mse_copyin(q, mp, sizeof(struct cmd_320), M_IN_DATA);
				break;

			case MOUSEIOC3BE:
				mse_copyin(q, mp, sizeof(int), M_IN_DATA);
				break;

			default:
				mse_iocnack(q, mp, iocbp, EINVAL, 0);
				break;
		}
		break;

	case M_COPYIN:
		mse3ioc(q, mp);
		break;

	case M_IOCDATA:
		mse3ioc(q, mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
	
	return (0);
}


/*
 * STATIC void
 * mse3ioc(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */

STATIC void
mse3ioc(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocbp;
	struct copyresp *csp;
	struct cmd_320 *cmd320;
	struct msecopy *stp;
	mblk_t *bp;	
	int retval;

	/*
	 * Process the IOCDATA packets sent down by the stream head 
	 * from an M_COPYIN/M_COPYOUT request. M320 cmd has an IOCDATA
	 * indicating MOUSEIOCREAD data copied out OK, or not. The 
	 * IOC3BE ioctl copies an int downstream, and we get the M_COPYIN
	 * data and call mse3ioc(). We parse the command (MOUSEIOC3BE).
	 */

	csp = (struct copyresp *)mp->b_rptr;
	iocbp = (struct iocblk *)mp->b_rptr;

	if (csp->cp_rval) {
		freemsg(mp);
		return;
	}

	switch (csp->cp_cmd) {

	case MOUSE320:
		stp = (struct msecopy *)csp->cp_private;

		switch (stp->state) {
		case M_IN_DATA:
			/* LINTED pointer alignment */
			cmd320 = (struct cmd_320 *)mp->b_cont->b_rptr;
			if ((retval = mcafunc(cmd320)) == -1) {
				mse_iocnack(q, mp, iocbp, EINVAL, 0);
				break;
			}
			if (retval) {
				bp = copyb(mp->b_cont);
				mse3ptr->copystate.state = M_OUT_DATA;
				mse_copyout(q, mp, bp, sizeof(struct cmd_320), 0);
			} else {
				mse_iocack(q, mp, iocbp, 0);
			}
			break;

		case M_OUT_DATA:
			mse_iocack(q, mp, iocbp, 0);
			break;

		default:
			mse_iocnack(q, mp, iocbp, EINVAL, 0);
			break;
		}
		break;

	/* 3 buttom emulation delay 	*/
	case MOUSEIOC3BE:

		stp = (struct msecopy *)csp->cp_private;

		switch (stp->state) {
			case M_IN_DATA:
				if (mp->b_cont) {
					mse3_3bdly = *((int *)mp->b_cont->b_rptr);
					((struct strmseinfo *)q->q_ptr)->fsm_timeout = mse3_3bdly;
					freemsg(mp->b_cont); 
					mp->b_cont = 0;
					mse_iocack(q, mp, iocbp, 0);
				} else 
					mse_iocnack(q, mp, iocbp, EINVAL, 0);
				break;

			case M_OUT_DATA:
			default:
				mse_iocnack(q, mp, iocbp, EINVAL, 0);
				break;
		}
		break;

	default:
		/* Not MOUSEIOC3BE or MOUSEIOCREAD	*/
		freemsg(mp);
		break;
	}
}


/*
 * void
 * mse3intr(unsigned)
 *
 * Calling/Exit State:
 *	None.
 */
void
mse3intr(unsigned vect)
{
#define MSE3_TIMEOUT_SPIN	5000

	int	mdata;

			
	if (vect == 12) {		/* vector 12 is AT&T 320 mouse */
		if (!mcastat.present)
			return;
		if (!mse3ptr)
			return;

		if (mcastat.mode == MSESTREAM) {
			int	waitcnt;

			/* wait until the 8042 output buffer is full */
			waitcnt = 0;
			do {
				if ((inb(MSE_STAT) & MSE_OUTBF) == MSE_OUTBF) {
					/* get data byte from controller */
					mdata = inb(MSE_OUT);
					ps2parse(mdata);
#if defined(DEBUG) || defined(DEBUG_TOOLS)
					if (waitcnt > 0)
						cmn_err(CE_NOTE, 
							"!mse3intr: waitcnt is %d\n", waitcnt);
#endif /* DEBUG || DEBUG_TOOLS */
					return;
				}
				drv_usecwait(10);
			} while (++waitcnt < MSE3_TIMEOUT_SPIN);

			DEBUG1(("mse3intr: timeout!!!, state is %d\n", i8042_aux_state));
			if (i8042_aux_state != 0) 
				clear_sickness();
		}
	}
}


/*
 * STATIC void
 * clear_sickness(void)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
clear_sickness(void)
{
	char    buf[2];


	/* take ownership of 8042 */
	i8042_acquire();

	snd_320_cmd(MSEOFF, MSE_ACK, 1, (uchar_t *) buf);
	snd_320_cmd(MSEON, MSE_ACK, 1, (uchar_t *) buf);

	i8042_aux_state = 0;

	i8042_release();
}


/*
 * STATIC void
 * ps2parse(uchar_t char)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
ps2parse(uchar_t c)
{
	uchar_t	tmp;
	struct strmseinfo *m = mse3ptr;


	/* Parse the next byte of input.  */

	switch (i8042_aux_state) {

	case 0:
		/*
		 * Interpretation of the bits in byte 1:
		 *
		 *	Yo Xo Ys Xs 1 M R L
		 *
		 *	L:  left button state (1 == down)
		 *	R:  right button state
		 *	M:  middle button state
		 *	-:  usually 1, 0 with some mice (e.g. some HP mice)
		 *	Xs: X delta (byte 2) sign bit (1 == Negative)
		 *	Ys: Y delta (byte 3) sign bit
		 *	Xo: X overflow bit, never used by mouse driver
		 *	Yo: Y overflow bit, never used by mouse driver
		 */
		
		/*
		 * Shift the buttons bits into the order required: LMR
		 */

		/*
		 * Kludge: Check if the 4th bit is NOT 1, because an
		 * interrupt is missed or the i8042 buffer overflowed?
		*/

		if (!(c & 0x08)) {
						/* L000 vvv	*/
			/*
			 * For some mice, the 4th bit is zero.  In this case,
			 * as an additional test, check the overflow bits
			 * are clear.
			 */

			if (c & 0x0c0) {
						/* L000 ^^^	*/
				clear_sickness();
				return;
			}
		}

		tmp = (c & 0x01) << 2;
		tmp |= (uchar_t) (c & 0x06) >> 1;	
		m->button = (tmp ^ 0x07);	/* Buttons */
		m->x = (c & 0x10);		/* X sign bit */
		m->y = (c & 0x20);		/* Y sign bit */
		i8042_aux_state = 1;
		break;

	case 1:	
		/*
		 * Second byte is X movement as a delta
		 *
		 *	This byte should be interpreted as a 9-bit
		 *	2's complement number where the Most Significant
		 *	Bit (i.e. the 9th bit) is the Xs bit from byte 1.
		 *
		 *	But since we store the value as a 2's complement
		 *	8-bit number (i.e. signed char) we need to
		 *	truncate to 127 or -128 as needed.
		 */

		if (m->x) {			/* Negative delta */
			/*
			 * The following blocks of code are of the form
			 *
			 *	statement1;
			 *	if (condition)
			 *		statement2;
			 *
			 * rather than
			 *
			 *	if (condition)
			 *		statement2;
			 *	else
			 *		statement1;
			 *
			 * because it generates more efficent assembly code.
			 */

			m->x = -128;		/* Set to signed char min */

			if (c & 0x80)		/* NO truncate    */
				m->x = c;

		} else {			/* Positive delta */

			m->x = 127;		/* Set to signed char max */

			if (!(c & 0x80))	/* Truncate       */
				m->x = c;
		}

		i8042_aux_state = 2;
		break;

	case 2:	
		/*
		 * Third byte is Y movement as a delta
		 *
		 *	See description of byte 2 above for how to
		 *	interpret byte 3.
		 *
		 *	The driver assumes position (0,0) to be in the
		 *	upper left hand corner, BUT the PS/2 mouse
		 *	assumes (0,0) is in the lower left hand corner
		 *	so the truncated delted also needs to be
		 *	negated for the Y movement.
		 *
		 * The logic is a little contorted, however if you dig
		 * through it, it should be correct.  Remember the part
		 * about the 9-bit 2's complement number system
		 * mentioned above.
		 *
		 * For complete details see "Logitech Technical
		 * Reference & Programming Guide."
		 */
	
		if (m->y) {	/* Negative delta treated as Positive */

			m->y = 127;	/* set to signed char max */

			if ((unsigned char)c > 128) /* just negate */
				m->y = -c;

		} else {	/* Positive delta treated as Negative */

			m->y = -128;	/* set to signed char min */

			if ((unsigned char)c < 128) /* just negate */
				m->y = -c;
		}

		i8042_aux_state = 0;
		mseproc(m);
		break;
	}
}


/*
 * STATIC int
 * snd_320_cmd(unsigned char, unsigned char, int, unchar *)
 *
 * Calling/Exit State:
 *	- Return 0 on success, otherwise return FAILED.
 *	
 * Description:
 *	Send command byte to the 320 mouse device. Expect bufsize bytes
 *	in return from 320 mouse and store them in buf. Verify that the
 *	first byte read back is ans. If command fails or first byte read
 *	back is SNDERR or SNDERR2, retry. Give up after two attempts.
 */
STATIC int
snd_320_cmd(unsigned char cmd, unsigned char ans, int bufsize, unchar *buf)
{
	int	sndcnt = 0;
	int	rv;


	DEBUG1(("entered snd_320_cmd() cmd = %x\n", cmd));

	while (sndcnt < 2) {
		/*
		 * Send the command to the 8042. rv == 1 --> success.
		 */
		rv = i8042_send_cmd(cmd, P8042_TO_AUX, buf, bufsize);

		if (rv && ((ans == MSE_ANY) || (buf[0] == ans)))
			/* command succeeded, and first byte matches */
			return (0);


		if (buf[0] == SNDERR || buf[0] == SNDERR2) {
			DEBUG1(("snd_320_cmd() SNDERR\n"));
			if (buf[0] == SNDERR2 || sndcnt++ > 1) {
				DEBUG1(("snd_320_cmd() FAILED two resends\n"));
				return (FAILED);
			}
		} else
		   	return (FAILED);
	}

	DEBUG1(("leaving snd_320_cmd() \n"));
	return (FAILED);
}


/*
 * STATIC int
 * mcafunc(struct cmd_320 *)
 *	320 mouse command execution function 
 *
 * Calling/Exit State:
 *	- Return 0 on success, otherwise return FAILED.
 */
STATIC int
mcafunc(struct cmd_320 *mca)
{
	int	retflg = 0;
	unchar	buf[10];


	DEBUG1(("entered mcafunc(): cmd = %x\n", mca->cmd));

	/* take ownership of 8042 */
	i8042_acquire();

	/* must turn mouse off if streaming mode set */
	if (mcastat.mode == MSESTREAM) {
		if (snd_320_cmd(MSEOFF, MSE_ACK, 1, buf) == FAILED) {
			DEBUG1(("mcafunc(): MSEOFF failed\n"));
			i8042_release();
			return (FAILED);
		}
		if (mca->cmd == MSEOFF) { /* we just did requested cmd */
			i8042_release();
			return (0);
		}
	}

	DEBUG1(("mcafunc: doing switch statement\n"));

	switch (mca->cmd & 0xff) {
	case MSESETDEF: /* these commands have no args */
	case MSEOFF:
	case MSEON:
	case MSESPROMPT:
	case MSEECHON:
	case MSEECHOFF:
	case MSESTREAM:
	case MSESCALE2:
	case MSESCALE1:
		if (snd_320_cmd(mca->cmd, MSE_ACK, 1, buf) == FAILED) {
			retflg = FAILED;
			break;
		}
		if (mca->cmd == MSESTREAM || mca->cmd == MSESPROMPT)
			mcastat.mode = mca->cmd;
		break;

	case MSECHGMOD:
		if (snd_320_cmd(mca->cmd, MSE_ACK, 1, buf) == FAILED) {
			retflg = FAILED;
			break;
		}

		/* received ACK. Now send arg */

		DEBUG1(("mcafunc: do arg1 of MSECHGMOD = %x\n",mca->arg1));

		if (snd_320_cmd(mca->arg1, MSE_ACK, 1, buf) == FAILED) {
			retflg = FAILED;
			DEBUG1(("mcafunc(): MSECHGMOD failed\n"));
		}
		break;

	case MSERESET: /* expecting ACK and 2 add'tl bytes */
		if (snd_320_cmd(mca->cmd, MSE_ACK, 3, buf) == FAILED) {
			retflg = FAILED;
			break;
		}
		/*
		 * command succeeded and got ACK as first byte 
		 * Now verify next two bytes.
		 */
		if (buf[1] != 0xaa || buf[2] != 0x00) {
			retflg = FAILED;
			DEBUG1(("mcafunc(): MSERESET failed\n"));
		}
		break;

	case MSEREPORT: /* expect ACK and then 3 add'tl bytes */
	case MSESTATREQ:
		if (snd_320_cmd(mca->cmd, MSE_ACK, 4, buf) == FAILED) {
			retflg = FAILED;
			break;
		}
		mca->arg1 = buf[1];
		mca->arg2 = buf[2];
		mca->arg3 = buf[3];
		retflg = 1;
		break;

	case MSERESEND: /* expect 3 bytes back. Don't care what
			 * the first byte is 
			 */
		if (snd_320_cmd(mca->cmd, MSE_ANY, 3, buf) == FAILED) {
			retflg = FAILED;
			break;
		}
		mca->arg1 = buf[0];
		mca->arg2 = buf[1];
		mca->arg3 = buf[2];
		retflg = 1;
		break;

	case MSEGETDEV: /* expect 2 bytes back */
		if (snd_320_cmd(mca->cmd, MSE_ACK, 2, buf) == FAILED) {
			retflg = FAILED;
			break;
		}
		/* got an ACK, the second byte is the return val */
		mca->arg1 = buf[1];
		retflg = 1;
		break;

	case MSESETRES: /* cmd has one arg */
		if (snd_320_cmd(mca->cmd, MSE_ACK, 1, buf) == FAILED) {
			retflg = FAILED;
			break;
		}
		/*
		 * sent cmd successfully. Now send arg. If
		 * return val is not arg echoed back, fail the cmd
		 */
		if (snd_320_cmd(mca->arg1, MSE_ACK, 1, buf) == FAILED)
			retflg = FAILED;
		break;

	default:
		DEBUG1(("mcafunc:SWITCH default \n"));
		retflg = FAILED;
		break;
	}

	/*
	 * turn the mouse back on if we were streaming and cmd was not
	 * MSEOFF
	 */
	if (mcastat.mode == MSESTREAM) { 
		if (mca->cmd != MSEOFF) {
			if (snd_320_cmd(MSEON, MSE_ACK, 1, buf) == FAILED) {
				retflg = FAILED;
				DEBUG1(("mcafunc(): MSEON failed\n"));
			}
		}
	}

	/* finished cmd. Release ownership of 8042 */
	i8042_release();

	return (retflg);
}
