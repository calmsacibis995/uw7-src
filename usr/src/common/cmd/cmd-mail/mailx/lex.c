#ident	"@(#)lex.c	11.1"
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

#ident "@(#)lex.c	1.27 'attmail mail(1) command'"
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
 * Lexical processing of commands.
 */

#ifdef SIGCONT
static void		contin ARGS((void));
#endif
static int		isprefix ARGS((char *as1, char *as2));
static struct cmd	*lex ARGS((char word[]));
static int		Passeren ARGS((void));
static void		setmsize ARGS((int sz));
int DelHit	= 0;

/*
 * Set up editing on the given file name.
 * If isedit is true, we are considered to be editing the file,
 * otherwise we are reading our mail which has signficance for
 * mbox and so forth.
 */

setfile(filename, isedit)
	const char *filename;
	int isedit;
{
	MAILSTREAM *ibuf;
	static int shudclob;
	static char efile[PATHSIZE];
	char *fname;
	char fortest[128];
	struct stat stbuf;
	int exrc = 1;
	int rc = -1;
	unsigned long oMsgCnt = ulgMsgCnt,
		      oNewMsgCnt = ulgMsgCnt,
		      oUnseenMsgCnt = ulgUnseenMsgCnt,
		      newfileMsgCnt = 0;

	/*
	 * locking is done by c-client. We'll rely on that.
	 * isedit tells us if we're reading inbox (0), or other (1).
	 * Use remotehost to build proper filename.
	 */
	if (remotehost) {
		fname = salloc(strlen(filename) + strlen(remotehost) + 3);
		sprintf(fname, "{%s}%s", remotehost, filename);
	}
	else {
		fname = salloc(strlen(filename) + 1);
		sprintf(fname, "%s", filename);
	}

	if (mail_status(itf, fname, SA_MESSAGES) == NIL) {
		extern int errno;

		ulgMsgCnt = oMsgCnt;
		ulgNewMsgCnt = oNewMsgCnt;
		ulgUnseenMsgCnt = oUnseenMsgCnt;
                if (exitflg)
                        goto doexit;    /* no mail, return error */
                if (isedit && fflag)
                        pfmt(stderr, MM_ERROR, errmsg, filename, Strerror(errno));
                else if (!Hflag)
                        if (strrchr(filename,'/') == NOSTR)
                                pfmt(stderr, MM_NOSTD, hasnomail);
                        else {
				char *p = strrchr(filename,'/')+1;
                                pfmt(stderr, MM_NOSTD, hasnomailfor,
				    (*p=='.')?filename:p);
			}
                goto doret;
	}
	newfileMsgCnt = ulgMsgCnt;
	ulgMsgCnt = oMsgCnt;
	ulgNewMsgCnt = oNewMsgCnt;
	ulgUnseenMsgCnt = oUnseenMsgCnt;

	if (newfileMsgCnt == 0L)
	{
		if (exitflg)
			goto doexit;	/* no mail, return error */
		if (isedit && fflag)
			pfmt(stderr, MM_INFO, ":270:%s: empty file\n",
				filename);
		else if (!Hflag)
                        if (strrchr(filename,'/') == NOSTR)
                                pfmt(stderr, MM_NOSTD, hasnomail);
                        else {
				char *p = strrchr(filename,'/')+1;
                                pfmt(stderr, MM_NOSTD, hasnomailfor,
				    (*p=='.')?filename:p);
			}
		goto doret;
	}

	/*
	 * This seems to be closing down an already opened
	 * mail session. Isn't done the first time around
	 */
	if (shudclob) {
		holdsigs();
		/*
		 * this is the global edit. I don't know why we use this
		 * instead of the passed in value. Perhaps the 2 can get
		 * out of sync.
		 */
		if (edit)
			edstop();
		else {
			quit();
			Verhogen();
		}
		relsesigs();
	}

	if ((ibuf = (MAILSTREAM *)mail_open(itf, (char *)fname, 0)) == NULL)
	{
		extern int errno;
		/*
		 * mail_open couldn't find a driver,
		 * this isn't a valid mailbox.
		 */
		if (exitflg)
			goto doexit;	/* no mail, return error */
		if (isedit && fflag)
			pfmt(stderr, MM_ERROR, errmsg, filename,
				Strerror(errno));
		else if (!Hflag)
                        if (strrchr(filename,'/') == NOSTR)
                                pfmt(stderr, MM_NOSTD, hasnomail);
                        else {
				char *p = strrchr(filename,'/')+1;
                                pfmt(stderr, MM_NOSTD, hasnomailfor,
				    (*p=='.')?filename:p);
			}
		goto doret;
	}
	itf = ibuf;
	/* clean out any deleted mail messages, may fail if opened read only */
	if (!(ibuf->rdonly))
		mail_expunge(ibuf);

	if (exitflg) {
		exrc = 0;
		goto doexit;	/* there is mail, return success */
	}

	/*
	 * Looks like all will be well. Must hold signals
	 * while we are reading the new file, else we will ruin
	 * the message[] data structure.
	 */

	readonly = 0;
	if (!isedit && !Hflag)
		readonly = Passeren()==-1;

	holdsigs();
	if (!readonly)
		readonly = itf->rdonly;

	if (shudclob) {
		free((char*)message);
		message = (struct message *)0;
		space=0;
	}
	shudclob = 1;
	edit = isedit;
	strncpy(efile, filename, PATHSIZE);
	editfile = efile;
	if (filename != mailname)
		strcpy(mailname, filename);
	setptr(ibuf);
	setmsize(msgCount);
	relsesigs();
	sawcom = 0;
	rc = 0;

    doret:
	return(rc);

    doexit:
	exit(exrc);
	/* NOTREACHED */
}

