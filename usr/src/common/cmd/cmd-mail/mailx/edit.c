#ident	"@(#)edit.c	11.1"
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

#ident "@(#)edit.c	1.17 'attmail mail(1) command'"
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
 * Perform message editing functions.
 */

static void edit1 ARGS((int *msgvec, const char *ed, int allow_binary));

/*
 * Edit a message list.
 */

editor(msgvec)
	int *msgvec;
{
	const char *edname;

	if (((edname = value("EDITOR")) == NOSTR) || (*edname == '\0'))
		edname = EDITOR;
	edit1(msgvec, edname, 0);
	return(0);
}

/*
 * Edit a message list, allowing binary content.
 */

beditor(msgvec)
	int *msgvec;
{
	const char *edname;

	if (((edname = value("EDITOR")) == NOSTR) || (*edname == '\0'))
		edname = EDITOR;
	edit1(msgvec, edname, 1);
	return(0);
}

/*
 * Invoke the visual editor on a message list.
 */

visual(msgvec)
	int *msgvec;
{
	const char *edname;

	if (((edname = value("VISUAL")) == NOSTR) || (*edname == '\0'))
		edname = VISUAL;
	edit1(msgvec, edname, 0);
	return(0);
}

/*
 * Invoke the visual editor on a message list, allowing binary content.
 */

bvisual(msgvec)
	int *msgvec;
{
	const char *edname;

	if (((edname = value("VISUAL")) == NOSTR) || (*edname == '\0'))
		edname = VISUAL;
	edit1(msgvec, edname, 1);
	return(0);
}

/*
 * Edit a message by writing the message into a funny-named file
 * (which should not exist) and forking an editor on it.
 * We get the editor from the stuff above.
 */

