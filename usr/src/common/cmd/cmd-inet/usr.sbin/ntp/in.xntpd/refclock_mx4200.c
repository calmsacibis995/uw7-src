#ident "@(#)refclock_mx4200.c	1.2"

/*
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66.
 *
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 * 4. The name of the University may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(REFCLOCK) && defined(MX4200) && defined(PPS)

#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_refclock.h"
#include "ntp_calendar.h"
#include "ntp_unixtime.h"

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "mx4200.h"

#include "ntp_stdlib.h"

#ifdef PPS
#include <sys/ppsclock.h>
#endif /* PPS */

/*
 * This driver supports the Magnavox Model MX4200 GPS Receiver.
 */

/*
 * Definitions
 */
#define	DEVICE		"/dev/gps%d"	/* device name and unit */
#define	SPEED232	B4800		/* baud */

/*
 * The number of raw samples which we acquire to derive a single estimate.
 */
#define	NSTAMPS	64

/*
 * Radio interface parameters
 */
#define	PRECISION	(-18)	/* precision assumed (about 4 us) */
#define	REFID	"GPS"		/* reference id */
#define	DESCRIPTION	"Magnavox MX4200 GPS Receiver" /* who we are */
#define	DEFFUDGETIME	0	/* default fudge time (ms) */

/* Leap stuff */
extern u_long leap_hoursfromleap;
extern u_long leap_happened;
static int leap_debug;

/*
 * Imported from the timer module
 */
extern u_long current_time;
extern struct event timerqueue[];

/*
 * Imported from ntp_loopfilter module
 */
extern int fdpps;		/* pps file descriptor */

/*
 * Imported from ntpd module
 */
extern int debug;		/* global debug flag */

/*
 * MX4200 unit control structure.
 */
struct mx4200unit {
	u_long gpssamples[NSTAMPS];	/* the GPS time samples */
	l_fp unixsamples[NSTAMPS];	/* the UNIX time samples */


	l_fp lastsampletime;		/* time of last estimate */
	u_int lastserial;		/* last pps serial number */
	u_long lasttime;		/* last time clock heard from */
	u_char nsamples;		/* samples collected */
};

/*
 * We demand that consecutive PPS samples are more than 0.995 seconds
 * and less than 1.005 seconds apart.
 */
#define	PPSLODIFF_UI	0		/* 0.900 as an l_fp */
#define	PPSLODIFF_UF	0xe6666610

#define	PPSHIDIFF_UI	1		/* 1.100 as an l_fp */
#define	PPSHIDIFF_UF	0x19999990

static char pmvxg[] = "PMVXG";

/*
 * Function prototypes
 */
static	int	mx4200_start	P((int, struct peer *));
static	void	mx4200_shutdown	P((int, struct peer *));
static	void	mx4200_receive	P((struct recvbuf *));
static	void	mx4200_process	P((struct peer *));
static	void	mx4200_poll	P((int, struct peer *));

static	char *	mx4200_parse	P((char *, struct calendar *, int *, int *));
static	int	mx4200_needconf	P((char *));
static	void	mx4200_config	P((struct peer *));
static	void	mx4200_send	P((int, char *, ...));
static	int	mx4200_cmpl_fp	P((void *, void *));
static	u_char	cksum		P((char *, u_int));

/*
 * Transfer vector
 */
struct	refclock refclock_mx4200 = {
	mx4200_start,		/* start up driver */
	mx4200_shutdown,	/* shut down driver */
	mx4200_poll,		/* transmit poll message */
	noentry,		/* not used (old mx4200_control) */
	noentry,		/* initialize driver (not used) */
	noentry,		/* not used (old mx4200_buginfo) */
	NOFLAGS			/* not used */
};


/*
 * mx4200_start - open the devices and initialize data for processing
 */
static int
mx4200_start(unit, peer)
	int unit;
	struct peer *peer;
{
	register struct mx4200unit *up;
	struct refclockproc *pp;
	int fd;
	char gpsdev[20];