/* global to semaphores */
static int rdlock = 0;

/*
 *  return -1 if file is already being read, 0 otherwise
 */
static
Passeren()
{
    /*
     * assuming that mail_open has already been called, itf
     * is set. If itf->rdonly is set, it means we opened
     * this box readonly. This translates into a failure in
     * obtaining the lock witch allows us to be the sole
     * mailfolder editor. If the flag isn't set, we own
     * edit rights to the mailfolder.
     *
     * rdlock is a misnamed variable. It means we've obtained
     * the lock, so everyone else must open the mailfolder
     * readonly. We have write permission.
     */
    int reason = (itf) ? ((itf->rdonly) ? L_MAXTRYS : L_SUCCESS) : L_ERROR;
    int retval = -1;

    switch (reason)
	{
	case L_SUCCESS:
	    rdlock = 1;
	    retval = 0;
	    break;

	case L_MAXTRYS:
	    pfmt(stderr, MM_WARNING, ":271:mailbox already open for edit.\n");
	    pfmt(stderr, MM_NOSTD, ":466:\tThis instance of mail is read only.\n");
	    rdlock = 0;
	    retval = -1;
	    break;

	default:
	    pfmt(stderr, MM_WARNING,
		":465:Cannot create read lock: %s\n",
		Strerror(errno));
	    rdlock = 1;
	    retval = 0;
	    break;
	}
	return(retval);
}

void
Verhogen()
{
    if (rdlock)
	rdlock = 0;
}

/*
    Check for and read in any new mail messages
*/
int newmail()
{
	MAILSTREAM *ibuf;
	int OmsgCount = msgCount, Odot, i;

	ibuf = setinput(0);

	/* mail_ping does 2 things for us.
	 * 1) it determines if the connection is active.
	 * 2) it updates information, like current message count.
	 * (see the mm_exists() function in callbacks.c)
	 */
	if (mail_ping(ibuf) == NIL) {
		return(0);
	}
	holdsigs();
	if (ulgMsgCnt+1 > space) {
		space = ulgMsgCnt + 1;
		Odot = dot - &(message[0]);
		message = (struct message *)realloc((char*)message,
			space*(sizeof( struct message)));
		nomemcheck(message);
		dot = &message[Odot];
	}

	setptr(ibuf);
	setmsize(msgCount);
	/* prehaps we should call mail_status and use the ibuf->recent flag */
	if (msgCount-OmsgCount > 0) {
		pfmt(stdout, MM_NOSTD, newmailarrived);
		if((msgCount-OmsgCount) == 1)
		    pfmt(stdout, MM_NOSTD, 
			":273:Loaded 1 new message\n");
		else
		    pfmt(stdout, MM_NOSTD, 
			":274:Loaded %d new messages\n", 
			msgCount-OmsgCount);
		if (value("header") != NOSTR)
			for (i = OmsgCount+1;
			     i <= msgCount; i++) {
				printhead(i, 0);
			}
	}
	relsesigs();
	
	return(0);
}

