#ident	"@(#)ws_8042.c	1.26"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	5Jun97		rodneyh@sco.com
 *	- Tightened up i8042_program's disabling and enabling of the auxiliary
 *	  8042 port. The old code never actually set or cleared the disable bit.
 *
 */

/*
 * i8042 universal peripheral controller interface for keyboard and
 * auxiliary device (PS/2 mouse).
 */

#include <io/kd/kd.h>
#include <io/kd/kb.h>
#include <io/ws/8042.h>
#include <io/ws/ws.h>
#include <mem/kmem.h>
#include <svc/systm.h>
#include <svc/bootinfo.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/types.h>

#include <io/ddi.h>


#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int i8042_debug = 0;
#define DEBUG1(a)       if (i8042_debug >= 1) printf a
#define DEBUG2(a)       if (i8042_debug >= 2) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG || DEBUG_TOOLS */

#define I8042HIER		3	/* must be greater than KDHIER+1 */

#define AUX_DISAB		0x20	/* == KB_AUXOUTBUF */
#define	KBD_DISAB		0x10	/* == KB_DISAB */

#define	I8042_RESERVED_BITS	(0x80 | 0x40 | 0x8 | 0x4)
#define	KBD_ENABLE_INTERFACE	0xAE	/* == KB_ENAB */
#define	KBD_DISABLE_INTERFACE	0xAD
#define	AUX_ENABLE_INTERFACE	0xA8
#define	AUX_DISABLE_INTERFACE	0xA7

#define I8042_TIMEOUT_SPIN	50000
#define	I8042_TIME_TO_SOAK	50000

struct i8042 {
	int	s_spl;		/* saved spl level */
	uchar_t	s_state;	/* state of 8042 */
	uchar_t	s_saved;	/* indicates data was saved */
	uchar_t	s_data;		/* saved data (scan code or 320 mouse input) */
	uchar_t	s_dev2;		/* device saved character is meant for */
} i8042_state = { 0, AUX_DISAB, 0, 0, 0 };

extern int i8042_timeout_spin;
int i8042_has_aux_port = 0;
int i8042_aux_state = 0;
boolean_t i8042_initialized = B_FALSE;
lock_t *i8042_mutex;		/* i8042 mutex lock */

LKINFO_DECL(i8042_mutex_lkinfo, "WS:8042:i8042 mutex lock", 0);


/*
 * void
 * i8042_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
i8042_init(void)
{
	i8042_mutex = LOCK_ALLOC(I8042HIER, I8042PL, 
					&i8042_mutex_lkinfo, KM_NOSLEEP);
	if (!i8042_mutex)
                /*
                 *+ There isn't enough memory available to allocate for
                 *+ i8042 and i8042_state mutex locks.
                 */
                cmn_err(CE_PANIC,
			"i8042_init: out of memory for i8042 locks");

	i8042_initialized = B_TRUE;
}

/*
 * UTILITIES ROUTINES 
 */

/*
 * uchar_t 
 * i8042_read(void)
 *	Read a byte from the data port. 
 *
 * Calling/Exit State:
 *	- Return byte if available from the port, otherwise return 0.
 *	- i8042_mutex lock is held on entry/exit.
 */
uchar_t
i8042_read(void)
{
	ulong count = 0;
	int	ii;
	int 	tmp_spin = i8042_timeout_spin;
	
	for (ii=0; ii < 10; ii++) {
		do {
			if (inb(KB_STAT) & KB_OUTBF)
				return(inb(KB_OUT));
			drv_usecwait(10);
		} while (++count < tmp_spin);

		/* Perform a retry with a larger loop count */
		tmp_spin << 1;
	}
	cmn_err(CE_WARN, "!i8042_read: timeout!!!");
	return(0);  
}	


/*
 * void
 * time_to_soak(void)
 *	Wait until registers soaked.
 *
 * Calling/Exit State:
 *	- i8042_mutex lock is held on entry/exit.
 */
void
time_to_soak(void)
{
	ulong	waitcnt;


	waitcnt = I8042_TIME_TO_SOAK; 
	while ((inb(KB_STAT) & KB_INBF) != 0 && waitcnt-- != 0)
		drv_usecwait(10);

	if (waitcnt == 0)
		cmn_err(CE_WARN, "!time_to_soak: timeout!!!");
}


/*
 * int
 * i8042_write(unsigned char, unsigned char)
 *	write byte to the data/command port
 *
 * Calling/Exit State:
 *	- Return 1 if byte was successfully written to the port, otherwise
 *	  return 0.
 *	- i8042_mutex lock is held on entry/exit.
 */
