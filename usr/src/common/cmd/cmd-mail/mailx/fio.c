#ident	"@(#)fio.c	11.3"
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

#ident "@(#)fio.c	1.19 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * File I/O.
 */

#if 0
extern char *maildir;
#endif

static char Unknown[] = "Unknown";

static char *getencdlocale ARGS((const char *line));

/*
 * Set up the input pointers while copying the mail file into
 * /tmp.
 */

void
setptr(ibuf)
	MAILSTREAM *ibuf;
{
	int n;				/* size of input line */
	int newline = 1;		/* previous header had a newline */
	int StartNewMsg = TRUE;
	int ToldUser = FALSE;
	int ctf = FALSE; 		/* header continuation flag */
	long clen = 0L;			/* value of content-length header */
	int hdr = 0;			/* type of header */
	int prevhdr = 0;		/* previous header */
	int cflg = 0;			/* found Content-length in header */
	register int l;
	register long s;
	int inhead = 1, Odot;
	short flag;
	char *cp;
	STRINGLIST *mime;		/* Mime-Version */
	STRINGLIST *cont;		/* Content-Type */
	STRINGLIST *enc;		/* Content-Transfer-Encoding */

	if ( space == 0 ) {
		msgCount = 0;
		space = ibuf->nmsgs + 1;
		message =
		    (struct message *)malloc(space * sizeof(struct message));
		if ( message == NULL ) {
			pfmt(stderr, MM_ERROR,
				":226:Not enough memory for %d messages: %s\n",
				space, Strerror(errno));
			exit(1);
			/* NOTREACHED */
		}
		memset(message, '\0', space * sizeof(struct message));
		message[0].m_text = M_text;
		message[0].m_encoding_type = Unknown;
		message[0].m_content_type = 0;
		message[0].m_mime_version = 0;
		/* message[0].m_clen = -1; */
		dot = message;
	}
	s = 0L;
	l = 0;
	flag = 0;
        if (msgCount < 0) {
                /* no work is done to save state */
                mail_close(ibuf);
                mm_fatal("msgCount went to 0 while adding new mail");
        }

	mime = mail_newstringlist();
	cont = mail_newstringlist();
	enc = mail_newstringlist();
	mime->text = strdup("Mime-Version");
	mime->size = strlen(mime->text);
	cont->text = strdup("Content-Type");
	cont->size = strlen(cont->text);
	enc->text = strdup("Content-Transfer-Encoding");
	enc->size = strlen(enc->text);

	while (msgCount < ibuf->nmsgs)
	{
                memset(message + msgCount, 0, sizeof(struct message));
                message[msgCount].m_text = M_text;
                message[msgCount].m_encoding_type = Unknown;
		cp = mail_fetchheader_full(ibuf, msgCount+1, mime, 0, 0);
		if (cp) {
			trimcrlf(cp);
			if (*cp)
		                message[msgCount].m_mime_version = 1;
		}
		cp = mail_fetchheader_full(ibuf, msgCount+1, cont, 0, 0);
		if (cp) {
			trimcrlf(cp);
			if (*cp)
		                message[msgCount].m_content_type = strdup(skipspace(cp + cont->size + 1));
		}
		cp = mail_fetchheader_full(ibuf, msgCount+1, enc, 0, 0);
		if (cp) {
			trimcrlf(cp);
			if (*cp)
		                message[msgCount].m_encoding_type = strdup(skipspace(cp + enc->size + 1));
		}
		setflags(message + msgCount);
		msgCount++;
	}

	mail_free_stringlist(&cont);
	mail_free_stringlist(&mime);
	mail_free_stringlist(&enc);

	/* last plus 1, why is this here? */
	message[msgCount].m_text = M_text;
	message[msgCount].m_encoding_type = Unknown;
	message[msgCount].m_content_type = 0;
	message[msgCount].m_mime_version = 0;
	message[msgCount].m_flag = 0;
}

