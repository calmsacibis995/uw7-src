#ident	"@(#)cmd3.c	11.1"
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

#ident "@(#)cmd3.c	1.23 'attmail mail(1) command'"
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
 * Still more user commands.
 */

static int	bangexp ARGS((char *str));
static const char	*getfilename ARGS((char *name, int *aedit));
static int	shell1 ARGS((char *str));
static void	sort ARGS((char **list));

static char	prevfile[PATHSIZE];
static char	origprevfile[PATHSIZE];
static char	lastbang[256];

/*
 * Process a shell escape or interactive shell by saving signals, ignoring signals,
 * and forking a sh -c
 */

shell(str)
	char *str;
{
	shell1(str[0] ? str : (char*)0);
	if (str[0])
		printf("!\n");
	return(0);
}

static int
shell1(str)
	char *str;
{
	void (*sig[2])();
	int stat[1];
	register int t;
	register pid_t p;
	char *Shell;
	char cmd[BUFSIZ];

	if (str) {
		strcpy(cmd, str);
		if (bangexp(cmd) < 0)
			return(-1);
	}
	if ((Shell = value("SHELL")) == NOSTR || *Shell=='\0')
		Shell = SHELL;
	for (t = SIGINT; t <= SIGQUIT; t++)
		sig[t-SIGINT] = sigset(t, SIG_IGN);
	p = vfork();
	if (p == 0) {
		sigchild();
		for (t = SIGINT; t <= SIGQUIT; t++)
			if (sig[t-SIGINT] != SIG_IGN)
				sigset(t, SIG_DFL);
		if (str)
			execlp(Shell, Shell, "-c", cmd, (char *)0);
		else
			execlp(Shell, Shell, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, Shell, Strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	while (wait(stat) != p)
		;
	if (p == (pid_t)-1)
		pfmt(stderr, MM_ERROR, failed, "fork", Strerror(errno));
	for (t = SIGINT; t <= SIGQUIT; t++)
		sigset(t, sig[t-SIGINT]);
	return(0);
}

/*
 * Expand the shell escape by expanding unescaped !'s into the
 * last issued command where possible.
 */
static int
bangexp(str)
	char *str;
{
	char bangbuf[BUFSIZ];
	register char *cp, *cp2;
	register int n;
	int changed = 0;
	int bangit = (value("bang")!=NOSTR);

	cp = str;
	cp2 = bangbuf;
	n = BUFSIZ;
	while (*cp) {
		if (*cp=='!' && bangit) {
			if (n < (int)strlen(lastbang)) {
overf:
				pfmt(stdout, MM_ERROR, 
					":197:Command buffer overflow\n");
				return(-1);
			}
			changed++;
			strcpy(cp2, lastbang);
			cp2 += strlen(lastbang);
			n -= strlen(lastbang);
			cp++;
			continue;
		}
		if (*cp == '\\' && cp[1] == '!') {
			if (--n <= 1)
				goto overf;
			*cp2++ = '!';
			cp += 2;
			changed++;
		}
		if (--n <= 1)
			goto overf;
		*cp2++ = *cp++;
	}
	*cp2 = 0;
	if (changed) {
		printf("!%s\n", bangbuf);
		fflush(stdout);
	}
	strcpy(str, bangbuf);
	strncpy(lastbang, bangbuf, sizeof lastbang);
	lastbang[(sizeof lastbang)-1] = 0;
	return(0);
}

/*
 * Print out a nice help message.
 */

help()
{
	return pghelpfile(HELPFILE);
}

/*
 * Print out a nice help message from some file or another.
 */

pghelpfile(helpfile)
const char *helpfile;
{
	const char *pg = pager();
	void (*sigint)(), (*sigpipe)();
	register FILE *infp = fopen(helpfile, "r");
	register FILE *outfp;

	if (infp == NULL) {
		pfmt(stdout, MM_WARNING, nohelp);
		return(1);
	}
	outfp = npopen(pg, "w");
	if (outfp == NULL) {
		pfmt(stderr, MM_ERROR, failed, pg, Strerror(errno));
		outfp = stdout;
	}
	sigint = sigset(SIGINT, SIG_IGN);
	sigpipe = sigset(SIGPIPE, SIG_IGN);
	copystream(infp, outfp);
	(void) sigset(SIGINT, sigint);
	(void) sigset(SIGPIPE, sigpipe);
	fclose(infp);
	if (outfp != stdout)
		npclose(outfp);
	return(0);
}

/*
 * Change user's working directory.
 */

schdir(str)
	const char *str;
{
	register const char *cp = skipspace(str);
	if (*cp == '\0')
		cp = homedir;
	else
		if ((cp = expand(cp)) == NOSTR)
			return(1);
	if (chdir(cp) < 0) {
		pfmt(stderr, MM_ERROR, badchdir, cp, Strerror(errno));
		return(1);
	}
	return(0);
}

/*
 * Reply to a list of messages.  Extract each name from the
 * message header and send them off to mail1()
 */

respond(msgvec)
	int *msgvec;
{
	if (value("flipr") != NOSTR) return(replysender(msgvec, 0));
	else return(respondall(msgvec, 0));
}

followup(msgvec)
int *msgvec;
{
	if (value("flipf") != NOSTR) return(replysender(msgvec, 1));
	else return(respondall(msgvec, 1));
}

followupall(msgvec)
int *msgvec;
{
	return(respondall(msgvec, 1));
}

int
respondall(msgvec, useauthor)
	int *msgvec;
{
	char recfile[128];
	struct message *mp;
	char *rcv, **ap;
	struct name *np;
	struct header head;

	if (msgvec[1] != 0) {
		pfmt(stdout, MM_ERROR, 
			":198:Sorry, cannot reply to multiple messages at once\n");
		return(1);
	}

	rcv = nameof(dot = mp = &message[msgvec[0]-1]);
	strncpy(recfile, rcv, sizeof recfile);
	np = elide(cat(extract(rcv, GTO),
		   cat(extract(hfield("to", mp, addto),
				value("replycc") ? GCC : GTO),
		       extract(hfield("cc", mp, addto), GCC))));

	mapf(np, skin(rcv));

	/*
	 * Delete my name from the reply list,
	 * and with it, all my alternate names.
	 * delname() normalizes names, so we only need to
	 * worry about one form of the network name.
	 */
	if (!value("metoo")) {
		np = delname(np, myname);
		np = delname(np, mylocalname);
		np = delname(np, mydomname);
	}
	if (altnames != 0)
		for (ap = altnames; *ap; ap++)
			np = delname(np, *ap);
	head.h_to = detract(np, GTO);
	if (!head.h_to)
		head.h_to = rcv;
	head.h_cc = detract(np, GCC);
	head.h_bcc = head.h_defopt = head.h_encodingtype = head.h_content_type = 
		head.h_content_transfer_encoding = head.h_mime_version = head.h_date = NOSTR;
	head.h_others = NOSTRPTR;
	head.h_seq = 1;
	head.h_subject = hfield("subject", mp, addone);
	if (head.h_subject == NOSTR)
		head.h_subject = hfield("subj", mp, addone);
	head.h_subject = reedit(head.h_subject);
	mail1(&head, useauthor ? recfile : (char*)0, (int*)0);
	return(0);
}

void
getrecf(buf, recfile, useauthor)
	char *buf;
	char *recfile;
{
	register char *bp, *cp;
	register char *recf = recfile;
	register int folderize = (value("outfolder") != NOSTR && 
				 (!value("posix2") || 
				 ((cp = value("folder")) != NOSTR && 
				 !equal(cp, ""))));

	if (useauthor) {
		if (folderize)
			*recf++ = '+';
		if (debug) fprintf(stderr, "buf='%s'\n", buf);

	/*  If the address is of the format
		@host1, @host2, ... @hostn:username@host.domain
	    this will get rid of the first @hosts so that the name
	    can be retrieved to save the mail as username.
	*/

		if((bp = strchr(buf, ':')) != NULL) {
			buf = bp+1;
			if (debug) fprintf(stderr, "buf='%s'\n", buf);
		}
		for (bp=skin(buf), cp=recf; *bp && !any(*bp, "@%, "); bp++) {
			if (*bp=='!')
				cp = recf;
			else
				*cp++ = *bp;
		}
		*cp = '\0';
		if (cp==recf)
			*recfile = '\0';
	} else if ((cp = value("record")) != 0) {
		if (folderize && *cp!='+' && *cp!='/'
		 && *safeexpand(cp)!='/')
			*recf++ = '+';
		strcpy(recf, cp);
	} else {
		*recf = '\0';
	}
	if (debug) fprintf(stderr, "recfile='%s'\n", recfile);
}

/*
 * Modify the subject we are replying to to begin with Re: if
 * it does not already.
 */

char *
reedit(subj)
	char *subj;
{
	char sbuf[10];
	register char *newsubj;

	if (subj == NOSTR)
		return(NOSTR);
	strncpy(sbuf, subj, 3);
	sbuf[3] = 0;
	if (icequal(sbuf, "re:"))
		return(subj);
	newsubj = salloc((unsigned)(strlen(subj) + 5));
	sprintf(newsubj, "Re: %s", subj);
	return(newsubj);
}

/*
 * Preserve the named messages, so that they will be sent
 * back to the system mailbox.
 */

preserve(msgvec)
	int *msgvec;
{
	register struct message *mp;
	register int *ip, mesg;

	if (edit) {
		pfmt(stdout, MM_ERROR, ":200:Cannot \"preserve\" in edit mode\n");
		return(1);
	}
	for (ip = msgvec; *ip != NULL; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		setflags(mp);
		mp->m_flag |= MPRESERVE;
		mp->m_flag &= ~MBOX;
		dot = mp;
	}
	return(0);
}

/*
 * Print the size of each message.
 */

messize(msgvec)
	int *msgvec;
{
	register struct message *mp;
	register int *ip, mesg;

	for (ip = msgvec; *ip != NULL; ip++) {
		mesg = *ip;
		dot = mp = &message[mesg-1];
		setinput(mp);
		pfmt(stdout, MM_NOSTD, ":201:%d: %ld\n", mesg, mp->m_size);
	}
	return(0);
}

/*
 * Quit quickly.  If we are sourcing, just pop the input level
 * by returning an error.
 */

int
rexit(e)
{
	if (sourcing)
		return(1);
	if (Tflag != NOSTR)
		close(creat(Tflag, TEMPPERM));
	if (!edit)
		Verhogen();
	exit(e);
	/* NOTREACHED */
}

/*
 * Set or display a variable value.  Syntax is similar to that
 * of csh.
 */

set(arglist)
	char **arglist;
{
	register struct var *vp;
	register char *cp, *cp2;
	char varbuf[BUFSIZ], **ap, **p;
	int errs, h, s;

	if (argcount(arglist) == 0) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NOVAR; vp = vp->v_link)
				s++;
		ap = (char **) salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NOVAR; vp = vp->v_link)
				*p++ = vp->v_name;
		*p = NOSTR;
		sort(ap);
		for (p = ap; *p != NOSTR; p++)
			if (((cp = value(*p)) != 0) && *cp)
				printf("%s=\"%s\"\n", *p, cp);
			else
				printf("%s\n", *p);
		return(0);
	}
	errs = 0;
	for (ap = arglist; *ap != NOSTR; ap++) {
		cp = *ap;
		cp2 = varbuf;
		while (*cp != '=' && *cp != '\0')
			*cp2++ = *cp++;
		*cp2 = '\0';
		if (*cp == '\0')
			cp = "";
		else
			cp++;
		if (equal(varbuf, "")) {
			pfmt(stdout, MM_ERROR, 
				":202:Non-null variable name required\n");
			errs++;
			continue;
		}
		assign(varbuf, cp);
	}
	return(errs);
}

