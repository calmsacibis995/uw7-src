#ident	"@(#)i8042.c	1.1"
#ident	"$Header$"


#ifdef AT_KEYBOARD

/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 *
 */

/*
 *  Intel 8042 common code.
 *
 *  REFERENCES
 *	IBM Personal System/2 Model 80 Technical Reference,
 *	Chapter 4 (System Board I/O Controllers),
 *	"Keyboard/Auxiliary Device Controller" (section 1).
 *	First edition (April 1987),
 *	IBM 84X1508 (a.k.a. S84X-1508-00).
 *
 *	8042 Technical Reference Manual.
 *	Undated, COMPAQ.
 *
 *  MODIFICATION HISTORY
 *	14 April 1988	scol!blf
 *		- Created 
 *	05apr89		scol!hughd
 *		- brought into 3.2 MC from 2.3 MC: for AT also
 *	L002	04 August 1989	scol!howardf
 *		- Test the AUX port to see if it is physically connected.
 *		  If not then disallow further accesses to that port.
 *	L003	10 August 1989	scol!howardf
 *		- Make sure we are actually talking about the keyboard or
 *		  AUX port before checking whether it is connected.
 *	L004	10 August 1989	scol!howardf
 *		- For some reason some AT machines have 8042's which don't
 *		  like having their AUX port disabled when it doesn't exist.
 *		  We need to disable therefore *only* if the port is connected.
 *	S005	Thu Aug 10 18:41:46 PDT 1989 jamescb
 *		- cleanup to only do reset8042() code if a kbmouse is
 *		  configured, and assume kb is OK otherwise.
 *	L006	08jan90		scol!hughd from andrewj
 *		- Unix port to Amstrad: add delay() before inb(I8042_CTRL)
 *		  to make keyboard LEDs work - only 1/4 ms, so I don't think
 *		  it need be made vendor-specific
 *	S007	sco!chapman	Fri Feb 23 04:49:17 PST 1990
 *		- Some machines (a model of Mitac to be exact) always
 *		  hold ABF high.  These machines don't have keyboard
 *		  mice (thay wouldn't work very well with this condition
 *		  would they?).  So in on8042intr() only look at the
 *		  ABF bit iff keyboard_mouse is set.
 *		- Also removed the clearing of kbmouse_present at the
 *		  end of reset8042().  It didn't look neccessary and
 *		  I now need it to stay set so on8042intr() will
 *		  work w/ keyboard mice.
 *	L008	scol!hughd	11mar90
 *		- I've been trying this out on the Apricot 486 with ODT,
 *		  systty=sio, and it turns out that it is necessary to
 *		  avoid that code in reset8042() after the first time
 *		  (can't input from console after kbmouse redoes it)
 */

#include <io/kd/i8042.h>



/*
 * SPL	-- Block both keyboard and mouse interrupts
 */
#define SPL		spl1	/* must block both keyboard & AUX (mouse) */

#define AMSTRADDELAY	250	/* microseconds L006 */


extern short kbmouse_present;	/* S005 */


static unsigned char	wr42cmd(unsigned char, int);
static unsigned char	rd42dat(unsigned char *, int);
static unsigned char	wr42dat(unsigned char, int);


/*
 * i8042poll	-- External patchable polling delays
 *
 * How many `int' decrements to spin when reading
 * or writing 8042 registers before giving up.
 * This list is patchable in case there are strange
 * machines out there in The Real World.
 */
int i8042poll[] = {
	0,		/*  0: read data byte on interrupts	*/
# define INTR	0
	0x4000,		/*  1: read 8042 cmd reply data byte	*/
# define REPLY	1
	0,		/*  2: send 8042 disable keyboard cmd	*/
# define DISKBY	2
	0,		/*  3: send 8042 disable AUX dev cmd	*/
# define DISAUX	3
	0x100,		/*  4: send data byte to keyboard	*/
# define OUTKBY	4
	0x200,		/*  5: send 8042 write AUX dev cmd	*/
# define SNDAUX	5
	0x100,		/*  6: send data byte to AUX dev	*/
# define OUTAUX	6
	0x200,		/*  7: send some command to 8042	*/
# define SNDCMD	7
	0,		/*  8: send 8042 enable AUX dev cmd	*/
# define ENBAUX	8
	0,		/*  9: send 8042 enable keyboard cmd	*/
# define ENBKBY	9
	0,		/* 10: read all data bytes to drain	*/
# define DRAIN	10
	0x100,		/* 11: send 8042 a new "command" byte	*/
# define OUTCMD 11
};

