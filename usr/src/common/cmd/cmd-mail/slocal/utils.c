/*
 * conglomeration of all of the mh utilities
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#ifdef OpenServer
# include <sys/select.h>
#endif
#include <time.h>
#include <sys/resource.h>
#include <pwd.h>
#include <stdlib.h>
#include "tws.h"
#include "string.h"
#include "mh.h"

extern int  daylight;
extern long timezone;
extern char *tzname[];

#ifndef	abs
#define	abs(a)	(a >= 0 ? a : -a)
#endif

#define	dysize(y)	\
	(((y) % 4) ? 365 : (((y) % 100) ? 366 : (((y) % 400) ? 365 : 366)))

char *tw_moty[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL
};

char *tw_dotw[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL
};

char *tw_ldotw[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday", NULL
};

static struct zone {
    char   *std,
           *dst;
    int     shift;
}   zones[] = {
	"GMT", "BST", 0,
        "EST", "EDT", -5,
        "CST", "CDT", -6,
        "MST", "MDT", -7,
        "PST", "PDT", -8,
        NULL
};

#define CENTURY 1900

long    time ();
struct tm *localtime ();

/*  */

char *dtimenow () {
    long    clock;

    (void) time (&clock);
    return dtime (&clock);
}

struct tws *dtwstime () {
    long    clock;

    (void) time (&clock);
    return dlocaltime (&clock);
}


struct tws *dlocaltime (clock)
register long   *clock;
{
    register struct tm *tm;
    static struct tws   tw;

    if (!clock)
	return NULL;
    tw.tw_flags = TW_NULL;

    tm = localtime (clock);
    tw.tw_sec = tm -> tm_sec;
    tw.tw_min = tm -> tm_min;
    tw.tw_hour = tm -> tm_hour;
    tw.tw_mday = tm -> tm_mday;
    tw.tw_mon = tm -> tm_mon;
    tw.tw_year = tm -> tm_year + CENTURY;
    tw.tw_wday = tm -> tm_wday;
    tw.tw_yday = tm -> tm_yday;
    if (tm -> tm_isdst)
	tw.tw_flags |= TW_DST;
    tzset ();
    tw.tw_zone = -(timezone / 60);
    tw.tw_flags &= ~TW_SDAY, tw.tw_flags |= TW_SEXP;
    tw.tw_flags &= ~TW_SZONE, tw.tw_flags |= TW_SZEXP;
    tw.tw_clock = *clock;

    return (&tw);
}

char   *dasctime (tw, flags)
register struct tws *tw;
int	flags;
{
    char buffer[80];
    static char result[80];

    if (!tw)
	return NULL;

    /* Display timezone if known */
    if ((tw->tw_flags & TW_SZONE) == TW_SZNIL)
	    result[0] = '\0';
    else
	    (void) sprintf(result, " %s",
		dtimezone(tw -> tw_zone, tw->tw_flags | flags));
    (void) sprintf(buffer, "%02d %s %0*d %02d:%02d:%02d%s",
	    tw->tw_mday, tw_moty[tw->tw_mon],
	    tw -> tw_year < 100 ? 2 : 4, tw -> tw_year,
	    tw->tw_hour, tw->tw_min, tw->tw_sec, result);

    if ((tw -> tw_flags & TW_SDAY) == TW_SEXP)
	(void) sprintf (result, "%s, %s", tw_dotw[tw -> tw_wday], buffer);
    else
	if ((tw -> tw_flags & TW_SDAY) == TW_SNIL)
	    (void) strcpy (result, buffer);
	else
	    (void) sprintf (result, "%s (%s)", buffer, tw_dotw[tw -> tw_wday]);

    return result;
}

