/*		copyright	"%c%" 	*/

#ident	"@(#)line.c	1.3"
#ident "$Header$"

/* This is a new line.c, which consists of line.c and culine.c
 * merged together.
 */

/*  "VERBOSE_CU()" has been internationalized. The string require the string to
 *  be output must at least include the message number and optionary.
 *  The catalog name the string is output using <severity>.
 */

#include "uucp.h"
#include <pfmt.h>

#define PACKSIZE	64
#define HEADERSIZE	6
#define VERBOSE_CU(sev,fmt,str)\
	{ if(Verbose>0) pfmt(stderr, sev, fmt, str); }

int
     packsize = PACKSIZE,
    xpacksize = PACKSIZE;

#define SNDFILE	'S'
#define RCVFILE 'R'
#define RESET	'X'

int donap;	/* for speedup hook in pk1.c */
static int Saved_line;		/* was savline() successful?	*/
int
	Oddflag = 0,	/* Default is no parity */
	Noneflag = 0,	/* Default is no parity */
	Evenflag = 0,	/* Default is no parity */
	Duplex = 1,	/* Default is full duplex */
	Terminal = 0,	/* Default is no terminal */
	term_8bit = -1,	/* Default to terminal setting or 8 bit */
	line_8bit = -1;	/* Default is same as terminal */

static char *P_PARITY  = ":50:Parity option error\r\n";

static struct termios Savettyb;

/*
 * set speed/echo/mode...
 *	tty 	-> terminal name
 *	spwant 	-> speed
 *	type	-> type
 *
 *	if spwant == 0, speed is untouched
 *	type is unused, but needed for compatibility
 *
 * return:  
 *	none
 */
/*ARGSUSED*/
void
fixline(tty, spwant, type)
int	tty, spwant, type;
{
	struct termios		ttbuf;
	speed_t			speed;

	DEBUG(6, "fixline(%d, ", tty);
	DEBUG(6, "%d)\n", spwant);

	if (Ioctl(tty, TCGETS, &ttbuf) != 0)
		return;
	if (spwant <= 0)
		spwant = tcgetspeed(TCS_OUT, &ttbuf);

	ttbuf.c_iflag = ttbuf.c_oflag = ttbuf.c_lflag = ttbuf.c_cflag = 0;
	ttbuf.c_cflag &= ~CLOCAL;

	tcsetspeed(TCS_ALL, &ttbuf, spwant);
	if ((speed = tcgetspeed(TCS_ALL, &ttbuf)) != spwant)
	    DEBUG(5, "speed (%d) not supported\n", spwant);
	ASSERT(speed == spwant, "BAD SPEED", "", spwant);

	if ( EQUALS(Progname, "cu") ) {
		DEBUG(5, "Progname is %s\n", "cu");

		/* set attributes associated with -h, -t, -e, and -o options */

		ttbuf.c_iflag = (IGNPAR | IGNBRK | IXON | IXOFF);
		if ( line_8bit ) {
		    ttbuf.c_cflag |= CS8;
		    ttbuf.c_iflag &= ~ISTRIP;
		} else {
		    ttbuf.c_cflag |= CS7;
		    ttbuf.c_iflag |= ISTRIP;
		}

		ttbuf.c_cc[VMIN] = 1;
		ttbuf.c_cflag |= ( CREAD | (speed ? HUPCL : 0));
	
		if (Evenflag) {				/*even parity -e */
		    if(ttbuf.c_cflag & PARENB) {
			VERBOSE_CU(MM_ERROR,P_PARITY, 0);
			exit(1);
		    } else 
			ttbuf.c_cflag |= PARENB;
		} else if(Oddflag) {			/*odd parity -o */
		    if(ttbuf.c_cflag & PARENB) {
			VERBOSE_CU(MM_ERROR,P_PARITY, 0);
			exit(1);
		    } else {
			ttbuf.c_cflag |= PARODD;
			ttbuf.c_cflag |= PARENB;
		    }
		}

		if(!Duplex)				/*half duplex -h */
		    ttbuf.c_iflag &= ~(IXON | IXOFF);
		if(Terminal)				/* -t */
		    ttbuf.c_oflag |= (OPOST | ONLCR);

	} else { /* non-cu */
DEBUG(5, "NON-cu\n%s", "");
		ttbuf.c_cflag |= (CS8 | CREAD | (speed ? HUPCL : 0));
		ttbuf.c_cc[VMIN] = HEADERSIZE;
		ttbuf.c_cc[VTIME] = 1;
	}

	donap = ( spwant > 0 && spwant < 4800 );
	
	ASSERT(Ioctl(tty, TCSETSW, &ttbuf) >= 0,
	    "RETURN FROM fixline ioctl", "", errno);
	return;
}