/*
 * Since we don't really know, apriori, which 8042-controlled device
 * the data is coming from, we should/must buffer the incoming data.
 */
#define CQSIZE		16
#define CQMASK		(CQSIZE - 1)

static struct Dev8042 {
	unsigned char	enabled;	/* non-0 if known to be enabled	*/
	struct {
		unsigned char	cmd;	/* disable/enable 8042 command	*/
		unsigned char	poll;	/* related i8042poll[] index	*/
	} ctrl[2];			/* 0 = disable, 1 = enable	*/
	unsigned char	ip;		/* next byte received from 8042	*/
	unsigned char	op;		/* return this byte to driver	*/
	struct {
		unsigned char	data;	/* received data byte		*/
		unsigned char	status;	/* status of this data byte	*/
	} buf[CQSIZE];
} dev8042tab[2] = {			/* 0 = Keyboard, 1 = AUX device	*/
	{ 0, I42X_DKB, DISKBY, I42X_EKB, ENBKBY },
	{ 0, I42X_DAD, DISAUX, I42X_EAD, ENBAUX }
};

int aux_42_disable = 0;					/* L002 */
static unsigned char port_connected[2] = "\001\000";	/* L002, S005 */



/*
 * void
 * on8042intr(int)
 *	Common 8042 device interrupt handler
 *
 * Calling/Exit State:
 *
 * Note:
 *	Different 8042 devices interrupt at different IRQs.
 */
void
on8042intr(int intrp)
{
	register struct Dev8042 *dp;
	register unsigned char x;
	unsigned char y;
	int s;


	if (intrp) 
		s = SPL();

	while ((x = rd42dat(&y, i8042poll[INTR])) & I42S_OBF) {
		if (x & (I42S_PED | I42S_TOE))
			continue;	/* error */

		dp = &dev8042tab[0];

		if (((dp->ip + 1) & CQMASK) == dp->op)
			continue;	/* readahead buffer full */

		dp->buf[dp->ip].status = x;
		dp->buf[dp->ip].data   = y;
		dp->ip++;
		dp->ip &= CQMASK;
	}

	if (intrp) 
		splx(s);
}


/*
 * int
 * getc8042(int)
 *	Get a data byte from the 8042
 *
 * Calling/Exit State:
 *	Does not wait: Returns either the next available byte or -1.
 */
int
getc8042(int dev)
{
	register struct Dev8042 *dp;
	int x;


	if (!port_connected[dev])				/* L002 */
		return(-1);					/* L002 */

	dp = &dev8042tab[dev];
	if (dp->op == dp->ip)
		return (-1);		/* nothing in readahead buffer */

	x = dp->buf[dp->op].data;
	dp->op++;
	dp->op &= CQMASK;
	return (x & 0xFF);
}


/*
 * int
 * send8042(int, unsigned char, unsigned char)
 *
 * Calling/Exit State:
 *
 * Description:
 *	-- Send one data byte to a 8042 device
 *
 *	Sends <obyte> to the specified 8042 device:
 *		0	Keyboard
 *		1	AUX (mouse)
 *		other	The 8042 itself
 *	If <ibyte> is non-0, the next received 8042 data
 *	byte is placed in that location.  Returns -1 if
 *	something went wrong, else 0.
 *
 *	xmit8042 must be mutexed by SPL/splx;
 *	send8042 does this.
 */
int
send8042(int dev, unsigned char obyte, unsigned char *ibyte)
{
	int r, s;


	s = SPL();
	r = xmit8042(dev, obyte, ibyte);
	splx(s);

	return (r);
}

#define	IS_PORT(a)	((a & 1) == a)				/* L003 */

/*
 * int
 * xmit8042(int, unsigned char, unsigned char *)
 *
 * Calling/Exit State:
 */