/*  HMF:  Code from fio.c. (getln)                                         */

int
getln(line, max, f)
	char *line;
	int max;
	FILE	*f;
{
	int	i,ch;
	for (i=0; i < max-1 && (ch=getc(f)) != EOF;)
		if ((line[i++] = (char)ch) == '\n') break;
	line[i] = '\0';
	return(i);
}

/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 */

readline(ibuf, linebuf)
	FILE *ibuf;
	char *linebuf;
{
	register char *cp;
	register int c;
	extern int DelHit;

	do {
		if(DelHit) {
			DelHit = 0;
		} else {
			clearerr(ibuf);
			}

		c = getc(ibuf);
		for (cp = linebuf; c != '\n' && c != EOF; c = getc(ibuf)) {
			if (c == 0) {
				pfmt(stderr, MM_WARNING, ":229:NUL changed to @\n");
				c = '@';
			}
			if (cp - linebuf < LINESIZE-2)
				*cp++ = (char)c;
		}
	} while (ferror(ibuf) && ibuf == stdin);
	*cp = 0;
	if (c == EOF && cp == linebuf)
		return(0);
	return(cp - linebuf + 1);
}

/*
    NAME
	getencdlocale - get the locale information from Encoding-Type: header

    SYNOPSIS
	char *getencdlocale(const char *line)

    DESCRIPTION
	Getencdlocale looks through the header for the locale information,
	such as "/locale=french". Just the "french" part is returned. Other
	/xyz fields may also be present, all prefaced with a "/".
*/

static char *getencdlocale(line)
const char *line;
{
    /* look for strings starting with "/" */
    for (line = skipspace(line); line && *line; line = strchr(line, '/'))
	/* Did we find "/locale="? */
	if (casncmp(line, "/locale=", 8) == 0) {
	    /* find the end of the locale name and copy it */
	    int localelen = strcspn(line+8, " \t\n,/");
	    char *duparea = malloc(localelen + 1);
	    nomemcheck(duparea);
	    strncpy(duparea, line+8, localelen);
	    duparea[localelen] = '\0';
	    return duparea;
	}

    return Unknown;
}

/*
 * Added to help processes C-Client string values. The mail_fetchtext_full()
 * and mail_fetchheader_full routines can return strings that have the
 * CR/LF character pairs. This routine matches the sequence \015\012 and
 * replaces it with \012. The remainder of the string is copied back to
 * fill the gap, ultimately ending with a NULL byte.
 * the routine returns the number of bytes in the new (shorter) string.
 */
int
strip_crlf_pairs(p)
register char *p;
{
	register char *q = p;
	register n = 0;

	for(;;) {
		*q = *p;
		if (*p == '\0')
			break;
		p++;
		/* if \015 not followed by '\n', leave it */
		if (*q == '\015' && *p == '\n')
			continue;
		q++;
		n++;
	}
	return n;
}

/*
 * Added to help processes C-Client string values. The mail_fetchtext_full()
 * and mail_fetchheader_full routines can return strings that have the
 * CR/LF character pairs. This routine matches the sequence \015\012 and
 * replaces it with \012. The remainder of the string is copied back to
 * fill the gap, ultimately ending with a NULL byte.
 *
 * Header lines can be split across multiple lines. Additional lines are
 * denoted by white space at the first position of the line. These lines
 * are merged with the previous lines in this routine.
 *
 * it is up to the calling routine to pass in the part of the message
 * which should have lines mreged. Passing in the body of the message
 * will have undesireable effects.
 */
void
strip_and_merge(p)
register char *p;
{
	register char *q = p;
	for(;;) {
		*q = *p;
		if (*p == '\0')
			break;
		p++;
		if (*q == '\015')
			continue;
		if (*q == '\n' && Isspace(*p)) {
			while (Isspace(*p))
				p++;
			if (*p == '\n' || *p == '\0') {
				*(++q) = '\0';
				break;
			}
			*q++ = ' ';
			continue;
		}
		q++;
	}
}

