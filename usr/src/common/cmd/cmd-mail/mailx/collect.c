#ident	"@(#)collect.c	11.1"
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

#ident "@(#)collect.c	1.28 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Collect input from standard input, handling
 * ~ escapes.
 */

#include "rcv.h"

/* Codes for encoding_type_needed */
#define ENC_NONE 0
#define ENC_QP 1 /* quoted-printable */
#define ENC_B64 2 /* base64 */

#ifdef SIGCONT
static void	collcont ARGS((int));
static void	(*sigrset ARGS((int,void (*)(int))))ARGS((int));
#endif
static void	collrub ARGS((int));
static char	*cpstr ARGS((const char *cp));
static void	cpout ARGS((const char *, FILE *));
static int	exwrite ARGS((const char[], FILE *));
static int	forward ARGS((const char[], FILE *, int));
static int	forwardmsgs ARGS((int*, FILE *, int, int));
static const char *getboundary ARGS(());
static int	gomime ARGS((char *linebuf, struct header *hp, long flags, FILE *obuf, FILE *ibuf, char *iprompt, int eof, int escape));
static void	intack ARGS((int));
static FILE	*mesedit ARGS((FILE *, FILE *, int, struct header *));
static FILE	*mespipe ARGS((FILE *, FILE *, char[]));
static char	*mkdate ARGS((void));
static void	resetsigs ARGS((int));
static void	rev ARGS((char *linebuf, int nread));
static void	set8bit ARGS((char *linebuf, int nread));
static void	showthelp ARGS((const char *linebuf));
static int	stripnulls ARGS((char *, int));
static void	xhalt ARGS((void));
static char	**Xaddone ARGS((char **, const char[]));
static int 	TranslateInputToEncodedOutput ARGS((FILE *, FILE *, int, char *));
static void	encode_file_to_base64 ARGS((FILE *, FILE *, char *));
static void	encode_file_to_quoted_printable ARGS((FILE *, FILE *, int));

/*
 * Read a message from standard output and return a read file to it
 * or NULL on error.
 */

/*
 * The following hokiness with global variables is so that on
 * receipt of an interrupt signal, the partial message can be salted
 * away on dead.letter.  The output file must be available to flush,
 * and the input to read.  Several open files could be saved all through
 * mailx if stdio allowed simultaneous read/write access.
 */

static void		(*savesig)();	/* Previous SIGINT value */
static void		(*savehup)();	/* Previous SIGHUP value */
#ifdef SIGCONT
static void		(*savecont)();	/* Previous SIGCONT value */
#endif
static FILE		*newi;		/* File for saving away */
static FILE		*newo;		/* Output side of same */
static int		ig_ints;	/* Ignore interrupts */
static int		hadintr;	/* Have seen one SIGINT so far */
static struct header	*savehp;
static jmp_buf		coljmp;		/* To get back to work */

/* some flags used when collecting the message */
#define MIME_8BIT	 0x0001
#define MIME_REV	 0x0002
#define TEXT_PLAIN	 0x0004
#define TEXT_ENRICHED	 0x0008
#define RICH_BOLD	 0x0010
#define RICH_ITALIC	 0x0020
#define RICH_UNDERLINE	 0x0040
#define RICH_FIXED	 0x0080
#define RICH_JUST_CENTER 0x0100
#define RICH_JUST_RIGHT	 0x0200
#define RICH_JUST_LEFT	 0x0400
#define RICH_INDENT	 0x1000
#define RICH_INDENTRIGHT 0x2000
#define RICH_EXCERPT	 0x4000
#define RICH_VERBATIM	 0x8000

/* some macros that make dealing with the above flags easier */
#define bitson(flags, bit)	(flags & (bit))
#define turnon(flags, bit)	(flags |= (bit))
#define turnoff(flags, bit)	(flags &= ~(bit))
#define toggle(flags, bit)	(bitson(flags, (bit)) ? turnoff(flags, (bit)) : turnon(flags, (bit)))