int
xmit8042(int dev, unsigned char obyte, unsigned char *ibyte)
{
	register unsigned char x;
	int r;


	if (IS_PORT(dev) && !port_connected[dev])		/* L002,L003 */
		return(-1);					/* L002 */

	if (ibyte != (unsigned char *)0)
		*ibyte = 0;

	for (x = 0; x < 2; x++)		/* Disable devices */
		if (dev8042tab[x].enabled)
			able8042(x, 0, 0);

	on8042intr(0);			/* Get all pending input */

	switch (dev) {
	case 0:				/* Keyboard device */
		x = wr42dat(obyte, i8042poll[OUTKBY]);
		break;

	case 1:				/* AUX device (mouse) */
		x = wr42cmd(I42X_WAD, i8042poll[SNDAUX]);
		if (x & (I42S_IBF | I42S_OBF)) {
			r = -1;
			goto done;
		}
		x = wr42dat(obyte, i8042poll[OUTAUX]);
		break;

	default:			/* 8042 command */
		x = wr42cmd(obyte, i8042poll[SNDCMD]);
		break;
	}

	if (x & (I42S_IBF | I42S_OBF))
		r = -1;
	else if (ibyte == (unsigned char *)0)
		r = 0;
	else if ((x = rd42dat(ibyte, i8042poll[REPLY])) & I42S_OBF)
		r = 0;
	else
		r = -1;
done:
	for (x = 0; x < 2; x++)		/* Re-enable devices */
		if (dev8042tab[x].enabled)
			able8042(x, 1, 0);
	return (r);
}

/*
 * void
 * drain8042(int)
 *	reset8042 -- Reset Intel 8042 controller chip
 *
 * Calling/Exit State:
 *	Should only be called during init time with interrupts disabled.
 *	May be called by more than one driver, but only the first call
 *	does anything.
 */

int	xt_keyboard = 1;	/* 1 = IBM PC/XT keyboard mode!	*/

void
drain8042(int intrp)
{
	unsigned char junk;
	int s;


	if (intrp) 
		s = SPL();

	while (rd42dat(&junk, i8042poll[DRAIN]) & I42S_OBF)
			;
	if (intrp) 
		splx(s);
}


/*
 * void
 * able8042(int, int, int)
 *	-- Enable/disable 8042 devices
 *
 * Calling/Exit State:
 *	Enable (<flag> is 1) or disable (<flag> is 0) the 8042
 *	device (Keyboard <dev> is 0, AUX device <dev> is 1).
 *	Note that the device interrupts are always enabled.
 */
void
able8042(int dev, int flag, int intrp)
{
	register struct Dev8042 *dp;
	int s;


	if (!port_connected[dev])				/* L002 */
		return;						/* L002 */

	dp = &dev8042tab[dev];

	if (intrp) 
		s = SPL();

	(void) wr42cmd(dp->ctrl[flag].cmd, i8042poll[dp->ctrl[flag].poll]);
	dp->enabled = flag;

	if (intrp) 
		splx(s);
}


/*
 * static unsigned char
 * rd42dat(unsigned char *, int)
 *	-- Get next data byte from 8042
 *
 * Calling/Exit State:
 *	Return value is the last-read 8042 status,
 *	and (if a byte was available) the received
 *	byte in *<pbyte>.
 *	rd42dat should be mutexed by SPL/splx when called.
 */
static unsigned char
rd42dat(unsigned char *pbyte, int poll)
{
	register unsigned char x;


	do {
		delay(AMSTRADDELAY);				/* L006 */
		x = inb(I8042_CTRL);
		if (x & I42S_OBF) {
			*pbyte = inb(I8042_DATA);
			break;
		}
	} while (poll-- > 0);

	return (x);
}


/*
 * static unsigned char
 * wr42dat(unsigned char, int)
 *	-- Send one data byte to the 8042
 *
 * Calling/Exit State:
 *	Return value is the last-read 8042 status.
 *	wr42dat should be mutexed by SPL/splx when called.
 */
static unsigned char
wr42dat(unsigned char byte, int poll)
{
	register unsigned char x;


	do {
		delay(AMSTRADDELAY);				/* L006 */
		x = inb(I8042_CTRL);
		if (!(x &= (I42S_IBF | I42S_OBF))) {
			outb(I8042_DATA, byte);
			break;
		}
	} while (poll-- > 0);

	return (x);
}


/*
 * static unsigned char
 * wr42cmd(unsigned char, int)
 *	-- Send a one byte command to the 8042
 *
 * Calling/Exit State:
 *	Return value is the last-read 8042 status.
 *	wr42cmd should be mutexed by SPL/splx when called.
 */
static unsigned char
wr42cmd(unsigned char byte, int poll)
{
	register unsigned char x;


	do {
		delay(AMSTRADDELAY);				/* L006 */
		x = inb(I8042_CTRL);
		if (!(x &= (I42S_IBF | I42S_OBF))) {
			outb(I8042_CTRL, byte);
			break;
		}
	} while (poll-- > 0);

	return (x);
}
#else
/*
 * suppress cc empty translation warning 
 */
static char empty_translat_unit;

#endif /* AT_KEYBOARD */
