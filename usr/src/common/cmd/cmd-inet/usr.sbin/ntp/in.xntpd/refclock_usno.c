#ident "@(#)refclock_usno.c	1.2"

/*
 * refclock_usno - clock driver for the Naval Observatory dialup
 * Michael Shields <shields@tembel.org> 1995/02/25
 */
#if defined(REFCLOCK) && defined(USNO)

#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_unixtime.h"
#include "ntp_refclock.h"
#include "ntp_stdlib.h"

/*
 * This driver supports the Naval Observatory dialup at +1 202 653 0351.
 * It is a hacked-up version of the ACTS driver.
 *
 * This driver does not support the `phone' configuration because that
 * is needlessly global; it would clash with the ACTS driver.
 *
 * The Naval Observatory does not support the echo-delay measurement scheme.
 *
 * However, this driver *does* support UUCP port locking, allowing the
 * line to be shared with other processes when not actually dialing
 * for time.
 */

/*
 * Interface definitions
 */

#define	DEVICE		"/dev/cua%d" /* device name and unit */
#define LOCKFILE	"/var/lock/LCK..cua%d"
/* #define LOCKFILE	"/usr/spool/uucp/LCK..cua%d" */

#define PHONE		"atdt 202 653 0351"
/* #define PHONE	"atdt 1 202 653 0351" */

#define	SPEED232	B1200	/* uart speed (1200 cowardly baud) */
#define	PRECISION	(-10)	/* precision assumed (about 1 ms) */
#define	REFID		"USNO"	/* reference ID */
#define	DESCRIPTION	"Naval Observatory dialup"

#define MODE_AUTO	0	/* automatic mode */
#define MODE_BACKUP	1	/* backup mode */
#define MODE_MANUAL	2	/* manual mode */

#define MSGCNT		10	/* we need this many time messages */
#define SMAX		80	/* max token string length */
#define LENCODE		20	/* length of valid timecode string */
#define USNO_MINPOLL	10	/* log2 min poll interval (1024 s) */
#define USNO_MAXPOLL	14	/* log2 max poll interval (16384 s) */
#define MAXOUTAGE	3600	/* max before USNO kicks in (s) */

/*
 * Modem control strings. These may have to be changed for some modems.
 *
 * AT	command prefix
 * B1	initiate call negotiation using Bell 212A
 * &C1	enable carrier detect
 * &D2	hang up and return to command mode on DTR transition
 * E0	modem command echo disabled
 * l1	set modem speaker volume to low level
 * M1	speaker enabled untill carrier detect
 * Q0	return result codes
 * V1	return result codes as English words
 */
#define MODEM_SETUP	"ATB1&C1&D2E0L1M1Q0V1" /* modem setup */
#define MODEM_HANGUP	"ATH"	/* modem disconnect */

/*
 * Timeouts
 */
#define IDLE		60	/* idle timeout (s) */
#define WAIT		2	/* wait timeout (s) */
#define ANSWER		30	/* answer timeout (s) */
#define CONNECT		10	/* connect timeout (s) */
#define TIMECODE	(MSGCNT+16)	/* timecode timeout (s) */

/*
 * Imported from ntp_timer module
 */
extern	u_long	current_time;	/* current time (s) */
extern	u_long	last_time;	/* last clock update time (s) */
extern	struct event timerqueue[]; /* inner space */

/*
 * Imported from ntpd module
 */
extern	int	debug;		/* global debug flag */

/*
 * Imported from ntp_proto module
 */
extern	struct peer *sys_peer;	/* who is running the show */
extern	u_char sys_poll;	/* log2 of system poll interval */
extern	struct peer *sys_peer;	/* system peer structure pointer */

/*
 * Unit control structure
 */
struct usnounit {
	struct	event timer;	/* timeout timer */
	int	pollcnt;	/* poll message counter */

	int	state;		/* the first one was Delaware */
	int	run;		/* call program run switch */
	int	msgcnt;		/* count of time messages received */
	long	redial;		/* interval to next automatic call */
	int	unit;		/* unit number (= port) */
};

/*
 * Function prototypes
 */