/*
 * Interpret user commands one by one.  If standard input is not a tty,
 * print no prompt.
 */

static int	*msgvec;
static int	shudprompt;

void
commands()
{
	int eofloop;
	register int n;
	char linebuf[LINESIZE];

#ifdef SIGCONT
# ifdef preSVr4
	sigset(SIGCONT, SIG_DFL);
# else
	{
	struct sigaction nsig;
	nsig.sa_handler = SIG_DFL;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = SA_RESTART;
	(void) sigaction(SIGCONT, &nsig, (struct sigaction*)0);
	}
# endif
#endif
	if (rcvmode && !sourcing) {
		if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
			sigset(SIGINT, stop);
		if (sigset(SIGHUP, SIG_IGN) != SIG_IGN)
			sigset(SIGHUP, hangup);
	}
	for (;;) {
		setjmp(srbuf);

		/*
		 * Print the prompt, if needed.  Clear out
		 * string space, and flush the output.
		 */

		if (!rcvmode && !sourcing)
			return;
		eofloop = 0;
looptop:
		if ((shudprompt = (intty && !sourcing)) != 0) {
			if (prompt==NOSTR)
				prompt = "";
#ifdef SIGCONT
# ifdef preSVr4
			sigset(SIGCONT, contin);
# else
			{
			struct sigaction nsig;
			nsig.sa_handler = contin;
			sigemptyset(&nsig.sa_mask);
			nsig.sa_flags = SA_RESTART;
			(void) sigaction(SIGCONT, &nsig, (struct sigaction*)0);
			}
# endif
#endif
			if (value("newmail") != NOSTR)
				newmail();
			printf("%s", prompt);
		}
		flush();
		sreset();

		/*
		 * Read a line of commands from the current input
		 * and handle end of file specially.
		 */

		n = 0;
		for (;;) {
			if (readline(input, &linebuf[n]) <= 0) {
				if (n != 0)
					break;
				if (loading)
					return;
				if (sourcing) {
					unstack();
					goto more;
				}
				if (value("ignoreeof") != NOSTR && shudprompt) {
					if (++eofloop < 25) {
						pfmt(stdout, MM_NOSTD, 
							":275:Use \"quit\" to quit.\n");
						goto looptop;
					}
				}
				if (edit)
					edstop();
				return;
			}
			if ((n = strlen(linebuf)) == 0)
				break;
			n--;
			if (linebuf[n] != '\\')
				break;
			linebuf[n++] = ' ';
		}
#ifdef SIGCONT
# ifdef preSVr4
		sigset(SIGCONT, SIG_DFL);
# else
		{
		struct sigaction nsig;
		nsig.sa_handler = SIG_DFL;
		sigemptyset(&nsig.sa_mask);
		nsig.sa_flags = SA_RESTART;
		(void) sigaction(SIGCONT, &nsig, (struct sigaction*)0);
		}
# endif
#endif
		if (execute(linebuf, 0))
			return;
more:		;
	}
}

/*
 * Execute a single command.  If the command executed
 * is "quit," then return non-zero so that the caller
 * will know to return back to main, if he cares.
 * Contxt is non-zero if called while composing mail.
 */