static void
edit1(msgvec, ed, allow_binary)
	int *msgvec;
	const char *ed;
{
	pid_t pid;
	int *ip, mesg;
	void (*sigint)(), (*sigquit)();
	FILE *ibuf, *obuf;
	struct message *mp;
	struct stat statb;
	long modtime;

	/*
	 * Set signals; locate editor.
	 */

	sigint = sigset(SIGINT, SIG_IGN);
	sigquit = sigset(SIGQUIT, SIG_IGN);
	ed = safeexpand(ed);

	/*
	 * Deal with each message to be edited . . .
	 */

	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		dot = mp;
		/* make sure message is loaded internally */
		setinput(mp);
		if (mp->m_text != M_binary || allow_binary || value("binaryokay")) {
			int n;				/* size of input line */
			int ctf = FALSE; 		/* header continuation flag */
			int hdr;			/* type of header */
			char linebuf[LINESIZE];

			mp->m_flag |= MODIFY;

			if (!access(tempZedit, 2)) {
				pfmt(stdout, MM_ERROR, filedothexist,
					tempZedit);
				continue;
			}

			/*
			 * Copy the message into the edit file.
			 */

			close(creat(tempZedit, TEMPPERM));
			if ((obuf = fopen(tempZedit, "w")) == NULL) {
				pfmt(stderr, MM_ERROR, badopen,
					tempZedit, Strerror(errno));
				continue;
			}
			if (sendmsg(mp, obuf, 0, 0, 1, 0) < 0) {
				pfmt(stderr, MM_ERROR, badwrite,
					tempZedit, Strerror(errno));
				fclose(obuf);
				removefile(tempZedit);
				continue;
			}
			fflush(obuf);
			if (ferror(obuf)) {
				removefile(tempZedit);
				fclose(obuf);
				continue;
			}
			fclose(obuf);

			/*
			 * If we are in read only mode, make the
			 * temporary message file readonly as well.
			 */

			if (readonly)
				chmod(tempZedit, 0400);

			/*
			 * Fork/execl the editor on the edit file.
			 */

			if (stat(tempZedit, &statb) < 0)
				modtime = 0;
			modtime = statb.st_mtime;
			pid = vfork();
			if (pid == (pid_t)-1) {
				pfmt(stderr, MM_ERROR, failed, "fork",
					Strerror(errno));
				removefile(tempZedit);
				continue;
			}
			if (pid == 0) {
				sigchild();
				if (sigint != SIG_IGN)
					sigset(SIGINT, SIG_DFL);
				if (sigquit != SIG_IGN)
					sigset(SIGQUIT, SIG_DFL);
				execlp(ed, ed, tempZedit, (char *)0);
				pfmt(stderr, MM_ERROR, errmsg, ed,
					Strerror(errno));
				fflush(stderr);
				_exit(1);
			}
			while (wait(&mesg) != pid)
				;

			/*
			 * If in read only mode, just remove the editor
			 * temporary and return.
			 */

			if (readonly) {
				removefile(tempZedit);
				continue;
			}

			/*
			 * Now copy the message to the end of the
			 * temp file.
			 */

			if (stat(tempZedit, &statb) < 0) {
				pfmt(stderr, MM_ERROR,
					":119:Cannot access %s: %s\n",
					tempZedit, Strerror(errno));
				continue;
			}
			if (modtime == statb.st_mtime) {
				removefile(tempZedit);
				continue;
			}
			if ((ibuf = fopen(tempZedit, "r")) == NULL) {
				pfmt(stderr, MM_ERROR, badopen,
					tempZedit, Strerror(errno));
				removefile(tempZedit);
				continue;
			}
			removefile(tempZedit);
#ifndef CCLIENT
			fseek(otf, (long) 0, 2);
			mp->m_offset = fsize(otf);

			mp->m_clen = mp->m_lines = mp->m_size = 0;
			mp->m_text = M_text;
			while ((n = getln(linebuf, sizeof linebuf, ibuf)) > 0) {
				if ((hdr = isheader(linebuf, &ctf)) == FALSE) {
					ctf = FALSE;	/* next line can't be cont. */
				}
				mp->m_size += n;
				if (!hdr && (n > 1)) {
					putc('\n', otf);
					mp->m_size++;
					mp->m_lines++;
				}
				fwrite(linebuf, 1, n, otf);
				if (mp->m_text != M_binary)
					mp->m_text = istext((unsigned char*)linebuf, (long)n, mp->m_text);
				/* partial line */
				if (linebuf[n-1] != '\n') {
					if (!hdr)
						mp->m_clen = n;
					/* get rest of line */
					while ((n = getln(linebuf, sizeof linebuf, ibuf)) > 0) {
						mp->m_size += n;
						if (!hdr)
							mp->m_clen += n;
						fwrite(linebuf, 1, n, otf);
						if (mp->m_text != M_binary)
							mp->m_text = istext((unsigned char*)linebuf, (long)n, mp->m_text);
						if (linebuf[n-1] == '\n')
							break;
					}
				} else if (!hdr) {
					mp->m_clen = n;
				}
				mp->m_lines++;
				if (!hdr)
					break;
			}

			if (n > 0) {	/* at non-header, read rest of content */
				while ((n = getln(linebuf, sizeof(linebuf), ibuf)) > 0) {
					mp->m_size += n;
					mp->m_clen += n;
					fwrite(linebuf, 1, n, otf);
					if (mp->m_text != M_binary)
						mp->m_text = istext((unsigned char*)linebuf, (long)n, mp->m_text);
					/* partial line */
					if (linebuf[n-1] != '\n') {
						/* get rest of line */
						while ((n = getln(linebuf, sizeof linebuf, ibuf)) > 0) {
							mp->m_size += n;
							mp->m_clen += n;
							fwrite(linebuf, 1, n, otf);
							if (mp->m_text != M_binary)
								mp->m_text = istext((unsigned char*)linebuf, (long)n, mp->m_text);
							if (linebuf[n-1] == '\n')
								break;
						}
					}
					mp->m_lines++;
					if (ferror(otf))
						break;
				}
			}

			if (ferror(otf))
				pfmt(stderr, MM_ERROR, errmsg, "/tmp", Strerror(errno));
#endif /* !CCLIENT */
			fclose(ibuf);
		} else {
			putchar('\n');
			pfmt(stdout, MM_NOSTD, ":524:*** Message content is not printable ***\n*** Use \"bedit\" or \"bvisual\" to edit message ***\n");
		}
	}

	/*
	 * Restore signals and return.
	 */

	sigset(SIGINT, sigint);
	sigset(SIGQUIT, sigquit);
}