/*
 * Unset a bunch of variable values.
 */

unset(arglist)
	char **arglist;
{
	register int errs;
	register char **ap;

	errs = 0;
	for (ap = arglist; *ap != NOSTR; ap++)
		errs += deassign(*ap);
	return(errs);
}

/*
 * Add users to a group.
 */

group(argv)
	char **argv;
{
	register struct grouphead *gh;
	register struct mgroup *gp;
	register int h;
	int s;
	char **ap, *gname, **p;

	if (argcount(argv) == 0) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (gh = groups[h]; gh != NOGRP; gh = gh->g_link)
				s++;
		ap = (char **) salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (gh = groups[h]; gh != NOGRP; gh = gh->g_link)
				*p++ = gh->g_name;
		*p = NOSTR;
		sort(ap);
		for (p = ap; *p != NOSTR; p++)
			printgroup(*p);
		return(0);
	}
	if (argcount(argv) == 1) {
		printgroup(*argv);
		return(0);
	}
	gname = *argv;
	h = hash(gname);
	if ((gh = findgroup(gname)) == NOGRP) {
		gh = (struct grouphead *) pcalloc(sizeof *gh);
		gh->g_name = vcopy(gname);
		gh->g_list = NOGE;
		gh->g_link = groups[h];
		groups[h] = gh;
	}

	/*
	 * Insert names from the command list into the group.
	 * Who cares if there are duplicates?  They get tossed
	 * later anyway.
	 */

	for (ap = argv+1; *ap != NOSTR; ap++) {
		gp = (struct mgroup *) pcalloc(sizeof *gp);
		gp->ge_name = vcopy(*ap);
		gp->ge_link = gh->g_list;
		gh->g_list = gp;
	}
	return(0);
}

