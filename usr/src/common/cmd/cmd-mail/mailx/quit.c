#ident	"@(#)quit.c	11.1"
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

#ident "@(#)quit.c	1.20 'attmail mail(1) command'"
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
 * Rcv -- receive mail rationally.
 *
 * Termination processing.
 */

static void		writeback ARGS((void));
static int		copyowner ARGS((const char *, const char *));

static
chkmsgseq(seq)
char *seq;
{
	int *msgvec = (int *)salloc(sizeof(int *) * (msgCount + 2));
	char *msgseq = (char *)salloc(strlen(seq) + 1);

	if (seq) {
		register char *cp = seq, *cp1 = msgseq;
		memset(msgseq, '\0', strlen(seq) + 1);
		while (*cp)
			if (*cp != ',')
				*cp1++ = *cp++;
			else
				cp++;
	}

	if (seq && *seq) {
		register int *ip, mesg;
		register struct message *mp;

		getmsglist(msgseq, msgvec, 0);
		for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
			mesg = *ip;
			mp = &message[mesg-1];
			if (mp->m_flag & MUSED)
				printf("%d) loaded", mesg);
			else
				printf("%d)", mesg);
			if (mp->m_flag & MDELETED)
				printf(" deleted\n");
			else
				printf("\n");
		}
	}
}

static
delmsgseq(seq)
char *seq;
{
	int *msgvec = (int *)salloc(sizeof(int *) * (msgCount + 2));
	char *msgseq = (char *)salloc(strlen(seq) + 1);

	if (seq) {
		register char *cp = seq, *cp1 = msgseq;
		memset(msgseq, '\0', strlen(seq) + 1);
		while (*cp)
			if (*cp != ',')
				*cp1++ = *cp++;
			else
				cp++;
	}

	if (seq && *seq) {
		register int *ip, mesg;
		register struct message *mp;

		getmsglist(msgseq, msgvec, 0);
		for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
			mesg = *ip;
			mp = &message[mesg-1];
			mp->m_flag |= MDELETED;
		}
	}
}
/*
 * Save all of the undetermined messages at the top of "mbox"
 * Save all untouched messages back in the system mailbox.
 * Remove the system mailbox, if none saved there.
 */