	/*
	 * Open serial port
	 */
	(void)sprintf(gpsdev, DEVICE, unit);
	if (!(fd = refclock_open(gpsdev, SPEED232, 0)))
		return (0);

	/*
	 * Allocate unit structure
	 */
	if (!(up = (struct mx4200unit *)
	    emalloc(sizeof(struct mx4200unit)))) {
		(void) close(fd);
		return (0);
	}
	memset((char *)up, 0, sizeof(struct mx4200unit));
	pp = peer->procptr;
	pp->io.clock_recv = mx4200_receive;
	pp->io.srcclock = (caddr_t)peer;
	pp->io.datalen = 0;
	pp->io.fd = fd;
	if (!io_addclock(&pp->io)) {
		(void) close(fd);
		free(up);
		return (0);
	}
	pp->unitptr = (caddr_t)up;

	/*
	 * Initialize miscellaneous variables
	 */
	peer->precision = PRECISION;
	pp->clockdesc = DESCRIPTION;
	memcpy((char *)&pp->refid, REFID, 4);

	/* Insure the receiver is properly configured */
	mx4200_config(peer);
	return (1);
}


/*
 * mx4200_shutdown - shut down the clock
 */
static void
mx4200_shutdown(unit, peer)
	int unit;
	struct peer *peer;
{
	register struct mx4200unit *up;
	struct refclockproc *pp;

	pp = peer->procptr;
	up = (struct mx4200unit *)pp->unitptr;
	io_closeclock(&pp->io);
	free(up);
}


static void
mx4200_config(peer)
	struct peer *peer;
{
	register struct mx4200unit *up;
	struct refclockproc *pp;

	/*
	 * Zero the output list (do it twice to flush possible junk)
	 */
	pp = peer->procptr;
	up = (struct mx4200unit *)pp->unitptr;
	mx4200_send(pp->io.fd, "%s,%03d,,%d,,,,,,", pmvxg,
	    PMVXG_S_PORTCONF, 1);
	mx4200_send(pp->io.fd, "%s,%03d,,%d,,,,,,", pmvxg,
	    PMVXG_S_PORTCONF, 1);

	/*
	 * Switch to 2d mode
	 */
	mx4200_send(pp->io.fd, "%s,%03d,%d,,%.1f,%.1f,,%d,%d,%c,%d",
	    pmvxg, PMVXG_S_INITMODEB,
	    2,		/* 2d mode */
	    0.1,	/* hor accel fact as per Steve */
	    0.1,	/* ver accel fact as per Steve */
	    10,		/* hdop limit as per Steve */
	    5,		/* elevation limit as per Steve */
	    'U',	/* time output mode */
	    0);		/* local time offset from gmt */

	/*
	 * Configure time recovery
	 */
	mx4200_send(pp->io.fd, "%s,%03d,%c,%c,%c,%d,%d,%d,", pmvxg,
	    PMVXG_S_TRECOVCONF,
#ifdef notdef
	    'K',	/* known position */
	    'D',	/* dynamic position */
#else
	    'S',	/* static position */
#endif
	    'U',	/* steer clock to gps time */
	    'A',	/* always output time pulse */
	    500,	/* max time error in ns */
	    0,		/* user bias in ns */
	    1);		/* output to control port */
}


/*
 * mx4200_poll - mx4200 watchdog routine
 */
static void
mx4200_poll(unit, peer)
	int unit;
	struct peer *peer;
{
	register struct mx4200unit *up;
	struct refclockproc *pp;

	pp = peer->procptr;
	up = (struct mx4200unit *)pp->unitptr;
	if ((current_time - up->lasttime) > 150) {
		refclock_report(peer, CEVNT_FAULT);

		/*
		 * Request a status message which should trigger a
		 * reconfig
		 */
		mx4200_send(pp->io.fd, "%s,%03d", "CDGPQ",
		    PMVXG_D_STATUS);
	}
	pp->polls++;
}

