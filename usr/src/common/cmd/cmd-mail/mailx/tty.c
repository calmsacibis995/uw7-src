#ident	"@(#)tty.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)tty.c	1.19 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Generally useful tty stuff.
 */

#include "rcv.h"

#ifdef	USG_TTY

static void	Echo ARGS((int cc));
static int	countcol ARGS((void));
static void	outstr ARGS((const char *s));
static char	*readtty ARGS((const char *prid, const char *pr, char *src));
static void	resetty ARGS((void));
static char	*rubout ARGS((char*));
static int	savetty ARGS((void));
static int	setty ARGS((void));

static	unsigned char	c_erase;	/* Current erase char */
static	unsigned char	c_kill;		/* Current kill char */
static	unsigned char	c_intr;		/* Current interrupt char */
static	unsigned char	c_quit;		/* Current quit character */
static	unsigned char	c_word;		/* Current word erase char */
static	unsigned char	c_eof;		/* Current eof char */
static	int	Col;			/* current output column */
static	int	Pcol;			/* end column of prompt string */
static	int	Out;			/* file descriptor of stdout */
#ifdef USE_TERMIOS
static	struct termios savtty, ttybuf;
#else
static	struct termio savtty, ttybuf;
#endif
static	char canonb[LINESIZE];		/* canonical buffer for input */
					/* processing */
static	int	erasing;		/* we are erasing characters */

#ifdef SIGCONT
# ifdef preSVr4
typedef int	sig_atomic_t;
# endif
static	sig_atomic_t	hadcont;		/* Saw continue signal */
/*ARGSUSED*/
static void
ttycont(s)
{
	hadcont++;
}

/*ARGSUSED*/
static void
ttystop(s)
{
	resetty();
	kill(mypid, SIGSTOP);
}
#endif

/*
 * Read all relevant header fields.
 */

