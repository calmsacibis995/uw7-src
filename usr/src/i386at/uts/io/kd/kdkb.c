#ident	"@(#)kdkb.c	1.23"
#ident	"$Header$"


/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	30Apr97		rodneyh@sco.com
 *	- Changed kdkb_setled() to also check for ALT_LOCK state.
 *	- Moved actuall setting of kb_state from kdkb_setled() into seperate
 *	  function to be callable directly without haveing the LEDS update.
 *
 */

#include <io/kd/kb.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termios.h>
#include <io/ws/8042.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>


/*
 * MACRO
 * SET_TONE_TIMER(uchar_t regval)
 *
 * Calling/Exit State:
 *	- w_mutex lock is held.
 *
 * Desription:
 *	Set up timer mode and load initial count value.
 */
#define SET_TONE_TIMER(freq) { \
		outb(TIMERCR, T_CTLWORD); \
		outb(TIMER2, (freq) & 0xFF); \
		outb(TIMER2, (freq) >> 8); \
}

/*
 * MACRO
 * TONEON(uchar_t regval)
 *
 * Calling/Exit State:
 *	- w_mutex lock is held.
 *
 * Desription:
 *	Turn tone generator on.
 */
#define TONEON(regval) { \
		/* turn tone generation on */  \
		(regval) = (inb(TONE_CTL) | TONE_ON); \
		outb(TONE_CTL, (regval)); \
}

/*
 * MACRO
 * TONEOFF(uchar_t regval)
 *
 * Calling/Exit State:
 *	- w_mutex lock is held.
 *
 * Desription:
 *	Turn tone generator off.
 */
#define TONEOFF(regval) { \
		(regval) = (inb(TONE_CTL) & ~TONE_ON); \
		outb(TONE_CTL, (regval)); \
}

#ifdef DEBUG
STATIC int kdkb_debug = 0;
#define DEBUG1(a)       if (kdkb_debug == 1) printf a
#define DEBUG2(a)       if (kdkb_debug >= 2) printf a /* allocations */
#define DEBUG5(a)       if (kdkb_debug >= 5) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#define DEBUG5(a)
#endif /* DEBUG */

void		kdkb_cktone(void);
void		kdkb_toneoff(void);

STATIC unchar	kdkb_type(void);
STATIC int	kdkb_resend(unchar, unchar);

boolean_t	kdkb_printwarn = B_TRUE;  /* print warning if no keyboard */

extern wstation_t Kdws;
extern ws_channel_t  Kd0chan;
extern unchar  	  kd_typematic;


/*
 * int
 * kdkb_init(int *, kbstate_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode. 
 */
int
kdkb_init(unchar *kbtype, kbstate_t *kbp) 
{
	unchar	cb;
	pl_t 	opl;
	extern void i8042_init(void);


	i8042_init();
	
	opl = splhi();

	if (inb(KB_STAT) & KB_OUTBF)	/* clear output buffer */
		(void) inb(KB_OUT);

	*kbtype = kdkb_type();		/* find out keyboard type */

	i8042_write(KB_ICMD, KB_RCB);
	cb = i8042_read();
	/* clear disable keyboard flag */
	cb &= ~KB_DISAB;
	/* set interrupt on output buffer full flag */
	cb |= KB_EOBFI;	
	i8042_write(KB_ICMD, KB_WCB);
	i8042_write(KB_IDAT, cb);

	kbp->kb_state = 0;

	splx(opl);

	(void) i8042_aux_port();	/* ign ret val -- this is for init */

	kdkb_cmd(LED_WARN);
	kdkb_cmd(TYPE_WARN);

	return (0);
}


/*
 * STATIC int
 * kdkb_type(void)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Remark:
 *	call with interrupts off only!! (Since this is called at init time,
 *	we cannot use i8042* interface since it spl's.
 */
STATIC unchar 
kdkb_type(void)
{
	int	cnt;
	unchar	byt;
	unchar  kbtype;


	kbtype = KB_OTHER;

	i8042_write(KB_OUT, KB_READID);
	(void) i8042_read();

	/* wait for up to about a quarter-second for response */
	for (cnt = 0; cnt < 20000 && !(inb(KB_STAT) & KB_OUTBF); cnt++)
		drv_usecwait(10);

	if (!(inb(KB_STAT) & KB_OUTBF))
		kbtype = KB_84;		/* no response indicates 84-key */

	else if (inb(KB_OUT) == 0xAB) { /* first byte of 101-key response */

		/* wait for up to about a quarter-second for next byte */
		for (cnt = 0; cnt < 20000 && !(inb(KB_STAT) & KB_OUTBF); cnt++)
			drv_usecwait(10);

		if ((byt = inb(KB_OUT)) == 0x41 || byt == 0x83 || byt == 0x85)
			/* these are apparently all valid 2nd bytes */
			kbtype = KB_101;
	}

	i8042_write(KB_ICMD, KB_ENAB);

	return (kbtype);
}