/*
 * retrieve_message -
 *	get the text of the message based on the
 *	values of arguments part and lines.
 *
 *	part describes the part of the message to
 *	retrieve. These values have these significance:
 *		0 - entire message
 *		1 - header only (includes trailing blank line)
 *		2 - body only
 *
 *	lines describes the number of lines to return. If the
 *	value is 0, return all lines, otherwise return the
 *	number of lines indicated from the parts specified.
 */
char *
retrieve_message(mp, slist, part, lines)
	register struct message *mp;
	STRINGLIST *slist;
{
	unsigned long c = 0, mesg = mp - &message[0] + 1;
	register char *cp;
	char *s = (char *)0;
	MAILSTREAM *ibuf;
	MESSAGECACHE *elt;

	/* make sure the message is loaded */
	ibuf = setinput(mp);

	/* mp->m_size = mp->m_lines = mp->m_clen = 0; */
	if (part == 0 || part == 1) {
		/* get a pointer to the header text in c-client cache */
		cp = mail_fetchheader_full(ibuf, mesg, slist, &c, FT_INTERNAL);

		/* fake a "From " line, c-client does not provide one */
		if (slist == 0) {
			BODY *body;
			ENVELOPE *env;
			MESSAGECACHE *elt;
			int l;
			char hn[81];

			env = mail_fetchstructure_full(ibuf, mesg, &body, 0);
			elt = mail_elt(ibuf, mesg);
			l = strlen(env->sender->mailbox)
				+ strlen(env->sender->host) + 64 + c;
			s = (char *)salloc(l);
			hn[0] = 0;
			gethostname(hn, 80);
			if (strcmp(env->sender->host, hn))
				sprintf(s, "From %s@%s ", env->sender->mailbox,
					env->sender->host);
			else
				sprintf(s, "From %s ", env->sender->mailbox);
			mail_cdate(s + strlen(s), elt);
			l = strlen(s);
			memcpy(s + l, cp, c);
			c += l;
			s[c] = 0;
		}
		else {
			s = (char *)salloc(c + 1);
			memset(s, '\0', c + 1);
			memcpy(s, cp, c); /* -1 for newline */
		}

		
		s[c] = '\0';
		c = strip_crlf_pairs(s);
		c = linecount(s, c);
	}

	if ((c < lines || lines == 0) && (part == 0 || part == 2)) {
		/* get a pointer to the body text in c-client cache */
		cp = mail_fetchtext_full(ibuf, mesg, &c, FT_PEEK);
		if (s) {
			int i = c + strlen(s);
			s = (char *)srealloc(s, i + 1);
			memcpy(&s[strlen(s)], cp, c);
			s[i] = '\0';
			/* c = linecount(s, i); */
		}
		else {
			s = (char *)salloc(c + 1);
			memset(s, '\0', c + 1);
			memcpy(s, cp, c);
			s[c] = '\0';
			/* c = linecount(s, c); */
		}
		/* mp->m_size = */ c = strip_crlf_pairs(s);
		/* mp->m_lines = */ c = linecount(s, c);
		/* mp->m_clen = strlen(&s[c]); */
		/* c = mp->m_lines; */
	}

	cp = s;

	if (c > lines && lines != 0) {
	    for (c = 0; *cp && c < lines; c++) {
		cp = strchr(cp, '\n');
		if (cp)
			cp++;
	    }

	    if (cp != s)
		    *cp = '\0';
	}

	/* strip CTRL-M characters that are part of the CR/LF pair */
	strip_crlf_pairs(s);

	return s;
}

