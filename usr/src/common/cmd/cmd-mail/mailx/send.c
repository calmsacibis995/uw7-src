#ident	"@(#)send.c	11.1"
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

#ident "@(#)send.c	1.27 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef preSVr4
# include <nl_types.h>
# include <langinfo.h>
# undef NOSTR
#endif
#include <regex.h>
#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Mail to others.
 */

static void		fmt ARGS((char *str, FILE *fo));
static FILE		*infix ARGS((struct header *hp, FILE *fi));
/* made non-static so it could be used in another module */
       int		statusput ARGS((struct message *mp, FILE *obuf, int doign, char *mprefix));
static int		clenput ARGS((struct message *mp, FILE *obuf, int doign, char *mprefix));
static int		doforwardmail ARGS((char str[], int subjflag));

static off_t textpos;

/*
 * Send message described by the passed pointer to the
 * passed output buffer.  Return -1 on error, but normally
 * the number of lines written.  Adjust the status: and content-length:
 * fields if need be.  If doign is set, suppress ignored header fields.
 * If prefix is set, insert a tab (or value("mprefix")) in front of each
 * line.
 */
long
sendmsg(mp, obuf, doign, prefix, prheader, mlc)
	register struct message *mp;	/* pointer to message */
	register FILE *obuf;		/* where to write message */
	int doign;			/* check headers for ignore/retain */
	int prefix;			/* print indentprefix or forwardprefix */
	int prheader;			/* print headers */
	int mlc;			/* max number of lines to print */
{
	char line[LINESIZE+1];	/* where message is read into */
	char field[BUFSIZ];	/* temporary storage for name of header field */
	char *linebuf, *lp;
	MAILSTREAM *ibuf;	/* input FILE* */
	long clen;		/* content length from mp->m_clen */
	long n;			/* number characters read */
	long c;			/* size of message from mp->m_size */
	int ishead = 1;		/* inside header */
	int infld = 0;		/* printed a header field */
	int fline = 1;		/* first UNIX From header line */
	int dostat = 1;		/* still need to output Status: header */
	int doclen = 1;		/* still need to output Content-Length: header */
	int nread;		/* number characters read */
	int unused;		/* unused variable */
	char *cp;		/* temporary loop variable */
	char *cp2;		/* temporary loop variable */
	int oldign = 0;		/* previous line was ignored */
	long lc = 0;		/* line count */
	char *mprefix = 0;	/* indentprefix or forwardprefix */

	if (tolower(prefix) == 'm') {
		mprefix = value("indentprefix");
		if (!mprefix) mprefix = value("mprefix");
		if (!mprefix) mprefix = "\t";
	} else if (prefix == 'B') {
		mprefix = value("forwardprefix");
		if (!mprefix) mprefix = "> ";
	}

	ibuf = setinput(mp);
	c = mp->m_size;
	clearerr(obuf);

	lp = linebuf = retrieve_message(mp, 0, 0, 0);
	if (c != strlen(lp))
		c = strlen(lp);
	while (c > 0L && (!mlc || lc < mlc)) {
		nread = getline2(line, LINESIZE, &lp, &unused);
		c -= nread;
		if (ishead) {
			/*
			 * If line is blank, we've reached end of
			 * headers, so force out status: and content-length: fields
			 * and note that we are no longer in header
			 * fields
			 */
			if (line[0] == '\n') {
				ishead = 0;
				if (!prheader)
					continue;
				if (dostat) {
					lc += statusput(mp, obuf, doign, mprefix);
					if (mlc && lc >= mlc)
						break;
					dostat = 0;
				}
				if (doclen) {
					lc += clenput(mp, obuf, doign, mprefix);
					doclen = 0;
				}
				goto writeit;
			}
			/*
			 * If this line is a continuation
			 * of a previous header field, just echo it.
			 */
			if (Isspace(line[0]) && infld)
				if (oldign)
					continue;
				else if (!prheader)
					continue;
				else
					goto writeit;
			infld = 0;

			/*
			 * If we are no longer looking at real
			 * header lines, force out status: and content-length:.
			 * This happens in uucp style mail where
			 * there are no headers at all.
			 */
			if (!headerp(line)) {
				ishead = 0;
				if (prheader) {
					if (dostat) {
						lc += statusput(mp, obuf, doign, mprefix);
						if (mlc && lc >= mlc)
							break;
						dostat = 0;
					}
					if (doclen) {
						lc += clenput(mp, obuf, doign, mprefix);
						doclen = 0;
					}
					putc('\n', obuf);
					lc++;
				}
				goto writeit;
			}
			infld++;

			/*
			 * Pick up the header field.
			 * If it is an ignored field and
			 * we care about such things, skip it.
			 */
			cp = line;
			cp2 = field;
			while (*cp && *cp != ':' && !Isspace(*cp))
				*cp2++ = *cp++;
			*cp2 = 0;
			oldign = doign && isign(field);
			if (oldign)
				continue;
			if (!prheader)
				continue;

			/*
			 * If the field is "status," go compute and print the
			 * real Status: field
			 */
			if (icequal(field, "status")) {
				if (dostat) {
					lc += statusput(mp, obuf, doign, mprefix);
					dostat = 0;
				}
				continue;
			}
			/*
			 * If the field is "content-length," print the
			 * real Content-Length: field
			 */
			if (icequal(field, "content-length")) {
				if (doclen) {
					lc += clenput(mp, obuf, doign, mprefix);
					doclen = 0;
				}
				continue;
			}
		}
writeit:
		/* may have printed a status or content length field */
		if (mlc && lc >= mlc)
			break;
		if (ishead || mprefix) {
			if (mprefix) fputs(mprefix, obuf);
			fwrite(line, 1, nread, obuf);
			lc++;
			if (ferror(obuf)) {
				return(-1);
			}
		} else {
			clen = mp->m_clen-1;
			if (line[0] == '\n') {
				putc('\n', obuf);
				lc++;
			} else {
				if (fwrite(line, 1, nread, obuf) != nread) {
					pfmt(stderr,MM_ERROR,badwrite1, Strerror(errno));
					fflush(obuf);
					if (ferror(obuf)) {
						return (-1);
					}
				}
				clen -= nread;
				lc++;
			}
			while (clen > 0 && (!mlc || lc < mlc)) {
				if (mlc) {
					if (getline2(line, sizeof line, &lp, &unused)==0
					 || (n = strlen(line)) == 0) {
						pfmt(stderr, MM_WARNING, 
								unexpectedEOF);
						clen = 0;
						break;
					} 
					if (line[n-1] == '\n')
						lc++;
				} else {
					n=clen<sizeof line ? clen : sizeof line;
					if ((n=getline2(line, n, &lp, &unused)) <= 0)
					{
						pfmt(stderr, MM_WARNING, 
								unexpectedEOF);
						clen = 0;
						break;
					} 
					lc += linecount(line, n);
				}
				if (fwrite(line, 1, n, obuf) != n) {
					pfmt(stderr,MM_ERROR,badwrite1, 
							Strerror(errno));
					fflush(obuf);
					if (ferror(obuf)) {
						return (-1);
					}
				}
				clen -= n;
			}
			if (mlc && lc >= mlc)
				break;
			if ((mp->m_clen > 1) && prheader) {
				putc('\n', obuf);
				lc++;
			}
			c = 0L;
		}
	}

	/* still in header with no message body */
	if (ishead && dostat && (!mlc || lc < mlc))
		lc += statusput(mp, obuf, doign, mprefix);

	fflush(obuf);
	if (ferror(obuf))
		return(-1);
	return(lc);
}