static FILE *
readmailfromfile(fbuf, hp, zapheaders)
	FILE *fbuf;
	struct header *hp;
	int zapheaders;
{
	FILE *ibuf, *obuf;
	char hdr[LINESIZE];

	if (zapheaders) {
		hp->h_to = hp->h_subject = hp->h_cc = hp->h_bcc = hp->h_defopt =
			hp->h_encodingtype = hp->h_content_type =
			hp->h_content_transfer_encoding = hp->h_mime_version =
			hp->h_date = NOSTR;
		hp->h_others = NOSTRPTR;
		hp->h_seq = 0;
	}
	while (gethfield(fbuf, hdr, 9999L) > 0) {
		if (ishfield(hdr, "to"))
			hp->h_to = addto(hp->h_to, hcontents(hdr));
		else if (ishfield(hdr, "subject"))
			hp->h_subject = addone(hp->h_subject, hcontents(hdr));
		else if (ishfield(hdr, "cc"))
			hp->h_cc = addto(hp->h_cc, hcontents(hdr));
		else if (ishfield(hdr, "bcc"))
			hp->h_bcc = addto(hp->h_bcc, hcontents(hdr));
		else if (ishfield(hdr, "date"))
			hp->h_date = addone(hp->h_date, hcontents(hdr));
		else if (ishfield(hdr, "default-options"))
			hp->h_defopt = addone(hp->h_defopt, hcontents(hdr));
		else if (ishfield(hdr, "encoding-type"))
			hp->h_encodingtype = addone(hp->h_encodingtype, hcontents(hdr));
		else if (ishfield(hdr, "from"))
			assign("postmark", hcontents(hdr));
		else if (ishfield(hdr, "content-type"))
			hp->h_content_type = addone(hp->h_content_type, hcontents(hdr));
		else if (ishfield(hdr, "mime-version"))
			hp->h_mime_version = addone(hp->h_mime_version, hcontents(hdr));
		else if (ishfield(hdr, "content-transfer-encoding"))
			hp->h_content_transfer_encoding = addone(hp->h_content_transfer_encoding, hcontents(hdr));
		else if (ishfield(hdr, "content-length"))
			/* EMPTY */;
		else
			hp->h_others = Xaddone(hp->h_others, hdr);
		hp->h_seq++;
	}
	if ((obuf = fopen(tempMail, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, Strerror(errno));
		fclose(fbuf);
		return (FILE*)NULL;
	}
	if ((ibuf = fopen(tempMail, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, Strerror(errno));
		removefile(tempMail);
		fclose(fbuf);
		fclose(obuf);
		return (FILE*)NULL;
	}
	removefile(tempMail);
	if (strlen(hdr) > 0) {
		fputs(hdr, obuf);
		putc('\n', obuf);
	}
	copystream(fbuf, obuf);
	fclose(newo);
	fclose(newi);
	newo = obuf;
	newi = ibuf;
	return newo;
}

/*
 * Return the number of lines in the message in the
 * file specified by buf.
 */

static int
countlines(FILE *buf) {
int nls = 0;
char buffer[LINESIZE];
int c;

  /* buf already rewound by caller */
  while ((c = fread(buffer, 1, sizeof(buffer), buf)) > 0)
	while (c) 
		if (buffer[--c] == '\n')
			nls++;
  
  rewind(buf);
  return nls;
}

FILE *
collect(hp, forwardvec)
	struct header *hp;
	int *forwardvec;
{
	FILE *ibuf, *fbuf, *obuf;
	int escape, eof;
	long lc, cc;
	register int c, t;
	char linebuf[LINESIZE+1];
	const char *cp;
	char *iprompt;
	long flags = 0;

	noreset++;
	if (tflag) {
		hadintr = 1;
		if ((savesig = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
			sigset(SIGINT, collrub);
		if ((savehup = sigset(SIGHUP, SIG_IGN)) != SIG_IGN)
			sigset(SIGHUP, collrub);
#ifdef SIGCONT
# ifdef preSVr4
		savecont = sigset(SIGCONT, collcont);
# else
		savecont = sigrset(SIGCONT, collcont);
# endif
#endif
		(void) readmailfromfile(stdin, hp, 1);
		ibuf = newi;
		obuf = newo;
		goto eofl;
	}

	ibuf = obuf = NULL;
	newi = newo = NULL;
	if ((obuf = fopen(tempMail, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, Strerror(errno));
		goto err;
	}
	newo = obuf;
	if ((ibuf = fopen(tempMail, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, Strerror(errno));
		newo = NULL;
		fclose(obuf);
		goto err;
	}
	newi = ibuf;
	removefile(tempMail);

	ig_ints = !!value("ignore");
	hadintr = 1;
	savehp = hp;
	if ((savesig = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
		if (ig_ints) {
			sigset(SIGINT, intack);
		} else {
			sigset(SIGINT, collrub);
		}
	if ((savehup = sigset(SIGHUP, SIG_IGN)) != SIG_IGN)
		sigset(SIGHUP, collrub);
#ifdef SIGCONT
# ifdef preSVr4
	savecont = sigset(SIGCONT, collcont);
# else
	savecont = sigrset(SIGCONT, collcont);
# endif
#endif
	/*
	 * If we are going to prompt for subject/cc/bcc,
	 * refrain from printing a newline after
	 * the headers (since some people mind).
	 */

	if (hp->h_subject == NOSTR) {
		hp->h_subject = sflag;
		sflag = NOSTR;
	}
	t = GMASK;
	c = 0;
	if (hp->h_date == NOSTR && value("add_date")) {
		hp->h_date = mkdate();
	}
	if (intty) {
		if (hp->h_to == NOSTR && forwardvec)
			c |= GTO;
		if (hp->h_subject == NOSTR && value("asksub"))
			c |= GSUBJECT;
		if (!value("askatend")) {
			if (hp->h_cc == NOSTR && value("askcc"))
				c |= GCC;
			if (hp->h_bcc == NOSTR && value("askbcc"))
				c |= GBCC;
		}
		if (c)
			t &= ~GNL;
	}
	if ((cp = value("add_headers")) != NOSTR) {
		char *newcp = cpstr(cp);
		trimnl(newcp);	/* puthead() will restore the trailing \n. */
		hp->h_others = Xaddone(hp->h_others, newcp);
	}
	if (hp->h_seq != 0) {
		puthead(hp, stdout, t, (FILE*)0, 1);
		fflush(stdout);
	}
	if (setjmp(coljmp))
		goto err;
	if (c)
		grabh(hp, c, 1);
	escape = SENDESC;
	if ((cp = value("escape")) != NOSTR)
		escape = *cp;
	if (escape == '\0')	/* if escape=nothing, disable escape processing */
		escape = -1;
	eof = 0;
	if ((cp = value("MAILX_HEAD")) != NOSTR) {
	      cpout( cp, obuf);
	      if (isatty(fileno(stdin)))
		    cpout( cp, stdout);
	}
	iprompt = value("iprompt");
	fflush(obuf);
	hadintr = 0;

	if (intty && (value("autovedit") || value("autoedit"))) {
		if (forwardvec) {
			if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
				goto err;
			forwardvec = 0;
		}
		if ((obuf = mesedit(ibuf, obuf, value("autovedit") ? 'v' : 'e', hp)) == NULL)
			goto err;
		newo = obuf;
		ibuf = newi;
		pfmt(stdout, MM_NOSTD, usercontinue);
	}

	for (;;) {
		int nread, hasnulls;
		setjmp(coljmp);
		sigrelse(SIGINT);
		sigrelse(SIGHUP);
		if (intty && outtty && iprompt)
			fputs(iprompt, stdout);
		flush();
		if ((nread = getline(linebuf,LINESIZE,stdin,&hasnulls)) == NULL) {
			if (intty && value("ignoreeof") != NOSTR) {
				if (++eof > 35)
					break;
				pfmt(stdout, MM_NOSTD,
					":212:Use \".\" to terminate letter\n");
				continue;
			}
			break;
		}
		eof = 0;
		hadintr = 0;
		if (intty && equal(".\n", linebuf) &&
		    (value("dot") != NOSTR || value("ignoreeof") != NOSTR))
			break;
		if ((linebuf[0] != escape) || (escape == -1) || (!intty && !escapeokay)) {
			if (bitson(flags,MIME_REV)) rev(linebuf, nread);
			if (bitson(flags,MIME_8BIT)) set8bit(linebuf, nread);
			if (fwrite(linebuf, 1, nread, obuf) != nread)
				goto err;
			continue;
		}
		/*
		 * On double escape, just send the single one.
		 */
		if ((nread > 1) && (linebuf[1] == escape)) {
			if (bitson(flags,MIME_REV)) rev(linebuf+1, nread-1);
			if (bitson(flags,MIME_8BIT)) set8bit(linebuf, nread);
			if (fwrite(linebuf+1, 1, nread-1, obuf) != (nread-1))
				goto err;
			continue;
		}
		if (hasnulls)
			nread = stripnulls(linebuf, nread);
		c = linebuf[1];
		linebuf[nread - 1] = '\0';

		switch (c) {
		default:
			/*
			 * Otherwise, it's an error.
			 */
			pfmt(stdout, MM_ERROR, ":213:Unknown tilde escape.\n");
			break;

		case 'a':
		case 'A':
			/*
			 * autograph; sign the letter.
			 */

			if ((cp = value(c=='a' ? "sign":"Sign")) != 0 &&
				!equal(cp,"")) {
			      cpout( cp, obuf);
			      if (isatty(fileno(stdin)))
				    cpout( cp, stdout);
			}

			break;

		case 'i':
			/*
			 * insert string
			 */
			cp = skipspace(linebuf+2);
			if (*cp)
				cp = value(cp);
			if (cp != NOSTR && !equal(cp,"")) {
				cpout(cp, obuf);
				if (isatty(fileno(stdout)))
					cpout(cp, stdout);
			}
			break;

		case '!':
			/*
			 * Shell escape, send the balance of the
			 * line to sh -c.
			 */

			shell(&linebuf[2]);
			break;

		case ':':
		case '_':
			/*
			 * Escape to command mode, but be nice!
			 */

			execute(&linebuf[2], 1);
			iprompt = value("iprompt");
			if ((cp = value("escape")) != 0)
				escape = *cp;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case '.':
			/*
			 * Simulate end of file on input.
			 */
			goto eofl;

		case 'q':
		case 'Q':
			/*
			 * Force a quit of sending mail.
			 * Act like an interrupt happened.
			 */

			hadintr++;
			collrub(SIGINT);
			exit(1);
			/* NOTREACHED */

		case 'x':
			xhalt();
			break; 	/* not reached */

		case 'h':
			/*
			 * Grab a bunch of headers.
			 */
			if (!intty || !outtty) {
				pfmt(stdout, MM_ERROR, ":214:~h: Not a tty\n");
				break;
			}
			grabh(hp, GMASK, 0);
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case 't':
			/*
			 * Add to the To list.
			 */

			hp->h_to = addto(hp->h_to, &linebuf[2]);
			hp->h_seq++;
			break;

		case 's':
			/*
			 * Set the Subject list.
			 */

			cp = skipspace(linebuf+2);
			hp->h_subject = savestr(cp);
			hp->h_seq++;
			break;

		case 'c':
			/*
			 * Add to the CC list.
			 */

			hp->h_cc = addto(hp->h_cc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'b':
			/*
			 * Add stuff to blind carbon copies list.
			 */
			hp->h_bcc = addto(hp->h_bcc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'R':
			hp->h_defopt = addone(hp->h_defopt, "/receipt");
			hp->h_seq++;
			pfmt(stderr, MM_INFO, ":215:Return receipt marked.\n");
			break;

		case 'd':
			copy(Getf("DEAD"), &linebuf[2]);
			/* FALLTHROUGH */

		case '<':
		case 'r': {
			int	ispip;
			void (*sigint)();
			/*
			 * Invoke a file:
			 * Search for the file name,
			 * then open it and copy the contents to obuf.
			 *
			 * if name begins with '!', read from a command
			 */

			cp = skipspace(linebuf+2);
			if (*cp == '\0') {
				pfmt(stdout, MM_ERROR,
					":216:Interpolate what file?\n");
				break;
			}
			if (*cp=='!') {
				/* take input from a command */
				ispip = 1;
				if ((fbuf = npopen(++cp, "r"))==NULL) {
					pfmt(stderr, MM_ERROR, failed, "pipe",
						Strerror(errno));
					break;
				}
				sigint = sigset(SIGINT, SIG_IGN);
			} else {
				ispip = 0;
				cp = expand(cp);
				if (cp == NOSTR)
					break;
				if (isdir(cp)) {
					pfmt(stdout, MM_ERROR,
						":217:%s: directory\n", cp);
					break;
				}
				if ((fbuf = fopen(cp, "r")) == NULL) {
					pfmt(stderr, MM_ERROR, badopen,
						cp, Strerror(errno));
					break;
				}
			}
			printf("\"%s\" ", cp);
			flush();
			lc = cc = 0;
			while ((t = getc(fbuf)) != EOF) {
				if (t == '\n')
					lc++;
				putc(t, obuf);
				cc++;
			}
			fflush(obuf);
			if (ferror(obuf)) {
				if (ispip) {
					npclose(fbuf);
					sigset(SIGINT, sigint);
				} else
					fclose(fbuf);
				goto err;
			}
			if (ispip) {
				npclose(fbuf);
				sigset(SIGINT, sigint);
			} else
				fclose(fbuf);
			printf("%ld/%ld\n", lc, cc);
			fflush(obuf);
			break;
			}

		case 'w':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
				forwardvec = 0;
			}

			/*
			 * Write the message on a file.
			 */

			cp = skipspace(linebuf+2);
			if (*cp == '\0') {
				pfmt(stderr, MM_ERROR,
					":218:Write what file!?\n");
				break;
			}
			if ((cp = expand(cp)) == NOSTR)
				break;
			fflush(obuf);
			if (ferror(obuf)) {
				pfmt(stderr, MM_ERROR, badwrite1, Strerror(errno));
			} else {
				rewind(ibuf);
				exwrite(cp, ibuf);
			}
			break;

		case 'm':
		case 'f':
		case 'M':
		case 'F':
			/*
			 * Interpolate the named messages, if we
			 * are in receiving mail mode.  Does the
			 * standard list processing garbage.
			 * If ~f is given, we don't shift over.
			 */

			if (!rcvmode) {
				pfmt(stdout, MM_ERROR,
					":219:No messages to send from!?!\n");
				break;
			}
			cp = skipspace(linebuf+2);
			if (forward(cp, obuf, c) < 0)
				goto err;
			fflush(obuf);
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case '?':
			showthelp(linebuf+2);
			break;

		case 'p': {
			void (*sigpipe)(), (*sigint)();
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
				forwardvec = 0;
			}

			/*
			 * Print out the current state of the
			 * message without altering anything.
			 */

			fflush(obuf);
			rewind(ibuf);
			if ((cp=value("crt")) != 0 && 
			    (puthead(hp,NULL,GMASK, NULL, 0) + countlines(ibuf)
			     > atoi(cp))) {
				const char *pg = pager();
				if ((fbuf = npopen(pg, "w")) != 0) {
					sigint = sigset(SIGINT, SIG_IGN);
					sigpipe = sigset(SIGPIPE, SIG_IGN);
				} else
					fbuf = stdout;
			} else
				fbuf = stdout;
			pfmt(fbuf, MM_NOSTD, ":220:-------\nMessage contains:\n");
			puthead(hp, fbuf, GMASK, (FILE*)0, 1);
			copystream(ibuf, fbuf);
			if (fbuf != stdout) {
				npclose(fbuf);
				sigset(SIGPIPE, sigpipe);
				sigset(SIGINT, sigint);
			}
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;
			}

		case '^':
		case '|':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
				forwardvec = 0;
			}

			/*
			 * Pipe message through command.
			 * Collect output as new message.
			 */

			obuf = mespipe(ibuf, obuf, &linebuf[2]);
			newo = obuf;
			ibuf = newi;
			newi = ibuf;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case 'v':
		case 'e':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
				forwardvec = 0;
			}

			/*
			 * Edit the current message.
			 * 'e' means to use EDITOR
			 * 'v' means to use VISUAL
			 */

			if ((obuf = mesedit(ibuf, obuf, c, hp)) == NULL)
				goto err;
			newo = obuf;
			ibuf = newi;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case 'T':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
			}
			if (gomime(linebuf, hp, flags, obuf, ibuf, iprompt, eof, escape))
				goto eof_mime;
			goto err;

		case '*':
			/* first make certain the forwarded messages are copied */
			if (forwardvec) {
				if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
					goto err;
			}
			switch (*skipspace(linebuf+2)) {
#if 1
			case '+':	/* ~*+ Enter 8-bit mode for non-ASCII characters */
			case '-':	/* ~*- Leave 8-bit mode (return to ASCII) */
			case '^':	/* ~*^ Toggle ``Upside-down'' (right-to-left) mode. */
			case 'S':	/* ~*S Toggle Semitic mode (right-to-left AND eight-bit) */
#endif
			case '*':	/* ~** Add non-text data (pictures, sounds, etc.) as a new MIME part */
			case 'Z':	/* ~*Z Add the contents of ~/.SIGNATURE as a NON-TEXT (MIME-format) signature. */
				if (gomime(linebuf, hp, flags, obuf, ibuf, iprompt, eof, escape))
					goto eof_mime;
				goto err;
#if 0
			case '+':	/* ~*+ Enter 8-bit mode for non-ASCII characters */
				turnon(flags, MIME_8BIT);
				pfmt(stdout, MM_INFO, ":598:Entering text in eight-bit mode\n");
				break;
			case '-':	/* ~*- Leave 8-bit mode (return to ASCII) */
				turnoff(flags, MIME_8BIT);
				pfmt(stdout, MM_INFO, ":595:Entering text in seven-bit (normal) mode\n");
				break;
			case '^':	/* ~*^ Toggle ``Upside-down'' (right-to-left) mode. */
				toggle(flags, MIME_REV);
				if (bitson(flags, MIME_REV))
					pfmt(stdout, MM_INFO, ":596:Entering right-to-left mode\n");
				else
					pfmt(stdout, MM_INFO, ":597:Exiting right-to-left mode\n");
				break;
			case 'S':	/* ~*S Toggle Semitic mode (right-to-left AND eight-bit) */
				toggle(flags, MIME_REV);
				toggle(flags, MIME_8BIT);
				if (bitson(flags, MIME_REV))
					pfmt(stdout, MM_INFO, ":596:Entering right-to-left mode\n");
				else
					pfmt(stdout, MM_INFO, ":597:Exiting right-to-left mode\n");
				if (bitson(flags, MIME_8BIT))
					pfmt(stdout, MM_INFO, ":598:Entering text in eight-bit mode\n");
				else
					pfmt(stdout, MM_INFO, ":595:Entering text in seven-bit (normal) mode\n");
				break;
#endif
			default:
				/*
				 * Otherwise, it's an error.
				 */
				pfmt(stdout, MM_ERROR, ":213:Unknown tilde escape.\n");
				break;
			}
		}
		fflush(obuf);
	}
eofl:
	/*
	 * simulate a ~a/~A autograph and sign the letter.
	 */
	if ((cp = value("autosign")) != 0) {
	      cpout( cp, obuf);
	      if (isatty(fileno(stdin)))
		    cpout( cp, stdout);
	}
	if ((cp = value("autoSign")) != 0) {
	      cpout( cp, obuf);
	}

	/* first make certain the forwarded messages are copied */
	if (forwardvec) {
		if (forwardmsgs(forwardvec, obuf, 'B', 0) < 0)
			goto err;
		forwardvec = 0;
	}

	if ((cp = value("MAILX_TAIL")) != NOSTR) {
	      cpout( cp, obuf);
	      if (isatty(fileno(stdin)))
		    cpout( cp, stdout);
	}

eof_mime:
	if (intty && value("askatend")) {
		if (hp->h_cc == NOSTR && value("askcc"))
			grabh(hp, GCC, 0);
		if (hp->h_bcc == NOSTR && value("askbcc"))
			grabh(hp, GBCC, 0);
	}
	fflush(obuf);
	fclose(obuf);
	rewind(ibuf);
	resetsigs(0);
	noreset = 0;
	return(ibuf);

err:
	if (ibuf != NULL)
		fclose(ibuf);
	if (obuf != NULL)
		fclose(obuf);
	resetsigs(0);
	noreset = 0;
	return(NULL);
}

static void resetsigs(resethup)
int resethup;
{
	(void) sigset(SIGINT, savesig);
	if (resethup)
		(void) sigset(SIGHUP, savehup);
#ifdef SIGCONT
# ifdef preSVr4
	(void) sigset(SIGCONT, savecont);
# else
	(void) sigrset(SIGCONT, savecont);
# endif
#endif
}

/*
 * Write a file ex-like.
 */

static int
exwrite(fname, ibuf)
	const char fname[];
	FILE *ibuf;
{
	register FILE *of;
	struct stat junk;
	void (*sigint)(), (*sigpipe)();
	int pi = (*fname == '!');

	if (!pi && !value("posix2") && stat(fname, &junk) >= 0
	 && (junk.st_mode & S_IFMT) == S_IFREG) {
		pfmt(stderr, MM_ERROR, filedothexist, fname);
		return(-1);
	}
	if ((of = pi ? npopen(++fname, "w") : fopen(fname, value("posix2") ?
						"a" : "w")) == NULL) {
		pfmt(stderr, MM_ERROR, errmsg, fname, Strerror(errno));
		return(-1);
	}
	if (pi) {
		sigint = sigset(SIGINT, SIG_IGN);
		sigpipe = sigset(SIGPIPE, SIG_IGN);
	}
	lcwrite(fname, ibuf, of);
	pi ? npclose(of) : fclose(of);
	if (pi) {
		sigset(SIGPIPE, sigpipe);
		sigset(SIGINT, sigint);
	}
	return(0);
}

void
lcwrite(fn, fi, fo)
const char *fn;
FILE *fi, *fo;
{
	register int c;
	long lc, cc;

	printf("\"%s\" ", fn);
	fflush(stdout);
	lc = cc = 0;
	while ((c = getc(fi)) != EOF) {
		cc++;
		if (putc(c, fo) == '\n')
			lc++;
		if (ferror(fo)) {
			pfmt(stderr, MM_ERROR, badwrite1, Strerror(errno));
			return;
		}
	}
	printf("%ld/%ld\n", lc, cc);
	fflush(stdout);
}

/*
 * Edit the message being collected on ibuf and obuf.
 * Write the message out onto some poorly-named temp file
 * and point an editor at it.
 *
 * On return, make the edit file the new temp file.
 */

static FILE *
mesedit(ibuf, obuf, c, hp)
	FILE *ibuf, *obuf;
	struct header *hp;
{
	pid_t pid;
	FILE *fbuf;
	void (*sigint)();
	struct stat sbuf;
	register const char *editor_value;

	if (stat(tempEdit, &sbuf) >= 0) {
		pfmt(stdout, MM_ERROR, filedothexist, tempEdit);
		goto out;
	}
	close(creat(tempEdit, TEMPPERM));
	if ((fbuf = fopen(tempEdit, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempEdit, Strerror(errno));
		goto out;
	}
	fflush(obuf);
	rewind(ibuf);
	if (value("editheaders"))
		puthead(hp, fbuf, GMASK, (FILE*)0, 1);
	copystream(ibuf, fbuf);
	fflush(fbuf);
	if (ferror(fbuf)) {
		pfmt(stderr, MM_ERROR, badwrite, tempEdit, Strerror(errno));
		removefile(tempEdit);
		goto out;
	}
	fclose(fbuf);
	if (((editor_value = value(c == 'e' ? "EDITOR" : "VISUAL")) == NOSTR) || (*editor_value == '\0'))
		editor_value = c == 'e' ? EDITOR : VISUAL;
	editor_value = safeexpand(editor_value);

	/*
	 * Fork/execlp the editor on the edit file
	*/

	pid = vfork();
	if ( pid == (pid_t)-1 ) {
		pfmt(stderr, MM_ERROR, failed, "fork", Strerror(errno));
		removefile(tempEdit);
		goto out;
	}
	if ( pid == 0 ) {
		sigchild();
		execlp(editor_value, editor_value, tempEdit, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, editor_value, Strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	sigint = sigset(SIGINT, SIG_IGN);
	while (wait((int *)0) != pid)
		;
	sigset(SIGINT, sigint);
	/*
	 * Now switch to new file.
	 */

	if ((fbuf = fopen(tempEdit, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempEdit, Strerror(errno));
		removefile(tempEdit);
		goto out;
	}
	removefile(tempEdit);
	newo = readmailfromfile(fbuf, hp, (value("editheaders") != 0));
	fclose(fbuf);
out:
	return(newo);
}

/*
 * Pipe the message through the command.
 * Old message is on stdin of command;
 * New message collected from stdout.
 * Sh -c must return 0 to accept the new message.
 */

static FILE *
mespipe(ibuf, obuf, cmd)
	FILE *ibuf, *obuf;
	char cmd[];
{
	register FILE *ni, *no;
	pid_t pid;
	int s;
	void (*sigint)();
	char *Shell;

	newi = ibuf;
	if ((no = fopen(tempEdit, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempEdit, Strerror(errno));
		return(obuf);
	}
	if ((ni = fopen(tempEdit, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempEdit, Strerror(errno));
		fclose(no);
		removefile(tempEdit);
		return(obuf);
	}
	removefile(tempEdit);
	fflush(obuf);
	rewind(ibuf);
	if ((Shell = value("SHELL")) == NULL || *Shell=='\0')
		Shell = SHELL;
	if ((pid = vfork()) == (pid_t)-1) {
		pfmt(stderr, MM_ERROR, failed, "fork", Strerror(errno));
		goto err;
	}
	if (pid == 0) {
		/*
		 * stdin = current message.
		 * stdout = new message.
		 */

		sigchild();
		close(0);
		dup(fileno(ibuf));
		close(1);
		dup(fileno(no));
		for (s = 4; s < 15; s++)
			close(s);
		execlp(Shell, Shell, "-c", cmd, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, Shell, Strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	sigint = sigset(SIGINT, SIG_IGN);
	while (wait(&s) != pid)
		;
	sigset(SIGINT, sigint);
	if (s != 0 || pid == (pid_t)-1) {
		pfmt(stderr, MM_ERROR, cmdfailed, cmd);
		goto err;
	}
	if (fsize(ni) == 0) {
		pfmt(stderr, MM_ERROR, ":221:No bytes from \"%s\" !?\n", cmd);
		goto err;
	}

	/*
	 * Take new files.
	 */

	newi = ni;
	fclose(ibuf);
	fclose(obuf);
	return(no);

err:
	fclose(no);
	fclose(ni);
	return(obuf);
}

/*
 * Interpolate the named messages into the current
 * message, preceding each line with a tab.
 * Return 0 on success, or -1 if an error is encountered writing
 * the message temporary.  The flag argument is 'm' if we
 * should shift over and 'f' if not.
 */
static int
forward(ms, obuf, f)
	const char ms[];
	FILE *obuf;
{
	register int *msgvec;

	msgvec = (int *) salloc((msgCount+1) * sizeof *msgvec);
	if (getmsglist(ms, msgvec, 0) < 0)
		return(0);
	if (*msgvec == NULL) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			pfmt(stdout, MM_ERROR, ":222:No appropriate messages\n");
			return(0);
		}
		msgvec[1] = NULL;
	}
	pfmt(stdout, MM_NOSTD, ":223:Interpolating:");
	if (forwardmsgs(msgvec, obuf, f, 1) < 0)
		return(-1);
	printf("\n");
	return(0);
}

/*
 * Interpolate the named messages into the current
 * message, preceding each line with a tab.
 * Return 0 on success, or -1 if an error is encountered writing
 * the message temporary.  The flag argument is:
 *	'm','M'		we should shift over with "mprefix"
 *	'f','F'		we should not
 *
 *	'm','f'		ignore headers
 *	'M','F'		do not ignore headers
 *
 * If value("forwardbracket")
 *	'f','F'		Bracket with "forwardbegin"/"forwardend" messages & shift with "forwardprefix"
 * 			Use defaults if those are not set.
 */
static int
forwardmsgs(msgvec, obuf, flag, printnum)
	int *msgvec;
	FILE *obuf;
	int flag;
	int printnum;
{
	register int *ip;
	int doign = Islower(flag);

	if ((tolower(flag) == 'f') && value("forwardbracket"))
		flag = 'B';

	if (flag == 'B') {
		const char *cp = value("forwardbegin");
		if (cp) cpout(cp, obuf);
		else cpout(gettxt(forwardbeginid, forwardbegin), obuf);
	}

	for (ip = msgvec; *ip != NULL; ip++) {
		if (printnum)
			printf(" %d", *ip);
		if (sendmsg(dot = &message[*ip-1], obuf, doign, flag, 1, 0) < 0) {
			pfmt(stderr, MM_ERROR, badwrite,
				tempMail, Strerror(errno));
			return(-1);
		}
		touch(*ip);
	}

	if (flag == 'B') {
		const char *cp = value("forwardend");
		if (cp) cpout(cp, obuf);
		else cpout(gettxt(forwardendid, forwardend), obuf);
	}
	return(0);
}

/*
 * Print (continue) when continued after ^Z.
 */
#ifdef SIGCONT
/*ARGSUSED*/
static void
collcont(s)
{
	pfmt(stdout, MM_NOSTD, usercontinue);
	fflush(stdout);
}

# ifndef preSVr4
static void (*sigrset(int sig, void (*sigfunc)(int)))(int)
{
	struct sigaction nsig, osig;
	nsig.sa_handler = sigfunc;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = SA_RESTART;
	(void) sigaction(sig, &nsig, &osig);
	return osig.sa_handler;
}
# endif /* preSVr4 */
#endif /* SIGCONT */

/*
 * On interrupt, go here to save the partial
 * message on ~/dead.letter.
 * Then restore signals and execute the normal
 * signal routine.  We only come here if signals
 * were previously set anyway.
 */
static void
collrub(s)
{
	if (s == SIGHUP)
		sigignore(SIGHUP);
	if (s == SIGINT && hadintr == 0) {
		hadintr++;
		putchar('\n');
		pfmt(stdout, MM_NOSTD,
			":224:(Interrupt -- one more to kill letter)\n");
		sigrelse(s);
		longjmp(coljmp, 1);
	}
	fclose(newo);
	rewind(newi);
	if (fsize(newi) > 0 && (value("save") || s != SIGINT))
		savemail(Getf("DEAD"), DEADPERM, savehp,
			newi, (off_t)0, s == SIGINT, !value("posix2"));
	fclose(newi);
	resetsigs(1);
	if (rcvmode) {
		if (s == SIGHUP)
			hangup(s);
		else
			stop(s);
	} else
		exit(1);
}

/*
 * Acknowledge an interrupt signal from the tty by typing an @
 */
/* ARGSUSED */
static void
intack(s)
{
	puts("@");
	fflush(stdout);
	clearerr(stdin);
	longjmp(coljmp,1);
}

/* Read line from stdin, noting any NULL characters.
   Return the number of characters read. Note that the buffer
   passed must be 1 larger than "size" for the trailing NUL byte.
 */
int
getline(line,size,f,hasnulls)
char *line;
int size;
FILE *f;
int *hasnulls;
{
	register int i, ch;
	for (i = 0; (i < size) && ((ch=getc(f)) != EOF); ) {
		if ( ch == '\0' )
			*hasnulls = 1;
		if ((line[i++] = (char)ch) == '\n') break;
	}
	line[i] = '\0';
	return(i);
}

/* Read line from character pointer.
   Return the number of characters read. Note that the buffer
   passed must be 1 larger than "size" for the trailing NUL byte.
 */
int
getline2(line,size,lp,hasnulls)
char *line;
int size;
char **lp;
int *hasnulls;
{
	register int i = 0, ch;
	if (**lp == '\0')
		return 0;
	memset(line, '\0', size + 1);
	while (i < size && (ch=**lp)) {
		line[i++] = ch;
		(*lp)++;
		if (ch == '\n')
			break;
	}
	line[i] = '\0';
	return(i);
}

/* ARGSUSED */
void
savedead(s)
{
	collrub(SIGINT);
	exit(1);
	/* NOTREACHED */
}

/*
 * Add a list of addresses to the end of a header entry field.
 */
char *
addto(hf, news)
	char hf[], news[];
{
	char fname[LINESIZE];
	int comma = docomma(news);

	while ((news = yankword(news, fname, comma)) != 0) {
		strcat(fname, ", ");
		hf = addone(hf, fname);
	}
	return hf;
}

/*
 * Add a string to the end of a header entry field.
 */
char *
addone(hf, news)
	char hf[], news[];
{
	register const char *cp;
	register char *cp2, *linebuf;

	if (hf == NOSTR)
		hf = "";
	if (*news == '\0')
		return(hf);
	linebuf = salloc((unsigned)(strlen(hf) + strlen(news) + 2));
	cp = skipspace(hf);
	for (cp2 = linebuf; *cp;)
		*cp2++ = *cp++;
	if (cp2 > linebuf && cp2[-1] != ' ')
		*cp2++ = ' ';
	cp = skipspace(news);
	while (*cp != '\0')
		*cp2++ = *cp++;
	*cp2 = '\0';
	return(linebuf);
}

/*
    count the number of pointers in an array of char*'s.
*/
static int nptrs(hf)
char **hf;
{
	register int i;
	if (!hf)
		return 0;
	for (i = 0; *hf; hf++)
		i++;
	return i;
}

/*
 * Add a non-standard header to the end of the non-standard headers.
 */
static char **
Xaddone(hf, news)
	char **hf;
	const char news[];
{
	register char *linebuf;
	int nhf = nptrs(hf);

	if (hf == NOSTRPTR)
		hf = (char**)salloc(sizeof(char*) * 2);
	else
		hf = (char**)srealloc((char*)hf, sizeof(char*) * (nhf + 2));
	linebuf = salloc((unsigned)(strlen(news) + 1));
	strcpy(linebuf, news);
	hf[nhf++] = linebuf;
	hf[nhf] = NOSTR;
	return(hf);
}

/*
    Copy a string to the file, interpreting \t and \n along the way.
*/
static void
cpout( str, ofd)
const char *str;
FILE *ofd;
{
      register const char *cp = str;

      for ( ; *cp; cp++ ) {
	    if ( *cp == '\\' ) {
		  switch ( *(cp+1) ) {
			case 'n':
			      putc('\n',ofd);
			      cp++;
			      break;
			case 't':
			      putc('\t',ofd);
			      cp++;
			      break;
			default:
			      putc('\\',ofd);
		  }
	    }
	    else {
		  putc(*cp,ofd);
	    }
      }
      putc('\n',ofd);
      fflush(ofd);
}

/*
    Copy a string to another string, interpreting \t and \n along the way.
*/
static char *
cpstr( cp)
const char *cp;
{
    register char *newstr = salloc(strlen(cp) + 1);	/* 1 for trailing NUL */
    char *retstr = newstr;

    for ( ; *cp; cp++ ) {
	if ( *cp == '\\' ) {
	    switch ( *(cp+1) ) {
		case 'n':
		    *newstr++ = '\n';
		    cp++;
		    break;
		case 't':
		    *newstr++ = '\t';
		    cp++;
		    break;
		default:
		    *newstr++ = '\\';
	    }
	}
	else {
	    *newstr++ = *cp;
	}
    }
    *newstr++ = '\0';
    return retstr;
}

static void
xhalt()
{
	fclose(newo);
	fclose(newi);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
	if (rcvmode)
		stop(0);
	exit(1);
	/* NOTREACHED */
}

/*
	Strip the nulls from a buffer of length n
*/
static int
stripnulls(linebuf, nread)
register char *linebuf;
register int nread;
{
	register int i, j;
	for (i = 0; i < nread; i++)
		if (linebuf[i] == '\0')
			break;
	for (j = i; j < nread; j++)
		if (linebuf[j] != '\0')
			linebuf[i++] = linebuf[j];
	linebuf[i] = '\0';
	return i;
}


/*
    reverse the characters in a line
*/
static void
rev(linebuf, nread)
register char *linebuf;
int nread;
{
    register char *endp = linebuf + nread - 1;
    for ( ; linebuf < endp; linebuf++, endp--)
	{
	char tmp = *endp;
	*endp = *linebuf;
	*linebuf = tmp;
	}
}

/*
    turn on the 8th bit of all characters in a line
*/
static void
set8bit(linebuf, nread)
register char *linebuf;
int nread;
{
    register char *endp = linebuf + nread - 1;
    for ( ; linebuf < endp; linebuf++)
	*linebuf = *linebuf | 0x80;
}

/* which characters absolutely have to be encoded? */
/* 0 == no encoding. 1 == encode */
static char encode_map[128] =
    {
    /* nul soh stx etx eot enq ack bel bs  ht  nl  vt  np  cr  so  si  */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* dle dc1 dc2 dc3 dc4 nak syn etb can em  sub esc fs  gs  rs  us  */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* sp   !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /  */
	0,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?  */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,
    /*  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O  */
	1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*  P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _  */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,
    /*  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o  */
	1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~  del */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1
    };

#if 0
/* convert the input stream into quoted printable. Keep lines < ~72 characters. */
/* If double_lessthans is set, any '<' becomes '<<'. */
static void
encode_file_to_quoted_printable(in, out, double_lessthans)
FILE *in;
FILE *out;
int double_lessthans;
{
    int c;
    int linelength = 0;
    int lastchar = '\n';
    long ret = 0;

    while ((c = getc(in)) != EOF)
	{
	/* Output newlines as newlines. However, if the previous character was a */
	/* blank, output a soft newline first so that there is no trailing white */
	/* space on the line. Such white space can be wiped out by some gateways. */
	if (c == '\n')
	    {
	    if ((lastchar == ' ') || (lastchar == '\t'))
		{
		putc('=', out);
		putc('\n', out);
		ret += 2;
		}
	    putc(c, out);
	    ret++;
	    linelength = 0;
	    }

	/* encoded characters print as =XX, where XX is the hex value of the character */
	else if ((c > 127) || encode_map[c])
	    {
	    putc('=', out);
	    (void) fprintf (out, "%2.2X", c);
	    ret += 3;
	    linelength += 3;
	    }

	/* '<' may become '<<' for text/enriched */
	else if ((c == '<') && double_lessthans)
	    {
	    putc(c, out);
	    putc(c, out);
	    ret += 2;
	    linelength += 2;
	    }

	/* normal characters output as themselves */
	else
	    {
	    putc(c, out);
	    ret++;
	    linelength++;
	    }

	/* output soft newlines for long lines */
	if (linelength > 72)
	    {
	    linelength = 0;
	    putc('=', out);
	    putc('\n', out);
	    ret += 2;
	    }
	lastchar = c;
	}

    /* if a newline wasn't just output, end the stream with a soft newline */
    if (linelength > 0)
	{
	putc('=', out);
	putc('\n', out);
	ret += 2;
	}

    return ret;
}
#else
/* ARGSUSED */
static void
encode_file_to_quoted_printable(in, out, double_lessthans)
FILE *in;
FILE *out;
int double_lessthans;
{
(void)TranslateInputToEncodedOutput(in, out, ENC_QP, "");
}
#endif

/* convert the input stream into quoted printable. Keep lines < ~72 characters. */
/* If double_lessthans is set, any '<' becomes '<<'. */
static long 
qp_write(str, len, out, double_lessthans, do_backslash)
const char *str;
int len;
FILE *out;
int double_lessthans, do_backslash;
{
    int c;
    const char *end = str + len;
    int linelength = 0;
    int lastchar = '\n';
    long ret = 0;

    for ( ; str < end; str++)
	{
	c = *((unsigned char *)str);

	/* Output newlines as newlines. However, if the previous character was a */
	/* blank, output a soft newline first so that there is no trailing white */
	/* space on the line. Such white space can be wiped out by some gateways. */
	if (c == '\n')
	    {
	    if ((lastchar == ' ') || (lastchar == '\t'))
		{
		putc('=', out);
		putc('\n', out);
		ret += 2;
		}
	    putc(c, out);
	    ret++;
	    linelength = 0;
	    }

	/* encoded characters print as =XX, where XX is the hex value of the character */
	else if ((c > 127) || encode_map[c])
	    {
	    putc('=', out);
	    (void) fprintf (out, "%2.2X", c);
	    ret += 3;
	    linelength += 3;
	    }

	/* '<' may become '<<' for text/enriched */
	else if ((c == '<') && double_lessthans)
	    {
	    putc(c, out);
	    putc(c, out);
	    ret += 2;
	    linelength += 2;
	    }

	/* \n and \t may become newline and tab */
	else if ((c == '\\') && do_backslash && ((str[1] == 't') || (str[1] == 'n')))
	    {
	    if (str[1] == 't') { putc('\t', out); linelength++; }
	    else if (str[1] == 'n') { putc('\n', out); linelength = 0; }
	    str++;
	    ret++;
	    }

	/* normal characters output as themselves */
	else
	    {
	    putc(c, out);
	    ret++;
	    linelength++;
	    }

	/* output soft newlines for long lines */
	if (linelength > 72)
	    {
	    linelength = 0;
	    putc('=', out);
	    putc('\n', out);
	    ret += 2;
	    }
	lastchar = c;
	}

    /* if a newline wasn't just output, end the stream with a soft newline */
    if (linelength > 0)
	{
	putc('=', out);
	putc('\n', out);
	ret += 2;
	}

    return ret;
}

/* copy a file, changing '<' to '<<' and going to quoted-printable */
static void 
qp_copyfile(filename, outfp)
const char *filename;
FILE *outfp;
{
    FILE *infp = fopen(filename, "r");
    if (!infp) {
	pfmt(stderr, MM_ERROR, badopen, filename, Strerror(errno));
	return;
    }
    encode_file_to_quoted_printable(infp, outfp, 1);
    fclose(infp);
}

/* copy a stream, changing any '<' into '<<' */
static void 
copy_stream_lt(infp, outfp)
FILE *infp, *outfp;
{
    register int c;
    while ((c = getc(infp)) != EOF) {
	putc(c, outfp);
	if (c == '<')
	    putc(c, outfp);
    }
}

#if 0
static int getc_expand_nl(in)
FILE *in;
{
    static int in_crlf = 0;
    int c;
    if (in_crlf)
	{
	in_crlf = 0;
	return '\n';
	}
    c = getc(in);
    if (c == '\n')
	{
	in_crlf = 1;
	return '\r';
	}
    return c;
}

static char base64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void
encode_file_to_base64(in, out)
FILE *in;
FILE *out;
{
    int encoding = 1;
    unsigned int c[3];
    int quadcount = 0;
    long ret = 0;

    while (encoding)
	{
	int converted = 0;
	int i;

	/* read 3 characters to be converted into 4 */
	for (i = 0; i < 3; i++)
	    {
	    int ch = getc_expand_nl(in);
	    if (ch == EOF)
		c[i] = 0;
	    else
		{
		c[i] = ch;
		converted++;
		}
	    }
	if (converted == 0)
	    encoding = 0;

	else	/* convert the characters */
	    {
	    putc(base64[c[0] >> 2], out);
	    putc(base64[((c[0] & 03) << 4) | ((c[1] & 0xF0) >> 4)], out);
	    ret += 2;

	    if (converted > 1)
		{
		putc(base64[((c[1] & 0xF) << 2) | ((c[2] & 0xC0) >> 6)], out);
		ret++;
		}

	    if (converted > 2)
		{
		putc(base64[c[2] & 0x3F], out);
		ret++;
		}

	    switch (converted)
		{
		case 1:
		    putc('=', out);
		    ret++;
		    /* FALLTHROUGH */

		case 2:
		    putc('=', out);
		    ret++;
		    break;
		}
	    }

	/* add a newline once in a while */
	if (quadcount++ > 16)
	    {
	    putc('\n', out);
	    ret++;
	    quadcount = 0;
	    }
	}

    /* add a final newline */
    if (quadcount > 0)
	{
	putc('\n', out);
	ret++;
	}

    return ret;
}
#else

static int
TranslateInputToEncodedOutput(InputFP, OutputFP, Ecode, ctype)
FILE *InputFP, *OutputFP;
int Ecode;
char *ctype;
{
    int c, EightBitSeen = 0;

    switch(Ecode) {
	case ENC_B64:
	    to64(InputFP, OutputFP, DoesNeedPortableNewlines(ctype));
	    break;
	case ENC_QP:
	    toqp(InputFP, OutputFP);
	    break;
	default:
	    while ((c = getc(InputFP)) != EOF){
		if (c > 127) EightBitSeen = 1;
		putc(c, OutputFP);
	    }
    }
    return(EightBitSeen);
}

static void
encode_file_to_base64(in, out, type)
FILE *in;
FILE *out;
char *type;
{
(void) TranslateInputToEncodedOutput(in, out, ENC_B64, type);
}
#endif

static char list_mime_types[] =
    "cat -s /.mailcap /usr/local/etc/mailcap /usr/etc/mailcap /etc/mailcap /etc/mail/mailcap /usr/public/lib/mailcap | grep '^[A-Za-z_]' | sed 's/;.*//' | fgrep -v '*' | sort -u";

/* flush out a part of a multipart mime message */
static int flush_part(ppart_fp, obuf, boundary, prheaders, part_content_type, part_content_transfer_encoding, recreate)
FILE **ppart_fp;
FILE *obuf;
const char *boundary;
int prheaders;
const char *part_content_type;
const char *part_content_transfer_encoding;
int recreate;
{
	fflush(*ppart_fp);
	if (fsize(*ppart_fp)) {
		if (prheaders) {
			if (boundary)
				(void) fprintf (obuf, "\n--%s\n", boundary);
			if (part_content_type)
				(void) fprintf (obuf, "Content-Type: %s\n", part_content_type);
			if (part_content_transfer_encoding)
				(void) fprintf (obuf, "Content-Transfer-Encoding: %s\n", part_content_transfer_encoding);
			(void) fprintf (obuf, "\n");
		}

		rewind(*ppart_fp);
		copystream(*ppart_fp, obuf);

		if (recreate) {
			fclose(*ppart_fp);
			*ppart_fp = tmpfile();
			if (!*ppart_fp) {
				pfmt(stderr, MM_ERROR, badtmpopen, Strerror(errno));
				return 0;
			}
		}
	}
	return 1;
}

/*
    convert the message to mime format
*/

static int
gomime(linebuf, hp, flags, obuf, ibuf, iprompt, eof, escape)
char *linebuf;
struct header *hp;
long flags;
FILE *obuf;
FILE *ibuf;
char *iprompt;
int eof;
int escape;
{
	FILE *part_fp = tmpfile();
	const char *cp;
	const char *lb2;
	char contentTypeBuff[256];
	int smallness = 0, largeness = 0;
	const char *boundary = 0;
	const char *part_content_type = 0;
	const char *part_content_transfer_encoding = 0;

	turnon(flags, RICH_JUST_LEFT);
	hp->h_mime_version = addone(hp->h_mime_version, "1.0");

	/* check part_fp for NULL */
	if (!part_fp) {
		pfmt(stderr, MM_ERROR, badtmpopen, Strerror(errno));
		return 0;
	}

	fflush(obuf);
	rewind(ibuf);
	if (linebuf[1] == 'T') {
		sprintf(contentTypeBuff, "text/enriched; charset=%s", mail_get_charset());
		hp->h_content_type = addone(hp->h_content_type, contentTypeBuff);
		hp->h_content_transfer_encoding = addone(hp->h_content_transfer_encoding, "quoted-printable");
		encode_file_to_quoted_printable(ibuf, part_fp, 1);
		flags |= TEXT_ENRICHED;
	} else {
		sprintf(contentTypeBuff, "text/plain; charset=%s", mail_get_charset());
		hp->h_content_type = addone(hp->h_content_type, contentTypeBuff);
		hp->h_content_transfer_encoding = addone(hp->h_content_transfer_encoding, "quoted-printable");
		encode_file_to_quoted_printable(ibuf, part_fp, 0);
		flags |= TEXT_PLAIN;
	}

	rewind(obuf);

	for (;;) {
		int nread, hasnulls;

		switch (linebuf[1]) {
		default:
			/*
			 * Otherwise, it's an error.
			 */
			pfmt(stdout, MM_ERROR, ":213:Unknown tilde escape.\n");
			break;

		case 'a':
		case 'A':
			/*
			 * autograph; sign the letter.
			 */

			if ((cp = value(linebuf[1]=='a' ? "sign":"Sign")) != 0 
				&& !equal(cp,"")) {
				if (flags | TEXT_ENRICHED)
					qp_write(cp, strlen(cp), part_fp, 1, 1);
				else
					qp_write(cp, strlen(cp), part_fp, 0, 1);
				if (isatty(fileno(stdin)))
					cpout(cp, stdout);
			}

			break;

		case 'i':
			/*
			 * insert string
			 */
			cp = skipspace(linebuf+2);
			if (*cp)
				cp = value(cp);
			if (cp != NOSTR && !equal(cp,"")) {
				if (flags | TEXT_ENRICHED)
					qp_write(cp, strlen(cp), part_fp, 1, 1);
				else
					qp_write(cp, strlen(cp), part_fp, 0, 1);
				if (isatty(fileno(stdout)))
					cpout(cp, stdout);
			}
			break;

		case '!':
			/*
			 * Shell escape, send the balance of the
			 * line to sh -c.
			 */

			shell(&linebuf[2]);
			break;

		case ':':
		case '_':
			/*
			 * Escape to command mode, but be nice!
			 */

			execute(&linebuf[2], 1);
			iprompt = value("iprompt");
			if ((cp = value("escape")) != 0)
				escape = *cp;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case '.':
			/*
			 * Simulate end of file on input.
			 */
			goto eofl;

		case 'q':
		case 'Q':
			/*
			 * Force a quit of sending mail.
			 * Act like an interrupt happened.
			 */

			hadintr++;
			collrub(SIGINT);
			exit(1);
			/* NOTREACHED */

		case 'x':
			xhalt();
			break; 	/* not reached */

		case 'h':
			/*
			 * Grab a bunch of headers.
			 */
			if (!intty || !outtty) {
				pfmt(stdout, MM_ERROR, ":214:~h: Not a tty\n");
				break;
			}
			grabh(hp, GMASK, 0);
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case 't':
			/*
			 * Add to the To list.
			 */

			hp->h_to = addto(hp->h_to, &linebuf[2]);
			hp->h_seq++;
			break;

		case 's':
			/*
			 * Set the Subject list.
			 */

			cp = skipspace(linebuf+2);
			hp->h_subject = savestr(cp);
			hp->h_seq++;
			break;

		case 'c':
			/*
			 * Add to the CC list.
			 */

			hp->h_cc = addto(hp->h_cc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'b':
			/*
			 * Add stuff to blind carbon copies list.
			 */
			hp->h_bcc = addto(hp->h_bcc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'R':
			hp->h_defopt = addone(hp->h_defopt, "/receipt");
			hp->h_seq++;
			pfmt(stderr, MM_INFO, ":215:Return receipt marked.\n");
			break;

		case 'd':
			copy(Getf("DEAD"), &linebuf[2]);
			/* FALLTHROUGH */

		case '<':
		case 'r': {
			int ispip, lc, cc, t;
			FILE *fbuf;
			void (*sigint)();

			/*
			 * Invoke a file:
			 * Search for the file name,
			 * then open it and copy the contents to part_fp.
			 *
			 * if name begins with '!', read from a command
			 */

			cp = skipspace(linebuf+2);
			if (*cp == '\0') {
				pfmt(stdout, MM_ERROR,
					":216:Interpolate what file?\n");
				break;
			}
			if (*cp=='!') {
				/* take input from a command */
				ispip = 1;
				if ((fbuf = npopen(++cp, "r"))==NULL) {
					pfmt(stderr, MM_ERROR, failed, "pipe",
						Strerror(errno));
					break;
				}
				sigint = sigset(SIGINT, SIG_IGN);
			} else {
				ispip = 0;
				cp = expand(cp);
				if (cp == NOSTR)
					break;
				if (isdir(cp)) {
					pfmt(stdout, MM_ERROR,
						":217:%s: directory\n", cp);
					break;
				}
				if ((fbuf = fopen(cp, "r")) == NULL) {
					pfmt(stderr, MM_ERROR, badopen,
						cp, Strerror(errno));
					break;
				}
			}
			printf("\"%s\" ", cp);
			flush();
			lc = cc = 0;
			while ((t = getc(fbuf)) != EOF) {
				if (t == '\n')
					lc++;
				putc(t, part_fp);
				if ((flags | TEXT_ENRICHED) && (t == '<'))
					putc(t, part_fp);
				cc++;
			}
			fflush(part_fp);
			if (ferror(part_fp)) {
				if (ispip) {
					npclose(fbuf);
					sigset(SIGINT, sigint);
				} else
					fclose(fbuf);
				goto err;
			}

			if (ispip) {
				npclose(fbuf);
				sigset(SIGINT, sigint);
			} else
				fclose(fbuf);
			printf("%d/%d\n", lc, cc);
			fflush(part_fp);
			break;
			}

		case 'w':
			/*
			 * Write the message on a file.
			 */

			if (!flush_part(&part_fp, obuf, boundary, boundary?1:0, part_content_type, part_content_transfer_encoding, 1))
				return 0;

			cp = skipspace(linebuf+2);
			if (*cp == '\0') {
				pfmt(stderr, MM_ERROR,
					":218:Write what file!?\n");
				break;
			}
			if ((cp = expand(cp)) == NOSTR)
				break;
			fflush(part_fp);
			rewind(part_fp);
			if (ferror(part_fp)) {
				pfmt(stderr, MM_ERROR, badwrite1, Strerror(errno));
			} else {
				exwrite(cp, part_fp);
			}
			break;

		case 'm':
		case 'f':
		case 'M':
		case 'F':
			/*
			 * Interpolate the named messages, if we
			 * are in receiving mail mode.  Does the
			 * standard list processing garbage.
			 * If ~f is given, we don't shift over.
			 */

			if (!rcvmode) {
				pfmt(stdout, MM_ERROR,
					":219:No messages to send from!?!\n");
				break;
			}
			cp = skipspace(linebuf+2);
			if (flags & TEXT_ENRICHED)
				fputs("<nofill>", part_fp);
			if (forward(cp, part_fp, linebuf[1]) < 0)
				goto err;
			if (flags & TEXT_ENRICHED)
				fputs("</nofill>", part_fp);
			fflush(part_fp);
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case '?':
			showthelp(linebuf+2);
			break;

		case 'p': {
			FILE *fbuf;
			void (*sigpipe)(), (*sigint)();
			const char *mm_cmd = value("metamail_cmd");
			char *Cmd;
			int crt;
			if (!mm_cmd) mm_cmd = mm_cmd_default;

			/*
			 * Print out the current state of the
			 * message without altering anything.
			 * Use metamail to do the printing.
			 */

			if (!flush_part(&part_fp, obuf, boundary, boundary?1:0, part_content_type, part_content_transfer_encoding, 1))
				return 0;
			fflush(obuf);
			rewind(obuf);
			set_metamail_env();
			Cmd = salloc(strlen(mm_cmd) + 5);
			if (cp = value("crt"))
				crt = (puthead(hp, NULL, GMASK, NULL, 0) +
						countlines(obuf)) > atoi(cp);
			sprintf(Cmd, "%s %s", mm_cmd, crt ? "-p" : "");
			if ((fbuf = npopen(mm_cmd, "w")) != 0) {
				sigint = sigset(SIGINT, SIG_IGN);
				sigpipe = sigset(SIGPIPE, SIG_IGN);
			} else {
				if (crt) {
					const char *pg = pager();
					if ((fbuf = npopen(pg, "w")) != 0) {
						sigint = sigset(SIGINT, SIG_IGN);
						sigpipe = sigset(SIGPIPE, SIG_IGN);
					} else
						fbuf = stdout;
				} else
					fbuf = stdout;
			}
			pfmt(fbuf, MM_NOSTD, ":220:-------\nMessage contains:\n");
			puthead(hp, fbuf, GMASK, (FILE*)0, 1);
			copystream(obuf, fbuf);
			if (fbuf != stdout) {
				npclose(fbuf);
				sigset(SIGPIPE, sigpipe);
				sigset(SIGINT, sigint);
			}
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;
			}

		case '^':
		case '|':
			/*
			 * Pipe message through command.
			 * Collect output as new message.
			 */

			if (!flush_part(&part_fp, obuf, boundary, boundary?1:0, part_content_type, part_content_transfer_encoding, 1))
				return 0;
			obuf = mespipe(obuf, obuf, &linebuf[2]);
			newo = obuf;
			newi = obuf;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case 'v':
		case 'e':
			/*
			 * Edit the current message.
			 * 'e' means to use EDITOR
			 * 'v' means to use VISUAL
			 */

			if (!flush_part(&part_fp, obuf, boundary, boundary?1:0, part_content_type, part_content_transfer_encoding, 1))
				return 0;
			if ((obuf = mesedit(obuf, obuf, linebuf[1], hp)) == NULL)
				goto err;
			newo = obuf;
			pfmt(stdout, MM_NOSTD, usercontinue);
			break;

		case 'T':
			/* convert to text/enriched, output ~T stuff to part temp file */
			if (flags & TEXT_PLAIN) {
				FILE *tmpfile2 = tmpfile();
				if (!tmpfile2) {
					pfmt(stderr, MM_ERROR, badtmpopen, Strerror(errno));
					if (part_fp) fclose(part_fp);
					return 0;
				}
				hp->h_content_type = addone(hp->h_content_type, "text/enriched");
				fflush(part_fp);
				rewind(part_fp);
				copy_stream_lt(part_fp, tmpfile2);
				fclose(part_fp);
				part_fp = tmpfile2;
				turnoff(flags, TEXT_PLAIN);
				turnon(flags, TEXT_ENRICHED);
			}
			switch (*(lb2 = skipspace(linebuf+2))) {
			case 'b':	/* ~Tb Toggle bold mode (turn bold on or off) */
				if (flags & RICH_BOLD)
					{
					turnoff(flags, RICH_BOLD);
					fputs("</bold>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":599:Ending bold\n");
					}
				else
					{
					turnon(flags, RICH_BOLD);
					fputs("<bold>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":600:Beginning bold\n");
					}
				break;
			case 'f':	/* ~Tf Toggle fixed mode (turn fixed mode on or off) */
				if (flags & RICH_FIXED)
					{
					turnoff(flags, RICH_FIXED);
					fputs("</fixed>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":601:Ending fixed\n");
					}
				else
					{
					turnon(flags, RICH_FIXED);
					fputs("<fixed>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":602:Beginning fixed\n");
					}
				break;
			case 'i':	/* ~Ti Toggle italic mode (turn italic/reverse-video on or off) */
				if (flags & RICH_ITALIC)
					{
					turnoff(flags, RICH_ITALIC);
					fputs("</italic>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":603:Ending italic\n");
					}
				else
					{
					turnon(flags, RICH_ITALIC);
					fputs("<italic>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":604:Beginning italic\n");
					}
				break;
			case 'j':	/* ~Tj Alter Justification, in particular: */
				if (flags & RICH_JUST_LEFT)
					{
					turnoff(flags, RICH_JUST_LEFT|RICH_JUST_CENTER|RICH_JUST_RIGHT);
					fputs("</flushleft>\n\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":605:Ending: flushleft\n");
					}
				else if (flags & RICH_JUST_CENTER)
					{
					turnoff(flags, RICH_JUST_LEFT|RICH_JUST_CENTER|RICH_JUST_RIGHT);
					fputs("</center>\n\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":606:Ending: center\n");
					}
				else if (flags & RICH_JUST_RIGHT)
					{
					turnoff(flags, RICH_JUST_LEFT|RICH_JUST_CENTER|RICH_JUST_RIGHT);
					fputs("</flushright>\n\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":607:Ending: flushright\n");
					}

				switch (*skipspace(lb2+1)) {
				default:
				case 'l':	/* ~Tjl Make subsequent text flush-left */
					turnon(flags, RICH_JUST_LEFT);
					fputs("<flushleft>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":608:Beginning: flushleft\n");
					break;
				case 'c':	/* ~Tjc Center subsequent text */
					turnon(flags, RICH_JUST_CENTER);
					fputs("<center>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":609:Beginning: center\n");
					break;
				case 'r':	/* ~Tjr Make subsequent text flush-right */
					turnon(flags, RICH_JUST_RIGHT);
					fputs("<flushright>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":610:Beginning: flushright\n");
					break;
				}
				break;

			case 'n':	/* ~Tn Force newline (hard line break) */
				fputs("\n\n", part_fp);
				break;

			case 'u':	/* ~Tu Toggle underline mode (turn underline on or off) */
				if (flags & RICH_UNDERLINE)
					{
					turnoff(flags, RICH_UNDERLINE);
					fputs("</underline>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":611:Ending underline\n");
					}
				else
					{
					turnon(flags, RICH_UNDERLINE);
					fputs("<underline>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":612:Beginning underline\n");
					}
				break;

			case 'v':	/* ~Tv Toggle verbatim mode (turn verbatim on or off) */
				if (flags & RICH_VERBATIM)
					{
					turnoff(flags, RICH_VERBATIM);
					fputs("</nofill>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":613:Ending nofill\n");
					}
				else
					{
					turnon(flags, RICH_VERBATIM);
					fputs("<nofill>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":614:Beginning nofill\n");
					}
				break;

			case '>':	/* ~T>,~T>R Indent */
				switch (*skipspace(lb2+1)) {
				default:	/* ~T> Indent Left Margin */
					if (bitson(flags, RICH_INDENT))
						pfmt(stdout, MM_WARNING, ":615:You are already in indent style\n");
					else
						{
						turnon(flags, RICH_INDENT);
						fputs("<indent>\n", part_fp);
						pfmt(stdout, MM_NOSTD, ":616:Beginning indent\n");
						}
					break;
				case 'R':	/* ~T>R Unindent Right Margin */
					if (bitson(flags, RICH_INDENTRIGHT))
						{
						turnoff(flags, RICH_INDENTRIGHT);
						fputs("</indentright>\n", part_fp);
						pfmt(stdout, MM_NOSTD, ":621:Ending indentright\n");
						}
					else
						pfmt(stdout, MM_WARNING, ":622:You are not in indentright style\n");
					break;
				case 's':	/* ~T>s smaller */
					smallness++;
					fputs("<smaller>\n", part_fp);
					break;
				case 'l':	/* ~T>l larger */
					largeness++;
					fputs("<bigger>\n", part_fp);
					break;
				}
				break;

			case '<':	/* ~T<,~T<R Unindent */
				switch (*skipspace(lb2+1)) {
				default:	/* ~T< Unindent Left Margin */
					if (bitson(flags, RICH_INDENT))
						{
						turnoff(flags, RICH_INDENT);
						fputs("</indent>\n", part_fp);
						pfmt(stdout, MM_NOSTD, ":619:Ending indent\n");
						}
					else
						pfmt(stdout, MM_WARNING, ":620:You are not in indent style\n");
					break;
				case 'R':	/* ~T<R Indent Right Margin */
					if (bitson(flags, RICH_INDENTRIGHT))
						pfmt(stdout, MM_WARNING, ":617:You are already in indentright style\n");
					else
						{
						turnon(flags, RICH_INDENTRIGHT);
						fputs("<indentright>\n", part_fp);
						pfmt(stdout, MM_NOSTD, ":618:Beginning indentright\n");
						}
					break;
				case 's':	/* ~T<s unsmaller */
					if (smallness) {
						smallness--;
						fputs("</smaller>\n", part_fp);
					} else {
						pfmt(stdout, MM_WARNING, ":623:Text is not smaller\n");
					}
					break;
				case 'l':	/* ~T<l unlarger */
					if (smallness) {
						largeness--;
						fputs("</bigger>\n", part_fp);
					} else {
						pfmt(stdout, MM_WARNING, ":624:Text is not bigger\n");
					}
					break;
				}
				break;
			case 'Q':	/* ~TQ Toggle quotation (excerpt) mode */
				if (flags & RICH_EXCERPT)
					{
					turnoff(flags, RICH_EXCERPT);
					fputs("</excerpt>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":625:Ending excerpt\n");
					}
				else
					{
					turnon(flags, RICH_EXCERPT);
					fputs("<excerpt>\n", part_fp);
					pfmt(stdout, MM_NOSTD, ":626:Beginning excerpt\n");
					}
				break;
			case 'z':	/* ~Tz Add the contents of ~/.signature as a text/enriched nofill signature */
				fputs("<nofill>\n", part_fp);
				qp_copyfile(expand("$HOME/.signature"), part_fp);
				fputs("</nofill>\n", part_fp);
				break;
			}
			break;

		case '*': {
			char c2 = *skipspace(linebuf+2);
			switch (c2) {
			case '*':	/* ~** Add non-text data (pictures, sounds, etc.) as a new MIME part */
			case 'Z':	/* ~*Z Add the contents of ~/.SIGNATURE as a NON-TEXT (MIME-format) signature. */
				if (!boundary) {
					char *cp;
					if ((cp = value("mime-intro")) != 0) {
						cpout(cp, obuf);
					} else {
						(void) fprintf (obuf, "> THIS IS A MESSAGE IN 'MIME' FORMAT.  Your mail reader does not support MIME.\n");
						(void) fprintf (obuf, "> Some parts of this will be readable as plain text.\n");
						(void) fprintf (obuf, "> To see the rest, you will need to upgrade your mail reader.\n\n");
					}
					boundary = getboundary();
					cp = salloc(strlen(boundary) + 30);
					sprintf(cp, "multipart/mixed; boundary=\"%s\"", boundary);
					(void) fprintf (obuf, "--%s\n", boundary);
					if (!flush_part(&part_fp, obuf, (char*)0, 1, hp->h_content_type, hp->h_content_transfer_encoding, 1))
						return 0;
					hp->h_content_type = cp;
					hp->h_content_transfer_encoding = NOSTR;
				} else {
					if (!flush_part(&part_fp, obuf, (char*)0, 1, part_content_type, part_content_transfer_encoding, 1))
						return 0;
				}

				if (c2 == '*') {
					FILE *fp;

					pfmt(stdout, MM_NOSTD, ":627:If you want to include non-textual data from a file, enter the file name.\n");
					pfmt(stdout, MM_NOSTD, ":628:If not, just press ENTER (RETURN): ");
					fflush(stdout);
					linebuf[0] = '\0';
					getline(linebuf,LINESIZE,stdin,&hasnulls);
					trimnl(linebuf);
					if (linebuf[0] == '\0')
						break;
					fp = fopen(linebuf, "r");
					if (!fp) {
						pfmt(stderr, MM_ERROR, badopen, linebuf, Strerror(errno));
						break;
					}

					pfmt(stdout, MM_NOSTD, ":629:Please choose which kind of data you wish to insert:\n\n");
					pfmt(stdout, MM_NOSTD, ":630:0: A raw file, possibly binary, of no particular data type.\n");
					pfmt(stdout, MM_NOSTD, ":631:1: Raw data from a file, with you specifying the content-type by hand.\n");
					pfmt(stdout, MM_NOSTD, ":632:Enter your choice as a number from 0 to 1: ");
					fflush(stdout);

					if ((nread = getline(linebuf,LINESIZE,stdin,&hasnulls)) == NULL) {
						fclose(fp);
						break;
					} else {
						int ans = atoi(linebuf);
						char *type = 0;
						if ((ans != 0) && (ans != 1)) {
							pfmt(stdout, MM_NOSTD, ":633:Data insertion cancelled\n");
							fclose(fp);
							break;
						}
						(void) fprintf (obuf, "--%s\n", boundary);

						if (ans == 0) {
							type = "application/octet-stream";
						} else {
							for (;;) {
								pfmt(stdout, MM_NOSTD, ":634:Enter the MIME Content-type value for the data from file\n");
								pfmt(stdout, MM_NOSTD, ":635:    (type '?' for a list of locally-valid content-types): ");
								fflush(stdout);
								linebuf[0] = '\0';
								getline(linebuf,LINESIZE,stdin,&hasnulls);
								trimnl(linebuf);
								if (!strchr(linebuf, '/')) {
									pfmt(stdout, MM_NOSTD, ":636:MIME content-type values are type/format pairs, and always include a '/'.\n");
									pfmt(stdout, MM_NOSTD, ":637:The types supported at your site include, but are not limited to:\n");
									system(list_mime_types);
									pfmt(stdout, MM_NOSTD, ":638:The MIME content-type for file inclusion is 'application/octet-stream'.\n");
								} else {
									type = savestr(linebuf);
									break;
								}
							}
						}
						(void) fprintf (obuf, "Content-Type: %s\n", type);
						(void) fprintf (obuf, "Content-Transfer-Encoding: base64\n");

						pfmt(stdout, MM_NOSTD, ":639:Type a description to include with the file, RETURN to skip: ");
						fflush(stdout);
						linebuf[0] = '\0';
						getline(linebuf,LINESIZE,stdin,&hasnulls);
						trimnl(linebuf);
						if (linebuf[0] != '\0')
							(void) fprintf (obuf, "Content-Description: %s\n", linebuf);
						(void) fprintf (obuf, "\n");

#if 0
						encode_file_to_base64(fp, obuf);
#else
						encode_file_to_base64(fp, obuf, type);
#endif
						pfmt(stdout, MM_NOSTD, ":647:Included data in '%s' format\n", type);
						fclose(fp);
						part_content_type = "text/plain";
						part_content_transfer_encoding = "quoted-printable";
						flags = 0;
						turnon(flags, RICH_JUST_LEFT);
					}
				} else {
					const char *SIG = safeexpand("$HOME/.SIGNATURE");
					FILE *tmpin = fopen(SIG, "r");
					if (!tmpin) {
						pfmt(stderr, MM_ERROR, badopen, SIG, Strerror(errno));
					} else {
						(void) fprintf (obuf, "--%s\n", boundary);
						copystream(tmpin, obuf);
						fclose(tmpin);
						part_content_type = "text/plain";
						part_content_transfer_encoding = "quoted-printable";
						flags = 0;
						turnon(flags, RICH_JUST_LEFT);
						fflush(obuf);
						if (ferror(obuf))
							pfmt(stderr, MM_ERROR, badwrite1, Strerror(errno));
						pfmt(stdout, MM_NOSTD, usercontinue);
					}
				}
				break;

			/* set flag on 8-bit, right-to-left */
			case '+':	/* ~*+ Enter 8-bit mode for non-ASCII characters */
				turnon(flags, MIME_8BIT);
				pfmt(stdout, MM_INFO, ":598:Entering text in eight-bit mode\n");
				break;

			case '-':	/* ~*- Leave 8-bit mode (return to ASCII) */
				turnoff(flags, MIME_8BIT);
				pfmt(stdout, MM_INFO, ":595:Entering text in seven-bit (normal) mode\n");
				break;

			case '^':	/* ~*^ Toggle ``Upside-down'' (right-to-left) mode. */
				toggle(flags, MIME_REV);
				if (bitson(flags, MIME_REV))
					pfmt(stdout, MM_INFO, ":596:Entering right-to-left mode\n");
				else
					pfmt(stdout, MM_INFO, ":597:Exiting right-to-left mode\n");
				break;

			case 'S':	/* ~*S Toggle Semitic mode (right-to-left AND eight-bit) */
				toggle(flags, MIME_8BIT);
				toggle(flags, MIME_REV);
				if (bitson(flags, MIME_REV))
					pfmt(stdout, MM_INFO, ":596:Entering right-to-left mode\n");
				else
					pfmt(stdout, MM_INFO, ":597:Exiting right-to-left mode\n");
				if (bitson(flags, MIME_8BIT))
					pfmt(stdout, MM_INFO, ":598:Entering text in eight-bit mode\n");
				else
					pfmt(stdout, MM_INFO, ":595:Entering text in seven-bit (normal) mode\n");
				break;

			default:
				/*
				 * Otherwise, it's an error.
				 */
				pfmt(stdout, MM_ERROR, ":213:Unknown tilde escape.\n");
				break;
			}
			}
		}
		fflush(part_fp);

		setjmp(coljmp);
		sigrelse(SIGINT);
		sigrelse(SIGHUP);
		if (intty && outtty && iprompt)
			fputs(iprompt, stdout);
		flush();

		for (;;) {
			if ((nread = getline(linebuf,LINESIZE,stdin,&hasnulls)) == NULL) {
				if (intty && value("ignoreeof") != NOSTR) {
					if (++eof > 35)
						goto eofl;
					pfmt(stdout, MM_NOSTD,
						":212:Use \".\" to terminate letter\n");
					continue;
				}
				goto eofl;
			}
			eof = 0;
			hadintr = 0;
			if (intty && equal(".\n", linebuf) &&
			    (value("dot") != NOSTR || value("ignoreeof") != NOSTR))
				goto eofl;
			if ((linebuf[0] != escape) || (escape == -1) || (!intty && !escapeokay)) {
				if (bitson(flags, MIME_REV)) rev(linebuf, nread);
				if (bitson(flags, MIME_8BIT)) set8bit(linebuf, nread);
				if (bitson(flags, TEXT_ENRICHED))
					qp_write(linebuf, nread, part_fp, 1, 0);
				else
					qp_write(linebuf, nread, part_fp, 0, 0);
				fflush(part_fp);
				if (ferror(part_fp))
					goto err;
				continue;
			}
			/*
			 * On double escape, just send the single one.
			 */
			if ((nread > 1) && (linebuf[1] == escape)) {
				if (bitson(flags, MIME_REV)) rev(linebuf+1, nread-1);
				if (bitson(flags, MIME_8BIT)) set8bit(linebuf+1, nread-1);
				if (bitson(flags, TEXT_ENRICHED))
					qp_write(linebuf+1, nread-1, part_fp, 1, 0);
				else
					qp_write(linebuf+1, nread-1, part_fp, 0, 0);
				fflush(part_fp);
				if (ferror(part_fp))
					goto err;
				continue;
			}
			break;
		}
		if (hasnulls)
			nread = stripnulls(linebuf, nread);
		linebuf[nread - 1] = '\0';
	}

eofl:
	/*
	 * simulate a ~a/~A autograph and sign the letter.
	 */
	if ((cp = value("autosign")) != 0) {
		if (bitson(flags, TEXT_ENRICHED))
			qp_write(cp, strlen(cp), part_fp, 1, 1);
		else
			qp_write(cp, strlen(cp), part_fp, 0, 1);
		if (isatty(fileno(stdin)))
			cpout( cp, stdout);
	}
	if ((cp = value("autoSign")) != 0) {
		if (bitson(flags, TEXT_ENRICHED))
			qp_write(cp, strlen(cp), part_fp, 1, 1);
		else
			qp_write(cp, strlen(cp), part_fp, 0, 1);
	}

	if ((cp = value("MAILX_TAIL")) != NOSTR) {
		if (bitson(flags, TEXT_ENRICHED))
			qp_write(cp, strlen(cp), part_fp, 1, 1);
		else
			qp_write(cp, strlen(cp), part_fp, 0, 1);
		if (isatty(fileno(stdin)))
			cpout( cp, stdout);
	}

	flush_part(&part_fp, obuf, boundary, boundary?1:0, part_content_type, part_content_transfer_encoding, 0);

	if (boundary)
		(void) fprintf (obuf, "\n--%s--\n", boundary);

	(void) fclose(part_fp);
	return 1;

err:
	if (part_fp) fclose(part_fp);
	return 0;
}

/*
    Show the help file for tilde escapes
*/
static void showthelp(linebuf)
const char *linebuf;
{
	switch (*skipspace(linebuf)) {
	case '+':	/* ~?+  show 8-bit mode mapping */
		if (cascmp("us-ascii", mail_get_charset()) == 0) {
			pfmt(stdout, MM_WARNING, ":640:There are no extended characters available for your US-ASCII terminal.\n");
			pfmt(stdout, MM_NOSTD, ":641:If you are actually using a terminal or terminal emulator with a richer\n");
			pfmt(stdout, MM_NOSTD, ":642:character set, you must set your locale appropriately or use the 'MM_CHARSET' environment\n");
			pfmt(stdout, MM_NOSTD, ":643:variable to inform this program of that fact.\n");
		} else {
			static const char *qwerty[] =
			    { "1234567890-= !@#$%^&*()_+",
			      "qwertyuiop[] QWERTYUIOP{}",
			      "asdfghjkl;'` ASDFGHJKL:\"~",
			      "zxcvbnm,./\\  ZXCVBNM<>?|",
			      0
			    };
			int i;

			pfmt(stdout, MM_NOSTD, ":644:Here is the keyboard map for the character set %s\n", mail_get_charset());
			pfmt(stdout, MM_NOSTD, ":645:Note: If your terminal does not really use this character set, this may look strange\n\n");

			for (i = 0; qwerty[i]; i++) {
				const char *p = qwerty[i];
				for ( ; *p; p++) {
					if (*p != ' ') {
						putchar(*p);
						putchar((*p) | 0200);
					}
					putchar(' ');
				}
				putchar('\n');
			}
		}
		break;

	default:
		(void) pghelpfile(THELPFILE);
		break;
	}
}

/* create and return a string appropriate for use as a MIME boundary marker */
static const char *getboundary()
{
    static long thiscall = 0;
    const char *cluster = mailsystem(0);
    const char *nodename = mailsystem(1);
    const char *domain = maildomain();
    char *msgid = salloc(strlen(cluster) + strlen(nodename) + strlen(domain) + 40);
    msgid[0] = '\0';

    sprintf(msgid, "_%lx.%lx.%lx@", (long)time((long*)0), (long)getpid(), thiscall++);

    if (strcmp(cluster, nodename) == 0) {
	strcat(msgid, nodename);
	strcat(msgid, ".");
	strcat(msgid, domain);
    } else {
	strcat(msgid, nodename);
	strcat(msgid, ".");
	strcat(msgid, cluster);
	strcat(msgid, ".");
	strcat(msgid, domain);
    }
    strcat(msgid, "=_");
    return msgid;
}

/* Create a date string appropriate for a Date: header. */
/* Do it according to RFC 822's standards. */
static char *mkdate()
{
    char *datestring = salloc(30);
    long ltmp;		/* temp variable for time() */
    struct tm	*bp;	/* return value of localtime() */
    const char *tp;	/* return value of asctime() */
    const char *zp;	/* return value of tzname() */

    time(&ltmp);
    bp = localtime(&ltmp);
    tp = asctime(bp);
    zp = tzname[bp->tm_isdst];

    /* asctime: Fri Sep 30 00:00:00 1986\n */
    /*          0123456789012345678901234  */

    /* RFCtime: Fri, 28 Jul 89 10:30 EDT   */

    sprintf(datestring, "%.3s, %.2s %.3s %.4s %.5s %.3s",
	/* Fri */	tp,
	/* 28  */	tp+8,
	/* Jul */	tp+4,
	/* 89  */	tp+20,
	/* 10:30 */	tp+11,
	/* EDT */	zp);
    return datestring;
}