char   *dtimezone (offset, flags)
register int     offset,
		 flags;
{
    register int    hours,
                    mins;
    register struct zone *z;
    static char buffer[10];

    if (offset < 0) {
	mins = -((-offset) % 60);
	hours = -((-offset) / 60);
    }
    else {
	mins = offset % 60;
	hours = offset / 60;
    }

    if (!(flags & TW_ZONE) && mins == 0)
    {
	tzset();
	return ((flags & TW_DST) ? tzname[1] : tzname[0]);
    }
#if	defined(DSTXXX)
    if (flags & TW_DST)
	hours += 1;
#endif	/* defined(DSTXXX) */
    (void) sprintf (buffer, "%s%02d%02d",
	    offset < 0 ? "-" : "+", abs (hours), abs (mins));
    return buffer;
}

/* r1bindex.c - right plus 1 or beginning index */

char *r1bindex(str, chr)
register char *str;
register int chr;
{
    register char  *cp;

    for (cp = str; *cp; cp++)
	continue;
    --cp;
    while (cp >= str && *cp != chr)
	--cp;
    return (++cp);
}


/* smatch.c - match a switch */

smatch(string, swp)
register char *string;
register struct swit *swp;
{
    register char  *sp,
                   *tcp;
    struct swit *tp;
    int     firstone,
            stringlen;

    firstone = UNKWNSW;

    if (string == 0)
	return firstone;

    for (stringlen = strlen (string), tp = swp; tcp = tp -> sw; tp++) {
	if (stringlen < abs (tp -> minchars))
	    continue;		/* no match */
	for (sp = string; *sp == *tcp++;) {
	    if (*sp++ == 0)
		return (tp - swp);/* exact match */
	}
	if (*sp != 0) {
	    if (*sp != ' ')
		continue;	/* no match */
	    if (*--tcp == 0)
		return (tp - swp);/* exact match */
	}
	if (firstone == UNKWNSW)
	    firstone = tp - swp;
	else
	    firstone = AMBIGSW;
    }

    return (firstone);
}

/* advise.c - print out error message */

/* VARARGS2 */
void advise (what, fmt, a, b, c, d, e, f)
char   *what,
       *fmt,
       *a,
       *b,
       *c,
       *d,
       *e,
       *f;
{
    advertise (what, NULLCP, fmt, a, b, c, d, e, f);
}


/* advertise.c - the heart of adios */

extern int  errno;
extern int  sys_nerr;
extern char *sys_errlist[];

/* VARARGS3 */
void advertise (what, tail, fmt, a, b, c, d, e, f)
char   *what,
       *tail,
       *fmt,
       *a,
       *b,
       *c,
       *d,
       *e,
       *f;
{
    int	    eindex = errno;

    (void) fflush (stdout);

    if (invo_name && *invo_name)
	fprintf (stderr, "%s: ", invo_name);
    fprintf (stderr, fmt, a, b, c, d, e, f);
    if (what) {
	if (*what)
	    fprintf (stderr, " %s: ", what);
	if (eindex > 0 && eindex < sys_nerr)
	    fprintf (stderr, "%s", sys_errlist[eindex]);
	else
	    fprintf (stderr, "Error %d", eindex);
    }
    if (tail)
	fprintf (stderr, ", %s", tail);
    (void) fputc ('\n', stderr);
}


/* uleq.c - "unsigned" lexical compare */

#define TO_LOWER 040
#define NO_MASK  000
#include <ctype.h>

uleq (c1, c2)
register char  *c1,
               *c2;
{
    register int    c,
		    mask;

    if (!c1)
	c1 = "";
    if (!c2)
	c2 = "";

    while (c = *c1++)
    {
	mask = (isalpha(c) && isalpha(*c2)) ?  TO_LOWER : NO_MASK;
	if ((c | mask) != (*c2 | mask))
	    return 0;
	else
	    c2++;
    }
    return (*c2 == 0);
}


/* closefds.c - close-up fd:s */

void	closefds (i)
register int	i;
{
    int     nbits;
    struct rlimit r;

    getrlimit(RLIMIT_NOFILE, &r);

    nbits = r.rlim_cur;

    for (; i < nbits; i++)
	    (void) close (i);
}

/* pidwait.c - wait for child to exit */

