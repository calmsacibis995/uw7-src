/* slocal.c - MMDF style mailer to write to a local user's mailbox */

/*
   This program implements mail delivery in the MMDF style.

   Under SendMail, users should add the line

	"| /usr/bin/slocal"

   to their $HOME/.forward file.
 */

#include <stdio.h>
/* the c-client includes */
#include "c-client/mail.h"
#include "c-client/osdep.h"
#include "c-client/rfc822.h"
#include "c-client/misc.h"
/* hack to make the c-client includes work */
#undef setpgrp

#include <setjmp.h>
#include "mh.h"
#include "tws.h"
#include "sysexits.h"		/* sendmail's exit codes */

#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <utmp.h>
#include <sys/ioctl.h>

#define	NVEC	100

char *version = "version 6.8.3";
char *invo_name;

char *maildelivery = "/etc/mail/maildelivery";

static struct swit switches[] = {
#define	ADDRSW	0
    "addr address", 0,
#define	USERSW	1
    "user name", 0,
#define	FILESW	2
    "file file", 0,
#define	SENDSW	3
    "sender address", 0,
#define	MBOXSW	4
    "mailbox file", 0,
#define	HOMESW	5
    "home directory", -4,
#define	INFOSW	6
    "info data", 0,

#define	MAILSW	7
    "maildelivery file", 0,

#define	VERBSW	8
    "verbose", 0,
#define	NVERBSW	9
    "noverbose", 0,

#define	DEBUGSW	10
    "debug", 0,

#define	HELPSW	11
    "help", 4,

    NULL, 0
};

static int  debug = 0;
static int  globbed = 0;
static int  parsed = 0;
static int  utmped = 0;
static int  verbose = 0;

static char *addr = NULLCP;
static char *user = NULLCP;
static char *info = NULLCP;
static char *file = NULLCP;
static char *sender = NULLCP;
static char *mbox = NULLCP;
static char *home = NULLCP;


static struct passwd *pw;


static char ddate[BUFSIZ];

struct tws *now;


static jmp_buf myctx;

struct pair {
    char   *p_name;
    char   *p_value;

    char    p_flags;
#define	P_NIL	0x00
#define	P_ADR	0x01
#define	P_HID	0x02
#define	P_CHK	0x04
};

static struct pair *lookup ();


static struct pair  hdrs[NVEC + 1] = {
    "source", NULL, P_HID,
    "addr", NULL, P_HID,

    "Return-Path", NULL, P_ADR,
    "Reply-To", NULL, P_ADR,
    "From", NULL, P_ADR,
    "Sender", NULL, P_ADR,
    "To", NULL, P_ADR,
    "cc", NULL, P_ADR,
    "Resent-Reply-To", NULL, P_ADR,
    "Resent-From", NULL, P_ADR,
    "Resent-Sender", NULL, P_ADR,
    "Resent-To", NULL, P_ADR,
    "Resent-cc", NULL, P_ADR,

    NULL
};


static struct pair  vars[] = {
    "sender", NULL, P_NIL,
    "address", NULL, P_NIL,
    "size", NULL, P_NIL,
    "reply-to", NULL, P_CHK,
    "info", NULL, P_NIL,

    NULL
};

extern char **environ;

static void	adorn ();
static TYPESIG	alrmser ();


off_t    lseek ();

static int	localmail(), usr_delivery(), split(), parse();
static int	usr_file(), usr_pipe(), copyfile();
static expand(), glob(), copyinfo();

/* ARGSUSED */