static char char2hex[] = "0123456789ABCDEF";

/*
 * mx4200_receive - receive gps data
 */
static void
mx4200_receive(rbufp)
	struct recvbuf *rbufp;
{
	register struct mx4200unit *up;
	struct refclockproc *pp;
	struct peer *peer;
	register char *dpt, *cp;
	register u_long tmp_ui;
	register u_long tmp_uf;
	register u_long gpstime;
	struct ppsclockev ev;
	register struct calendar *jt;
	struct calendar sjt;
	register int n;
	int valid, leapsec;
	register u_char ck;

	peer = (struct peer *)rbufp->recv_srcclock;
	pp = peer->procptr;
	up = (struct mx4200unit *)pp->unitptr;

#ifdef DEBUG
	if (debug > 3)
		printf("mx4200_receive: nsamples = %d\n", up->nsamples);
#endif

	/* Record the time of this event */
	up->lasttime = current_time;

	/* Get the pps value */
	if (ioctl(fdpps, CIOGETEV, (char *)&ev) < 0) {
		/* XXX Actually, if this fails, we're pretty much screwed */
#ifdef DEBUG
		if (debug) {
			fprintf(stderr, "mx4200_receive: ");
			perror("CIOGETEV");
		}
#endif
		refclock_report(peer, CEVNT_FAULT);
		up->nsamples = 0;
		return;
	}
	tmp_ui = ev.tv.tv_sec + (u_long)JAN_1970;
	TVUTOTSF(ev.tv.tv_usec, tmp_uf);

	/* Get buffer and length; sock away last timecode */
	n = rbufp->recv_length;
	dpt = rbufp->recv_buffer;
	if (n <= 1)
		return;
	pp->lencode = n;
	memmove(pp->lastcode, dpt, n);

	/*
	 * We expect to see something like:
	 *
	 *    $PMVXG,830,T,1992,07,09,04:18:34,U,S,-02154,00019,000000,00*1D\n
	 *
	 * Reject if any important landmarks are missing.
	 */
	cp = dpt + n - 4;
	if (cp < dpt || *dpt != '$' || cp[0] != '*' || cp[3] != '\n') {
#ifdef DEBUG
		if (debug)
			printf("mx4200_receive: bad format\n");
#endif
		refclock_report(peer, CEVNT_BADREPLY);
		up->nsamples = 0;
		return;
	}

	/* Check checksum */
	ck = cksum(&dpt[1], n - 5);
	if (char2hex[ck >> 4] != cp[1] || char2hex[ck & 0xf] != cp[2]) {
#ifdef DEBUG
		if (debug)
			printf("mx4200_receive: bad checksum\n");
#endif
		refclock_report(peer, CEVNT_BADREPLY);
		up->nsamples = 0;
		return;
	}

	/* Truncate checksum (and the buffer for that matter) */
	*cp = '\0';

	/* Leap second debugging stuff */
	if ((leap_hoursfromleap && !leap_happened) || leap_debug > 0) {
		/* generate reports for awhile after leap */
		if (leap_hoursfromleap && !leap_happened)
			leap_debug = 3600;
		else
			--leap_debug;
		NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
		  syslog(LOG_INFO, "mx4200 leap: %s \"%s\"",
		    umfptoa(tmp_ui, tmp_uf, 6), dpt);
	}

	/* Parse time recovery message */
	jt = &sjt;
	if ((cp = mx4200_parse(dpt, jt, &valid, &leapsec)) != NULL) {
		/* Configure the receiver if necessary */
		if (mx4200_needconf(dpt))
			mx4200_config(peer);
#ifdef DEBUG
		if (debug)
			printf("mx4200_receive: mx4200_parse: %s\n",
			    cp);
#endif
		refclock_report(peer, CEVNT_BADREPLY);
		up->nsamples = 0;
		return;
	}