void
quit()
{
	int mboxcount, p, modify, autohold, anystat, holdbit, nohold;
	FILE *readstat;
	register struct message *mp;
	char *id;
	int appending;
	const char *mbox = Getf("MBOX");
	MAILSTREAM *ibuf, *obuf, *fbuf;
	char *seq = (char *)salloc(LINESIZE + 1);
	int seqsz = LINESIZE;
	char *tmpmbox, *mboxname;

	/*
	 * If we are read only, we can't do anything,
	 * so just return quickly.
	 */

	if (readonly)
		return;

	/*
	 * See if there any messages to save in mbox.  If no, we
	 * can save copying mbox to /tmp and back.
	 *
	 * Check also to see if any files need to be preserved.
	 * Delete all untouched messages to keep them out of mbox.
	 * If all the messages are to be preserved, just exit with
	 * a message.
	 *
	 * If the luser has sent mail to himself, refuse to do
	 * anything with the mailbox, unless mail locking works.
	 */

#ifndef CANLOCK
	if (selfsent) {
		pfmt(stdout, MM_NOSTD, ":324:You have new mail.\n");
		return;
	}
#endif

	if (*mbox == '{' ) {
		pfmt(stderr, MM_ERROR,
			":683:Invalid folder specification: %s\n", mbox);
		return;
	}
	/* } match the previous curly brace for editor purposes */

	memset(seq, '\0', LINESIZE + 1);

	/*
	 * Adjust the message flags in each message.
	 */

	anystat = 0;
	autohold = value("hold") != NOSTR;
	appending = value("append") != NOSTR;
	holdbit = autohold ? MPRESERVE : MBOX;
	nohold = MBOX|MSAVED|MDELETED|MPRESERVE;
	if (value("keepsave") != NOSTR)
		nohold &= ~MSAVED;
	modify = 0;
	mboxcount = 0;
	p = 0;
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		setflags(mp);
		if (mp->m_flag & MNEW) {
			receipt(mp);
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & MSTATUS)
			anystat++;
		if ((mp->m_flag & MTOUCH) == 0)
			mp->m_flag |= MPRESERVE;
		if ((mp->m_flag & nohold) == 0)
			mp->m_flag |= holdbit;
		if ((mp->m_flag & MPRESERVE) == 0) {
			char flagseq[25];

			sprintf(flagseq, "%d", mp - &message[0] + 1);
			if (mp->m_flag & (MSAVED|MDELETED)) {
				mail_setflag(itf, flagseq, "\\DELETED");
			}
			if (mp->m_flag & MREAD)
				mail_setflag(itf, flagseq, "\\SEEN");
		}

		if (mp->m_flag & MBOX) {
			char tmpnum[256];

			mboxcount++;
			sprintf(tmpnum, "%d", (mp - &message[0]) + 1);
			if ((strlen(tmpnum) + strlen(seq) + 3) > seqsz) {
				seq = (char *)srealloc(seq, seqsz + LINESIZE + 1);
				memset(&seq[seqsz], '\0', LINESIZE + 1);
				seqsz += LINESIZE;
			}
			if (strlen(seq))
				strcat(seq, ", ");
			strcat(seq, tmpnum);
		}
		if (mp->m_flag & MPRESERVE)
			p++;
		if (mp->m_flag & MODIFY)
			modify++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			id = hfield("message-id", mp, addone);
			if (id != NOSTR)
				fprintf(readstat, "%s\n", id);
			else {
				id = hfield("article-id", mp, addone);
				if (id != NOSTR)
					fprintf(readstat, "%s\n", id);
			}
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (p == msgCount && !modify && !anystat) {
		if (p == 1)
			pfmt(stdout, MM_NOSTD, heldonemsg, mailname);
		else
			pfmt(stdout, MM_NOSTD, heldmsgs, p, mailname);
		/* TODO - make sure mailbox is closed */
		return;
	}
	if (mboxcount == 0) {
		writeback();
		/* TODO - make sure mailbox gets closed */
		return;
	}

	ibuf = setinput(0);

	/*
	 * Create another temporary file and copy user's mbox file
	 * therein.  If there is no mbox, copy nothing.
	 * If s/he has specified "append" don't copy the mailbox,
	 * just copy saveable entries at the end.
	 */

	if (remotehost) {
		mboxname = salloc(strlen(remotehost) + 2 + strlen(mbox) + 1);
		sprintf(mboxname, "{%s}%s", remotehost, mbox);
		tmpmbox = salloc(strlen(remotehost) + 2 + strlen(tempQuit) + 1);
		sprintf(tmpmbox, "{%s}%s", remotehost, tempQuit);
	}
	else {
		mboxname = salloc(strlen(mbox) + 1);
		sprintf(mboxname, "%s", mbox);
		tmpmbox = salloc(strlen(tempQuit) + 1);
		sprintf(tmpmbox, "%s", tempQuit);
	}
	if (!appending) {
		if (mail_create(ibuf, (char *)mboxname) == NIL) {
			if (mail_rename(ibuf, (char *)mboxname, tmpmbox) == NIL)
			{
				pfmt(stderr, MM_ERROR, ":260:badwrite 1 %s: %s\n",
					tmpmbox, Strerror(errno));
				return;
			}
			if (mail_create(ibuf, (char *)mboxname) == NIL) {
				pfmt(stderr, MM_ERROR, ":2:badopen 2 %s: %s\n",
					mboxname, Strerror(errno));
				if (mail_rename(ibuf,
					(char *)mboxname, tmpmbox) == NIL) {

					pfmt(stderr, MM_ERROR, ":260:badwrite 2 %s: %s\n",
						tmpmbox, Strerror(errno));
				}
				return;
			}
		}
	}

	if (mail_move(ibuf, seq, (char *)mbox) == NIL) {
		/* TODO * what happens to tmpmbox? */
		/* If failure, can I just restore mbox? */
		pfmt(stderr, MM_ERROR, badwrite, 
			mbox, Strerror(errno));
	}

	/* mark the internal list of messages as deleted */
	/* chkmsgseq(seq); */
	/* delmsgseq(seq); */

	/*
	 * Copy the user's old mbox contents back
	 * to the end of the stuff we just saved.
	 * If we are appending, this is unnecessary.
	 */

	if (!appending) {
		/*
		 * We need to open another MAILSTREAM to be able to move
		 * messages from the temporary mbox back into mbox. This
		 * will require a reauthentication if we don't do something
		 * to save the authentication info from the open of the
		 * current mail folder.
		 *
		 * Leave it as is for now, we'll come back and revisit
		 * later. TODO
		 */
		ibuf = mail_open(NIL, (char *)tmpmbox, 0);
		if (ibuf != NIL) {
			if (ibuf->nmsgs) {
				sprintf(seq, "1:%d", ibuf->nmsgs);
				if (mail_move(ibuf, seq, (char *)mbox) == NIL) {
					/* TODO * what happens to mbox? */
					pfmt(stderr, MM_ERROR, badwrite, 
						mbox, Strerror(errno));

					/* remove the temporary work file */
					mail_delete(itf, tmpmbox);
					return;
				}
				mail_expunge(ibuf);
			}
			mail_close(ibuf);
		}
	}
	if (mboxcount == 1)
		pfmt(stdout, MM_NOSTD, ":325:Saved 1 message in %s\n",
			mboxname);
	else
		pfmt(stdout, MM_NOSTD, ":326:Saved %d messages in %s\n",
			mboxcount, mboxname);

	/*
	 * Now we are ready to copy back preserved files to
	 * the system mailbox, if any were requested.
	 */
	writeback();

	/* make sure we remove the temporary work file */
	mail_delete(itf, tmpmbox);
}