void
sethup(dcf)
int	dcf;
{
	struct termios ttbuf;

	if (Ioctl(dcf, TCGETS, &ttbuf) != 0)
		return;
	if (!(ttbuf.c_cflag & HUPCL)) {
		ttbuf.c_cflag |= HUPCL;
		(void) Ioctl(dcf, TCSETSW, &ttbuf);
	}
	return;
}

void
ttygenbrk(fn)
register int	fn;
{
	if (isatty(fn)) 
		(void) Ioctl(fn, TCSBRK, 0);
	return;
}


/*
 * optimize line setting for sending or receiving files
 * return:
 *	none
 */
void
setline(type)
register char	type;
{
	static struct termios tbuf;
	speed_t speed;
	int vtime;
	
	DEBUG(2, "setline - %c\n", type);
	if (Ioctl(Ifn, TCGETS, &tbuf) != 0)
		return;
	switch (type) {
	case RCVFILE:
		speed = tcgetspeed(TCS_IN, &tbuf);
		vtime = 8;
		if (speed >= 4800)
			vtime = 4;
		if (speed >= 9600)
			vtime = 1;
		if (tbuf.c_cc[VMIN] != packsize || tbuf.c_cc[VTIME] != vtime) {
		    tbuf.c_cc[VMIN] = packsize;
		    tbuf.c_cc[VTIME] = vtime;
		    if ( Ioctl(Ifn, TCSETSW, &tbuf) != 0 )
			DEBUG(4, "setline Ioctl failed errno=%d\n", errno);
		}
		break;

	case SNDFILE:
	case RESET:
		if (tbuf.c_cc[VMIN] != HEADERSIZE) {
		    tbuf.c_cc[VMIN] = HEADERSIZE;
		    if ( Ioctl(Ifn, TCSETSW, &tbuf) != 0 )
			DEBUG(4, "setline Ioctl failed errno=%d\n", errno);
		}
		break;
	}
	return;
}

int
savline()
{
	if ( Ioctl(0, TCGETS, &Savettyb) != 0 )
		Saved_line = FALSE;
	else {
		Saved_line = TRUE;
		Savettyb.c_cflag = (Savettyb.c_cflag & ~CSIZE) | CS7;
		Savettyb.c_oflag |= OPOST;
		Savettyb.c_lflag |= (ISIG|ICANON|ECHO);
	}
	return(0);
}

#ifdef SYTEK

/*
 *	sytfixline(tty, spwant)	set speed/echo/mode...
 *	int tty, spwant;
 *
 *	return codes:  none
 */

void
sytfixline(tty, spwant)
int tty, spwant;
{
	struct termios ttbuf;
	speed_t speed;
	int ret;

	if ( (*Ioctl)(tty, TCGETS, &ttbuf) != 0 )
		return;
	ttbuf.c_iflag = ttbuf.c_oflag = ttbuf.c_lflag = 0;
	ttbuf.c_cflag = (CS8|CLOCAL);
	tcsetspeed(TCS_ALL, &ttbuf, spwant);
	speed = tcgetspeed(TCS_ALL, &ttbuf);
	DEBUG(4, "sytfixline - speed= %d\n", speed);
	ASSERT(speed == spwant, "BAD SPEED", "", spwant);
	ttbuf.c_cc[VMIN] = 6;
	ttbuf.c_cc[VTIME] = 1;
	ret = (*Ioctl)(tty, TCSETSW, &ttbuf);
	ASSERT(ret >= 0, "RETURN FROM sytfixline", "", ret);
	return;
}

void
sytfix2line(tty)
int tty;
{
	struct termios ttbuf;
	int ret;

	if ( Ioctl(tty, TCGETS, &ttbuf) != 0 )
		return;
	ttbuf.c_cflag &= ~CLOCAL;
	ttbuf.c_cflag |= CREAD|HUPCL;
	ret = Ioctl(tty, TCSETSW, &ttbuf);
	ASSERT(ret >= 0, "RETURN FROM sytfix2line", "", ret);
	return;
}

#endif /* SYTEK */

int
restline()
{
	if ( Saved_line == TRUE )
		return(Ioctl(0, TCSETSW, &Savettyb));
	return(0);
}