execute(linebuf, contxt)
	char linebuf[];
{
	char word[LINESIZE];
	char *arglist[MAXARGC];
	struct cmd *com;
	register char *cp, *cp2;
	register int c, e;
	int muvec[2];

	/*
	 * Strip the white space away from the beginning
	 * of the command, then scan out a word, which
	 * consists of anything except digits and white space.
	 *
	 * Handle |, ! and # differently to get the correct
	 * lexical conventions.
	 */

	cp = (char*)skipspace(linebuf);
	cp2 = word;
	if (any(*cp, "!|#"))
		*cp2++ = *cp++;
	else
		while (*cp && !any(*cp, " \t0123456789$^.:/-+*'\""))
			*cp2++ = *cp++;
	*cp2 = '\0';

	/*
	 * Look up the command; if not found, complain.
	 * Normally, a blank command would map to the
	 * first command in the table; while sourcing,
	 * however, we ignore blank lines to eliminate
	 * confusion.
	 */

	if (equal(word,"") && sourcing)
		return 0;

	com = lex(word);
	if (com == NONE) {
		pfmt(stdout, MM_ERROR, ":276:Unknown command: \"%s\"\n", word);
		if (loading) {
			cond = CANY;
			return(1);
		}
		if (sourcing) {
			cond = CANY;
			unstack();
		}
		return(0);
	}

	/*
	 * See if we should execute the command -- if a conditional
	 * we always execute it, otherwise, check the state of cond.
	 */

	if ((com->c_argtype & F) == 0)
		if (cond == CRCV && !rcvmode || cond == CSEND && rcvmode)
			return(0);

	/*
	 * Special case so that quit causes a return to
	 * main, who will call the quit code directly.
	 * If we are in a source file, just unstack.
	 */

	if (com->c_func == edstop && sourcing) {
		if (loading)
			return(1);
		unstack();
		return(0);
	}
	if (!edit && com->c_func == edstop) {
		sigset(SIGINT, SIG_IGN);
		return(1);
	}

	/*
	 * Process the arguments to the command, depending
	 * on the type he expects.  Default to an error.
	 * If we are sourcing an interactive command, it's
	 * an error.
	 */

	if (!rcvmode && (com->c_argtype & M) == 0) {
		pfmt(stdout, MM_ERROR, 
			":277:May not execute \"%s\" while sending\n",
			com->c_name);
		if (loading)
			return(1);
		if (sourcing)
			unstack();
		return(0);
	}
	if (sourcing && com->c_argtype & I) {
		pfmt(stdout, MM_ERROR, 
			":278:May not execute \"%s\" while sourcing\n",
			com->c_name);
		if (loading)
			return(1);
		unstack();
		return(0);
	}
	if (readonly && com->c_argtype & W) {
		pfmt(stdout, MM_ERROR, 
			":279:May not execute \"%s\" -- message file is read only\n",
			com->c_name);
		if (loading)
			return(1);
		if (sourcing)
			unstack();
		return(0);
	}
	if (contxt && com->c_argtype & R) {
		pfmt(stdout, MM_ERROR, ":280:Cannot recursively invoke \"%s\"\n",
			com->c_name);
		return(0);
	}
	e = 1;
	switch (com->c_argtype & ~(F|P|I|M|TX|W|R)) {
	case MSGLIST:
		/*
		 * A message list defaulting to nearest forward
		 * legal message.
		 */
		if (msgvec == 0) {
			pfmt(stdout, MM_ERROR, illegalmsglist);
			return(-1);
		}
		if ((c = getmsglist(cp, msgvec, com->c_msgflag)) < 0)
			break;
		if (c  == 0)
			if (msgCount == 0)
				*msgvec = NULL;
			else {
				*msgvec = first(com->c_msgflag,
					com->c_msgmask);
				msgvec[1] = NULL;
			}
		if (*msgvec == NULL) {
			pfmt(stdout, MM_WARNING, noapplicmsgs);
			break;
		}
		e = (*com->c_func)(msgvec);
		break;

	case NDMLIST:
		/*
		 * A message list with no defaults, but no error
		 * if none exist.
		 */
		if (msgvec == 0) {
			pfmt(stdout, MM_ERROR, illegalmsglist);
			return(-1);
		}
		if (getmsglist(cp, msgvec, com->c_msgflag) < 0)
			break;
		e = (*com->c_func)(msgvec);
		break;

	case STRLIST:
		/*
		 * Just the straight string, with
		 * leading blanks removed.
		 */
		cp = (char*)skipspace(cp);
		e = (*com->c_func)(cp);
		break;

	case RAWLIST:
		/*
		 * A vector of strings, in shell style.
		 */
		if ((c = getrawlist(cp, arglist)) < 0)
			break;
		if (c < com->c_minargs) {
			if (com->c_minargs == 1)
				pfmt(stdout, MM_ERROR, 
					":281:%s requires at least 1 arg\n",
					com->c_name);
			else
				pfmt(stdout, MM_ERROR, 
					":282:%s requires at least %d args\n", 
					com->c_name, com->c_minargs);
			break;
		}
		if (c > com->c_maxargs) {
			if (com->c_maxargs == 1)
				pfmt(stdout, MM_ERROR, 
					":283:%s takes no more than 1 arg\n",
					com->c_name);
			else
				pfmt(stdout, MM_ERROR, 
					":284:%s takes no more than %d args\n",
					com->c_name, com->c_maxargs);
			break;
		}
		e = (*com->c_func)(arglist);
		break;

	case NOLIST:
		/*
		 * Just the constant zero, for exiting,
		 * eg.
		 */
		e = (*com->c_func)(0);
		break;

	default:
		panic(":285:Unknown argtype");
	}

	/*
	 * Exit the current source file on
	 * error.
	 */

	if (e && loading)
		return(1);
	if (e && sourcing)
		unstack();
	if (com->c_func == edstop)
		return(1);
	if (value("autoprint") != NOSTR && com->c_argtype & P) {
		int i = first(0, MDELETED);
		muvec[0] = i;
		muvec[1] = 0;
		type(muvec);
	}
	if (!sourcing && (com->c_argtype & TX) == 0)
		sawcom = 1;
	return(0);
}

