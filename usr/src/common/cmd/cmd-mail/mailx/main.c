#ident	"@(#)main.c	11.3"
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

#ident "@(#)main.c	1.23 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"
#ifndef preSVr4
#include <locale.h>
# ifdef SVR4ES
#  include <mac.h>
# endif
#endif

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Startup -- interface with user.
 */

static void		hdrstop ARGS((int));

static jmp_buf	hdrjmp;

/*
 * Find out who the user is, copy his mail file (if exists) into
 * /tmp/Rxxxxx and set up the message pointers.  Then, print out the
 * message headers and read user commands.
 *
 * Command line syntax:
 *	mailx [ -i ] [ -r address ] [ -h number ] [ -f [ name ] ]
 * or:
 *	mailx [ -i ] [ -r address ] [ -h number ] people ...
 * or:
 *	mailx -t
 *
 * and a bunch of other options.
 */

main(argc, argv)
	char **argv;
{
	register const char *ef = NOSTR;
	register int argp = -1;
	int mustsend = 0, goerr = 0;
	void (*prevint)();
#ifdef USE_TERMIOS
	struct termios tbuf;
#else
	struct termio tbuf;
#endif
	int c;
	char *cwd = NOSTR, *mf;

	/*
	 * c-client initialization is here.
	 */
#include "../c-client/linkage.c"
	/*
	 * Set up a reasonable environment.
	 * Figure out whether we are being run interactively, set up
	 * all the temporary files, buffer standard output, and so forth.
	 */

#ifndef preSVr4
	(void)setlocale(LC_ALL, "");
# ifdef SVR4ES
	(void)setcat("uxemail");
	(void)setlabel("UX:mailx");
	(void)mldmode(MLD_VIRT);
	if (getenv("POSIX2")) 
		assign("posix2","");
	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;
	maxeucw = wp._eucw1 > wp._eucw2 ?
			wp._eucw1 > wp._eucw3 ?
				wp._eucw1 : wp._eucw3 :
			wp._eucw2 > wp._eucw3 ?
				wp._eucw2 : wp._eucw3 ;
# else
#  ifdef preSVr4
#  else
	{	/* Older versions of gettxt() didn't have setcat(), */
		/* so this has to be done by hand. Unfortunately, to */
		/* do this means we can't use dynamic libraries. */
		extern char *_Msgdb;
		_Msgdb = "uxemail";
	}
#  endif
# endif
#endif
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
	progname = argv[0];
	myegid = getegid();
	myrgid = getgid();
	myeuid = geteuid();
	myruid = getuid();
	mypid = getpid();
	setgid(myrgid);
	setuid(myruid);
	inithost();
	intty = isatty(0);
#ifdef USE_TERMIOS
	if (tcgetattr(1, &tbuf) == 0) {
		outtty = 1;
		baud = cfgetospeed(&tbuf);
	}
#else
	if (ioctl(1, TCGETA, &tbuf)==0) {
		outtty = 1;
		baud = tbuf.c_cflag & CBAUD;
	}
#endif
	else
		baud = B9600;
	image = -1;

	/*
	 * Now, determine how we are being used.
	 * We successively pick off instances of -r, -h, -f, and -i.
	 * If there is anything left, it is the base of the list
	 * of users to mail to.  Argp will be set to point to the
	 * first of these users.
	 */

	while ((c = getopt(argc, argv, "efFh:HinNr:s:u:UdIT:V~tBR:")) != -1)
		switch (c) {
		case 'B':
			/*
			 * Unbuffered input and output.
			 */
			setbuf(stdin, (char*)0);
			setbuf(stdout, (char*)0);
			break;

		case 'd':
			/*
			 * Turn on debugging output.
			 */
			assign("debug", "");
			break;

		case 'e':
			/*
			 * exit status only
			 */
			exitflg++;
			break;

		case 'f':
			/*
			 * use filename instead of $MAIL or /var/mail/user
			 */
			fflag++;
			break;

		case 'F':
			Fflag++;
			mustsend++;
			break;

		case 'h':
			/*
			 * Specified sequence number for network.
			 * This is the number of "hops" made so
			 * far (count of times message has been
			 * forwarded) to help avoid infinite mail loops.
			 */
			mustsend++;
			hflag = atoi(optarg);
			if (hflag == 0) {
				pfmt(stderr, MM_ERROR, 
					":313:-h needs non-zero number\n");
				goerr++;
			}
			break;

		case 'H':
			/*
			 * Print headers and exit
			 */
			Hflag++;
			break;

		case 'i':
			/*
			 * User wants to ignore interrupts.
			 * Set the variable "ignore"
			 */
			assign("ignore", "");
			break;

		case 'n':
			/*
			 * User doesn't want to source
			 *	/etc/mail/mailx.rc
			 */
			nosrc++;
			break;

		case 'N':
			/*
			 * Avoid initial header printing.
			 */
			noheader++;
			break;

		case 'R':
			/*
			 * Next argument is remote hostname where mail
			 * folders will be opened. This implies the IMAP
			 * protocol.
			 */
			/* can't set REMOTEHOST more than once --
			 * the -R flag overrides REMOTEHOST value
			 * set in .mailrc
			 */
			if (value_noenv("REMOTEHOST")) {
				if (strlen((char *)value_noenv("REMOTEHOST")))
				  pfmt(stderr, MM_ERROR, 
				    ":685:Cannot change remotehost: \"%s\"\n",
				    value_noenv("REMOTEHOST"));
				goerr++;
			}
			remotehost = optarg;
			/* this fails if already set */
			assign("REMOTEHOST", remotehost);
			break;
		case 'r':
			/*
			 * Next argument is address to be sent along
			 * to the mailer.
			 */
			mustsend++;
			rflag = optarg;
			break;

		case 's':
			/*
			 * Give a subject field for sending from
			 * non terminal
			 */
			mustsend++;
			sflag = optarg;
			break;

		case 't':
			/*
			 * Use To: fields to determine
			 * recipients.
			 */
			tflag++;
			break;

		case 'T':
			/*
			 * Next argument is temp file to write which
			 * articles have been read/deleted for netnews.
			 */
			{
			int f;
			Tflag = optarg;
			if ((f = creat(Tflag, TEMPPERM)) < 0) {
				pfmt(stderr, MM_ERROR,
					":123:Cannot create %s: %s\n",
					Tflag, Strerror(errno));
				exit(1);
			}
			close(f);
			}
			/* fall through for -I too */
			/* FALLTHROUGH */

		case 'I':
			/*
			 * print newsgroup in header summary
			 */
			newsflg++;
			break;

		case 'u':
			/*
			 * Next argument is person's mailbox to use.
			 * Treated the same as "-f /var/mail/user".
			 */
			/*
			 * with c-client, you can't specify another user's
			 * ibox by user name. The default directory for a
			 * user's mailbox may no longer be /var/mail (or
			 * whatever happens to be in maildir). This means
			 * that unless the users mailbox name is in
			 * /var/mail, or their home directory, c-client
			 * won't have access to it.
			 *
			 * Also, you can't specify the REMOTEHOST value in
			 * conjunction with -u. Folder paths may not make
			 * sense in the remote case.
			 *
			 * -u will override the use of REMOTEHOST in the
			 * .mailrc file.
			 */
			{
			static char u[PATHSIZE];
			/*
			 * if set to non-empty value, then -R was used.
			 * multiple -u's are allowed (for whatever reason).
			 * this is explicitly prevented.
			 */
			if (value_noenv("REMOTEHOST")) {
				/* only an error if set non-empty */
				if (strlen((char *)value_noenv("REMOTEHOST")))
					goerr++;
			}
			else
			    assign("REMOTEHOST", 0);
			strcpy(u, "/var/mail/");
			strncat(u, optarg, PATHSIZE);
			/*
			 * test /var/mail/user first, if not there, look in
			 * the users home directory for .mailbox
			 * After that, fail. We won't go parsing through all
			 * the various config files for the users inbox.
		         * Let mail_open issue the complaints about missing
			 * mail folders.
			 */
			if (!goerr) {
				if (mail_valid(0, u, 0) == NIL) {
				    struct passwd *pw;

				    pw = (struct passwd *)getpwnam(optarg);
				    strncpy(u, (pw)?pw->pw_dir:"", PATHSIZE);
				    if (strlen(u) == 0) {
				        strcpy(u, "/var/mail/");
				        strncat(u, optarg, PATHSIZE);
				    }
				    else
				        strncat(u, "/.mailbox", PATHSIZE);
					if (mail_valid(0, u, 0) == NIL) {
				            strcpy(u, "/var/mail/");
				            strncat(u, optarg, PATHSIZE);
					}
				}
			}
			ef = u;
			break;
			}

		case 'U':
			UnUUCP++;
			break;

		case 'V':
			puts(version);
			return 0;

		case '~':
			/*
			 * Permit tildas no matter where
			 * the input is coming from.
			 */
			escapeokay++;
			break;

		case '?':
		default:
			goerr++;
			break;
		}

	if (fflag) {
		/*
		 * User is specifying file to "edit" with mailx,
		 * as opposed to reading system mailbox (INBOX).
		 * If no argument is given after -f, we read his/her
		 * $MBOX file or mbox in his/her home directory.
		 *
		 * The filename "INBOX" has special meaning to
		 * c-client. The actual path and filename are
		 * determined by configuration files. If we are
		 * trying to read mail from "INBOX" on a remote
		 * system (via IMAP), the path and filename are
		 * determined by the configuration files on the
		 * remote host.
		 * 
		 * Setting cwd when the file is remote is meaningless.
		 * either the full path is specified, or the file
		 * is expected to exist in the remote home directory.
		 *
		 * The initial character '{' is part of the syntax
		 * when specifying remote files to c-client.
		 */
		ef = (argc == optind || *argv[optind] == '-')
			? "" : argv[optind++];
		if (*ef && *ef != '/' && *ef != '+' && *ef != '{' &&
			!remotehost)
				cwd = getcwd(NOSTR, PATHSIZE);
	}

	if ( optind != argc )
		argp = optind;

	/*
	 * Check for inconsistent arguments.
	 */

	if (ef && !fflag && 
	    (strlen((char *)value_noenv("REMOTEHOST")) || remotehost)) {
		pfmt(stderr, MM_ERROR, ":686:Cannot use -u and -R together\n");
		goerr++;
	}
	if (newsflg && ef==NOSTR) {
		pfmt(stderr, MM_ERROR, ":314:Need -f with -I or -T flags\n");
		goerr++;
	}
	if (ef != NOSTR && argp != -1) {
		pfmt(stderr, MM_ERROR, 
			":315:Cannot give -f and people to send to.\n");
		goerr++;
	}
	if (tflag && argp != -1) {
		pfmt(stderr, MM_ERROR, ":472:Cannot give -t and people to send to.\n");
		goerr++;
	}
	if (exitflg && (mustsend || argp != -1))
		exit(1);	/* nonsense flags involving -e simply exit */
	if (mustsend && argp == -1) {
		pfmt(stderr, MM_ERROR, 
			":316:The flags you gave are used only when sending mail.\n");
		goerr++;
	}
	if (goerr) {
		pfmt(stderr, MM_ACTION, 
			":471:Usage: %s [-BeiIUdFnNHV~] [-t] [-T FILE] [-R host] [-u USER] [-h hops] [-r address] [-s SUBJECT] [-f FILE] [users...]\n",
			progname);
		exit(1);
	}

	tinit();
	input = stdin;
	rcvmode = !tflag && (argp == -1);
	if (!nosrc)
		load(MASTER);
	load(Getf("MAILRC"));

	/*
	 * We don't want the user to be able to change the value of
	 * REMOTEHOST, because mailx hasn't been enhanced to handle
	 * multiple connections to different message stores. Allowing
	 * this kind of feature would entail adding a connection
	 * table and better authentication handling. This prevents
	 * us from being able to move message between message stores,
	 * but then mailx isn't designed with this feature in mind.
	 *
	 * The following calls are put here so that we can be sure
	 * that the REMOTEHOST variable is assigned a value. The low
	 * level routines that assign and unassign variables have been
	 * altered to only allow REMOTEHOST to be set once, and never
	 * unset. REMOTEHOST cannot be set from environment variables,
	 * it has to be set from the .mailrc, or the -R flag. If it
	 * isn't set by the time it gets here,we set it to be an empty
	 * string. This will prevent later attempts at setting it from
	 * the internal interactive prompt.
	 */
	/* if remotehost wasn't set by -R */
	if (!remotehost) {
		/* see if REMOTEHOST was set in .mailrc */
		remotehost=(char *)value_noenv("REMOTEHOST");
		/* if not set in .mailrc either, then set it to empty string */
		if (remotehost && strlen(remotehost) == 0)
			remotehost = (char *)0;
		assign("REMOTEHOST", remotehost);
	}
	/* TODO - determine if this is still neccessary, or remove */
	/* reset the mailname, now that we have read in the mailrc */
	/* findmail(); */

	/* check for MBOX environment to make sure the user has not
	 * set it to /var/mail or the "mailbox" directory.
	 *
	 * We don't need to do this any more. /var/mail is now writable.
	 */
	/* Getf("MBOX"); */
	if (tflag) {
		mailt();
		exit(senderr);
	}
	else if (argp != -1) {
		mail(&argv[argp]);
		exit(senderr);
	}
	/* check for mail forwarding in /var/mail/:forward/user */
	/* NOT USED */
	Getf("FORWARD");

	/* If $MAIL is set, and -f has not, use $MAIL for mailbox. */
	mf = getenv("MAIL");
	if ((mf != NOSTR) && (*mf != 0) && (ef == NOSTR))
		ef = mf;

	/*
	 * Ok, we are reading mail.
	 * Decide whether we are editing a mailbox or reading
	 * the system mailbox, and open up the right stuff.
	 */

	strcpy(origname, mailname);

	/* set by specifying either -u, -f, or $MAIL */
	if (ef != NOSTR) {
		ef = *ef ? safeexpand(ef) : Getf("MBOX");
		strcpy(origname, ef);
		if (*ef != '/' && *ef != '{' && !remotehost) {
			if (cwd == NOSTR && cascmp(ef, "INBOX")) {
				cwd = getcwd(NOSTR, PATHSIZE);
				strcat(cwd, "/");
				strcat(cwd, ef);
				ef = cwd;
			}
		}
		/* } match the previous curly brace for editor purposes */
		/* always an absolute path */
		strcpy(mailname, ef);

		if (cascmp(ef, "INBOX")) {
			editfile = ef;
			edit++;
		}
	}
	/* disallow specifying remote files using this syntax */
	if (*ef == '{' ) {
		pfmt(stderr, MM_ERROR,
			":683:Invalid folder specification: %s\n", ef);
		exit(1);
	}
	/* } match the previous curly brace for editor purposes */

	if (setfile(mailname, edit) < 0)
		exit(1);

	/* allow Hflag to print header summary even if noheader is set */
	if (msgCount > 0 && !noheader && ((value("header") != NOSTR) || Hflag)) {
		if (setjmp(hdrjmp) == 0) {
			if ((prevint = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
				sigset(SIGINT, hdrstop);
			announce();
			fflush(stdout);
			sigset(SIGINT, prevint);
		}
	}
	if (Hflag || (!edit && msgCount == 0)) {
		if (!Hflag) {
			pfmt(stderr, MM_NOSTD, hasnomailfor, myname);
			Verhogen();
		}
		fflush(stdout);
		exit(0);
	}
	commands();
	if (!edit) {
		sigset(SIGHUP, SIG_IGN);
		sigset(SIGINT, SIG_IGN);
		sigset(SIGQUIT, SIG_IGN);
		quit();
		Verhogen();
	}
	/* TODO - perhaps mail_close should be done in quit() */
	mail_close_full(itf, CL_EXPUNGE);
	exit(0);
	/* NOTREACHED */
}

/*
 * Interrupt printing of the headers.
 */
/* ARGSUSED */
static void
hdrstop(unused)
int unused;
{
	putchar('\n');
	pfmt(stdout, MM_WARNING, hasinterrupted);
	fflush(stdout);
	sigrelse(SIGINT);
	longjmp(hdrjmp, 1);
}