	/* Setup leap second indicator */
	if (leapsec == 0)
		pp->leap = LEAP_NOWARNING;
	else if (leapsec == 1)
		pp->leap = LEAP_ADDSECOND;
	else if (leapsec == -1)
		pp->leap = LEAP_DELSECOND;
	else
		pp->leap = LEAP_NOTINSYNC;	/* shouldn't happen */

	/* Check parsed time (allow for possible leap seconds) */
	if (jt->second >= 61 || jt->minute >= 60 || jt->hour >= 24) {
#ifdef DEBUG
		if (debug) {
			printf("mx4200_receive: bad time %d:%02d:%02d",
			    jt->hour, jt->minute, jt->second);
			if (leapsec != 0)
				printf(" (leap %+d)", leapsec);
			putchar('\n');
		}
#endif
		refclock_report(peer, CEVNT_BADTIME);
		up->nsamples = 0;
		/* Eat the next pulse which the clock claims will be bad */
		up->nsamples = -1;
		return;
	}

	/* Check parsed date */
	if (jt->monthday > 31 || jt->month > 12 || jt->year < 1900) {
#ifdef DEBUG
		if (debug)
			printf("mx4200_receive: bad date (%d/%d/%d)\n",
			    jt->monthday, jt->month, jt->year);
#endif
		refclock_report(peer, CEVNT_BADDATE);
		up->nsamples = 0;
		return;
	}

	/* Convert to ntp time */
	gpstime = caltontp(jt);

	/* The gps message describes the *next* pulse; pretend it's this one */
	--gpstime;

	/* Check pps serial number against last one */
	if (up->lastserial + 1 != ev.serial && up->lastserial != 0) {
#ifdef DEBUG
		if (debug) {
			if (ev.serial == up->lastserial)
				printf("mx4200_receive: no new pps event\n");
			else
				printf("mx4200_receive: missed %d pps events\n",
				    ev.serial - up->lastserial - 1);
		}
#endif
		refclock_report(peer, CEVNT_FAULT);
		up->nsamples = 0;
		/* fall through and this one collect as first sample */
	}
	up->lastserial = ev.serial;

/*
 * XXX
 * Since this message is for the next pulse, it's really the next pulse
 * that the clock might be telling us will be invalid.
 */
	/* Toss if not designated "valid" by the gps */
	if (!valid) {
#ifdef DEBUG
		if (debug)
			printf("mx4200_receive: pps not valid\n");
#endif
		refclock_report(peer, CEVNT_BADTIME);
		up->nsamples = 0;
		return;
	}

	/*
	 * Copy time data for billboard monitoring. Yes, the day is
	 * wrong.
	 */
	pp->year = jt->year;
	pp->day = jt->monthday;
	pp->hour = jt->hour;
	pp->minute = jt->minute;
	pp->second = jt->second;

	/* Sock away the GPS and UNIX timesamples */
	n = up->nsamples++;
	if (n < 0)
		return;			/* oops, this pulse is bad */
	up->gpssamples[n] = gpstime;
	up->unixsamples[n].l_ui = up->lastsampletime.l_ui = tmp_ui;
	up->unixsamples[n].l_uf = up->lastsampletime.l_uf = tmp_uf;
	if (up->nsamples >= NSTAMPS) {
		/*
		 * Here we've managed to complete an entire NSTAMPS
		 * second cycle without major mishap. Process what has
		 * been received.
		 */
		mx4200_process(peer);
		up->nsamples = 0;
	}
}

/* Compare two l_fp's, used with qsort() */
static int
mx4200_cmpl_fp(p1, p2)
	register void *p1, *p2;
{

	if (!L_ISGEQ((l_fp *)p1, (l_fp *)p2))
		return (-1);
	if (L_ISEQU((l_fp *)p1, (l_fp *)p2))
		return (0);
	return (1);
}

/*
 * mx4200_process - process a pile of samples from the clock
 */
static void
mx4200_process(peer)
	struct peer *peer;
{
	register struct mx4200unit *up;
	struct refclockproc *pp;
	register int i, n;
	register l_fp *fp, *op;
	register u_long *lp;
	l_fp off[NSTAMPS];
	register u_long tmp_ui, tmp_uf;
	register u_long date_ui, date_uf;
	u_fp dispersion;