#ifdef SIGCONT
/*
 * When we wake up after ^Z, reprint the prompt.
 */
static void
contin()
{
	if (shudprompt)
		printf("%s", prompt);
	fflush(stdout);
}
#endif

/*
 * Branch here on hangup signal and simulate quit.
 */
/*ARGSUSED*/
void
hangup(s)
{

	holdsigs();
	sigignore(SIGHUP);
	if (edit) {
		if (setjmp(srbuf))
			exit(0);
		edstop();
	} else {
		Verhogen();
		if (value("exit") != NOSTR)
			exit(1);
		else
			quit();
	}
	exit(0);
}

/*
 * Set the size of the message vector used to construct argument
 * lists to message list functions.
 */

static void
setmsize(sz)
{

	if (msgvec != (int *) 0)
		free((char*)msgvec);
	if (sz < 1)
		sz = 1; /* need at least one cell for terminating 0 */
	msgvec = (int *) pcalloc((unsigned) (sz + 1) * sizeof *msgvec);
}

/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */

static struct cmd *
lex(word)
	char word[];
{
	register struct cmd *cp;

	for (cp = &cmdtab[0]; cp->c_name != NOSTR; cp++)
		if (isprefix(word, cp->c_name))
			return(cp);
	return(NONE);
}

/*
 * Determine if as1 is a valid prefix of as2.
 */
static int
isprefix(as1, as2)
	char *as1, *as2;
{
	register char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(1);
	return(*--s1 == '\0');
}

/*
 * The following gets called on receipt of a rubout.  This is
 * to abort printout of a command, mainly.
 * Dispatching here when command() is inactive crashes rcv.
 * Close all open files except 0, 1, 2, and the temporaries.
 * The special call to getuserid() is needed so it won't get
 * annoyed about losing its open file.
 * Also, unstack all source files.
 */

static int	inithdr;		/* am printing startup headers */

void
stop(s)
{
	register NODE *head, *nextnode = 0;

	noreset = 0;
	if (!inithdr)
		sawcom++;
	inithdr = 0;
	while (sourcing)
		unstack();
	getuserid((char *) 0);
	DelHit = 1;
	for (head = fplist; head != (NODE *)NULL; head = nextnode) {
		nextnode = head->next;
		if (head->fp == pipef) {
			npclose(pipef);
			pipef = NULL;
		} else if (head->fp != stdin
			&& head->fp != stdout
			&& head->fp != stderr) {
				fclose(head->fp);
				}
	}
	if (image >= 0) {
		close(image);
		image = -1;
	}
	if (s) {
		pfmt(stdout, MM_WARNING, hasinterrupted);
		sigrelse(s);
	}
	longjmp(srbuf, 1);
}