static	int	usno_start	P((int, struct peer *));
static	void	usno_shutdown	P((int, struct peer *));
static	void	usno_receive	P((struct recvbuf *));
static	void	usno_poll	P((int, struct peer *));
static	void	usno_timeout	P((struct peer *));
static	void	usno_disc	P((struct peer *));
static	int	usno_write	P((struct peer *, char *));

/*
 * Transfer vector
 */
struct	refclock refclock_usno = {
	usno_start,		/* start up driver */
	usno_shutdown,		/* shut down driver */
	usno_poll,		/* transmit poll message */
	noentry,		/* not used (usno_control) */
	noentry,		/* not used (usno_init) */
	noentry,		/* not used (usno_buginfo) */
	NOFLAGS			/* not used */
};


/*
 * usno_start - open the devices and initialize data for processing
 */
static int
usno_start(unit, peer)
	int unit;
	struct peer *peer;
{
	register struct usnounit *up;
	struct refclockproc *pp;

	/*
	 * Initialize miscellaneous variables
	 */
	pp = peer->procptr;
	peer->precision = PRECISION;
	pp->clockdesc = DESCRIPTION;
	memcpy((char *)&pp->refid, REFID, 4);
	peer->minpoll = USNO_MINPOLL;
	peer->maxpoll = USNO_MAXPOLL;

	/*
	 * Allocate and initialize unit structure
	 */
	if (!(up = (struct usnounit *)
	    emalloc(sizeof(struct usnounit))))
		return (0);
	memset((char *)up, 0, sizeof(struct usnounit));
	up->unit = unit;
	pp->unitptr = (caddr_t)up;

	/*
	 * Set up the driver timeout
	 */
	up->timer.peer = (struct peer *)peer;
	up->timer.event_handler = usno_timeout;
	up->timer.event_time = current_time + WAIT;
	TIMER_INSERT(timerqueue, &up->timer);
	return (1);
}


/*
 * usno_shutdown - shut down the clock
 */
static void
usno_shutdown(unit, peer)
	int unit;
	struct peer *peer;
{
	register struct usnounit *up;
	struct refclockproc *pp;

#ifdef DEBUG
	if (debug)
		printf("usno: clock %s shutting down\n", ntoa(&peer->srcadr));
#endif
	pp = peer->procptr;
	up = (struct usnounit *)pp->unitptr;
	TIMER_DEQUEUE(&up->timer);
	usno_disc(peer);
	TIMER_DEQUEUE(&up->timer);
	free(up);
}


/*
 * usno_receive - receive data from the serial interface
 */
static void
usno_receive(rbufp)
	struct recvbuf *rbufp;
{
	register struct usnounit *up;
	struct refclockproc *pp;
	struct peer *peer;
	char str[SMAX];
	u_fp disp;
	u_long mjd;		/* Modified Julian Day */
	static int day, hour, minute, second;

	/*
	 * Initialize pointers and read the timecode and timestamp. If
	 * the OK modem status code, leave it where folks can find it.
	 */
	peer = (struct peer *)rbufp->recv_srcclock;
	pp = peer->procptr;
	up = (struct usnounit *)pp->unitptr;
	pp->lencode = refclock_gtlin(rbufp, pp->lastcode, BMAX,
	    &pp->lastrec);
	if (pp->lencode == 0) {
		if (strcmp(pp->lastcode, "OK") == 0)
			pp->lencode = 2;
		return;
	}
#ifdef DEBUG
	if (debug)
        	printf("usno: timecode %d %s\n", pp->lencode,
		    pp->lastcode);
#endif