int     pidwait (id, sigsok)
register int     id,
		 sigsok;
{
    register int    pid;
    TYPESIG (*hstat) (), (*istat) (), (*qstat) (), (*tstat) ();
    int     status;

    if (sigsok == NOTOK) {
	istat = signal (SIGINT, SIG_IGN);
	qstat = signal (SIGQUIT, SIG_IGN);
    }

    while ((pid = wait (&status)) != NOTOK && pid != id)
	continue;

    if (sigsok == NOTOK) {
	signal (SIGINT, istat);
	signal (SIGQUIT, qstat);
    }

    return (pid == NOTOK ? NOTOK : status);
}

static char username[BUFSIZ];
static char fullname[BUFSIZ];

char *
getusr () {
    register char  *cp,
                   *np;
    register struct passwd *pw;

    if (username[0])
	return username;

    if ((pw = getpwuid (getuid ())) == NULL
	    || pw -> pw_name == NULL
	    || *pw -> pw_name == NULL) {
	(void) strcpy (username, "unknown");
	(void) sprintf (fullname, "The Unknown User-ID (%d)", getuid ());
	return username;
    }

    np = pw -> pw_gecos;
    for (cp = fullname; *np && *np != ','; *cp++ = *np++)
	continue;
    *cp = '\0';
    if (*np == '\0')
	(void) strcpy (username, pw -> pw_name);

    if ((cp = getenv ("SIGNATURE")) && *cp)
	(void) strcpy (fullname, cp);
    if (strchr(fullname, '.')) {		/*  quote any .'s */
	  char tmp[BUFSIZ];
      sprintf (tmp, "\"%s\"", fullname);/* should quote "'s too */
      strcpy (fullname, tmp);
    }

    return username;
}

int  stringdex (p1, p2)
register char  *p1,
               *p2;
{
    register char  *p;

    if (p1 == 0 || p2 == 0) return(-1);		/* XXX */

    for (p = p2; *p; p++)
	if (uprf (p, p1))
	    return (p - p2);

    return (-1);
}

uprf (c1, c2)
register char  *c1,
               *c2;
{
    register int    c,
		    mask;

    if (c1 == 0 || c2 == 0)
	return(0);         /* XXX */

    while (c = *c2++)
    {
	mask = (isalpha(c) && isalpha(*c1)) ?  TO_LOWER : NO_MASK;
	if ((c | mask) != (*c1 | mask))
	    return 0;
	else
	    c1++;
    }
    return 1;
}

char *
m_tmpfil (template)
register char  *template;
{
    static char tmpfil[BUFSIZ];

    /* should be /tmp but Unixware has ramdisk which is too small */
    (void) sprintf (tmpfil, "/var/mail/%sXXXXXX", template);
    (void) unlink (mktemp (tmpfil));

    return tmpfil;
}

/*
 * simplified version of m_putenv
 */
void
m_putenv(char *id, char *val)
{
	char *entry;

	entry = malloc(strlen(id) + strlen(val) + 2);
	sprintf(entry, "%s=%s", id, val);
	putenv(entry);
}

void help (str, swp)
register char  *str;
register struct swit   *swp;
{
    extern char *options[];
    int     nameoutput,
            len,
            linepos,
            outputlinelen;
    register char  *cp,
                  **ap;

    printf ("syntax: %s\n", str);
    printf ("  switches are:\n");
    printsw (ALL, swp, "-");

    printf ("\nadapted from %s\n", version);
}



void
printsw (substr, swp, prefix)
register char  *substr,
               *prefix;