int
i8042_write(unsigned char port, unsigned char byte)
{

	ulong_t	count = 0;
	uchar_t sts;
	int 	ii;
	int	tmp_spin = i8042_timeout_spin;

	for (ii=0; ii < 10; ii++) {
		do {
			if (((sts = inb(KB_STAT)) & (KB_OUTBF|KB_INBF)) == 0) {
				outb(port, byte);
				time_to_soak();
				return(1);
			}
			if (sts & KB_OUTBF) {
				i8042_state.s_data = inb(KB_OUT);
				i8042_state.s_saved = 1;
				i8042_state.s_dev2 = (sts & KB_AUXOUTBUF);
			}
			drv_usecwait(10);
		} while (++count < tmp_spin);

		/* Perform a retry with a larger loop count */
		tmp_spin << 1;
	}

	/*
	 *+ The keyboard data or command port timed out.
	 */
	cmn_err(CE_WARN, "!i8042_write: cmd 0x%x timeout!!!", byte);

	return(0);
}	


/*
 * int
 * i8042_aux_port(void)
 *
 * Calling/Exit State:
 *	- Return 1 machine has an auxiliary device port, 0 if no.
 *	- No locking assumptions.
 *
 * Description:
 *	Determine if the system has an auxiliary device?
 *
 *	Enable the auxiliary device and if the 5th bit of the
 *	8042 command port is set, then the device does not exist
 *	since this bit must only be set when the auxiliary 
 *	device is disabled.
 */
int
i8042_aux_port(void)
{
	int	tmp1 = 0;
	int	tmp2 =0; 
	pl_t	oldpri;


	/*
	 * If auxillary port already detected, just 
	 * return the state of the aux port.
	 */
	if (i8042_has_aux_port)
		return(i8042_has_aux_port++);

	/* disable all keyboard or auxiliary device interrupts */
	oldpri = splstr();

	/* enable auxiliary interface */
	i8042_write(KB_ICMD, AUX_ENABLE_INTERFACE);
	i8042_write(KB_ICMD, KB_RCB);	/* read command byte */
	tmp1 = i8042_read();
	if (tmp1 & AUX_DISAB) {		/* enable did not take */
		splx(oldpri);
		return (i8042_has_aux_port);
	}
	/* disable auxiliary interface */
	i8042_write(KB_ICMD, AUX_DISABLE_INTERFACE);
	i8042_write(KB_ICMD, KB_RCB);	/* read command byte */
	tmp2 = i8042_read();
	if (tmp2 & AUX_DISAB)		/* disable successful */
		i8042_has_aux_port++;

	/* enable keyboard and auxiliary device interrupts */
	splx(oldpri);

	return (i8042_has_aux_port);
}


/*
 * void
 * i8042_program(int cmd)
 *	Modify "state" of 8042 so that the next call to release_8042
 *	changes the 8042's state appropriately.
 *
 * Calling/Exit State:
 *	- No locking assumptions.
 */
void
i8042_program(int cmd)
{
	unchar	cb;


	DEBUG2(("!i8042_program cmd %x ", cmd));

	i8042_acquire();

	switch (cmd) {
	case P8042_KBDENAB:	
		i8042_state.s_state &= ~KBD_DISAB;
		break;

	case P8042_KBDDISAB:
		i8042_state.s_state |= KBD_DISAB;
		break;

	case P8042_AUXENAB:
		DEBUG2(("i8042_program: ENABLE AUX\n"));
		i8042_write(KB_ICMD, KB_RCB);	/* read command byte */
		cb = i8042_read() & I8042_RESERVED_BITS;
		cb &= ~0x20;			/* Clear aux disab bit, L000 */
		cb |= KB_AUXEOBFI; 		/* enable aux interrupt */
		cb |= KB_EOBFI;			/* enable kbd interrupt */
		i8042_write(KB_ICMD, KB_WCB);	/* send new command byte */
		i8042_write(KB_IDAT, cb);
		i8042_state.s_state &= ~AUX_DISAB;
		break;

	case P8042_AUXDISAB:	
		DEBUG2(("i8042_program: DISABLE AUX\n"));
		i8042_write(KB_ICMD, KB_RCB);	/* read command byte */
		cb = i8042_read() & I8042_RESERVED_BITS;
		cb &= ~KB_AUXEOBFI;		/* Clear aux int bit, L000 */
		cb |= 0x20;			/* Set aux diable bit, L000 */
		cb |= KB_EOBFI;			/* enable kbd interrupt */
		i8042_write(KB_ICMD, KB_WCB);	/* send new command byte */
		i8042_write(KB_IDAT, cb);
		i8042_state.s_state |= AUX_DISAB;
		break;

	default:
		i8042_release();
		/*
		 *+ An illegal 8042 command is issued.
		 */
		cmn_err(CE_PANIC, 
			"i8042_program: illegal command %x", cmd);
		return;
	}

	i8042_release();
}