/*
 * STATIC int
 * kdkb_resend(unchar, unchar)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	  If the w_rwlock is held in shared mode, then following
 *	  conditions must be true:
 *		- ch_mutex basic lock is held.
 *		- kdkb_resend() is called only by an active channel.
 */
STATIC int
kdkb_resend(unchar cmd, unchar ack)
{
	int cnt = 2;


	DEBUG1(("kdkb_resend: ack is 0x%x",ack));

	while (cnt--) {
		if (ack == 0xfe) {
			(void) i8042_send_cmd(cmd, P8042_TO_KBD, &ack,1);
			continue;
		} else if (ack == KB_ACK) {
			return (1);
	        } else 
			break;
	}

	cnt = 2;
	cmd = 0xf4;
	while (cnt--) {
		(void) i8042_send_cmd(cmd, P8042_TO_KBD, &ack, 1);
		if (ack == KB_ACK) 
			return (0);
	}

	DEBUG1(("kdkb_resend: did not enable keyboard"));

	if (!kdkb_printwarn)
		return (0);

	/*
	 *+ Could not find the integral console keyboard.
	 */
	cmn_err(CE_WARN,
		"Integral console keyboard not found. If you are using");
	cmn_err(CE_CONT,
		"the integral console, check the keyboard connection.\n");

	kdkb_printwarn = B_FALSE;

	return (0);
}


/*
 * void
 * kdkb_cmd(unchar)
 *	Send command to keyboard. Assumed to only be called 
 *	when spl's are valid
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	  If the w_rwlock is held in shared mode, then following
 *	  conditions must be true:
 *		- ch_mutex basic lock is held.
 *		- kdkb_cmd() is called by an active channel.
 *
 * Note:
 *	Must be minimally at plstr interrupt priority level.
 */
void
kdkb_cmd(unchar cmd)
{
	int		rv;
	unsigned char	ledstat;
	unchar		ack;
	kbstate_t	*kbp;
	ws_channel_t	*chp;

	switch (cmd) {
	case LED_WARN:
	{
		chp = ws_activechan(&Kdws);
		if (chp == (ws_channel_t *) NULL)
			chp = &Kd0chan;
		kbp = &chp->ch_kbstate;
		i8042_acquire();
               	i8042_update_leds(~kbp->kb_state, kbp->kb_state);
		i8042_release();
		return;
	}

	case TYPE_WARN:	/* send typematic */
		i8042_acquire();
		rv = i8042_send_cmd(cmd, P8042_TO_KBD, &ack, 1);
		DEBUG1(("!rv was %x", rv));
		if (ack != KB_ACK) {
			DEBUG1(("kdkb_cmd: unknown cmd %x ack %x", cmd, ack));
			if (!kdkb_resend(cmd, ack)) {
				i8042_release();
				return;
			}
		} 

		i8042_send_cmd(kd_typematic, P8042_TO_KBD, &ack, 1);
		if (ack != KB_ACK) {
			DEBUG1(("kdkb_cmd: TYPE_WARN, no ack from kbd"));
			if (!kdkb_resend(cmd, ack)) {
				i8042_release();
				return;
			}
		}
		i8042_release();
		break;

	default:
		/*
		 *+ An illegal keyboard command is send to program
		 *+ the keyboard controller.
		 */
		cmn_err(CE_WARN, 
			"kdkb_cmd: illegal kbd command %x",cmd);
		break;
	}
}


/*
 * void 
 * kdkb_cktone(void)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	If we are done with our bell, turn it off, otherwise set up a timeout
 *	to wake up when after another time interval and check again
 */
void
kdkb_cktone(void)
{
	unchar	regval;
	pl_t	pl;
	toid_t	tid;


	pl = LOCK(Kdws.w_mutex, plstr);

	if (--Kdws.w_ticks == 0) {
		/* turn tone generation off */
		TONEOFF(regval);
	} else {
		if ((tid = dtimeout((void(*)())kdkb_cktone, NULL, 
					BELLLEN, plstr, myengnum)) == 0) {
			Kdws.w_ticks = 0;
			UNLOCK(Kdws.w_mutex, pl);
			kdkb_toneoff();
			return;
		}
	}

	UNLOCK(Kdws.w_mutex, pl);
}