	switch (up->state) {

		case 0:

		/*
		 * State 0. We are not expecting anything. Probably
		 * modem disconnect noise. Go back to sleep.
		 */
		return;

		case 1:

		/*
		 * State 1. We are about to dial. Just drop it.
		 */
		return;

		case 2:

		/*
		 * State 2. We are waiting for the call to be answered.
		 * All we care about here is CONNECT as the first token
		 * in the string. If the modem signals BUSY, ERROR, NO
		 * ANSWER, NO CARRIER or NO DIALTONE, we immediately
		 * hang up the phone. If CONNECT doesn't happen after
		 * ANSWER seconds, hang up the phone. If everything is
		 * okay, start the connect timeout and slide into state
		 * 3.
		 */
		(void)strncpy(str, strtok(pp->lastcode, " "), SMAX);
		if (strcmp(str, "BUSY") == 0 || strcmp(str, "ERROR") ==
		     0 || strcmp(str, "NO") == 0) {
			TIMER_DEQUEUE(&up->timer);
			NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
			  syslog(LOG_NOTICE,
			    "clock %s USNO modem status %s",
			    ntoa(&peer->srcadr), pp->lastcode);
			usno_disc(peer);
		} else if (strcmp(str, "CONNECT") == 0) {
			TIMER_DEQUEUE(&up->timer);
			up->timer.event_time = current_time + CONNECT;
			TIMER_INSERT(timerqueue, &up->timer);
			up->msgcnt = 0;
			up->state++;
		} else {
			NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
			  syslog(LOG_WARNING,
			    "clock %s USNO unknown modem status %s",
			    ntoa(&peer->srcadr), pp->lastcode);
		}
		return;

		case 3:

		/*
		 * State 3. The call has been answered and we are
		 * waiting for the first message. If this doesn't
		 * happen within the timecode timeout, hang up the
		 * phone. We probably got a wrong number or they are
		 * down.
		 */
		TIMER_DEQUEUE(&up->timer);
		up->timer.event_time = current_time + TIMECODE;
		TIMER_INSERT(timerqueue, &up->timer);
		up->state++;
		return;

		case 4:

		/*
		 * State 4. We are reading a timecode.  It's an actual
		 * timecode, or it's the `*' OTM.
		 *
		 * jjjjj nnn hhmmss UTC
		 */
		if (pp->lencode == LENCODE) {
			if (sscanf(pp->lastcode, "%5ld %3d %2d%2d%2d UTC",
			    &mjd, &day, &hour, &minute, &second) != 5) {
#ifdef DEBUG
				if (debug)
					printf("usno: bad timecode format\n");
#endif
				refclock_report(peer, CEVNT_BADREPLY);
			} else
				up->msgcnt++;
			return;
		} else if (pp->lencode != 1 || !up->msgcnt)
			return;
		/* else, OTM; drop out of switch */
	}

	pp->leap = LEAP_NOWARNING;
	pp->day = day;
	pp->hour = hour;
	pp->minute = minute;
	pp->second = second;
	pp->lasttime = current_time;

	/*
	 * Colossal hack here. We process each sample in a trimmed-mean
	 * filter and determine the reference clock offset and
	 * dispersion. The fudge time1 value is added to each sample as
	 * received.
	 */
	if (!refclock_process(pp, up->msgcnt, up->msgcnt - up->msgcnt / 3)) {
#ifdef DEBUG
		if (debug)
			printf("usno: time rejected\n");
#endif
		refclock_report(peer, CEVNT_BADTIME);
		return;
	} else if (up->msgcnt < MSGCNT)
		return;

	/*
	 * We have a filtered sample offset ready for peer processing.
	 * We use lastrec as both the reference time and receive time in
	 * order to avoid being cute, like setting the reference time
	 * later than the receive time, which may cause a paranoid
	 * protocol module to chuck out the data. Finaly, we unhook the
	 * timeout, arm for the next call, fold the tent and go home.
	 */
	disp = LFPTOFP(&pp->fudgetime2);
	record_clock_stats(&peer->srcadr, pp->lastcode);
	refclock_receive(peer, &pp->offset, 0, pp->dispersion +
	    (u_fp)disp, &pp->lastrec, &pp->lastrec, pp->leap);
	pp->sloppyclockflag &= ~CLK_FLAG1;
	up->pollcnt = 0;
	TIMER_DEQUEUE(&up->timer);
	up->state = 0;
	usno_disc(peer);
}


/*
 * usno_poll - called by the transmit routine
 */
static void
usno_poll(unit, peer)
	int unit;
	struct peer *peer;
{
	register struct usnounit *up;
	struct refclockproc *pp;

	/*
	 * If the driver is running, we set the enable flag (fudge
	 * flag1), which causes the driver timeout routine to initiate a
	 * call. If not, the enable flag can be set using
	 * xntpdc. If this is the sustem peer, then follow the system
	 * poll interval.
	 */
	pp = peer->procptr;
	up = (struct usnounit *)pp->unitptr;
	if (up->run) {
		pp->sloppyclockflag |= CLK_FLAG1;
		if (peer == sys_peer)
			peer->hpoll = sys_poll;
		else
			peer->hpoll = peer->minpoll;
	}
}