main(argc, argv, envp)
int     argc;
char  **argv,
      **envp;
{
    int     fd;
    FILE   *fp    = stdin;
    char   *cp,
	   *mdlvr = NULL,
            buf[100],
            from[BUFSIZ],
            mailbox[BUFSIZ],
            tmpfil[BUFSIZ],
          **argp = argv + 1;

#include <c-client/linkage.c>

    invo_name = r1bindex (*argv, '/');

    while (cp = *argp++) {
	if (*cp == '-')
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    exit (1);
		case UNKWNSW: 
		    adios (NULLCP, "-%s unknown", cp);
		case HELPSW: 
		    (void) sprintf (buf, "%s [switches] [address info sender]",
			    invo_name);
		    help (buf, switches);
		    exit (1);

		case ADDRSW: 
		    if (!(addr = *argp++))/* allow -xyz arguments */
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case INFOSW: 
		    if (!(info = *argp++))/* allow -xyz arguments */
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case USERSW: 
		    if (!(user = *argp++))/* allow -xyz arguments */
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case FILESW: 
		    if (!(file = *argp++) || *file == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case SENDSW: 
		    if (!(sender = *argp++))/* allow -xyz arguments */
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case MBOXSW: 
		    if (!(mbox = *argp++) || *mbox == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case HOMESW: 
		    if (!(home = *argp++) || *home == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;

		case MAILSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    if (mdlvr)
			adios (NULLCP, "only one maildelivery file at a time!");
		    mdlvr = cp;
		    continue;

		case VERBSW: 
		    verbose++;
		    continue;
		case NVERBSW: 
		    verbose = 0;
		    continue;

		case DEBUGSW: 
		    debug++;
		    continue;
	    }

	switch (argp - (argv + 1)) {
	    case 1: 
		addr = cp;
		break;

	    case 2: 
		info = cp;
		break;

	    case 3: 
		sender = cp;
		break;
	}
    }

    if (addr == NULL)
	addr = getusr ();
    if (user == NULL)
	user = (cp = strchr (addr, '.')) ? ++cp : addr;
    if ((pw = getpwnam (user)) == NULL)
	adios (NULLCP, "no such local user as %s", user);

    if (chdir (pw -> pw_dir) == NOTOK)
	(void) chdir ("/");
    (void) umask (0077);

    if (geteuid () == 0) {
	(void) setgid (pw -> pw_gid);
	(void) setuid (pw -> pw_uid);
    }
    
    if (info == NULL)
	info = "";

    setbuf (stdin, NULLCP);

    if (file == NULL) {
	if ((fd = copyfile (fileno (stdin), file = tmpfil)) == NOTOK)
	    adios (NULLCP, "unable to create temporary file");
	if (debug)
	    fprintf (stderr, "temporary file \"%s\" selected\n", tmpfil);
	else
	    (void) unlink (tmpfil);
   	if ((fp = fdopen (fd, "r+")) == NULL)
	    adios (NULLCP, "unable to access temporary file");
    }
    else {
	fd = open(file, 0);
	if (fd < 0)
	    adios (NULLCP, "unable to access input file");
   	if ((fp = fdopen (fd, "r+")) == NULL)
	    adios (NULLCP, "unable to access temporary file");
    }

    from[0] = 0;
    if (sender == NULL)
	copyinfo (fp, from);


    if (mbox == NULL) {
	sprintf(mailbox, "INBOX");
	mbox = mailbox;
    }
    if (home == NULL)
	home = pw -> pw_dir;

    if ((now = dtwstime ()) == NULL)
	adios (NULLCP, "unable to ascertain local time");
    (void) sprintf (ddate, "Delivery-Date: %s\n", dtimenow ());

    if (debug) {
	fprintf (stderr, "addr=\"%s\" user=\"%s\" info=\"%s\" file=\"%s\"\n",
		addr, user, info, file);
	fprintf (stderr, "sender=\"%s\" mbox=\"%s\" home=\"%s\" from=\"%s\"\n",
		sender, mbox, home, from);
	fprintf (stderr, "ddate=\"%s\" now=%02d:%02d\n",
		ddate, now -> tw_hour, now -> tw_min);
    }

    exit (localmail (fd, from, mdlvr) == NOTOK ? EX_TEMPFAIL : EX_OK);
}

static int
localmail(fd, from, mdlvr)
int     fd;
char   *from,
       *mdlvr;
{
    if (usr_delivery (fd, mdlvr ? mdlvr : ".maildelivery", 0, from) != NOTOK)
	return OK;

    if (usr_delivery (fd, maildelivery, 1, from) != NOTOK)
	return OK;

    if (verbose)
	printf ("(trying normal delivery)\n");
    return usr_file (fd, mbox, from);
}

#define	matches(a,b)	(stringdex (b, a) >= 0)

static int
usr_delivery(fd, delivery, su, from)
int     fd,
	su;
char   *delivery,
       *from;
{
    int     i,
	    accept,
            status,
            won,
	    vecp,
            next;
    register char  *cp,
                   *action,
                   *field,
                   *vec4,
                   *pattern;
    char string[BUFSIZ*4];
    char    buffer[BUFSIZ],
           *vec[NVEC];
    struct stat st;
    register struct pair   *p;
    register FILE  *fp;

    if ((fp = fopen (delivery, "r")) == NULL)
	return NOTOK;
    if (fstat (fileno (fp), &st) == NOTOK
	    || (st.st_uid != 0 && (su || st.st_uid != pw -> pw_uid))
	    || st.st_mode & 0022) {
	if (verbose) {
	    printf ("%s: ownership/modes bad (%d, %d,%d,0%o)\n",
		    delivery, su, pw -> pw_uid, st.st_uid, st.st_mode);
	    (void) fflush (stdout);
	}
	return NOTOK;
    }

    won = 0;
    next = 1;
    while (fgets (buffer, sizeof buffer, fp) != NULL) {
	if (*buffer == '#')
	    continue;
	if (cp = strchr (buffer, '\n'))
	    *cp = 0;
	if ((vecp = split (buffer, vec)) < 5)
	    continue;
	if (debug)
	    for (i = 0; vec[i]; i++)
		fprintf (stderr, "vec[%d]: \"%s\"\n", i, vec[i]);

	field = vec[0];
	pattern = vec[1];
	action = vec[2];

	switch (vec[3][0]) {
	    case 'N':
	    case 'n':
 		if (! next)
			continue;  /* if previous condition failed, don't
				      do this - else fall through */

	    case '?': 
		if (won)
		    continue;	/* else fall */
	    case 'A': 
	    case 'a': 
		accept = 1;
		break;

	    case 'R': 
	    case 'r': 
	    default: 
		accept = 0;
		break;
	}

	vec4 = strdup(vec[4]);
	string[0] = 0;
    	for (i = 4; vec[i]; i++) {
    		if (string[0])
    			strcat(string, " ");
		expand(string + strlen(string), vec[i], fd);
    	}

	switch (*field) {
	    case '*': 
		break;

	    case 'd': 
		if (uleq (field, "default")) {
		    if (won)
			continue;
		    break;
		}		/* else fall */

	    default: 
		if (!parsed && parse (fd) == NOTOK) {
		    (void) fclose (fp);
		    return NOTOK;
		}
		if ((p = lookup (hdrs, field)) == NULL
			|| (p->p_value == NULL)			/* XXX */
			|| !matches (p -> p_value, pattern)) {
			next = 0;
			continue;
		}
		else
		    next = 1;
		break;
	}

	switch (*action) {
	    case 'q':
		/*if (!uleq (action, "qpipe"))
		    continue;	/* else fall */
	    case '^':
		if (split (string, vec) < 1)
		    continue;
		status = usr_pipe (fd, vec4, vec[0], vec);
		break;

	    case 'p': 
		/*if (!uleq (action, "pipe"))
		    continue;	/* else fall */
	    case '|': 
		vec[2] = "sh";
		vec[3] = "-c";
		vec[4] = string;
		vec[5] = NULL;
		status = usr_pipe (fd, vec4, "/bin/sh", vec + 2);
		break;

	    case 'f': 
		/*if (!uleq (action, "file"))
		    continue;	/* else fall */
	    case '>': 
		status = usr_file (fd, vec4,  from);	/* UUCP format? */
		break;

	    case 'm':
		/*if (!uleq (action, "mbox"))
		    continue;*/
		status = usr_file (fd, vec4,  from);
		break;

	    case 'd': 
		/*if (!uleq (action, "destroy"))
		    continue;*/
		status = OK;
		break;
	}

	if (accept && status == OK)
	    won++;
    }

    (void) fclose (fp);
    return (won ? OK : NOTOK);
}

#define	QUOTE	'\\'

static int
split(cp, vec)
char   *cp,
      **vec;
{
    register int    i;
    register char  *s;

    for (i = 0, s = cp; i <= NVEC;) {
	vec[i] = NULL;
	while (isspace (*s) || *s == ',')
	    *s++ = 0;
	if (*s == 0)
	    break;

	if (*s == '"') {
	    for (vec[i++] = ++s; *s != 0 && *s != '"'; s++)
		if (*s == QUOTE) {
		    if (*++s == '"')
			(void) strcpy (s - 1, s);
		    s--;
		}
	    if (*s == '"')
		*s++ = 0;
	    continue;
	}
	if (*s == QUOTE && *++s != '"')
	    s--;
	vec[i++] = s++;

	while (*s != 0 && !isspace (*s) && *s != ',')
	    s++;
    }
    vec[i] = NULL;

    return i;
}

static int
parse(fd)
register int    fd;
{
    register int    i,
                    state;
    int     fd1;
    register char  *cp,
                   *dp,
                   *lp;
    char    name[NAMESZ],
            field[BUFSIZ];
    register struct pair   *p,
			   *q;
    register FILE  *in;

    if (parsed++)
	return OK;

    if ((fd1 = dup (fd)) == NOTOK)
	return NOTOK;
    if ((in = fdopen (fd1, "r")) == NULL) {
	(void) close (fd1);
	return NOTOK;
    }
    rewind (in);

    if (p = lookup (hdrs, "source"))
	p -> p_value = strdup (sender);
    if (p = lookup (hdrs, "addr"))
	p -> p_value = strdup (addr);

    for (i = 0, state = FLD;;) {
	switch (state = m_getfld (state, name, field, sizeof field, in)) {
	    case FLD: 
	    case FLDEOF: 
	    case FLDPLUS: 
		lp = add (field, NULLCP);
		while (state == FLDPLUS) {
		    state = m_getfld (state, name, field, sizeof field, in);
		    lp = add (field, lp);
		}
		for (p = hdrs; p -> p_name; p++)
		    if (uleq (p -> p_name, name)) {
			if (!(p -> p_flags & P_HID)) {
			    if (cp = p -> p_value)
				if (p -> p_flags & P_ADR) {
				    dp = cp + strlen (cp) - 1;
				    if (*dp == '\n')
					*dp = 0;
				    cp = add (",\n\t", cp);
				}
				else
				    cp = add ("\t", cp);
			    p -> p_value = add (lp, cp);
			}
			free (lp);
			break;
		    }
		if (p -> p_name == NULL && i < NVEC) {
		    p -> p_name = strdup (name);
		    p -> p_value = lp;
		    p -> p_flags = P_NIL;
		    p++, i++;
		    p -> p_name = NULL;
		}
		if (state != FLDEOF)
		    continue;
		break;

	    case BODYM: 
	    case BODYEOF: 
	    case FILEEOF: 
		break;

	    case LENERR: 
	    case FMTERR: 
		advise (NULLCP, "format error in message");
		break;

	    default: 
		advise (NULLCP, "internal error");
		(void) fclose (in);
		return NOTOK;
	}
	break;
    }
    (void) fclose (in);

    if (p = lookup (vars, "reply-to")) {
	if ((q = lookup (hdrs, "reply-to")) == NULL || q -> p_value == NULL)
	    q = lookup (hdrs, "from");
	p -> p_value = strdup (q ? q -> p_value : "");
	p -> p_flags &= ~P_CHK;
	if (debug)
	    fprintf (stderr, "vars[%d]: name=\"%s\" value=\"%s\"\n",
		    p - vars, p -> p_name, p -> p_value);
    }
    if (debug)
	for (p = hdrs; p -> p_name; p++)
	    fprintf (stderr, "hdrs[%d]: name=\"%s\" value=\"%s\"\n",
		p - hdrs, p -> p_name, p -> p_value);

    return OK;
}

#define	LPAREN	'('
#define	RPAREN	')'

static
expand(s1, s2, fd)
register char  *s1,
               *s2;
int	fd;
{
    register char   c,
                   *cp;
    register struct pair   *p;

    if (!globbed)
	glob (fd);

    while (c = *s2++)
	if (c != '$' || *s2 != LPAREN)
	    *s1++ = c;
	else {
	    for (cp = ++s2; *s2 && *s2 != RPAREN; s2++)
		continue;
	    if (*s2 != RPAREN) {
		s2 = --cp;
		continue;
	    }
	    *s2++ = 0;
	    if (p = lookup (vars, cp)) {
		if (!parsed && (p -> p_flags & P_CHK))
		    (void) parse (fd);

		(void) strcpy (s1, p -> p_value);
		s1 += strlen (s1);
	    }
	}
    *s1 = 0;
}

static
glob(fd)
register int  fd;
{
    char buffer[BUFSIZ];
    struct stat st;
    register struct pair   *p;

    if (globbed++)
	return;

    if (p = lookup (vars, "sender"))
	p -> p_value = strdup (sender);
    if (p = lookup (vars, "address"))
	p -> p_value = strdup (addr);
    if (p = lookup (vars, "size")) {
	(void) sprintf (buffer, "%d",
		fstat (fd, &st) != NOTOK ? (int) st.st_size : 0);
	p -> p_value = strdup (buffer);
    }
    if (p = lookup (vars, "info"))
	p -> p_value = strdup (info);

    if (debug)
	for (p = vars; p -> p_name; p++)
	    fprintf (stderr, "vars[%d]: name=\"%s\" value=\"%s\"\n",
		    p - vars, p -> p_name, p -> p_value);
}

static struct pair *
lookup(pairs, key)
register struct pair   *pairs;
register char  *key;
{
    register char  *cp;

    for (; cp = pairs -> p_name; pairs++)
	if (uleq (cp, key))
	    return pairs;

    return NULL;
}

static int
usr_file(fd, mailbox, from)
int     fd;
char   *mailbox,
       *from;
{
    char sender[512];
    int ret;
    char fromlocal[512];
    int fd1;
    FILE *fp;

    if (verbose)
	printf ("\tdelivering to file \"%s\"", mailbox);

    (void) lseek (fd, (off_t)0, 0);

    /* get envelope sender and date */
    strcpy(fromlocal, from);
    if (sendmail_parse_from(fromlocal, sender, 0, 0) == 0)
    	return(NOTOK);

    /* now call deliver, dup needed so we can close the fp and leave fd open */
    fd1 = dup(fd);
    if (fd1 < 0)
    	return(NOTOK);
    fp = fdopen(fd1, "r");
    fp = fdopen(fd, "r");
    if (fp == 0)
    	return(NOTOK);
    ret = mail_deliver(0, mailbox, sender, fp);
    fclose(fp);

    if (ret == 0) {
	if (verbose)
	    printf (", * FAILED *\n");
	return(NOTOK);
    }

    if (verbose) {
	printf (", done.\n");
	(void) fflush (stdout);
    }
    return(OK);
}

static int
usr_pipe(fd, cmd, pgm, vec)
int     fd;
char   *cmd,
       *pgm,
      **vec;
{
    int     bytes,
	    i,
            child_id,
            status;
    struct stat st;

    if (verbose) {
	printf ("\tdelivering to pipe \"%s\"", cmd);
	(void) fflush (stdout);
    }
    (void) lseek (fd, (off_t)0, 0);

    for (i = 0; (child_id = fork ()) == NOTOK && i < 5; i++)
	sleep (5);
    switch (child_id) {
	case NOTOK: 
	    adorn ("fork", "unable to");
	    return NOTOK;

	case OK: 
	    if (fd != 0)
		(void) dup2 (fd, 0);
	    (void) freopen ("/dev/null", "w", stdout);
	    (void) freopen ("/dev/null", "w", stderr);
	    if (fd != 3)
		(void) dup2 (fd, 3);
	    closefds (4);

	    setpgrp ();

	    *environ = NULL;
	    m_putenv ("USER", pw -> pw_name);
	    m_putenv ("HOME", pw -> pw_dir);
	    m_putenv ("SHELL", pw -> pw_shell);

	    execvp (pgm, vec);
	    _exit (-1);

	default: 
	    switch (setjmp (myctx)) {
		case OK: 
		    (void) signal (SIGALRM, (void(*)())alrmser);
		    bytes = fstat (fd, &st) != NOTOK ? (int) st.st_size : 100;
		    if (bytes <= 0)
			bytes = 100;
		    (void) alarm ((unsigned) (bytes * 60 + 300));

		    status = pidwait (child_id, OK);

		    (void) alarm (0);
#ifdef	MMDFI
		    if (status == RP_MOK || status == RP_OK)
			status = 0;
#endif	/* MMDFI */
		    if (verbose) {
			if (status == 0)
			    printf (", wins.\n");
			else
			    if ((status & 0xff00) == 0xff00)
				printf (", system error\n");
			    else
				(void) pidstatus (status, stdout, ", loses");
			(void) fflush (stdout);
		    }
		    return (status == 0 ? OK : NOTOK);

		default: 
#ifndef	BSD42
		    (void) kill (child_id, SIGKILL);
#else	/* BSD42 */
		    (void) killpg (child_id, SIGKILL);
#endif	/* BSD42 */
		    if (verbose) {
			printf (", timed-out; terminated\n");
			(void) fflush (stdout);
		    }
		    return NOTOK;
	    }
    }
}

/* ARGSUSED */

static	TYPESIG
alrmser(i)
int     i;
{
    longjmp (myctx, DONE);
}

static
copyinfo(fp, from)
register FILE   *fp;
char	*from;
{
    int     i;
    register char  *cp;
    static char buffer[BUFSIZ];

    if (fgets (from, BUFSIZ, fp) == NULL)
	adios (NULLCP, "no message");

    if (strncmp (from, "From ", i = strlen ("From "))) {
	rewind (fp);
	*from = 0;
	return;
    }

    (void) strcpy (buffer, from + i);
    if (cp = strchr (buffer, '\n')) {
	*cp = 0;
	cp -= 24;
	if (cp < buffer)
	    cp = buffer;
    }
    else
	cp = buffer;
    *cp = 0;

    for (cp = buffer + strlen (buffer) - 1; cp >= buffer; cp--)
	if (isspace (*cp))
	    *cp = 0;
	else
	    break;
    sender = buffer;
    rewind (fp);
}

static int
copyfile(qd, tmpfil)
int     qd;
register char   *tmpfil;
{
    register int    i,
		    first = 0,
                    fd1,
                    fd2;
    char    buffer[BUFSIZ];
    register FILE  *qfp,
		   *ffp;

    (void) strcpy (tmpfil, m_tmpfil (invo_name));
    if ((fd1 = creat (tmpfil, 0600)) == NOTOK)
	return NOTOK;
    (void) close (fd1);
    if ((fd1 = open (tmpfil, 2)) == NOTOK) {
	(void) unlink (tmpfil);
	return NOTOK;
    }

    while ((i = read (qd, buffer, sizeof buffer)) > 0) {
	if (write (fd1, buffer, i) != i) {
you_lose:
	    (void) close (fd1);
	    (void) unlink (tmpfil);
	    return NOTOK;
	}
    }
    if (i == NOTOK)
	goto you_lose;
    (void) lseek (fd1, (off_t)0, 0);
    return fd1;
}

/* VARARGS2 */

static void
adorn(what, fmt, a, b, c, d, e, f)
char   *what,
       *fmt,
       *a,
       *b,
       *c,
       *d,
       *e,
       *f;
{
    char   *cp = invo_name;

    if (!verbose)
	return;
    printf (", ");

    invo_name = NULL;
    advise (what, fmt, a, b, c, d, e, f);
    invo_name = cp;
}