/*
 * void 
 * kdkb_tone(void)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	  If the w_rwlock is held in shared mode, then following
 *	  conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdkb_tone() is called by an active channel.
 *
 * Description:
 *	If we are the active VT, sound the bell.
 */
void
kdkb_tone(void)
{
	uchar_t	regval;
	pl_t	pl;
	uint_t	bfreq;
	int	btime;
	termstate_t *tsp;
	toid_t 	tid;
	ws_channel_t *chp;


	chp = WS_ACTIVECHAN(&Kdws);

	/* acquire the workstation mutex lock at plhi */
	pl = LOCK(Kdws.w_mutex, plhi);

	tsp = &chp->ch_tstate;
	btime = tsp->t_bell_time;
	bfreq = tsp->t_bell_freq;

	if (!Kdws.w_ticks) {
		Kdws.w_ticks = btime;
		/* set up timer mode and load initial value. */
		SET_TONE_TIMER(bfreq);
		/* turn tone generation on */
		TONEON(regval);
		/* go away and let tone ring a while */
		if ((tid = dtimeout((void(*)())kdkb_cktone, NULL, 
					BELLLEN, plstr, myengnum)) == 0) {
			Kdws.w_ticks = 0;
			UNLOCK(Kdws.w_mutex, pl);
			kdkb_toneoff();
			return;
		}
	} else {
		/* make it ring btime longer */
		Kdws.w_ticks = btime;
	}

	UNLOCK(Kdws.w_mutex, pl);
}


/*
 * void
 * kdkb_sound(int freq)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	  If the w_rwlock is held in shared mode, then following
 *	  conditions must be true.
 *		- kdkb_sound() is called by an active channel.
 *
 * Description:
 *	Sound generator.  This plays a tone at frequency freq
 *	for length milliseconds.
 */
void
kdkb_sound(int freq)
{
	unchar	regval;
	pl_t	pl;

	
	pl = LOCK(Kdws.w_mutex, plstr);

	if (freq) {			/* turn sound on? */
		/* set up timer mode and load initial value. */
		SET_TONE_TIMER(freq);
		/* turn tone generation on */
		TONEON(regval);
	} else {
		/* turn tone generation off */
		TONEOFF(regval);
	}

	UNLOCK(Kdws.w_mutex, pl);
}


/*
 * void 
 * kdkb_toneoff(void)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- Called after the timeout() is expired. 
 *	- w_mutex basic lock is acquired to protect the w_tone field.
 *
 * Descripion:
 *	Turn the sound generation off.
 */
void
kdkb_toneoff(void)
{
	unchar	regval;
	pl_t	pl;

	
	pl = LOCK(Kdws.w_mutex, plstr);

	Kdws.w_tone = 0;
	/* turn tone generation off */
	TONEOFF(regval);

	SV_SIGNAL(Kdws.w_tonesv, 0);

	UNLOCK(Kdws.w_mutex, pl);
}


/*
 * void
 * kdkb_mktone(ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- Since the caller can potentially sleep
 *	  in this routine, no locks are held on entry/exit.
 *
 * Description:
 *	Sound generator.  This starts a tone at frequency f.
 *	A frequency of 0 turns sound off.
 */
void
kdkb_mktone(ws_channel_t *chp, int arg)
{
	ushort	freq, length;
	int	tval;
	unchar	regval;
	pl_t	pl;
	toid_t	tid;


	freq = (ushort) ((long) arg & 0xffff);
	length = (ushort) (((long) arg >> 16) & 0xffff);
	if (!freq || !(tval = ((ulong) (length * HZ) / 1000L)))
		return;

	pl = LOCK(chp->ch_wsp->w_mutex, plstr);

	while (Kdws.w_tone) {
		/* In SVR4 the sleep priority was TTOPRI */
		SV_WAIT(chp->ch_wsp->w_tonesv, (primed + 3), 
						chp->ch_wsp->w_mutex);
		pl = LOCK(chp->ch_wsp->w_mutex, plstr);
	}

	Kdws.w_tone = 1;
	/* set up timer mode and load initial value. */
	SET_TONE_TIMER(freq);
	/* turn tone generation on */
	TONEON(regval);

	UNLOCK(chp->ch_wsp->w_mutex, pl);

	if ((tid = dtimeout((void(*)())kdkb_toneoff, NULL, 
				tval, plstr, myengnum)) == 0)
		kdkb_toneoff();
}