/*
 * Remove a group name.
 */

ungroup(argv)
	char **argv;
{
	register struct grouphead *gh, *ogh;
	register struct mgroup *gp;
	register int h;

	for ( ; *argv; argv++) {
		h = hash(*argv);
		/* find group head for name; ogh is previous link */
		for (gh = groups[h], ogh = 0; gh != NOGRP; ogh = gh, gh = gh->g_link) {
			if (equal(gh->g_name, *argv)) {
				/* unlink group head from group head list */
				if (ogh) {
					ogh->g_link = gh->g_link;
				} else {
					groups[h] = gh->g_link;
				}
				/* Now deallocate names in the group list. */
				vfree(gh->g_name);
				for (gp = gh->g_list; gp; ) {
					struct mgroup *ogp = gp;
					vfree(gp->ge_name);
					gp = gp->ge_link;
					free(ogp);
				}
				free(gh);
				break;
			}
		}
	}
	return 0;
}

/*
 * Sort the passed string vecotor into ascending dictionary
 * order.
 */

static void
sort(list)
	char **list;
{
	register char **ap;

	for (ap = list; *ap != NOSTR; ap++)
		;
	if (ap-list < 2)
		return;
	qsort((char*)list, ap-list, sizeof *list, compstrs);
}

/*
 * Do a dictionary order comparison of the arguments from
 * qsort.
 */
