#ident	"@(#)source.c	1.3"
/*
 *	@(#)source.c	1.3	"@(#)source.c	1.3"
 
 *
 *	Copyright (C) The Santa Cruz Operation, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * set up the data source.
 *
 * read from an event queue, translate the events
 * according to some rules, and write the ascii
 * strings to stdout.
 */
/*
 *	MODS
 *
 * S001	sco!daniel Tue Jun  7 11:11:52 PDT 1988
 * -	ev_open now takes a pointer to a mask, rather than a mask.
 * S002 sco!daniel Wed Jun 29 06:24:18 PDT 1988
 * -	Save partial events and add them into calculations so that
 *	the `cursor tracking' is more accurate and fluid.
 * S003 sco!daniel Fri Jul 15 15:17:34 PDT 1988
 *	Exit with a message if there is no mouse available.
 * S004	sco!hoeshuen	19 Apr 1989
 *	check return value of ev_read() in readq(). If 0, go back to loop.
 * S005 scol!sunil	4 Jan 1990
 *	add signal based screen switching, and suspend and resume the
 *	event queue on screen switches.
 * L006 scol!keithp	27 Feb 1997
 *	- ported to Gemini
 */

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/page.h>
#include <sys/event.h>
#include <sys/errno.h>						/* S005 */
#include <sys/vt.h>						/* L006 */
#include <sys/kd.h>						/* L006 */
#include <fcntl.h>
#include <sys/termio.h>
#include <signal.h>
#include <mouse.h>
#include "source.h"

#define VT_TRUE 1						/* L006 */

static int qfd;

extern char	progname[];
extern int	echopid;
extern int	shellpid;
extern struct termio tsave;

void	set_sensitivity(char, int);
void	readq(int);
static void	initq();
static void	print_key(int, EVENT*);
static void	print_mouse(int, EVENT*);
static int	makeflag(EVENT *);
static void	closeq(int);
static void	fail(char*);

static void	suspend(int);
static void	resume(int);

static int is_rbu(),  is_rbd(),  is_lbu(),  is_lbd(), is_mbu(), is_mbd();
static int is_up(),   is_dn(),   is_lt(),   is_rt();
static int is_uprt(), is_uplt(), is_dnrt(), is_dnlt();

/* for command line and default file information */
struct xlat xlat[] = {
/*	kywd	initial	length	isthis	*/
/*	====	=======	======	======	*/
	"rbu",	EMPTY,	0,	is_rbu,
	"rbd",	EMPTY,	0,	is_rbd,
	"mbu",	EMPTY,	0,	is_mbu,
	"mbd",	EMPTY,	0,	is_mbd,
	"lbu",	EMPTY,	0,	is_lbu,
	"lbd",	EMPTY,	0,	is_lbd,
	"ul",	EMPTY,	0,	is_uplt,
	"ur",	EMPTY,	0,	is_uprt,
	"dl",	EMPTY,	0,	is_dnlt,
	"dr",	EMPTY,	0,	is_dnrt,
	"up",	EMPTY,	0,	is_up,
	"lt",	EMPTY,	0,	is_lt,
	"dn",	EMPTY,	0,	is_dn,
	"rt",	EMPTY,	0,	is_rt,
	0
};

extern int	errno;						/* S005 */
static int	wait_for_acquire = 0;				/* S005 */
static int hsensitivity = HSENSITIVITY;
static int vsensitivity = VSENSITIVITY;

void
set_sensitivity(char c, int num)
{
	if ( c == 'h' || c == 'H' )
		hsensitivity = num;
	if ( c == 'v' || c == 'V' )
		vsensitivity = num;
}