/*
 * Test if the passed line is a header line, RFC 733 style.
 */
headerp(line)
	register char *line;
{
	register char *cp = line;

	if (*cp=='>' && strncmp(cp+1, "From ", 5)==0)
		return(1);
	if (strncmp(cp, "From ", 5)==0)
		return(1);
	while (*cp && !Isspace(*cp) && *cp != ':')
		cp++;
	while (*cp && Isspace(*cp))
		cp++;
	return(*cp == ':');
}

/*
 * Output a reasonable looking status field.
 * But if "status" is ignored and doign, forget it.
 */
int
statusput(mp, obuf, doign, mprefix)
	register struct message *mp;
	register FILE *obuf;
	char *mprefix;
{
	char statout[3];

	if (doign && isign("status"))
		return 0;
	if ((mp->m_flag & (MNEW|MREAD)) == MNEW)
		return 0;
	if (mprefix)
		fputs(mprefix, obuf);
	if (mp->m_flag & MREAD)
		strcpy(statout, "R");
	else
		strcpy(statout, "");
	if ((mp->m_flag & MNEW) == 0)
		strcat(statout, "O");
	fprintf(obuf, "Status: %s\n", statout);
	return 1;
}

/*
 * Output a reasonable looking Content-Length field.
 */
static int
clenput(mp, obuf, doign, mprefix)
	register struct message *mp;
	FILE *obuf;
	char *mprefix;
{
	if (doign && isign("content-length"))
		return 0;
	if (mp->m_clen <= 1)
		return 0;
	if (mprefix)
		fputs(mprefix, obuf);
	fprintf(obuf, "Content-Length: %ld\n", mp->m_clen);
	return 1;
}