/*
 * Announce the presence of the current mailx version,
 * give the message count, and print a header listing.
 */

static char	greeting[]	= ":482:mailx version %s  Type ? for help.\n";

void
announce()
{
	int vec[2], mdot;

	if (!Hflag && value("quiet")==NOSTR)
		pfmt(stdout, MM_NOSTD, greeting, version);
	mdot = newfileinfo();
	vec[0] = mdot;
	vec[1] = 0;
	dot = &message[mdot - 1];
	if (msgCount > 0 && !noheader) {
		inithdr++;
		headers(vec);
		inithdr = 0;
	}
}

/*
 * Announce information about the file we are editing.
 * Return a likely place to set dot.
 */
newfileinfo()
{
	register struct message *mp;
	register int u, n, mdot = 0, d, s;
	char fname[BUFSIZ], zname[BUFSIZ], *ename;
	MAILSTREAM *ibuf;

	if (Hflag)
		return(1);
	s = d = 0;

	ibuf = setinput(0);
	mail_status(ibuf, ibuf->mailbox, SA_MESSAGES | SA_RECENT | SA_UNSEEN);
	n = (int) ulgNewMsgCnt;
	u = (int) ulgUnseenMsgCnt;
	if (msgCount != (int) ulgMsgCnt) {
		n -= ((int) ulgMsgCnt - msgCount);
		u -= ((int) ulgMsgCnt - msgCount);
	}

	for (mp = &message[0]; mp < &message[msgCount]; mp++)
	{
		/*
		 * valid range for mdot is 1 to msgCount.
		 * mdot == 0 means we haven't calculated the
		 * current message pointer yet. Once we've run
		 * into a new or unread message, mdot will
		 * be set.
		 */
		if (mdot == 0)
			setflags(mp);

		if (mp->m_flag & MNEW)
			if (mdot == 0)
				mdot = mp - &message[0] + 1;
		if ((mp->m_flag & MREAD) == 0)
			if (mdot == 0)
				mdot = mp - &message[0] + 1;
		if (mp->m_flag & MDELETED)
			d++;
		if (mp->m_flag & MSAVED)
			s++;
	}
	ename=origname;
	if (getfold(fname) >= 0) {
		strcat(fname, "/");
		if (strncmp(fname, mailname, strlen(fname)) == 0) {
			sprintf(zname, "+%s", mailname + strlen(fname));
			ename = zname;
		}
	}
	pfmt(stdout, MM_NOSTD, ":287:\"%s\": ", ename);
	if (msgCount == 1)
		pfmt(stdout, MM_NOSTD, ":288:1 message");
	else
		pfmt(stdout, MM_NOSTD, ":289:%d messages", msgCount);
	if (n == 1)
		pfmt(stdout, MM_NOSTD, ":290: 1 new");
	else if (n > 1)
		pfmt(stdout, MM_NOSTD, ":291: %d new", n);
	if (u-n == 1)
		pfmt(stdout, MM_NOSTD, ":292: 1 unread");
	else if (u-n > 1)
		pfmt(stdout, MM_NOSTD, ":293: %d unread", u);
	if (d == 1)
		pfmt(stdout, MM_NOSTD, ":294: 1 deleted");
	else if (d > 1)
		pfmt(stdout, MM_NOSTD, ":295: %d deleted", d);
	if (s == 1)
		pfmt(stdout, MM_NOSTD, ":296: 1 saved");
	else if (s > 1)
		pfmt(stdout, MM_NOSTD, ":297: %d saved", s);
	if (readonly)
		pfmt(stdout, MM_NOSTD, ":298: [Read only]");
	printf("\n");
	return(mdot);
}

/*
 * Print the current version number.
 */

/* ARGSUSED */
pversion(e)
	char *e;
{
	printf("%s\n", version);
	return(0);
}

/*
 * Load a file of user definitions.
 */
void
load(filename)
	const char *filename;
{
	register FILE *in, *oldin;

	if ((in = fopen(filename, "r")) == NULL)
		return;
	oldin = input;
	input = in;
	loading = 1;
	sourcing = 1;
	commands();
	loading = 0;
	sourcing = 0;
	input = oldin;
	fclose(in);
}