VOID
setflags(mp)
	register struct message *mp;
{
	char seq[256];
	unsigned long mesg;

	/* make sure the message is loaded into c-client cache */
	MESSAGECACHE *elt;
	
	if ((mp->m_flag & MUSED) == 0) {
		int flag = MUSED|MNEW;

		mesg = (mp - &message[0]) + 1;
		sprintf(seq, "%d", mesg);
		mail_fetchflags(itf, seq);
		elt = mail_elt(itf, mesg);

		if (!elt->recent)
			flag &= ~MNEW;
		if (elt->seen)
			flag |= MREAD;
		if (elt->deleted)
			flag |= MDELETED;
		mp->m_flag = flag;
	}
}

/*
 * Return a file buffer all ready to read up the
 * passed message pointer.
 */

MAILSTREAM *
setinput(mp)
	register struct message *mp;
{
	char seq[256];
	unsigned long mesg;

	/* checkpoint the message store */
	/* mail_check(itf); */

	/* make sure the message is loaded into c-client cache */
	if (mp) {
		BODY *body;
		MESSAGECACHE *elt;
		unsigned long hlen, tlen;
		char *tp;
		
		mesg = (mp - &message[0]) + 1;
		sprintf(seq, "%d", mesg);
		mail_fetchstructure_full(itf, mesg, &body, 0);
		tp = mail_fetchheader_full(itf, mesg, 0, &hlen, FT_INTERNAL);
		mp->m_size = hlen + body->size.bytes - body->size.lines;
		mp->m_lines = linecount(tp, hlen) + 1 + body->size.lines;
		mp->m_clen = body->size.bytes - body->size.lines;
		tp = mail_fetchtext_full(itf, mesg, &tlen, FT_PEEK);
		mp->m_text = istext((unsigned char *)tp, tlen, M_text);
		elt = mail_elt(itf, mesg);
		if ((mp->m_flag & MUSED) == 0) {
			int flag = MUSED|MNEW;

			if (!elt->recent)
				flag &= ~MNEW;
			if (elt->seen)
				flag |= MREAD;
			if (elt->deleted)
				flag |= MDELETED;
			mp->m_flag = flag;
		}
	}
	return(itf);
}


/*
 * Delete a file, but only if the file is a plain file.
 */

removefile(filename)
	const char filename[];
{
	struct stat statb;

	if (stat(filename, &statb) < 0)
		return(-1);
	if ((statb.st_mode & S_IFMT) != S_IFREG) {
		errno = EISDIR;
		return(-1);
	}
	return(unlink(filename));
}

/*
 * Terminate an editing session by attempting to write out the user's
 * file from the temporary.  Save any new stuff appended to the file.
 */
int
edstop()
{
	register int gotcha;
	register struct message *mp;
	MAILSTREAM *ibuf;
	FILE *readstat;
	char *id;


	if (readonly) {
		return(0);
	}
	holdsigs();
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}

	for (mp = &message[0], gotcha = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MODIFY|MDELETED|MSTATUS))
			gotcha++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			if ((id = hfield("article-id", mp, addone)) != NOSTR)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (!gotcha || Tflag != NOSTR)
		goto done;
	ibuf = setinput(&message[0]);
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		char seq[25];

		sprintf(seq, "%d", mp - &message[0] + 1);
		if (mp->m_flag & MREAD) {
			mail_setflag(ibuf, seq, "\\SEEN");
		}
		if ((mp->m_flag & MDELETED) != 0) {
			mail_setflag(ibuf, seq, "\\DELETED");
			continue;
		}
	}
	/* what about removing empty mailfolders? */
	mail_expunge(ibuf);
	if (ibuf->nmsgs == 0 && !value("keep")) {
		mail_delete(ibuf, (char *)editfile);
		pfmt(stdout, MM_NOSTD, ":231:\"%s\" removed.\n", editfile);
	} else
		pfmt(stdout, MM_NOSTD, ":232:\"%s\" updated.\n", editfile);
	/* mail_close(ibuf); */
	
done:
	relsesigs();
	return(1);
}

/*
 * Hold signals SIGHUP - SIGQUIT.
 */
void
holdsigs()
{
	sighold(SIGHUP);
	sighold(SIGINT);
	sighold(SIGQUIT);
}