register struct swit   *swp;
{
    int     len,
	    optno;
    register int    i;
    register char  *cp,
                   *cp1,
		   *sp;
    char    buf[128];

    len = strlen (substr);
    for (; swp -> sw; swp++) {
	if (!*substr ||		/* null matches all strings */
		(ssequal (substr, swp -> sw) && len >= swp -> minchars)) {
	    optno = 0;
	    if (sp = (&swp[1]) -> sw) /* next switch */
		if (!*substr && sp[0] == 'n' && sp[1] == 'o' &&
			strcmp (&sp[2], swp -> sw) == 0 && (
			((&swp[1]) -> minchars == 0 && swp -> minchars == 0) ||
			((&swp[1]) -> minchars == (swp -> minchars) + 2)))
		    optno++;
	    if (swp -> minchars > 0) {
		cp = buf;
		*cp++ = '(';
		if (optno) {
		    (void) strcpy (cp, "[no]");
		    cp += strlen (cp);
		}
		for (cp1 = swp -> sw, i = 0; i < swp -> minchars; i++)
		    *cp++ = *cp1++;
		*cp++ = ')';
		while (*cp++ = *cp1++);
		printf ("  %s%s\n", prefix, buf);
	    }
	    else
		if (swp -> minchars == 0)
		    printf (optno ? "  %s[no]%s\n" : "  %s%s\n",
			    prefix, swp -> sw);
	    if (optno)
		swp++;	/* skip -noswitch */
	}
    }
}

/* ssequal.c - initially equal? */

ssequal (substr, str)
register char  *substr,
               *str;
{
    if (!substr)
	substr = "";
    if (!str)
	str = "";

    while (*substr)
	if (*substr++ != *str++)
	    return 0;
    return 1;
}

/* adios.c - print out error message and exit */

/* VARARGS2 */

void adios (what, fmt, a, b, c, d, e, f)
char   *what,
       *fmt,
       *a,
       *b,
       *c,
       *d,
       *e,
       *f;
{
    advise (what, fmt, a, b, c, d, e, f);
    exit (1);
}

/* add.c - concatenate two strings in managed memory */

char   *add (this, that)
register char  *this,
               *that;
{
    register char  *cp;

    if (!this)
	this = "";
    if (!that)
	that = "";
    if ((cp = malloc ((unsigned) (strlen (this) + strlen (that) + 1))) == NULL)
	adios (NULLCP, "unable to allocate string storage");

    (void) sprintf (cp, "%s%s", that, this);
    if (*that)
	free (that);
    return cp;
}

/* ambigsw.c - report an ambiguous switch */

void ambigsw (arg, swp)
register char   *arg;
register struct swit *swp;
{
    advise (NULLCP, "-%s ambiguous.  It matches", arg);
    printsw (arg, swp, "-");
}


/* pidstatus.c - report child's status */

static char *sigs[] = {
    NULL,
    "Hangup",
    "Interrupt",
    "Quit",
    "Illegal instruction",
    "Trace/BPT trap",
    "IOT trap",
    "EMT trap",
    "Floating exception",
    "Killed",
    "Bus error",
    "Segmentation fault",
    "Bad system call",
    "Broken pipe",
    "Alarm clock",
    "Terminated",
    NULL,
    "Stopped (signal)",
    "Stopped",
    "Continued",
    "Child exited",
    "Stopped (tty input)",
    "Stopped (tty output)",
    "Tty input interrupt",
    "Cputime limit exceeded",
    "Filesize limit exceeded",
    NULL
};

int	pidstatus (status, fp, cp)
register int   status;
register FILE *fp;
register char *cp;
{
    int     signum;

    if ((status & 0xff00) == 0xff00)
	return status;

    switch (signum = status & 0x007f) {
	case OK: 
	    if (signum = ((status & 0xff00) >> 8)) {
		if (cp)
		    fprintf (fp, "%s: ", cp);
		fprintf (fp, "Exit %d\n", signum);
	    }
	    break;

	case SIGINT: 
	    break;

	default: 
	    if (cp)
		fprintf (fp, "%s: ", cp);
	    if (signum >= sizeof sigs || sigs[signum] == NULL)
		fprintf (fp, "Signal %d", signum);
	    else
		fprintf (fp, "%s", sigs[signum]);
	    fprintf (fp, "%s\n", status & 0x80 ? " (core dumped)" : "");
	    break;
    }

    return status;
}