int
compstrs(_l, _r)
	const VOID *_l;
	const VOID *_r;
{
	const char **l = (const char**)_l, **r = (const char**)_r;
	return strcmp(*l, *r);
}

/*
 * The do nothing command for comments.
 */

/* ARGSUSED */
null(e)
	char *e;
{
	return(0);
}

/*
 * Print out the current edit file, if we are editing.
 * Otherwise, print the name of the person who's mail
 * we are reading.
 */
int
file(argv)
	char **argv;
{
	register const char *cp;
	int editflag;

	if (argv[0] == NOSTR) {
		newfileinfo();
		return(0);
	}

	/*
	 * Acker's!  Must switch to the new file.
	 * We use a funny interpretation --
	 *	# -- gets the previous file
	 *	% -- gets the invoker's post office box
	 *	%user -- gets someone else's post office box
	 *	& -- gets invoker's mbox file
	 *	string -- reads the given file
	 */

	cp = getfilename(argv[0], &editflag);
	if (cp == NOSTR)
		return(-1);
	if (setfile(cp, editflag)) {
		strcpy(origname, origprevfile);
		return(-1);
	}
	newfileinfo();
	return(0);
}

/*
 * Evaluate the string given as a new mailbox name.
 * Ultimately, we want this to support a number of meta characters.
 * Possibly:
 *	% -- for my system mail box
 *	%user -- for user's system mail box
 *	# -- for previous file
 *	& -- get's invoker's mbox file
 *	file name -- for any other file
 */