/*
 * Release signals SIGHUP - SIGQUIT
 */
void
relsesigs()
{
	sigrelse(SIGHUP);
	sigrelse(SIGINT);
	sigrelse(SIGQUIT);
}

/*
 * Flush the standard output.
 */

void
flush()
{
	fflush(stdout);
	fflush(stderr);
}

/*
 * Determine the size of the file possessed by
 * the passed buffer.
 */

off_t
fsize(iob)
	FILE *iob;
{
	register int f;
	struct stat sbuf;

	f = fileno(iob);
	if (fstat(f, &sbuf) < 0)
		return(0);
	return(sbuf.st_size);
}

/*
 * Take a file name, possibly with shell meta characters
 * in it and expand it by using "sh -c echo filename"
 * Return the file name as a dynamic string.
 * If the name cannot be expanded (for whatever reason)
 * return NULL.
 */

const char *
expand(filename)
	const char filename[];
{
	char xname[BUFSIZ];
	char cmdbuf[BUFSIZ];
	register pid_t pid;
	register int l;
	register const char *cp;
	register char *cp2;
	int s, pivec[2];
	struct stat sbuf;
	char *Shell;

	if (debug) fprintf(stderr, "expand(%s)=", filename);
	/* 
	 * If folder is not set, POSIX says to not remove the +.
	 */
	if (filename[0] == '+' && (!value("posix2") ||
		((cp2=value("folder")) != NOSTR && !equal(cp2,"")))) {

		cp = ++filename;
		if (getfold(cmdbuf) >= 0) {
			sprintf(xname, "%s/%s", cmdbuf, cp);
			cp = xname;
		}
		cp = safeexpand(cp);
		if (debug) fprintf(stderr, "%s\n", cp);
		return cp;
	}
	if (!anyof(filename, "~{[*?$`'\"\\ \t")) {
		if (debug) fprintf(stderr, "%s\n", filename);
		return(savestr(filename));
	}
	if (pipe(pivec) < 0) {
		pfmt(stderr, MM_ERROR, failed, "pipe", Strerror(errno));
		return(savestr(filename));
	}
	sprintf(cmdbuf, "echo %s", filename);
	if ((pid = vfork()) == 0) {
		sigchild();
		close(pivec[0]);
		close(1);
		dup(pivec[1]);
		close(pivec[1]);
		if ((Shell = value("SHELL")) == NOSTR || *Shell=='\0')
			Shell = SHELL;
		execlp(Shell, Shell, "-c", cmdbuf, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, cmdbuf, Strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	if (pid == (pid_t)-1) {
		pfmt(stderr, MM_ERROR, failed, "fork", Strerror(errno));
		close(pivec[0]);
		close(pivec[1]);
		return(NOSTR);
	}
	close(pivec[1]);
	l = read(pivec[0], xname, BUFSIZ);
	close(pivec[0]);
	while (wait(&s) != pid);
		;
	s &= 0377;
	if (s != 0 && s != SIGPIPE) {
		pfmt(stderr, MM_ERROR, cmdfailed, "Echo");
		goto err;
	}
	if (l < 0) {
		pfmt(stderr, MM_ERROR, badread1, Strerror(errno));
		goto err;
	}
	if (l == 0) {
		pfmt(stderr, MM_ERROR, ":233:\"%s\": No match\n", filename);
		goto err;
	}
	if (l == BUFSIZ) {
		pfmt(stderr, MM_ERROR, ":234:Buffer overflow expanding \"%s\"\n",
			filename);
		goto err;
	}
	xname[l] = 0;
	for (cp2 = &xname[l-1]; *cp2 == '\n' && cp2 > xname; cp2--)
		;
	*++cp2 = '\0';
	if (any(' ', xname) && stat(xname, &sbuf) < 0) {
		pfmt(stderr, MM_ERROR, ":235:\"%s\": Ambiguous\n", filename);
		goto err;
	}
	if (debug) fprintf(stderr, "%s\n", xname);
	return(savestr(xname));

err:
	printf("\n");
	return(NOSTR);
}

/*
 * Take a file name, possibly with shell meta characters
 * in it and expand it by using "sh -c echo filename"
 * Return the file name as a dynamic string.
 * If the name cannot be expanded (for whatever reason)
 * return the original file name.
 */

const char *
safeexpand(filename)
	const char filename[];
{
	const char *t = expand(filename);
	return t ? t : savestr(filename);
}

/*
 * Determine the current folder directory name.
 */
getfold(foldername)
	char *foldername;
{
	const char *folder;

	if ((folder = value("folder")) == NOSTR) {
		strcpy(foldername, homedir);
		return(0);
	}
	if ((folder = expand(folder)) == NOSTR)
		return(-1);
	if (*folder == '/')
		strcpy(foldername, folder);
	else
		sprintf(foldername, "%s/%s", homedir, folder);
	return(0);
}

/*
 * A nicer version of Fdopen, which allows us to fclose
 * without losing the open file.
 */

FILE *
Fdopen(fildes, mode)
	char *mode;
{
	register int f;

	f = dup(fildes);
	if (f < 0) {
		pfmt(stderr, MM_ERROR, failed, "dup", Strerror(errno));
		return(NULL);
	}
	return(fdopen(f, mode));
}

/*
 * return the filename associated with "s".  This function always
 * returns a non-null string (no error checking is done on the receiving end)
 */
const char *
Getf(s)
register const char *s;
{
	register char *cp;
	static char defbuf[PATHSIZE], forwardFile[PATHSIZE];
	FILE *forwarded;
	char forwardLine[1000];
	const char *tmpSafeexpand;
	int n;

	if (cp = value(s)) {
		tmpSafeexpand = safeexpand(cp);
	}
	else {
		tmpSafeexpand = NULL;
	}


	if (strcmp(s, "MBOX") == 0) {
		if (tmpSafeexpand &&
		    (strncmp(tmpSafeexpand, maildir, strlen(maildir)) == 0)) {
			tmpSafeexpand = NULL;
			pfmt(stderr, MM_WARNING, ":677:MBOX cannot be in the mail directory.\n\tUsing default mbox.\n");
		}
	}
	if (strcmp(s, "FORWARD") == 0) {
		/* see if there is a /var/mail/:forward/user */
		sprintf(forwardFile,"/var/mail/:forward/%s", myname);
		if((forwarded = fopen(forwardFile,"r")) != NULL) {
			n=fread(forwardLine, 1, sizeof(forwardLine), forwarded);
			fclose(forwarded);
			printf("\n\n");
			pfmt(stderr, MM_INFO, ":678:Your mail is currently being forwarded using\n\t%s\n\n", forwardFile);
			pfmt(stderr, MM_INFO, ":679:The following information is in the forward file:\n\t%s\n\n",forwardLine);
			sleep(4);
		}
	}
	if (tmpSafeexpand) {
		return tmpSafeexpand;
	} else if (strcmp(s, "MBOX")==0) {
		/*
		 * If this is a remote file, then c-client will make
		 * relative pathnames in the users home directory.
		 * $home locally may not be the same as $HOME on the
		 * remote system, so we won't use it here.
		 */
		if (remotehost)
			strcpy(defbuf, "mbox");
		else {
			strcpy(defbuf, Getf("HOME"));
			strcat(defbuf, "/");
			strcat(defbuf, "mbox");
			if (isdir(defbuf))
				strcat(defbuf, "/mbox");
		}
		return(defbuf);
	} else if (strcmp(s, "DEAD")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "dead.letter");
		return(defbuf);
	} else if (strcmp(s, "MAILRC")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, ".mailrc");
		return(defbuf);
	} else if (strcmp(s, "HOME")==0) {
		/* no recursion allowed! */
		return(".");
	}
	return("DEAD");	/* "cannot happen" */
}