/*
 * void
 * i8042_disable_aux_interface(void)
 *
 * Calling/Exit State:
 *	- i8042_mutex is held on entry/exit.
 *
 * Description:
 *	Disable the auxilliary device (if any).
 */
void
i8042_disable_aux_interface(void)
{
	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB)) 
		i8042_write(KB_ICMD, AUX_DISABLE_INTERFACE);
}


/*
 * void
 * i8042_disable_interface(void)
 *
 * Calling/Exit State:
 *	- i8042_mutex is held on entry/exit.
 *
 * Description:
 *	Acquire the 8042 by changing to splstr, disabling the 
 *	keyboard and auxiliary devices (if any), and saving
 *	any data currently in the 8042 output port.
 */
void
i8042_disable_interface(void)
{
	i8042_write(KB_ICMD, KBD_DISABLE_INTERFACE);
	i8042_disable_aux_interface();
}


/*
 * void
 * i8042_acquire(void)
 *
 * Calling/Exit State:
 *	- i8042_mutex lock is acquired and is held on exit also.
 *
 * Description:
 *	Acquire the 8042 by changing to splstr, disabling the 
 *	keyboard and auxiliary devices (if any), and saving any
 *	data currently in the 8042 output port.
 */
void
i8042_acquire(void)
{
	extern wstation_t Kdws;


	/*
	 * Do not disable the keyboard interface while kd 
	 * is in its interrupt handler, since it disables 
	 * and enables the keyboard interface to turn on 
	 * or off the leds. So wait until kd exits from
	 * its handler. The alternative is for the kd
	 * handler to hold the i8042_mutex lock for the
	 * entire duration of its servicing an interrupt,
	 * but that would cause an extreme locking overhead.
	 */
	while (WS_INKDINTR(&Kdws))
		;

	I8042_LOCK(i8042_state.s_spl);
	i8042_disable_interface();
}


/*
 * void
 * i8042_enable_aux_interface(void)
 *
 * Calling/Exit State:
 *	- i8042_mutex is held on entry/exit.
 *
 * Description:
 *	Enable the auxiliary device (if any).
 */
void
i8042_enable_aux_interface(void)
{
	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB)) 
		i8042_write(KB_ICMD, AUX_ENABLE_INTERFACE);
}


/*
 * void
 * i8042_enable_interface(void)
 *
 * Calling/Exit State:
 *	- i8042_mutex is held on entry/exit.
 *
 * Description:
 *	Release the 8042.  If data was saved by the acquire, write back the
 *	data to the appropriate port, enable the devices interfaces where
 *	appropriate and restore the interrupt level.
 */
void
i8042_enable_interface(void)
{
	i8042_write(KB_ICMD, KBD_ENABLE_INTERFACE);
	i8042_enable_aux_interface();
}


/*
 * void
 * i8042_release(void)
 *
 * Calling/Exit State:
 *	- i8042_state_mutex lock is held on entry and released on exit.
 *
 * Description:
 *	Release the 8042.  If data was saved by the acquire, write back the
 *	data to the appropriate port, enable the devices interfaces where
 *	appropriate and restore the interrupt level.
 */
void
i8042_release(void)
{
	i8042_enable_interface();
 	I8042_UNLOCK(i8042_state.s_spl);
}


/*
 * int
 * i8042_send_cmd(unchar, unchar, unchar *, unchar)
 *
 * Calling/Exit State:
 *	- Return 1 for success, 0 for failure.
 *	- i8042_mutex is held on entry/exit.
 *
 * Description:
 *	Send a command to a device attached to the 8042.  
 *	The <cmd> argument is the comand to send. <whence> is
 *	the device to send it to.  <bufp> is an array of
 *	unchars into which any responses are placed, and 
 *	<cnt> is the number of bytes expected in the response.
 */