	/* Compute offsets from the raw data. */
	pp = peer->procptr;
	up = (struct mx4200unit *)pp->unitptr;
	fp = up->unixsamples;
	op = off;
	lp = up->gpssamples;
	for (i = 0; i < NSTAMPS; ++i, ++lp, ++op, ++fp) {
		op->l_ui = *lp;
		op->l_uf = 0;
		L_SUB(op, fp);
	}

	/* Sort offsets into ascending order. */
	qsort((char *)off, NSTAMPS, sizeof(l_fp), mx4200_cmpl_fp);

	/*
	 * Reject the furthest from the median until 8 samples left
	 */
	i = 0;
	n = NSTAMPS;
	while ((n - i) > 8) {
		tmp_ui = off[n-1].l_ui;
		tmp_uf = off[n-1].l_uf;
		date_ui = off[(n+i)/2].l_ui;
		date_uf = off[(n+i)/2].l_uf;
		M_SUB(tmp_ui, tmp_uf, date_ui, date_uf);
		M_SUB(date_ui, date_uf, off[i].l_ui, off[i].l_uf);
		if (M_ISHIS(date_ui, date_uf, tmp_ui, tmp_uf)) {
			/*
			 * reject low end
			 */
			i++;
		} else {
			/*
			 * reject high end
			 */
			n--;
		}
	}

	/*
	 * Compute the dispersion based on the difference between the
	 * extremes of the remaining offsets.
	 */
	tmp_ui = off[n-1].l_ui;
	tmp_uf = off[n-1].l_uf;
	M_SUB(tmp_ui, tmp_uf, off[i].l_ui, off[i].l_uf);
	dispersion = MFPTOFP(tmp_ui, tmp_uf);

	/*
	 * Now compute the offset estimate.  If the sloppy clock
	 * flag is set, average the remainder, otherwise pick the
	 * median.
	 */
	if (pp->sloppyclockflag) {
		tmp_ui = tmp_uf = 0;
		while (i < n) {
			M_ADD(tmp_ui, tmp_uf, off[i].l_ui, off[i].l_uf);
			i++;
		}
		M_RSHIFT(tmp_ui, tmp_uf);
		M_RSHIFT(tmp_ui, tmp_uf);
		M_RSHIFT(tmp_ui, tmp_uf);
		i = 0;
		off[0].l_ui = tmp_ui;
		off[0].l_uf = tmp_uf;
	} else {
		i = (n + i) / 2;
	}

	/*
	 * Done. Use lastref as the reference time and lastrec
	 * as the receive time. ** note this can result in tossing
	 * out the peer in the protocol module if lastref > lastrec,
	 * so last rec is used for both values - dlm ***
	 */
	refclock_receive(peer, &off[i], 0, dispersion,
	    &up->unixsamples[NSTAMPS-1], &up->unixsamples[NSTAMPS-1],
	    pp->leap);
	refclock_report(peer, CEVNT_NOMINAL);
}

/*
 * Returns true if the this is a status message. We use this as
 * an indication that the receiver needs to be initialized.
 */
static int
mx4200_needconf(buf)
	char *buf;
{
	register int32 v;
	char *cp;

	cp = buf;

	if ((cp = strchr(cp, ',')) == NULL)
		return (0);
	++cp;

	/* Record type */
	v = strtol(cp, &cp, 10);
	if (v != PMVXG_D_STATUS)
		return (0);
	/*
	 * XXX
	 * Since we configure the receiver to not give us status
	 * messages and since the receiver outputs status messages by
	 * default after being reset to factory defaults when sent the
	 * "$PMVXG,018,C\r\n" message, any status message we get
	 * indicates the reciever needs to be initialized; thus, it is
	 * not necessary to decode the status message.
	 */
		return (1);
}