/*
 * void
 * kdkb_update_kbstate(kbstate_t *kbp, unchar led)
 *
 * Called from kdkbset_let or from KDSETLCK ioctl handler.
 *
 * Caliing/Exit state:
 *	- Called with w_rwlock held in shared/exclusive mode
 */
void
kdkb_update_kbstate(kbstate_t *kbp, unchar led)
{

	if (led & LED_CAP)
		kbp->kb_state |= CAPS_LOCK;
	else
		kbp->kb_state &= ~CAPS_LOCK;

	if (led & LED_NUM)
		kbp->kb_state |= NUM_LOCK;
	else
		kbp->kb_state &= ~NUM_LOCK;

	if (led & LED_SCR)
		kbp->kb_state |= SCROLL_LOCK;
	else
		kbp->kb_state &= ~SCROLL_LOCK;

        if (led & LED_KANA)
                kbp->kb_state |= KANA_LOCK;
        else
                kbp->kb_state &= ~KANA_LOCK;

	if(led & LED_ALCK)
                kbp->kb_state |= ALT_LOCK;
        else
                kbp->kb_state &= ~ALT_LOCK;
		
}	/* End function kdkb_update_kbstate, L000 end */


/*
 * void
 * kdkb_setled(ws_channel_t *, kbstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	  If the w_rwlock is held in shared mode, then following
 *	  conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdkb_setled() is called by an active channel.
 *
 * Note:
 *	SCO/Xenix compatibility to handle the differences in led flag bits.
 */
void
kdkb_setled(kbstate_t *kbp, unchar led)
{
	kdkb_update_kbstate(kbp, led);		/* L000 */

	kdkb_cmd(LED_WARN);
}


/*
 * int
 * kdkb_scrl_lock(void)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *	- Called by an active channel.
 */
int
kdkb_scrl_lock(void)
{
	kdkb_cmd(LED_WARN);
	return (0);
}


/*
 * int
 * kdkb_locked(ushort, unchar)
 *
 * Calling/Exit State:
 *	- Called from the keyboard interrupt handler (kdintr()).
 *	- w_rwlock is held in exclusive mode.
 *
 * Note:
 *	SCO/Xenix compatibility to handle special escape sequences from 
 *	the user to lock/unlock the keyboard.
 */
int
kdkb_locked(ushort ch, unchar kbrk)
{
	int	locked = (Kdws.w_flags & WS_LOCKED) ? 1 : 0;


	if (kbrk)
		return (locked);

	switch (Kdws.w_lkstate) {
	case 0:	/* look for ESC */
		if (ch == '\033')
			Kdws.w_lkstate++;
		else
			Kdws.w_lkstate = 0;
		break;

	case 1:	/* look for '[' */
		if (ch == '[')
			Kdws.w_lkstate++;
		else
			Kdws.w_lkstate = 0;
		break;

	case 2:	/* look for '2' */
		if (ch == '2')
			Kdws.w_lkstate++;
		else
			Kdws.w_lkstate = 0;
		break;

	case 3:	/* look for 'l' or 'h'*/
		if (Kdws.w_flags & WS_LOCKED) {
			/*
			 * we are locked, do we unlock?
			 */
			if (ch == 'l')
				Kdws.w_flags &= ~WS_LOCKED;
		} else {
			/*
			 * we are unlocked, do we lock?
			 */
			if (ch == 'h')
				Kdws.w_flags |= WS_LOCKED;
		}

		Kdws.w_lkstate = 0;
		break;

	} /* switch */

	return (locked);
}


/*
 * void
 * kdkb_keyclick(void)
 *
 * Calling/Exit State:
 *	- Called from the keyboard interrupt handler.
 *	- w_rwlock is held in exclusive mode.
 */
void
kdkb_keyclick(void)
{
	unchar	tmp;
	int	cnt;
	pl_t	pl;


	pl = LOCK(Kdws.w_mutex, plstr);

	if (Kdws.w_flags & WS_KEYCLICK) {
		/* start click */
		TONEON(tmp);
		for (cnt = 0; cnt < 0xff; cnt++)
			;
		/* end click */
		TONEOFF(tmp);
	}

	UNLOCK(Kdws.w_mutex, pl);
}


/*
 * void
 * kdkb_force_enable(void)
 * 
 * Calling/Exit State:
 *	This routine is called by the kdcngetc to force an enable of
 *	the keyboard. We do this to avoid the spls and flags of the
 *	i8042 interface
 */
void
kdkb_force_enable(void)
{
	i8042_write(KB_ICMD, KB_ENAB);
}