/*
 * usno_timeout - called by the timer interrupt
 */
static void
usno_timeout(peer)
	struct peer *peer;
{
	register struct usnounit *up;
	struct refclockproc *pp;
	int fd;
	char device[20];
	char lockfile[128], pidbuf[8];
	int dtr = TIOCM_DTR;

	/*
	 * If a timeout occurs in other than state 0, the call has
	 * failed. If in state 0, we just see if there is other work to
	 * do.
	 */
	pp = peer->procptr;
	up = (struct usnounit *)pp->unitptr;
	if (up->state) {
		if (up->state != 1) {
			usno_disc(peer);
			return;
		}
		/*
		 * Call, and start the answer timeout. We think it
		 * strange if the OK status has not been received from
		 * the modem, but plow ahead anyway.
		 *
		 * This code is *here* because we had to stick in a brief
		 * delay to let the modem settle down after raising DTR,
		 * and for the OK to be received.  State machines are
		 * contorted.
		 */
		if (strcmp(pp->lastcode, "OK") != 0)
			NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
			  syslog(LOG_NOTICE, "clock %s USNO no modem status",
			    ntoa(&peer->srcadr));
		(void)usno_write(peer, PHONE);
		NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
		  syslog(LOG_NOTICE, "clock %s USNO calling %s\n",
		    ntoa(&peer->srcadr), PHONE);
		up->state = 2;
		up->pollcnt++;
		pp->polls++;
		up->timer.event_time = current_time + ANSWER;
		TIMER_INSERT(timerqueue, &up->timer);
		return;
	}
	switch (peer->ttl) {

		/*
		 * In manual mode the calling program is activated
		 * by the xntpdc program using the enable flag (fudge
		 * flag1), either manually or by a cron job.
		 */
		case MODE_MANUAL:
		up->run = 0;
		break;

		/*
		 * In automatic mode the calling program runs
		 * continuously at intervals determined by the sys_poll
		 * variable.
		 */
		case MODE_AUTO:
		if (!up->run)
			pp->sloppyclockflag |= CLK_FLAG1;
		up->run = 1;
		break;

		/*
		 * In backup mode the calling program is disabled,
		 * unless no system peer has been selected for MAXOUTAGE
		 * (3600 s). Once enabled, it runs until some other NTP
		 * peer shows up.
		 */
		case MODE_BACKUP:
		if (!up->run && sys_peer == 0) {
			if (current_time - last_time > MAXOUTAGE) {
				up->run = 1;
				peer->hpoll = peer->minpoll;
				NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
				  syslog(LOG_NOTICE,
				    "clock %s USNO backup started ",
				    ntoa(&peer->srcadr));
			}
		} else if (up->run && sys_peer->refclktype !=
		    REFCLK_NIST_ACTS && sys_peer->refclktype != REFCLK_USNO) {
			peer->hpoll = peer->minpoll;
			up->run = 0;
			NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
			  syslog(LOG_NOTICE,
			    "clock %s USNO backup stopped",
			    ntoa(&peer->srcadr));
		}
		break;

		default:
		syslog(LOG_ERR,
		    "clock %s USNO invalid mode", ntoa(&peer->srcadr));
		
	}

	/*
	 * The fudge flag1 is used as an enable/disable; if set either
	 * by the code or via xntpdc, the calling program is
	 * started; if reset, the phones stop ringing.
	 */
	if (!(pp->sloppyclockflag & CLK_FLAG1)) {
		up->pollcnt = 0;
		up->timer.event_time = current_time + IDLE;
		TIMER_INSERT(timerqueue, &up->timer);
		return;
	}

	/*
	 * Lock the port.
	 */
	(void)sprintf(lockfile, LOCKFILE, up->unit);
	fd = open(lockfile, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) {
		syslog(LOG_ERR, "clock %s USNO port busy",
		    ntoa(&peer->srcadr));
		return;
	}
	sprintf(pidbuf, "%d\n", getpid());
	write(fd, pidbuf, strlen(pidbuf));
	close(fd);