static const char *
getfilename(fname, aedit)
	char *fname;
	int *aedit;
{
	register const char *cp;
	char savename[BUFSIZ];
	char oldmailname[BUFSIZ];
	char tmp[BUFSIZ];

	/*
	 * Assume we will be in "edit file" mode, until
	 * proven wrong.
	 */
	*aedit = 1;
	switch (*fname) {
	case '%':
		if (fname[1] != 0) {
			pfmt(stderr, MM_ERROR,
			    ":684:Cannot determine alternate users INBOX: %s\n",
			    fname+1);
			return(NOSTR);
			/*
			 * c-client doesn't allow this
			strcpy(savename, myname);
			strcpy(oldmailname, mailname);
			strncpy(myname, fname+1, PATHSIZE-1);
			myname[PATHSIZE-1] = 0;
			findmail();
			cp = savestr(mailname);
			strcpy(origname, cp);
			strcpy(myname, savename);
			strcpy(mailname, oldmailname);
			return(cp);
			 */
		}
		*aedit = 0;
		strcpy(prevfile, mailname);
		strcpy(origprevfile, origname);
		strcpy(oldmailname, mailname);
		findmail();
		cp = savestr(mailname);
		strcpy(mailname, oldmailname);
		strcpy(origname, cp);
		return(cp);

	case '#':
		if (fname[1] != 0)
			goto regular;
		if (prevfile[0] == 0) {
			pfmt(stdout, MM_ERROR, ":203:No previous file\n");
			return(NOSTR);
		}
		cp = savestr(prevfile);
		strcpy(prevfile, mailname);
		strcpy(tmp, origname);
		strcpy(origname, origprevfile);
		strcpy(origprevfile, tmp);
		*aedit = (cascmp(cp, "INBOX"))?1:0;
		return(cp);

	case '&':
		strcpy(prevfile, mailname);
		strcpy(origprevfile, origname);
		if (fname[1] == '\0') {
			const char *tmpmbox = Getf("MBOX");
			strcpy(origname, tmpmbox);
			*aedit = 1;
			return(tmpmbox);
		}
		/* Fall into . . . */

	default:
regular:
		/* TODO, needs a message catalog error */
		if (*fname == '{') {
			pfmt(stderr, MM_ERROR,
				":683:Invalid folder specification: %s\n",
				fname);
			return(NOSTR);
		}
		strcpy(prevfile, mailname);
		strcpy(origprevfile, origname);
		cp = safeexpand(fname);
		strcpy(origname, cp);
		*aedit = (cascmp(cp, "INBOX"))?1:0;
		if (cp[0] != '/' && !remotehost && *aedit) {
			fname = getcwd(NOSTR, PATHSIZE);
			strcat(fname, "/");
			strcat(fname, cp);
			cp = fname;
		}
		return(cp);
	}
}

/*
 * Expand file names like echo
 */

echo(str)
char *str;
{
	char cmd[LINESIZE];

	sprintf(cmd, "echo %s", str);
	shell1(cmd);
	return(0);
}

/*
 * Reply to a series of messages by simply mailing to the senders
 * and not messing around with the To: and Cc: lists as in normal
 * reply.
 */

Respond(msgvec)
	int msgvec[];
{
	if (value("flipr") != NOSTR) return(respondall(msgvec, 0));
	else return(replysender(msgvec, 0));
}