static void
initq()
{
	extern int ev_errlev;
	dmask_t	dmask = D_STRING | D_REL;			/*S001*/
	struct	kd_disparam	kdisp;				/* S005 */

	/* turn on error messages from the event API */
	ev_errlev = 1;	

	ev_init();
	if ( (qfd = ev_open(&dmask)) < 0 )
		fail("cannot open event queue");
	if ( (dmask & D_REL) == 0 )
		fail("no mouse available");			/*S003*/
	/* Set the close-on-exec flag */
	fcntl(qfd, F_SETFD, 1);
								/* v S005 v */
	/*
	 * If we are on the console, then notify the console driver
	 * that we want to be signalled on screen switches.
	 */
	if (ioctl(0, KDDISPTYPE, &kdisp) != -1) {
		static struct	vt_mode	vt = {VT_PROCESS, 0, 
					      SIGUSR1, SIGUSR2, 0};

		ioctl(0, VT_SETMODE, &vt);
		signal(SIGUSR1, suspend);
		signal(SIGUSR2, resume);
	}							/* ^ S005 ^ */
}

/* 
 * Read events from the event queue and write them to stdout.
 */
void
readq(outfd)
int	outfd;
{
	register EVENT	*evp;
	struct termio termio;

#ifdef DEBUG
	printf("readq: enter\n");
#endif
	signal(SIGHUP, closeq);
	signal(SIGQUIT, closeq);
	signal(SIGINT, closeq);
	signal(SIGTERM, closeq);
	signal(SIGCLD, closeq);
	initq();
	/* If we don't turn off ONLCR on the original tty, */
	/* we'll have nl->crlf mapping happening twice. bad. */
	ioctl(0, TCGETA, &termio);
	termio.c_oflag &= ~ONLCR; 
	ioctl(0, TCSETA, &termio);
	while( ioctl(qfd, EQIO_BLOCK, 0) != -1 || errno == EINTR ) { /* S005 */
		if ( (evp = ev_read()) == 0 )		/* S004 */
			continue;			/* S004 */
		if ( EV_TAG(*evp) & T_STRING )
			/* keyboard event */
			print_key(outfd, evp);
		else
			/* mouse event */
			print_mouse(outfd, evp);
		ev_pop();
	}
	closeq(0);
}

static void
print_key(fd, evp)
int	fd;
EVENT	*evp;
{
	char	buf[EV_STR_BUFSIZE+1];
	unsigned	cnt;
	int	i;

	cnt = EV_BUFCNT(*evp);
	for (i=0; i<cnt; i++)
		buf[i] = *(EV_BUF(*evp)+i);
	write(fd, buf, cnt);
}

static void
print_mouse(fd, evp)
int	fd;
EVENT	*evp;
{
	struct xlat *xp;
	int	i;
	static char oldbuttons=0;
	static int  oldx=0, oldy=0;				/*S002*/

	EV_DX(*evp) += oldx;					/*S002*/
	EV_DY(*evp) += oldy;					/*S002*/
	for (xp = xlat; xp->item; xp++)
		if ( i = (*xp->isthis)(evp, oldbuttons) )
			while ( i-- )
				write(fd, xp->buf, (unsigned) xp->len);
	oldbuttons = EV_BUTTONS(*evp);
	oldx = EV_DX(*evp) % hsensitivity;			/*S002*/
	oldy = EV_DY(*evp) % vsensitivity;			/*S002*/
}

static void
closeq(sig)
int	sig;
{
	extern struct termio tsave;
	struct	kd_disparam	kdisp;				/* v S005 v */

	if (ioctl(0, KDDISPTYPE, &kdisp) != -1) {
		static struct vt_mode	vt = {VT_AUTO, 0, 0, 0, 0};

		ioctl(0, VT_SETMODE, &vt);
		signal(SIGUSR1, SIG_IGN);
		signal(SIGUSR2, SIG_IGN);
	}							/* ^ S005 ^ */

#ifdef DEBUG
	printf("closeq: died on signal %d\n",sig);
#endif
	ev_close();
	/* retore tty */
	ioctl(0, TCSETA, &tsave);
	kill(echopid, SIGTERM);	
	kill(shellpid, SIGTERM);	
	exit(0);
}

static void
fail(s)
char	*s;
{
	extern int errno;

	printf("%s: %s(%d)\n", progname, s, errno);
	closeq(0);
}