	/*
	 * Open serial port. Use ACTS line discipline, if available. It
	 * pumps a timestamp into the data stream at every on-time
	 * character '*' found. Note: the port must have modem control
	 * or deep pockets for the phone bill. HP-UX 9.03 users should
	 * have very deep pockets.
	 */
	(void)sprintf(device, DEVICE, up->unit);
	if (!(fd = refclock_open(device, SPEED232, LDISC_ACTS))) {
		unlink(lockfile);
		return;
	}
	if (ioctl(fd, TIOCMBIC, (char *)&dtr) < 0)
		syslog(LOG_WARNING, "clock %s USNO no modem control",
		    ntoa(&peer->srcadr));

	pp->io.clock_recv = usno_receive;
	pp->io.srcclock = (caddr_t)peer;
	pp->io.datalen = 0;
	pp->io.fd = fd;
	if (!io_addclock(&pp->io)) {
		(void) close(fd);
		unlink(lockfile);
		free(up);
		return;
	}

	/*
	 * Initialize modem and kill DTR. We skedaddle if this comes
	 * bum.
	 */
	if (!usno_write(peer, MODEM_SETUP)) {
		syslog(LOG_ERR, "clock %s USNO couldn't write",
		    ntoa(&peer->srcadr));
		io_closeclock(&pp->io);
		unlink(lockfile);
		free(up);
		return;
	}

	/*
	 * Initiate a call to the Observatory. If we wind up here in
	 * other than state 0, a successful call could not be completed
	 * within minpoll seconds.
	 */
	if (up->pollcnt) {
		refclock_report(peer, CEVNT_TIMEOUT);
		NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
		  syslog(LOG_NOTICE,
		    "clock %s USNO calling program terminated",
		    ntoa(&peer->srcadr));
		pp->sloppyclockflag &= ~CLK_FLAG1;
		up->pollcnt = 0;
#ifdef DEBUG
		if (debug)
			printf("usno: calling program terminated\n");
#endif
		usno_disc(peer);
		return;
	}

	/*
	 * Raise DTR, and let the modem settle.  Then we'll dial.
	 */
	(void)ioctl(pp->io.fd, TIOCMBIS, (char *)&dtr);
	up->state = 1;
	up->timer.event_time = current_time + WAIT;
	TIMER_INSERT(timerqueue, &up->timer);
}


/*
 * usno_disc - disconnect the call and wait for the ruckus to cool
 */
static void
usno_disc(peer)
	struct peer *peer;
{
	register struct usnounit *up;
	struct refclockproc *pp;
	int dtr = TIOCM_DTR;
	char lockfile[128];

	/*
	 * We should never get here other than in state 0, unless a call
	 * has timed out. We drop DTR, which will reliably get the modem
	 * off the air, even while the modem is hammering away full tilt.
	 */
	pp = peer->procptr;
	up = (struct usnounit *)pp->unitptr;

	(void)ioctl(pp->io.fd, TIOCMBIC, (char *)&dtr);

	if (up->state > 0) {
		up->state = 0;
		syslog(LOG_NOTICE, "clock %s USNO call failed %d",
		    ntoa(&peer->srcadr), up->state);
#ifdef DEBUG
		if (debug)
			printf("usno: call failed %d\n", up->state);
#endif
	}

	io_closeclock(&pp->io);
	sprintf(lockfile, LOCKFILE, up->unit);
	unlink(lockfile);

	up->timer.event_time = current_time + WAIT;
	TIMER_INSERT(timerqueue, &up->timer);
}


/*
 * usno_write - write a message to the serial port
 */
static int
usno_write(peer, str)
	struct peer *peer;
	char *str;
{
	register struct usnounit *up;
	struct refclockproc *pp;
	int len;
	int code;
	char cr = '\r';

	/*
	 * Not much to do here, other than send the message, handle
	 * debug and report faults.
	 */
	pp = peer->procptr;
	up = (struct usnounit *)pp->unitptr;
	len = strlen(str);
#ifdef DEBUG
	if (debug)
		printf("usno: state %d send %d %s\n", up->state, len,
		    str);
#endif
	code = write(pp->io.fd, str, len) == len;
	code |= write(pp->io.fd, &cr, 1) == 1;
	if (!code)
		refclock_report(peer, CEVNT_FAULT);
	return (code);
}

#endif