/* Parse a mx4200 time recovery message. Returns a string if error */
static char *
mx4200_parse(buf, jt, validp, leapsecp)
	register char *buf;
	register struct calendar *jt;
	register int *validp, *leapsecp;
{
	register int32 v;
	char *cp;

	cp = buf;
	memset((char *)jt, 0, sizeof(*jt));

	if ((cp = strchr(cp, ',')) == NULL)
		return ("no rec-type");
	++cp;

	/* Record type */
	v = strtol(cp, &cp, 10);
	if (v != PMVXG_D_TRECOVOUT)
		return ("wrong rec-type");

	/* Pulse valid indicator */
	if (*cp++ != ',')
		return ("no pulse-valid");
	if (*cp == 'T')
		*validp = 1;
	else if (*cp == 'F')
		*validp = 0;
	else
		return ("bad pulse-valid");
	++cp;

	/* Year */
	if (*cp++ != ',')
		return ("no year");
	jt->year = strtol(cp, &cp, 10);

	/* Month of year */
	if (*cp++ != ',')
		return ("no month");
	jt->month = strtol(cp, &cp, 10);

	/* Day of month */
	if (*cp++ != ',')
		return ("no month day");
	jt->monthday = strtol(cp, &cp, 10);

	/* Hour */
	if (*cp++ != ',')
		return ("no hour");
	jt->hour = strtol(cp, &cp, 10);

	/* Minute */
	if (*cp++ != ':')
		return ("no minute");
	jt->minute = strtol(cp, &cp, 10);

	/* Second */
	if (*cp++ != ':')
		return ("no second");
	jt->second = strtol(cp, &cp, 10);

	/* Time indicator */
	if (*cp++ != ',' || *cp++ == '\0')
		return ("no time indicator");

	/* Time recovery mode */
	if (*cp++ != ',' || *cp++ == '\0')
		return ("no time mode");

	/* Oscillator offset */
	if ((cp = strchr(cp, ',')) == NULL)
		return ("no osc off");
	++cp;

	/* Time mark error */
	if ((cp = strchr(cp, ',')) == NULL)
		return ("no time mark err");
	++cp;

	/* User time bias */
	if ((cp = strchr(cp, ',')) == NULL)
		return ("no user bias");
	++cp;

	/* Leap second flag */
	if ((cp = strchr(cp, ',')) == NULL)
		return ("no leap");
	++cp;
	*leapsecp = strtol(cp, &cp, 10);

	return (NULL);
}

/* Calculate the checksum */
static u_char
cksum(cp, n)
	register char *cp;
	register u_int n;
{
	register u_char ck;

	for (ck = 0; n-- > 0; ++cp)
		ck ^= *cp;
	return (ck);
}

static void
#if __STDC__
mx4200_send(register int fd, char *fmt, ...)
#else
mx4200_send(fd, fmt, va_alist)
	register int fd;
	char *fmt;
	va_dcl
#endif
{
	register char *cp;
	register int n, m;
	va_list ap;
	char buf[1024];
	u_char ck;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	cp = buf;
	*cp++ = '$';
#ifdef notdef
	/* BSD is rational */
	n = vsnprintf(cp, sizeof(buf) - 1, fmt, ap);
#else
	/* SunOS sucks */
	(void)vsprintf(cp, fmt, ap);
	n = strlen(cp);
#endif
	ck = cksum(cp, n);
	cp += n;
	++n;
#ifdef notdef
	/* BSD is rational */
	n += snprintf(cp, sizeof(buf) - n - 5, "*%02X\r\n", ck);
#else
	/* SunOS sucks */
	sprintf(cp, "*%02X\r\n", ck);
	n += strlen(cp);
#endif

	m = write(fd, buf, n);
	if (m < 0)
		syslog(LOG_ERR, "mx4200_send: write: %m (%s)", buf);
#ifdef DEBUG
	if (debug)
		printf("mx4200_send: %d %s\n", m, buf);
#endif
	va_end(ap);
}

#endif