int
i8042_send_cmd(unchar cmd, unchar whence, unchar *bufp, unchar cnt)
{
	uchar_t	tcnt;
	int	rv = 1;
	ulong	lcnt;
	uchar_t sts;

	switch (whence) {
	case P8042_TO_KBD:	/* keyboard */
		break;
	case P8042_TO_AUX:	/* auxiliary */
		i8042_write(KB_ICMD, 0xd4);
		break;
	default:
		DEBUG2(("send_8042_dev: unknown device"));
		return 0;
	}

	if ((sts = inb(KB_STAT)) & KB_OUTBF) {
		i8042_state.s_data = inb(KB_OUT);
		i8042_state.s_saved = 1;
		i8042_state.s_dev2 = (sts & KB_AUXOUTBUF);
	}

	i8042_write(KB_IDAT, cmd);

	for (tcnt = 0; tcnt < cnt; tcnt++) {
		lcnt = i8042_timeout_spin;
		while ((inb(KB_STAT) & KB_OUTBF) == 0 && lcnt--)
			drv_usecwait(10);

		if (lcnt > 0) {
			bufp[tcnt] = inb(KB_OUT);
		} else {
			rv = 0;
			cmn_err(CE_WARN, 
				"!i8042_send_cmd: cmd 0x%x and whence 0x%x timeout!!!", cmd,whence);
			break;
		}
	}
	return(rv);
}


/*
 * void
 * i8042_update_leds(ushort, ushort)
 *
 * Calling/Exit State:
 *	- i8042_mutex lock is held on entry/exit.
 *
 * Description:
 *	Send command to turn the leds.
 *
 *	Reset the <i8042_aux_state> to zero whenever either the LEDs
 *	are programmed or channels are switched. It is not clear
 *	if the <i8042_aux_state> should be reset, since it is our
 *	understanding that the PS/2 mouse and keyboard data will
 *	be buffered although the interface is disabled. However,
 *	to be on the safe side we disable the mouse device which
 *	we know for sure resets the aux state.
 *
i* Remarks:
 *	The Set Status Indicators (LED_WARN == 0xED) is a 2-byte
 *	command that changes the state of the keyboard LED indicators.
 *	After receiving this command, the keyboard halts scanning,
 *	returns an ACK code to the system, and waits for the system
 *	to send the option byte. The option byte indicates which
 *	LED indicators are affected. When an option byte is received,
 *	the keyboard sets the status indicator, an ACK returns code
 *	and resumes scanning.
 */
void
i8042_update_leds(ushort okbstate, ushort nkbstate)
{
	uchar_t	ledstat;
	uchar_t	ack, ack1;


	ack = ack1 = 0;

	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB) && i8042_aux_state) {
		/* disable mouse */
		i8042_send_cmd(0xf5, P8042_TO_AUX, &ack, 1);
	}

	if (KBTOGLECHANGE(okbstate, nkbstate)) {
		/* tell keyboard the following byte is an led status */
		(void) i8042_send_cmd(LED_WARN, P8042_TO_KBD, &ack, 1);
		if (ack == KB_ACK) {
			ledstat = 0;
			if (nkbstate & CAPS_LOCK)
				ledstat |= LED_CAP;
			if (nkbstate & NUM_LOCK)
				ledstat |= LED_NUM;
			if (nkbstate & SCROLL_LOCK)
				ledstat |= LED_SCR;
                        if (nkbstate & KANA_LOCK)
                                ledstat |= LED_KANA;
			/* send led status */
			(void) i8042_send_cmd(ledstat, P8042_TO_KBD, &ack1, 1);
		} 

		if (ack != KB_ACK || ack1 != KB_ACK) {
			/*
			 *+ The keyboard responds to the set/status
			 *+ indicator (0xed) and option byte with an
			 *+ ACK. However, an unrecognized response
			 *+ byte is received from the keyboard. So
			 *+ re-enable keyboard if any such error.
			 */
			cmn_err(CE_NOTE, 
				"i8042_update_leds: unrecognized response "
				"from the keyboard.");
			i8042_send_cmd(0xf4, P8042_TO_KBD, &ack, 1);
		}
	}

	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB) && i8042_aux_state) {
		/*
		 *+ The i8042_aux_state represent the byte sequence
		 *+ number from the PS/2 mouse so that data is parsed
		 *+ correctly. This state must be zero when we switch
		 *+ VTs or program the leds.
		 */
		cmn_err(CE_NOTE, 
			"!i8042_aux_state is %d", i8042_aux_state);
		/* enable mouse */
		i8042_send_cmd(0xf4, P8042_TO_AUX, &ack, 1);
		i8042_aux_state = 0;
	}
}