static int
is_rbd(evp, oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	return !(oldbuttons & RT_BUTTON ) && EV_BUTTONS(*evp) & RT_BUTTON;
}

static int
is_rbu(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	return oldbuttons & RT_BUTTON && ! ( EV_BUTTONS(*evp) & RT_BUTTON);
}

static int
is_lbd(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	return !(oldbuttons & LT_BUTTON ) && EV_BUTTONS(*evp) & LT_BUTTON;
}

static int
is_lbu(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	return oldbuttons & LT_BUTTON && ! ( EV_BUTTONS(*evp) & LT_BUTTON);
}

static int
is_mbd(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	return !(oldbuttons & MD_BUTTON ) && EV_BUTTONS(*evp) & MD_BUTTON;
}

static int
is_mbu(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	return oldbuttons & MD_BUTTON && ! ( EV_BUTTONS(*evp) & MD_BUTTON);
}

static int 
is_rt(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	if ( EV_DX(*evp) < hsensitivity )
		return 0;
	return EV_DX(*evp) / hsensitivity;
}


static int 
is_lt(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	if ( EV_DX(*evp) > -hsensitivity )
		return 0;
	return EV_DX(*evp) / -hsensitivity;
}


static int 
is_up(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	if ( EV_DY(*evp) < vsensitivity )
		return 0;
	return EV_DY(*evp) / vsensitivity;
}


static int 
is_dn(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	if ( EV_DY(*evp) > -vsensitivity )
		return 0;
	return EV_DY(*evp) / -vsensitivity;
}

static int 
is_uprt(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	int	n;

	if ( ! ( EV_DY(*evp) >= vsensitivity && EV_DX(*evp) >= hsensitivity ) )
		return 0;
	n = MIN ( EV_DY(*evp)/vsensitivity, EV_DX(*evp)/hsensitivity );
	EV_DY(*evp) -= n * vsensitivity;
	EV_DX(*evp) -= n * hsensitivity;
	return n;
}

static int 
is_uplt(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	int	n;

	if ( ! ( EV_DY(*evp) >= vsensitivity && EV_DX(*evp) <= -hsensitivity ) )
		return 0;
	n = MIN ( EV_DY(*evp)/vsensitivity, EV_DX(*evp)/-hsensitivity );
	EV_DY(*evp) -= n * vsensitivity;
	EV_DX(*evp) += n * hsensitivity;
	return n;
}

static int 
is_dnrt(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	int	n;

	if ( ! ( EV_DY(*evp) <= -vsensitivity && EV_DX(*evp) >= hsensitivity ) )
		return 0;
	n = MIN ( EV_DY(*evp)/-vsensitivity, EV_DX(*evp)/hsensitivity );
	EV_DY(*evp) += n * vsensitivity;
	EV_DX(*evp) -= n * hsensitivity;
	return n;
}

static int 
is_dnlt(evp,oldbuttons)
EVENT	*evp;
char	oldbuttons;
{
	int	n;

	if ( ! ( EV_DY(*evp) <= -vsensitivity && EV_DX(*evp) <= -hsensitivity ) )
		return 0;
	n = MIN ( EV_DY(*evp)/-vsensitivity, EV_DX(*evp)/-hsensitivity );
	EV_DY(*evp) += n * vsensitivity;
	EV_DX(*evp) += n * hsensitivity;
	return n;
}

static void							/* v S005 v */
suspend(sig)
	int	sig;
{
	signal(SIGUSR1, suspend);
	if (!wait_for_acquire) {
		wait_for_acquire++;
		ioctl(0, VT_RELDISP, VT_TRUE);
		ev_suspend();
		pause();
	}
}

static void
resume(sig)
	int	sig;
{
	signal(SIGUSR2, resume);
	ioctl(0, VT_RELDISP, VT_ACKACQ);
	ev_resume();
	wait_for_acquire = 0;
}								/* ^ S005 ^ */