grabh(hp, gflags, subjtop)
register struct header *hp;
{
#ifdef SIGCONT
	void (*savecont)();
	void (*savestop)();
#endif
	if (savetty())
		return -1;
#ifdef SIGCONT
# ifdef preSVr4
	savecont = sigset(SIGCONT, ttycont);
	savestop = sigset(SIGTSTP, ttycont);
# else
	{
	struct sigaction nsig, osig;
	nsig.sa_handler = ttycont;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = 0;
	(void) sigaction(SIGCONT, &nsig, &osig);
	savecont = osig.sa_handler;
	nsig.sa_handler = ttystop;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = 0;
	(void) sigaction(SIGTSTP, &nsig, &osig);
	savestop = osig.sa_handler;
	}
# endif
#endif
	if (gflags & GSUBJECT && subjtop) {
		hp->h_subject = readtty(msgsubjectid, msgsubject, hp->h_subject);
		if (hp->h_subject != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GTO) {
		hp->h_to = addto(NOSTR, readtty(msgtoid, msgto, hp->h_to));
		if (hp->h_to != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GCC) {
		hp->h_cc = addto(NOSTR, readtty(msgccid, msgcc, hp->h_cc));
		if (hp->h_cc != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GBCC) {
		hp->h_bcc = addto(NOSTR, readtty(msgbccid, msgbcc, hp->h_bcc));
		if (hp->h_bcc != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GSUBJECT && !subjtop) {
		hp->h_subject = readtty(msgsubjectid, msgsubject, hp->h_subject);
		if (hp->h_subject != NOSTR)
			hp->h_seq++;
	}
#ifdef SIGCONT
# ifdef preSVr4
	(void) sigset(SIGCONT, savecont);
	(void) sigset(SIGTSTP, savestop);
# else
	{
	struct sigaction nsig;
	nsig.sa_handler = savecont;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = SA_RESTART;
	(void) sigaction(SIGCONT, &nsig, (struct sigaction*)0);
	nsig.sa_handler = savestop;
	(void) sigaction(SIGTSTP, &nsig, (struct sigaction*)0);
	}
# endif
#endif
	return(0);
}

#ifdef TIOCSTI
/*
 * If we have TIOCSTI available to us, we can do things much
 * more efficiently by letting the tty driver do all the work
 * for us. We only have to do it the hard way if the ioctl()
 * fails.
 */
static int readtty_sti(src)
	char src[];
{
	int c;
	char ch;
	char *cp, *cp2;

	/* copy string into the tty buffer */
	cp = (src == NOSTR) ? "" : src;
	cp2 = canonb;
	while ((c = *cp++) != 0) {
		if (c == c_erase || c == c_kill) {
			ch = '\\';
			if (ioctl(0, TIOCSTI, &ch) < 0)
				return 0;
		}
		ch = c;
		if (ioctl(0, TIOCSTI, &ch) < 0)
			return 0;
	}
	/* read the string back */
	for (cp2 = canonb; cp2 < canonb + LINESIZE; ) {
		c = getc(stdin);
		if (c == EOF || c == '\n')
			break;
		*cp2++ = c;
		*cp2 = '\0';
	}
	*cp2 = '\0';
	fflush(stdout);
	return 1;
}
#endif TIOCSTI

/*
 * Read up a header from standard input.
 * The source string has the preliminary contents to
 * be read.
 *
 */

static char *
readtty(prid, pr, src)
	const char prid[], pr[];
	char src[];
{
	int c;
	register char *cp, *cp2;

	erasing = 0;
	Col = 0;
	outstr(gettxt(prid, pr));
	Pcol = Col;
	fflush(stdout);
	if (src != NOSTR && strlen(src) > (unsigned)(LINESIZE - 2)) {
		putchar('\n');
		pfmt(stdout, MM_ERROR, toolongtoedit);
		return(src);
	}

#ifdef TIOCSTI
	if (readtty_sti(src))
		return (canonb[0] == '\0') ? NOSTR : savestr(canonb);
#endif /* TIOCSTI */

	if (setty()) {
		resetty();
		return(src);
	}
	cp2 = src==NOSTR ? "" : src;
	for (cp=canonb; *cp2; cp++, cp2++)
		*cp = *cp2;
	*cp = '\0';
	outstr(canonb);

	for (;;) {
		fflush(stdout);
#ifdef SIGCONT
		hadcont = 0;
#endif
		c = getc(stdin);

		if (c==c_erase) {
			if (cp > canonb)
				if (cp[-1]=='\\' && !erasing) {
					*cp++ = (char)c;
					Echo(c);
				} else {
					cp = rubout(--cp);
				}
		} else if (c==c_kill) {
			if (cp > canonb && cp[-1]=='\\') {
				*cp++ = (char)c;
				Echo(c);
			} else while (cp > canonb) {
				cp = rubout(--cp);
			}
		} else if (c==c_word) {
			if (cp > canonb)
				if (cp[-1]=='\\' && !erasing) {
					*cp++ = (char)c;
					Echo(c);
				} else {
					while (--cp >= canonb)
						if (!Isspace(*cp))
							break;
						else
							cp = rubout(cp);
					while (cp >= canonb)
						if (!Isspace(*cp))
						{
							cp = rubout(cp);
							cp--;
						}
						else
							break;
					if (cp < canonb)
						cp = canonb;
					else if (*cp)
						cp++;
				}
		} else if (c==EOF || ferror(stdin) || c==c_intr || c==c_quit || c==c_eof) {
#ifdef SIGCONT
			if (hadcont) {
				(void) setty();
				outstr(gettxt(usercontid, usercont));
				Col = 0;
				outstr(pr);
				*cp = '\0';
				outstr(canonb);
				clearerr(stdin);
				continue;
			}
#endif
			resetty();
			savedead(c==c_quit? SIGQUIT: SIGINT);
		} else switch (c) {
			case '\n':
			case '\r':
				resetty();
				putchar('\n');
				fflush(stdout);
				if (canonb[0]=='\0')
					return(NOSTR);
				return(savestr(canonb));
			default:
				erasing = 0;
				*cp++ = (char)c;
				*cp = '\0';
				Echo(c);
		}
	}
}

static int
savetty()
{
	Out = fileno(stdout);
#ifdef USE_TERMIOS
	if (tcgetattr(Out, &savtty) < 0)
#else
	if (ioctl(Out, TCGETA, &savtty) < 0)
#endif
	{
		pfmt(stderr, MM_ERROR, failed, "ioctl", Strerror(errno));
		return(-1);
	}
	c_erase = savtty.c_cc[VERASE];
	c_kill = savtty.c_cc[VKILL];
	c_intr = savtty.c_cc[VINTR];
	c_quit = savtty.c_cc[VQUIT];
	c_eof = savtty.c_cc[VEOF];
#ifdef VWERASE
	c_word = savtty.c_cc[VWERASE];	/* erase word character */
#else
	c_word = 'W' & 037;	/* erase word character */
#endif
	ttybuf = savtty;
#ifdef	u370
	ttybuf.c_cflag &= ~PARENB;	/* disable parity */
	ttybuf.c_cflag |= CS8;		/* character size = 8 */
#endif	/* u370 */
	ttybuf.c_cc[VTIME] = 0;
	ttybuf.c_cc[VMIN] = 1;
	ttybuf.c_iflag &= ~(BRKINT);
	ttybuf.c_lflag &= ~(ICANON|ISIG|ECHO);
	return 0;
}

static int
setty()
{
#ifdef USE_TERMIOS
	if (tcsetattr(Out, TCSADRAIN, &savtty) < 0)
#else
	if (ioctl(Out, TCSETAW, &ttybuf) < 0)
#endif
	{
		pfmt(stderr, MM_ERROR, failed, "ioctl", Strerror(errno));
		return(-1);
	}
	return(0);
}

static void
resetty()
{
#ifdef USE_TERMIOS
	if (tcsetattr(Out, TCSADRAIN, &savtty) < 0)
#else
	if (ioctl(Out, TCSETAW, &savtty) < 0)
#endif
		pfmt(stderr, MM_ERROR, failed, "ioctl", Strerror(errno));
}

static void
outstr(s)
	register const char *s;
{
	while (*s)
		Echo(*s++);
}

static char *
rubout(cp)
	char *cp;
{
	register int oldcol;
	register char ch = *cp;
#ifdef SVR4ES
	register char *bp;
	int i, scrnbak;
#endif

	erasing = 1;
	*cp = '\0';
	switch (ch) {
	case '\t':
		oldcol = countcol();
		do
			putchar('\b');
		while (--Col > oldcol);
		break;

	case '\b':
#ifdef SVR4ES
		bp = cp;
		if (ISASCII((unsigned char)(cp[-1]))) {
			scrnbak = 1;
			bp--;
		} else {
			for(i = 0; i < maxeucw; i++) {
				if (ISSET2((unsigned char)(*--bp))) {
					scrnbak = wp._scrw2;
					break;	
				} else if (ISSET3((unsigned char)(*bp))) {
					scrnbak = wp._scrw3;
					break;
				}
				if (bp == canonb) {
					i++;
					break;
				}
			}
			if (!ISSET2((unsigned char)(*bp)) && !ISSET3((unsigned char)(*bp))){
				scrnbak = wp._scrw1;
				bp += (i - wp._eucw1);
			}
		}
		for (i = 1; i < scrnbak; i++)
			putchar('\b');
		if (ISPRINT((unsigned char)(cp[-1]), wp))
			for (; bp < cp; bp++)
				putchar(*bp);
		else
			for (; bp < cp; bp++)
				putchar(' ');
#else
		if (Isprint(cp[-1]))
			putchar(*(cp-1));
		else
			putchar(' ');
#endif
		Col++;
		break;

	default:
#ifdef SVR4ES
		if (ISPRINT(ch, wp)) {
			if (ISASCII((unsigned char)(ch))) {
				fputs("\b \b", stdout);
				Col--;
			} else {
				for(i = 1; i < maxeucw; i++) {
					if (ISSET2((unsigned char)(*--cp))) {
						scrnbak = wp._scrw2;
						break;
					} else if (ISSET3((unsigned char)(*cp))) {
						scrnbak = wp._scrw3;
						break;
					}
					if (cp == canonb) {
						i++;
						break;
					}
				}
				if (!ISSET2((unsigned char)(*cp)) && !ISSET3((unsigned char)(*cp))){
					scrnbak = wp._scrw1;
					cp += (i - wp._eucw1);
				}
				*cp = '\0';
				for (i = 0; i < scrnbak; i++) {
					fputs("\b \b", stdout);
					Col--;
				}
			}
		}
#else
		if (Isprint(ch)) {
			fputs("\b \b", stdout);
			Col--;
		}
#endif
	}
	return cp;
}

static int
countcol()
{
	register int col;
	register char *s;

	for (col=Pcol, s=canonb; *s; s++)
		switch (*s) {
		case '\t':
			while (++col % 8)
				;
			break;
		case '\b':
			col--;
			break;
		default:
#ifdef SVR4ES
			if (ISPRINT((unsigned char)(*s), wp))
				if (ISASCII((unsigned char)(*s))) {
					col++;
				} else {
					if (ISSET2((unsigned char)(*s))) {
						s += wp._eucw2;
						col += wp._scrw2;
					} else if (ISSET3((unsigned char)(*s))) {
						s += wp._eucw3;
						col += wp._scrw3;
					} else {
						s += wp._eucw1;
						col += wp._scrw3;
					}
					s--;
				}
#else
			if (Isprint(*s))
				col++;
#endif
		}
	return(col);
}

static void
Echo(cc)
{
	char c = (char)cc;

#ifdef SVR4ES
	static int euccs = 0;	/* euc code set */
	static int eucwidth;
	static int scrwidth;
#endif

	switch (c) {
	case '\t':
		do
			putchar(' ');
		while (++Col % 8);
		break;
	case '\b':
		if (Col > 0) {
			putchar('\b');
			Col--;
		}
		break;
	case '\r':
	case '\n':
		Col = 0;
		fputs("\r\n", stdout);
		break;
	default:
#ifdef SVR4ES
		if (ISPRINT((unsigned char)(c), wp)) {
			if (ISASCII((unsigned char)(c))) {
				Col++;
			} else {
				if (euccs==0) {
					euccs = 1;
					if (ISSET2((unsigned char)c)) {
						eucwidth = wp._eucw2;
						scrwidth = wp._scrw2;
					} else if (ISSET3((unsigned char)c)) {
						eucwidth = wp._eucw3;
						scrwidth = wp._scrw3;
					} else {
						eucwidth = wp._eucw1;
						scrwidth = wp._scrw1;
					}
				}
				if (!(--eucwidth)) {
					euccs = 0;
					Col += scrwidth;
				}
			}
#else
		if (Isprint(c)) {
			Col++;
#endif
			putchar(c);
		}
	}
}

#else

static	int	c_erase;		/* Current erase char */
static	int	c_kill;			/* Current kill char */
static	int	hadcont;		/* Saw continue signal */
static	jmp_buf	rewrite;		/* Place to go when continued */
#ifndef TIOCSTI
static	int	ttyset;			/* We must now do erase/kill */
#endif

/*
 * Read all relevant header fields.
 */

grabh(hp, gflags, subjtop)
	struct header *hp;
{
	struct sgttyb ttybuf;
	void ttycont(), signull();
	void (*savecont)();
	register int s;
	int errs;
#ifndef TIOCSTI
	void (*savesigs[2])();
#endif

#ifdef SIGCONT
	savecont = sigset(SIGCONT, signull);
#endif
	errs = 0;
#ifndef TIOCSTI
	ttyset = 0;
#endif
	if (gtty(fileno(stdin), &ttybuf) < 0) {
		pfmt(stderr, MM_ERROR, failed, "gtty", Strerror(errno));
		return(-1);
	}
	c_erase = ttybuf.sg_erase;
	c_kill = ttybuf.sg_kill;
#ifndef TIOCSTI
	ttybuf.sg_erase = 0;
	ttybuf.sg_kill = 0;
	for (s = SIGINT; s <= SIGQUIT; s++)
		if ((savesigs[s-SIGINT] = sigset(s, SIG_IGN)) == SIG_DFL)
			sigset(s, SIG_DFL);
#endif
	if (gflags & GSUBJECT && subjtop) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_subject != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_subject = readtty(msgsubjectid, msgsubject, hp->h_subject);
		if (hp->h_subject != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GTO) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_to != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_to = addto(NOSTR, readtty(msgtoid, msgto, hp->h_to));
		if (hp->h_to != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GCC) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_cc != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_cc = addto(NOSTR, readtty(msgccid, msgcc, hp->h_cc));
		if (hp->h_cc != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GBCC) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_bcc != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_bcc = addto(NOSTR, readtty(msgbccid, msgbcc, hp->h_bcc));
		if (hp->h_bcc != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GSUBJECT && !subjtop) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_subject != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_subject = readtty(msgsubjectid, msgsubject, hp->h_subject);
		if (hp->h_subject != NOSTR)
			hp->h_seq++;
	}
#ifdef SIGCONT
	sigset(SIGCONT, savecont);
#endif
#ifndef TIOCSTI
	ttybuf.sg_erase = c_erase;
	ttybuf.sg_kill = c_kill;
	if (ttyset)
		stty(fileno(stdin), &ttybuf);
	for (s = SIGINT; s <= SIGQUIT; s++)
		sigset(s, savesigs[s-SIGINT]);
#endif
	return(errs);
}

/*
 * Read up a header from standard input.
 * The source string has the preliminary contents to
 * be read.
 *
 */

char *
readtty(prid, pr, src)
	char prid[], pr[], src[];
{
	char canonb[LINESIZE];
	int c, signull();
	char ch;
	register char *cp, *cp2;

	fputs(gettxt(prid, pr), stdout);
	fflush(stdout);
	if (src != NOSTR && strlen(src) > LINESIZE - 2) {
		putchar('\n');
		pfmt(stdout, MM_ERROR, toolongtoedit);
		return(src);
	}
#ifndef TIOCSTI
	if (src != NOSTR)
		cp = copy(src, canonb);
	else
		cp = copy("", canonb);
	fputs(canonb, stdout);
	fflush(stdout);
#else
	cp = src == NOSTR ? "" : src;
	while (c = *cp++) {
		if (c == c_erase || c == c_kill) {
			ch = '\\';
			ioctl(0, TIOCSTI, &ch);
		}
		ch = c;
		ioctl(0, TIOCSTI, &c);
	}
	cp = canonb;
	*cp = 0;
#endif
	cp2 = cp;
	while (cp2 < canonb + LINESIZE)
		*cp2++ = 0;
	cp2 = cp;
	if (setjmp(rewrite))
		goto redo;
#ifdef SIGCONT
	sigset(SIGCONT, ttycont);
#endif
	while (cp2 < canonb + LINESIZE) {
		c = getc(stdin);
		if (c == EOF || c == '\n')
			break;
		*cp2++ = c;
	}
	*cp2 = 0;
#ifdef SIGCONT
	sigset(SIGCONT, signull);
#endif
	if (c == EOF && ferror(stdin) && hadcont) {
redo:
		hadcont = 0;
		cp = strlen(canonb) > 0 ? canonb : NOSTR;
		clearerr(stdin);
		return(readtty(prid, pr, cp));
	}
#ifndef TIOCSTI
	if (cp == NOSTR || *cp == '\0')
		return(src);
	cp2 = cp;
	if (!ttyset)
		return(strlen(canonb) > 0 ? savestr(canonb) : NOSTR);
	while (*cp != '\0') {
		c = *cp++;
		if (c == c_erase) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2--;
			continue;
		}
		if (c == c_kill) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2 = canonb;
			continue;
		}
		*cp2++ = c;
	}
	*cp2 = '\0';
#endif
	if (equal("", canonb))
		return(NOSTR);
	return(savestr(canonb));
}

#ifdef SIGCONT
/*
 * Receipt continuation.
 */
void
ttycont(s)
{

	hadcont++;
	sigrelse(SIGCONT);
	longjmp(rewrite, 1);
}
#endif

/*
 * Null routine to allow us to hold SIGCONT
 */
void
signull(s)
{}
#endif	/* USG_TTY */