/*
 * Interface between the argument list and the mail1 routine
 * which does all the dirty work.
 */

mail(people)
	char **people;
{
	char **ap;
	struct header head;
	char recfile[128];

	head.h_to = NOSTR;
	for (ap = people; *ap; ap++)
		head.h_to = addto(head.h_to, *ap);
	strncpy(recfile, head.h_to ? head.h_to : "", sizeof recfile);
	head.h_subject = head.h_cc = head.h_bcc = head.h_defopt = head.h_encodingtype =
		head.h_content_type = head.h_content_transfer_encoding = head.h_mime_version =
		head.h_date = NOSTR;
	head.h_others = NOSTRPTR;
	head.h_seq = 0;
	mail1(&head, Fflag ? recfile : (char*)0, (int*)0);
	return(0);
}

/*
 * Interface between the -t option and the mail1 routine,
 * which does all the dirty work.
 */

mailt()
{
	struct header head;
	char recfile[128];

	head.h_to = head.h_subject = head.h_cc = head.h_bcc = head.h_defopt = head.h_encodingtype =
		head.h_content_type = head.h_content_transfer_encoding = head.h_mime_version =
		head.h_date = NOSTR;
	head.h_others = NOSTRPTR;
	head.h_seq = 0;
	mail1(&head, Fflag ? recfile : (char*)0, (int*)0);
	return(0);
}

sendm(str)
char *str;
{
	if (value("flipm") != NOSTR)
		return(Sendmail(str));
	else return(sendmail(str));
}

Sendm(str)
char *str;
{
	if (value("flipm") != NOSTR)
		return(sendmail(str));
	else return(Sendmail(str));
}

/*
 * Send mail to a bunch of user names.  The interface is through
 * the mail routine below.
 */
int
sendmail(str)
	char *str;
{
	struct header head;

	if (blankline(str))
		head.h_to = NOSTR;
	else
		head.h_to = addto(NOSTR, str);
	head.h_subject = head.h_cc = head.h_bcc = head.h_defopt = head.h_encodingtype =
		head.h_content_type = head.h_content_transfer_encoding = head.h_mime_version =
		head.h_date = NOSTR;
	head.h_others = NOSTRPTR;
	head.h_seq = 0;
	mail1(&head, (char*)0, (int*)0);
	return(0);
}

/*
 * Send mail to a bunch of user names.  The interface is through
 * the mail routine below.
 * save a copy of the letter
 */
int
Sendmail(str)
	char *str;
{
	char recfile[128];
	struct header head;

	if (blankline(str))
		head.h_to = NOSTR;
	else
		head.h_to = addto(NOSTR, str);
	strncpy(recfile, head.h_to ? head.h_to : "", sizeof recfile);
	head.h_subject = head.h_cc = head.h_bcc = head.h_defopt = head.h_encodingtype =
		head.h_content_type = head.h_content_transfer_encoding = head.h_mime_version =
		head.h_date = NOSTR;
	head.h_others = NOSTRPTR;
	head.h_seq = 0;
	mail1(&head, recfile, (int*)0);
	return(0);
}

/*
	forward [messages] [names]

	Forward "messages" (default current-message) mail to
	"names" (default prompts). Set subject based on first
	mail message in the list.
*/
int
forwardmail(str)
	char str[];
{
	return doforwardmail(str, 1);
}

/*
	Forward [messages] [names]

	Forward "messages" (default current-message) mail to
	"names" (default prompts). Prompt for subject.
*/
int
Forwardmail(str)
	char str[];
{
	return doforwardmail(str, 0);
}