/*
 * Copy the mode from one file to another.
 */
static int
copymode(newfile, oldfile)
const char *newfile, *oldfile;
{
	struct stat statb;
	if (stat(oldfile, &statb) == -1)
		return 0;
	if (chmod(newfile, statb.st_mode) == -1)
		return 0;
	if (chown(newfile, statb.st_uid, statb.st_gid) == -1)
		return 0;
	return 1;
}

/*
 * Copy the mode from one file to another.
 */
static int
copyowner(newfile, oldfile)
const char *newfile, *oldfile;
{
	struct stat statb;
	if (stat(oldfile, &statb) == -1)
		return 0;
	if (chown(newfile, statb.st_uid, statb.st_gid) == -1)
		return 0;
	return 1;
}

/*
 * Preserve all the appropriate messages back in the system
 * mailbox, and print a nice message indicated how many were
 * saved.  Incorporate any new mail that we found.
 */
static void
writeback()
{
	MAILSTREAM *sbuf = 0;
	register struct message *mp;
	register int copybackcnt, success = 0, havenewmail = 0;
	void (*fhup)(), (*fint)(), (*fquit)(), (*fpipe)();
	unsigned long oldNewMsgCnt = ulgNewMsgCnt;

	fhup = sigset(SIGHUP, SIG_IGN);
	fint = sigset(SIGINT, SIG_IGN);
	fquit = sigset(SIGQUIT, SIG_IGN);
	fpipe = sigset(SIGPIPE, SIG_IGN);

	sbuf = setinput(0);
	mail_expunge(sbuf);
	mail_status(sbuf, sbuf->mailbox, SA_MESSAGES | SA_RECENT);
	havenewmail = (ulgNewMsgCnt > oldNewMsgCnt);
	copybackcnt = ulgMsgCnt;
	if (havenewmail)
		pfmt(stdout, MM_NOSTD, newmailarrived);
	if (copybackcnt == 1)
		pfmt(stdout, MM_NOSTD, heldonemsg, mailname);
	else if (copybackcnt > 1)
		pfmt(stdout, MM_NOSTD, heldmsgs, copybackcnt, mailname);

	/* remove empty mailboxes */
	if ((sbuf->nmsgs == 0) && (value("keep") == NOSTR))
		if (mail_valid(sbuf, sbuf->mailbox, 0))
			(void) PRIV(delempty(MFMODE, sbuf));
	/* TODO - should we notify deletion? what if this is INBOX? */

	/* mail_close(sbuf); */
	/* itf = (MAILSTREAM *)0; */
	
	sbuf = 0;
	success = 1;

die:

	/* reset our signals */
	sigset(SIGHUP, fhup);
	sigset(SIGINT, fint);
	sigset(SIGQUIT, fquit);
	sigset(SIGPIPE, fpipe);
}