Followup(msgvec)
int *msgvec;
{
	if (value("flipf") != NOSTR) return(respondall(msgvec, 1));
	else return(replysender(msgvec, 1));
}

Followupall(msgvec)
int *msgvec;
{
	return(replysender(msgvec, 1));
}

int
replysender(msgvec, useauthor)
int *msgvec;
{
	char recfile[128];
	struct header head;
	struct message *mp;
	register int s, *ap;
	register char *cp, *subject;

	for (s = 0, ap = msgvec; *ap != 0; ap++) {
		mp = &message[*ap - 1];
		dot = mp;
		s += strlen(nameof(mp)) + 2;
	}
	if (s == 0)
		return(0);
	cp = salloc((unsigned)(++s));
	head.h_to = cp;
	for (ap = msgvec; *ap != 0; ap++) {
		mp = &message[*ap - 1];
		cp = copy(nameof(mp), cp);
		*cp++ = ',';
		*cp++ = ' ';
	}
	*cp = 0;
	strncpy(recfile, head.h_to, sizeof recfile);
	mp = &message[msgvec[0] - 1];
	subject = hfield("subject", mp, addone);
	head.h_seq = 1;
	if (subject == NOSTR)
		subject = hfield("subj", mp, addone);
	head.h_subject = reedit(subject);
	if (subject != NOSTR)
		head.h_seq++;
	head.h_cc = head.h_bcc = head.h_defopt = head.h_encodingtype = head.h_content_type =
		head.h_content_transfer_encoding = head.h_mime_version = head.h_date = NOSTR;
	head.h_others = NOSTRPTR;
	mail1(&head, useauthor ? recfile : (char*)0, (int*)0);
	return(0);
}

/*
 * Conditional commands.  These allow one to parameterize one's
 * .mailrc and do some things if sending, others if receiving.
 */

ifcmd(argv)
	char **argv;
{
	register char *cp;

	if (cond != CANY) {
		pfmt(stdout, MM_ERROR, ":204:Illegal nested \"if\"\n");
		return(1);
	}
	cond = CANY;
	cp = argv[0];
	switch (*cp) {
	case 'r': case 'R':
		cond = CRCV;
		break;

	case 's': case 'S':
		cond = CSEND;
		break;

	case 't': case 'T':
		cond = isatty(0);
		break;

	default:
		pfmt(stdout, MM_ERROR, ":205:Unrecognized if-keyword: \"%s\"\n", 
			cp);
		return(1);
	}
	return(0);
}

/*
 * Implement 'else'.  This is pretty simple -- we just
 * flip over the conditional flag.
 */

elsecmd()
{

	switch (cond) {
	case CANY:
		pfmt(stdout, MM_ERROR, nomatchingif, "Else");
		return(1);

	case CSEND:
		cond = CRCV;
		break;

	case CRCV:
		cond = CSEND;
		break;

	default:
		pfmt(stdout, MM_ERROR, ":206:Invalid condition encountered\n");
		cond = CANY;
		break;
	}
	return(0);
}

/*
 * End of if statement.  Just set cond back to anything.
 */

endifcmd()
{

	if (cond == CANY) {
		pfmt(stdout, MM_ERROR, nomatchingif, "Endif");
		return(1);
	}
	cond = CANY;
	return(0);
}

/*
 * Set the list of alternate names.
 */
alternates(namelist)
	char **namelist;
{
	register int c;
	register char **ap, **ap2, *cp;

	c = argcount(namelist) + 1;
	if (c == 1) {
		if (altnames == 0)
			return(0);
		for (ap = altnames; *ap; ap++)
			printf("%s ", *ap);
		printf("\n");
		return(0);
	}
	if (altnames != 0)
		free((char *) altnames);
	altnames = (char **) pcalloc((unsigned)(c * sizeof (char *)));
	for (ap = namelist, ap2 = altnames; *ap; ap++, ap2++) {
		cp = (char *) pcalloc((unsigned)(strlen(*ap) + 1));
		strcpy(cp, *ap);
		*ap2 = cp;
	}
	*ap2 = 0;
	return(0);
}