/*
	forward and Forward commands
*/
static int
doforwardmail(str, subjflag)
	char str[];
	int subjflag;
{
	int *msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	int f;
	struct header head;
	char *names = snarf2(str, &f);

	/* copy name list */
	if (!names)
		head.h_to = NOSTR;
	else {
		const char *p;
		char *q;
		unsigned lencount = 0;
		for (p = names; *p; p++)
			if (Isspace(*p))
				lencount += 2;
			else
				lencount++;
		head.h_to = salloc(lencount + 1);
		for (p = names, q = head.h_to; *p; )
			if (Isspace(*p)) {
				*q++ = ',';
				*q++ = ' ';
				p = skipspace(p);
			} else if (*p == ',') {
				p = skipspace(p + 1);
			} else
				*q++ = *p++;
		*q = '\0';
	}

	/* parse message list */
	if (f) {
		if (getmsglist(str, msgvec, 0) < 0)
			return(1);
	} else {
		msgvec[0] = first(0, MMNORM);
		if (*msgvec == NULL) {
			pfmt(stderr, MM_ERROR, ":497:No messages to forward.\n");
			return(1);
		}
		msgvec[1] = NULL;
	}

	/* set rest of header */
	head.h_cc = head.h_bcc = head.h_defopt = head.h_encodingtype =
		head.h_content_type = head.h_content_transfer_encoding = head.h_mime_version =
		head.h_date = NOSTR;
	head.h_others = NOSTRPTR;
	head.h_seq = 0;
	if (subjflag) {
		struct message *mp = &message[msgvec[0] - 1];
		char *subject = hfield("subject", mp, addone);
		if (!subject)
			subject = hfield("subj", mp, addone);
		head.h_subject = reedit(subject);
	} else {
		head.h_subject = NOSTR;
	}

	mail1(&head, (char*)0, msgvec);
	return(0);
}

/*
 * Mail a message on standard input to the people indicated
 * in the passed header.  (Internal interface).
 */
void
mail1(hp, rec, forwardvec)
	struct header *hp;
	char *rec;
	int *forwardvec;
{
	pid_t p, pid;
	int filecmdcnt, i, s, gotcha;
	char **namelist, *deliver;
	struct name *to, *np;
	FILE *mtf;
	char **t;
	char recfile[PATHSIZE];
	char *askme;

	/*
	 * Collect user's mail from standard input.
	 * Get the result as mtf.
	 */

	pid = (pid_t)-1;
	if ((mtf = collect(hp, forwardvec)) == NULL)
		return;
	hp->h_seq = 1;
	if (hp->h_subject == NOSTR)
		hp->h_subject = sflag;
	if (fsize(mtf) == 0 && hp->h_subject == NOSTR) {
		pfmt(stdout, MM_WARNING, ":328:No message !?!\n");
		fclose(mtf);
		return;
	}
	if (intty) {
		pfmt(stdout, MM_NOSTD, ":329:EOT\n");
		flush();
	}

	/*
	 * Now, take the user names from the combined
	 * to and cc lists and do all the alias
	 * processing.
	 */

	senderr = 0;
	to = cat(extract(hp->h_to, GTO),
		 cat(extract(hp->h_cc, GCC),
		     extract(hp->h_bcc, GBCC)));
	to = elide(outpre(translate(outpre(elide(usermap(to))))));
	if (!senderr)
		mapf(to, myname);
	mechk(to);
	for (gotcha = 0, np = to; np; np = np->n_link)
		if ((np->n_type & GDEL) == 0)
			gotcha++;
	hp->h_to = detract(to, GTO);
	hp->h_cc = detract(to, GCC);
	hp->h_bcc = detract(to, GBCC);
	if ((mtf = infix(hp, mtf)) == NULL) {
		pfmt(stderr, MM_WARNING, ":330:. . . message lost, sorry.\n");
		return;
	}
	rewind(mtf);
	askme = value("askme");
	if (askme && (casncmp(askme, "yes", strlen(askme)) == 0) && isatty(0)) {
		char ans[128];
		regex_t re;
#ifdef preSVr4
		const char *yesexpr = "^(Y|y)";
#else
		const char *yesexpr = nl_langinfo(YESEXPR);
#endif
		puthead(hp, stdout, GTO|GCC|GBCC, (FILE*)0, 1);
		pfmt(stdout, MM_NOSTD, ":331:Send? [yes] ");
		clearerr(stdin);
		if (fgets(ans, sizeof ans, stdin)) {
			const char *ap;
			int len;
			trimnl(ans);
			ap = skipspace(ans);
			len = strlen(ap);
			if (len > 0) {
				regcomp(&re, yesexpr, REG_EXTENDED|REG_NOSUB);
				if (regexec(&re, ap, 0, NULL, 0) != 0)
					senderr++;
				regfree(&re);
			}
		}
	}
	if (senderr)
		goto dead;
	/*
	 * Look through the recipient list for names with /'s
	 * in them which we write to as files directly.
	 */
	filecmdcnt = outof(to, hp, mtf, textpos);
	if (!gotcha && !filecmdcnt) {
		pfmt(stdout, MM_ERROR, ":332:No recipients specified\n");
		goto dead;
	}
	if (senderr)
		goto dead;

	getrecf(rec, recfile, !!rec);
	if (*recfile)
		savemail(safeexpand(recfile), 0, hp, mtf, textpos, 0, 1);
	if (!gotcha) {
		fclose(mtf);
		return;
	}
	namelist = unpack(to);
	if (debug) {
		for (t = namelist; *t != NOSTR; t++)
			fprintf(stderr, " \"%s\"", *t);
		fprintf(stderr, "\n");
		return;
	}

	/*
	 * Wait, to absorb a potential zombie, then
	 * fork, set up the temporary mail file as standard
	 * input for "mail" and exec with the user list we generated
	 * far above. Return the process id to caller in case it
	 * wants to await the completion of mail.
	 */

	wait(&s);
	rewind(mtf);
	pid = fork();
	if (pid == (pid_t)-1) {
		pfmt(stderr, MM_ERROR, failed, "fork", Strerror(errno));
		goto dead;
	}
	if (pid == 0) {
		sigchild();
#ifdef SIGTSTP
		sigset(SIGTSTP, SIG_IGN);
		sigset(SIGTTIN, SIG_IGN);
		sigset(SIGTTOU, SIG_IGN);
#endif
		signal(SIGHUP,SIG_IGN);
		signal(SIGINT,SIG_IGN);
		signal(SIGQUIT,SIG_IGN);
#ifdef preSVr4	/* block all tty generated signals from affecting our mailer */
		setpgrp();
#else
		setsid();
#endif
		s = fileno(mtf);
		for (i = 3; i < 32; i++)
			if (i != s)
				close(i);
		close(0);
		dup(s);
		close(s);
		if ((deliver = value("sendmail")) == NOSTR)
			deliver = MAIL;
		execvp(safeexpand(deliver), namelist);
		pfmt(stderr, MM_ERROR, errmsg, deliver, Strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	if (value("sendwait")) {
		while ((p = wait(&s)) != pid && p != (pid_t)-1)
			;
		if (s != 0)
			senderr++;
	}
	fclose(mtf);
	return;

	/* error return */
dead:
	savemail(Getf("DEAD"), DEADPERM, hp, mtf, textpos, 1, !value("posix2"));
	fclose(mtf);
	return;
}

/*
 * Prepend a header in front of the collected stuff
 * and return the new file.
 */

static FILE *
infix(hp, fi)
	struct header *hp;
	FILE *fi;
{
	register FILE *nfo, *nfi;

	rewind(fi);
	if ((nfo = fopen(tempMail, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, Strerror(errno));
		return(fi);
	}
	if ((nfi = fopen(tempMail, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempMail, Strerror(errno));
		fclose(nfo);
		return(fi);
	}
	removefile(tempMail);
	puthead(hp, nfo, GFULL & ~(GUFROM|GBCC), fi, 1);
	textpos = ftell(nfo);
	copystream(fi, nfo);
	if (ferror(fi)) {
		pfmt(stderr, MM_ERROR, badread1, Strerror(errno));
		return(fi);
	}
	fflush(nfo);
	if (ferror(nfo)) {
		pfmt(stderr, MM_ERROR, badwrite, tempMail, Strerror(errno));
		fclose(nfo);
		fclose(nfi);
		return(fi);
	}
	fclose(nfo);
	fclose(fi);
	rewind(nfi);
	return(nfi);
}

/*
 * Output one header line.
 */
static int
put1(fo, str, flag, dofmt, hdrname, print)
	FILE *fo;
	char *str, *hdrname;
	int flag, dofmt, print;
{
	if (flag && str && *str) {
		if (print) {
			fprintf(fo, "%s: ", hdrname);
			if (dofmt)
				fmt(str, fo);
			else
				fputs(str, fo);
			putc('\n', fo);
		}
		return 1;
	}
	return 0;
}

/*
 * Dump the message header on the
 * passed file buffer.
 *
 * If print is not set, just return the number of lines in the header.
 */

puthead(hp, fo, w, fi, print)
	struct header *hp;
	FILE *fo;
	FILE *fi;
	int print;
{
	register int gotcha;
	char *postmark;

	/* always a mime message now */
	if (hp->h_mime_version == 0) {
		hp->h_mime_version = strdup("1.0");
		w |= GMIMEV;
	}
	if (hp->h_content_type == 0) {
		hp->h_content_type = malloc(200);
		sprintf(hp->h_content_type, "text/plain; charset=%s", mail_get_charset());
		w |= GCTYPE;
	}
	gotcha = 0;
	if (w & GUFROM) {
		if (print) {
			time_t now = time((time_t *) 0);
			fprintf(fo, "From %s %s", myname, ctime(&now));
		}
		gotcha++;
	}
	if ((w & GFROM) && (((postmark = value("postmark")) != 0) || value("from"))) {
		if (print) {
			fputs("From: ", fo);
			/* 
			 * if postmark has a value with an @ in it, use that 
			 * for From: 
			 */
			if (postmark && any('@', postmark)) {
				fputs(postmark, fo);
			} else {
				fprintf(fo,"%s@%s%s",myname,host, maildomain());
				if (postmark && *postmark)
					fprintf(fo, " (%s)", postmark);
			}
			putc('\n', fo);
		}
		gotcha++;
	}
	gotcha += put1(fo, hp->h_to, w & GTO, 1, "To",print);
	gotcha += put1(fo, hp->h_cc, w & GCC, 1, "Cc",print);
	gotcha += put1(fo, hp->h_bcc, w & GBCC, 1, "Bcc",print);
	gotcha += put1(fo, hp->h_date, w & GDATE, 1, "Date",print);
	gotcha += put1(fo, hp->h_defopt, w & GDEFOPT, 0, "Default-Options",print);
	gotcha += put1(fo, hp->h_encodingtype, w & GENCTYP, 0, "Encoding-Type",print);
	gotcha += put1(fo, hp->h_subject, w & GSUBJECT, 0, "Subject",print);
	gotcha += put1(fo, hp->h_mime_version, w & GMIMEV, 0, "Mime-Version",print);
	gotcha += put1(fo, hp->h_content_type, w & GCTYPE, 0, "Content-Type",print);
	gotcha += put1(fo, hp->h_content_transfer_encoding, w & GCTE, 0, "Content-Transfer-Encoding",print);
	if (hp->h_others && (w & GOTHER)) {
		char **p;
		for (p = hp->h_others; *p; p++) {
			if (print)
				fprintf(fo, "%s\n", *p);
			gotcha++;
		}
	}
	if (fi && (w & GCLEN)) {
		if (print)
			fprintf(fo,"Content-Length: %ld\n",fsize(fi)-ftell(fi));
		gotcha++;
	}
	if (gotcha && (w & GNL)) {
		if (print)
			putc('\n', fo);
		gotcha++;
	}
	return(gotcha);
}

/*
 * Format the given text to not exceed 78 characters.
 */
static void
fmt(str, fo)
	register char *str;
	register FILE *fo;
{
	register int col = 4;
	char username[256];
	int len;

	str = strcpy(salloc((unsigned)(strlen(str)+1)), str);
	while ((str = yankword(str, username, 1)) != 0) {
		len = strlen(username);
		if (col > 4) {
			if (col + len > 76) {
				fputs(",\n    ", fo);
				col = 4;
			} else {
				fputs(", ", fo);
				col += 2;
			}
		}
		fputs(username, fo);
		col += len;
	}
}

/*
 * Save the outgoing mail on the passed file.
 */
int
savemail(filename, perm, hp, fi, pos, verbose, append)
	const char filename[];
	int perm;
	struct header *hp;
	FILE *fi;
	off_t pos;
	int verbose;
	int append;
{
	register FILE *fo;
	int ret = 0;

	if (debug)
		fprintf(stderr, "save in '%s'\n", filename);
	if ((fo = fopen(filename, append ? "a" : "w" )) == NULL) {
		pfmt(stderr, MM_ERROR, errmsg, filename, Strerror(errno));
		return(-1);
	}
	if (perm)
		chmod(filename, perm);
	fseek(fi, pos, 0);
	puthead(hp, fo, GFULL, fi, 1);
	if (verbose)
		lcwrite(filename, fi, fo);
	else
		copystream(fi, fo);
	putc('\n', fo);
	fflush(fo);
	if (ferror(fo)) {
		pfmt(stderr, MM_ERROR, errmsg, filename, Strerror(errno));
		ret = -1;
	}
	fclose(fo);
	return ret;
}
