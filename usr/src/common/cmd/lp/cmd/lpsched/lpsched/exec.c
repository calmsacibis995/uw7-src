/*		copyright	"%c%" 	*/


#ident	"@(#)exec.c	1.2"
#ident  "$Header$"

#include <limits.h>
#ifdef	__STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <sys/types.h>
#include <priv.h>
#include <wait.h>

#ifdef	NETWORKING
#include <dial.h>
#endif

#include "lpsched.h"

#define WHO_AM_I        I_AM_LPSCHED
#include "oam.h"
  
#define Done(EC,ERRNO)	done(((EC) << 8),ERRNO)

static MESG *		ChildMd;

static int		ChildPid;
static int		WaitedChildPid;
static int		slot;

#ifdef	NETWORKING
static int		do_undial = 0;
#endif

static char		argbuf[ARG_MAX];

static long		SpoolerKey;
static long		BannerKey;
static int		JobSeqNo;

#ifdef	__STDC__

static void		sigtrap ( int );
static void		done ( int , int );
static void		cool_heels ( void );
static void		addenv ( char * , char * );
static void		trap_fault_signals ( void );
static void		ignore_fault_signals ( void );
static void		child_mallocfail ( void );
static void		Fork2 (void);

static int		Fork1 ( EXEC * );

#else

static void		sigtrap();
static void		done();
static void		cool_heels();
static void		addenv();
static void		trap_fault_signals();
static void		ignore_fault_signals();
static void		child_mallocfail();
static void		Fork2();

static int		Fork1();

#endif


/*
 * Procedure:     exec
 *
 * Restrictions:
 *               Open: None
 *               lvlproc(2): None
 *               seteuid(2): None
 *               Chown: None
 *               setgid(2): None
 *               setuid(2): None
 *               execv: None
 * Notes - FORK AND EXEC CHILD PROCESS
 */

/*VARARGS1*/
int
#ifdef	__STDC__
exec (
	int			type,
	...
)
#else
exec (type, va_alist)
	int			type;
	va_dcl
#endif
{
	DEFINE_FNNAME (exec)

	va_list			args;

	int			i	= 0;
	uid_t			procuid	= 0;
	gid_t			procgid	= 0;
	level_t			proclid	= 0;
	int			ret	= 0;

	char *			cp	= (char *) 0;
	char *			infile	= (char *) 0;
	char *			outfile	= (char *) 0;
	char *			errfile	= (char *) 0;
	char *			sep	= (char *) 0;

	char **			argvp	  = (char **) 0;
	char **			envp	  = (char **) 0;
	char **			listp	  = (char **) 0;
	char **			file_list = (char **) 0;

	PSTATUS *		printer	= (PSTATUS *) 0;
	RSTATUS *		request	= (RSTATUS *) 0;
	FSTATUS *		form	= (FSTATUS *) 0;
	EXEC *			ep	= (EXEC *) 0;
	PWSTATUS *		pwheel	= (PWSTATUS *) 0;


	ENTRYP
#ifdef	__STDC__
	va_start (args, type);
#else
	va_start (args);
#endif

	switch (type) {

	case EX_INTERF:
		printer = va_arg(args, PSTATUS *);
		if (printer->status & PS_REMOTE) {
			errno = EINVAL;
			return (-1);
		}
		request = printer->request;
		ep = printer->exec;
		break;

	case EX_SLOWF:
		request = va_arg(args, RSTATUS *);
		ep = request->exec;
		break;

	case EX_NOTIFY:
		request = va_arg(args, RSTATUS *);
		if (request->request->actions & ACT_NOTIFY) {
			errno = EINVAL;
			return (-1);
		}
		ep = request->exec;
		break;

	case EX_ALERT:
		printer = va_arg(args, PSTATUS *);
		if (!(printer->printer->fault_alert.shcmd)) {
			errno = EINVAL;
			return(-1);
		}
		ep = printer->alert->exec;
		break;

	case EX_PALERT:
		pwheel = va_arg(args, PWSTATUS *);
		TRACEx (pwheel)
		TRACEx (pwheel->alert->exec)
		ep = pwheel->alert->exec;
		break;

	case EX_FALERT:
		form = va_arg(args, FSTATUS *);
		ep = form->alert->exec;
		break;

	default:
		errno = EINVAL;
		return(-1);

	}
	va_end (args);

	if (ep->pid > 0)
	{
		errno = EBUSY;
		return(-1);
	}
	if (printer)
	{
		JobSeqNo = printer->job_seq_no++;
		if (printer->job_seq_no > 9)
			printer->job_seq_no = 0;
		BannerKey = getkey() % 100000;
		if (BannerKey < 10000)
			BannerKey += 10000;
	}
	else
	{
		JobSeqNo = -1;
		BannerKey = -1;
	}
	ep->flags = 0;

	SpoolerKey = ep->key = getkey();
	slot = ep - Exec_Table;

TRACEP ("Before Fork1")
PrintProcPrivs ();

	switch ((ep->pid = Fork1(ep))) {

	case -1:
		return(-1);

	case 0:
		/*
		 * We want to be able to tell our parent how we died.
		 */
		lp_alloc_fail_handler = child_mallocfail;
		break;

	default:

TRACEP ("After Fork1");
PrintProcPrivs ();

		switch(type) {

		case EX_INTERF:
			request->request->outcome |= RS_PRINTING;
			CutStartJobAuditRec (0, 1, request->secure->user,
				request->secure->req_id);
			break;

		case EX_NOTIFY:
			request->request->outcome |= RS_NOTIFYING;
			break;

		case EX_SLOWF:
			request->request->outcome |= RS_FILTERING;
			request->request->outcome &= ~RS_REFILTER;
			CutStartJobAuditRec (0, 2, request->secure->user,
				request->secure->req_id);
			break;

		}
		return(0);

	}

	for (i = 0; i < NSIG; i++)
		(void)signal (i, SIG_DFL);
	(void)signal (SIGALRM, SIG_IGN);
	(void)signal (SIGTERM, sigtrap);
	
	for (i = 0; i < OpenMax; i++)
		if (i != ChildMd->writefd)
			(void) Close (i);

TRACEP ("Before setpgrp")
PrintProcPrivs ();

	(void)setpgrp();

TRACEP ("After setpgrp")
PrintProcPrivs ();

	(void) sprintf ((cp = BIGGEST_NUMBER_S), "%ld", SpoolerKey);
	addenv ("SPOOLER_KEY", cp);

	if (printer)
	{
		(void) sprintf ((cp = BIGGEST_NUMBER_S), "%d", JobSeqNo);
		addenv ("JOB_SEQ_NO", cp);
		(void) sprintf ((cp = BIGGEST_NUMBER_S), "%ld", BannerKey);
		addenv ("BANNER_KEY", cp);
	}

#ifdef	DEBUG
	addenv ("LPDEBUG", (debug? "1" : "0"));
#endif

	/*
	 * Open the standard input, standard output, and standard error.
	 */
	switch (type) {
		
	case EX_SLOWF:
	case EX_INTERF:
		/*
		 * stdin:  /dev/null
		 * stdout: /dev/null (EX_SLOWF), printer port (EX_INTERF)
		 * stderr: req#
		 */
		infile = 0;
		outfile = 0;
		errfile = makereqerr(request);
		break;

	case EX_NOTIFY:
		/*
		 * stdin:  req#
		 * stdout: /dev/null
		 * stderr: /dev/null
		 */
		infile = makereqerr(request);
		outfile = 0;
		errfile = 0;

		break;

	case EX_ALERT:
	case EX_FALERT:
	case EX_PALERT:
		/*
		 * stdin:  /dev/null
		 * stdout: /dev/null
		 * stderr: /dev/null
		 */
		infile = 0;
		outfile = 0;
		errfile = 0;
		break;

	}

	if (infile) {
		if (Open(infile, O_RDONLY, 0) == -1)
			Done (EXEC_EXIT_NOPEN, errno);
	} else {
		if (Open("/dev/null", O_RDONLY, 0) == -1)
			Done (EXEC_EXIT_NOPEN, errno);
	}

	if (outfile) {
		if (Open(outfile, O_CREAT|O_TRUNC|O_WRONLY, 0600) == -1)
			Done (EXEC_EXIT_NOPEN, errno);
	} else {
		/*
		 * If EX_INTERF, this is still needed to cause the
		 * standard error channel to be #2.
		 */
		if (Open("/dev/null", O_WRONLY, 0) == -1)
			Done (EXEC_EXIT_NOPEN, errno);
	}

	if (errfile)
	{
		if (Open(errfile, O_CREAT|O_TRUNC|O_WRONLY, 0600) == -1)
			Done (EXEC_EXIT_NOPEN, errno);
		(void)	Chown (errfile, Lp_Uid, Lp_Gid);
	}
	else {
		if (Open("/dev/null", O_WRONLY, 0) == -1)
			Done (EXEC_EXIT_NOPEN, errno);
	}

	switch (type) {

	case EX_INTERF:
		/*
		 * Opening a ``port'' can be dangerous to our health:
		 *
		 *	- Hangups can occur if the line is dropped.
		 *	- The printer may send an interrupt.
		 *	- A FIFO may be closed, generating SIGPIPE.
		 *
		 * We catch these so we can complain nicely.
		 */
		trap_fault_signals ();

		(void) Close (1);

		/*
		**  ES Note:
		**  We are about to open the device so it is time
		**  to become the user.  Everything but our privs.
		**  The open_dialup/direct will manipulate them
		**  as needed.
		**
		**  In the case of a remote request (NETWORKING)
		**  we do everything as LP except for the level.
		**  This avoids the server system from having to
		**  every user on the client systems.  Levels
		**  must still be mapped properly.
		*/
#ifdef	NETWORKING
		if (strchr (request->secure->user, '!'))
		{
			procuid = Lp_Uid;
			procgid = Lp_Gid;
		}
		else
		{
			procuid = request->secure->uid;
			procgid = request->secure->gid;
		}
		proclid = request->secure->lid;
#else
		procuid = request->secure->uid;
		procgid = request->secure->gid;
		proclid = request->secure->lid;

#endif	/*  NETWORKING  */

TRACEP ("Before lvlproc")
PrintProcPrivs ();

		(void)	lvlproc (MAC_SET, &proclid);

TRACEP ("Before setegid")
PrintProcPrivs ();

		(void)	setegid (procgid);

TRACEP ("Before seteuid")
PrintProcPrivs ();

		(void)	seteuid (procuid);

TRACEP ("Before procprivl")
PrintProcPrivs ();

		(void)	procprivl (SETPRV, ALLPRIVS_W, (level_t) 0);

PrintProcPrivs ();


		if (printer->printer->dial_info) {
#ifdef	NETWORKING
			ret = open_dialup(request->printer_type,
				printer->printer);
			if (ret == 0)
				do_undial = 1;
#else
			ret = 0;
#endif	/*  NETWORKING  */
		} else {
			ret = open_direct(request->printer_type,
				printer->printer);
		}

		/*
		 * Save errno that we are concerned with and
		 * ignore errno set by voided routines
		*/
		i = errno;
		(void)	lvlproc (MAC_SET, &Lp_Lid);
		(void)	setegid (Lp_Gid);
		(void)	seteuid (Lp_Uid);
		(void)	procprivl (SETPRV, ALLPRIVS_W, (level_t) 0);
		errno = i; 
		if (ret != 0)
			Done (ret, errno);
			
		if (!(request->request->outcome & RS_FILTERED))
			file_list = request->request->file_list;

		else {
			register int		count	= 0;
			register char *		num	= BIGGEST_REQID_S;
			register char *		prefix;

			prefix = makestr(
				Lp_Temp,
				"/F",
				getreqno(request->secure->req_id),
				"-",
				(char *)0
			);

			file_list = (char **)Malloc(
				(lenlist(request->request->file_list) + 1)
			      * sizeof(char *)
			);

			for (
				listp = request->request->file_list;
				*listp;
				listp++
			) {
				(void) sprintf (num, "%d", count + 1);
				file_list[count] = makestr(
					prefix,
					num,
					(char *)0
				);
				count++;
			}
			file_list[count] = 0;
		}

		if (request->printer_type)
			addenv("TERM", request->printer_type);

		if (!(printer->printer->daisy)) {
			register char *	chset = 0;
			register char *	csp;

			if (
				request->form
			     && request->form->form->chset
			     && request->form->form->mandatory
			     && !STREQU(NAME_ANY, request->form->form->chset)
			)
				chset = request->form->form->chset;

			else if (
				request->request->charset
			     && !STREQU(NAME_ANY, request->request->charset)
			)
				chset = request->request->charset;
			else {
			/*
			* Parse out the locale value here so
			* we have it to pass as LANG and get the default we
			* need to pass as CHARSET. 
			*/

			register char		**list,
						**pl,
						*locale;

			if (
				request->request->options
		     	&& (list = dashos(request->request->options))
			) {
			    for (pl = list ; *pl; pl++)
				if (STRNEQU(*pl, "locale=", 7))
				{
					locale = Strdup((*pl + 7));
					addenv("LANG", locale);
					pl = get_charset((locale));
					chset = Strdup (*pl);
					freelist (pl);
					Free (locale);
					break;
				}
				freelist (list);
		   	   }
			}

			if (chset) {
				csp = search_cslist(
					chset,
					printer->printer->char_sets
				);

				/*
				 * The "strtok()" below wrecks the string
				 * for future use, but this is a child
				 * process where it won't be needed again.
				 */
				addenv (
					"CHARSET",
					(csp? strtok(csp, "=") : chset)
				);
			}
		}

		if (request->fast)
			addenv("FILTER", request->fast);

		/*
		*/
		TRACEs (request->secure->user)
		TRACEs (request->request->user)
		if (strcmp (request->secure->user, request->request->user))
		{
			addenv ("ALIAS_USERNAME", request->request->user);
		}
		/*
		 * Add the system name to the user name (ala system!user)
		 * unless it is already there. RFS users may have trouble
		 * here, sorry!
		 */
		cp = strchr(request->secure->user, BANG_C);
		(void) sprintf (argbuf, "%s/%s",
			Lp_A_Interfaces,
			printer->printer->name);
		addlist2 (&argvp, argbuf);
		(void) sprintf (argbuf, "%s",
			request->secure->req_id);
		addlist2 (&argvp, argbuf);
		(void) sprintf (argbuf, "%s%s%s",
			(cp? "" : request->secure->system),
			(cp? "" : BANG_S),
			request->secure->user);
		addlist2 (&argvp, argbuf);
		(void) sprintf (argbuf, "%s",
			NB(request->request->title));
		addlist2 (&argvp, argbuf);
		(void) sprintf (argbuf, "%d",
			request->copies);
		addlist2 (&argvp, argbuf);

		/*
		 * Do the administrator defined ``stty'' stuff before
		 * the user's -o options, to allow the user to override.
		 */
		argbuf[0] = '\0';
		sep = "";

		if (printer->printer->stty)
		{
			(void) strcat (argbuf, sep);	sep = " ";
			(void) strcat (argbuf, "stty='");
			(void) strcat (argbuf, printer->printer->stty);
			(void) strcat (argbuf, "'");
		}
		/*
		 * Do all of the user's options except the cpi/lpi/etc.
		 * stuff, which is done separately.
		 */
		if (request->request->options)
		{
			listp = dashos(request->request->options);
			while (listp != NULL && *listp)
			{
				if (
					!STRNEQU(*listp, "cpi=", 4)
				     && !STRNEQU(*listp, "lpi=", 4)
				     && !STRNEQU(*listp, "width=", 6)
				     && !STRNEQU(*listp, "length=", 7)
				     && !STRNEQU(*listp, "locale=", 7)
				)
				{
					(void) strcat (argbuf, sep);
					sep = " ";
					(void) strcat (argbuf, *listp);
				}
				listp++;
			}
		}
		/*
		 * The "pickfilter()" routine (from "validate()")
		 * stored the cpi/lpi/etc. stuff that should be
		 * used for this request. It chose form over user,
		 * and user over printer.
		 */
		if (request->cpi)
		{
			(void) strcat (argbuf, sep);	sep = " ";
			(void) strcat (argbuf, "cpi=");
			(void) strcat (argbuf, request->cpi);
		}
		if (request->lpi)
		{
			(void) strcat (argbuf, sep);	sep = " ";
			(void) strcat (argbuf, "lpi=");
			(void) strcat (argbuf, request->lpi);
		}
		if (request->pwid)
		{
			(void) strcat (argbuf, sep);	sep = " ";
			(void) strcat (argbuf, "width=");
			(void) strcat (argbuf, request->pwid);
		}
		if (request->plen)
		{
			(void) strcat (argbuf, sep);	sep = " ";
			(void) strcat (argbuf, "length=");
			(void) strcat (argbuf, request->plen);
		}
		/*
		 * Do the ``raw'' bit last, to ensure it gets
		 * done. If the user doesn't want this, then he or
		 * she can do the correct thing using -o stty=
		 * and leaving out the -r option.
		 */
		if (request->request->actions & ACT_RAW)
		{
			(void) strcat (argbuf, sep); sep = " ";
			(void) strcat (argbuf, "stty=-opost");
		}
		addlist2 (&argvp, argbuf);

		for (listp = file_list; *listp; listp++)
		{
			addlist2 (&argvp, *listp);
		}
		/*
		**  ES Note:
		**  This is OK.
		**  Leave the MAC-level alone.
		*/
		(void)	chfiles2 (file_list, procuid, procgid);

		break;


	case EX_SLOWF:
		if (request->slow)
			addenv("FILTER", request->slow);
		/*
		**  ES Note:
		**  In the case of a remote request (NETWORKING)
		**  we do everything as LP except for the level.
		**  This avoids the server system from having to
		**  every user on the client systems.  Levels
		**  must still be mapped properly.
		*/
#ifdef	NETWORKING
		if (strchr (request->secure->user, '!'))
		{
			procuid = Lp_Uid;
			procgid = Lp_Gid;
		}
		else
		{
			procuid = request->secure->uid;
			procgid = request->secure->gid;
		}
		proclid = request->secure->lid;
#else
		procuid = request->secure->uid;
		procgid = request->secure->gid;
		proclid = request->secure->lid;

#endif	/*  NETWORKING  */

		cp = _alloc_files(
			lenlist(request->request->file_list),
			getreqno(request->secure->req_id),
			procuid,
			procgid,
			proclid
		);
		(void) sprintf (argbuf, "%s", Lp_Slow_Filter);
		addlist2 (&argvp, argbuf);
		(void) sprintf (argbuf, "%s/%s", Lp_Temp, cp);
		addlist2 (&argvp, argbuf);
		for (listp = request->request->file_list; *listp; listp++)
		{
			addlist2 (&argvp, *listp);
		}
		/*
		**  ES Note:
		**  This is OK.
		**  Leave the MAC-level alone.
		*/
		(void)	chfiles (request->request->file_list, procuid,
				procgid);
		break;

	case EX_ALERT:
		/*
		**  ES Note:
		**  This is OK.
		**  The MAC-level can stay as is.
		*/
		procuid = Lp_Uid;
		procgid = Lp_Gid;
		(void)Chown (printer->alert->msgfile, procuid, procgid);

		(void) sprintf (argbuf, "%s/%s/%s",
			Lp_A_Printers,
			printer->printer->name,
			ALERTSHFILE);
		addlist2 (&argvp, argbuf);
		(void) sprintf (argbuf, "%s", printer->alert->msgfile);
		addlist2 (&argvp, argbuf);
		break;

	case EX_PALERT:
		/*
		**  ES Note:
		**  This is OK.
		**  The MAC-level can stay as is.
		*/
		procuid = Lp_Uid;
		procgid = Lp_Gid;
		(void)Chown (pwheel->alert->msgfile, procuid, procgid);

		(void) sprintf (argbuf, "%s/%s/%s",
			Lp_A_PrintWheels,
			pwheel->pwheel->name,
			ALERTSHFILE);
		addlist2 (&argvp, argbuf);
		(void) sprintf (argbuf, "%s", pwheel->alert->msgfile);
		addlist2 (&argvp, argbuf);
		break;

	case EX_FALERT:
		/*
		**  ES Note:
		**  This is OK.
		**  The MAC-level can stay as is.
		*/
		procuid = Lp_Uid;
		procgid = Lp_Gid;
		(void)Chown (form->alert->msgfile, procuid, procgid);

		(void) sprintf (argbuf, "%s/%s/%s",
			Lp_A_Forms,
			form->form->name,
			ALERTSHFILE);
		addlist2 (&argvp, argbuf);
		(void) sprintf (argbuf, "%s", form->alert->msgfile);
		addlist2 (&argvp, argbuf);
		break;

	case EX_NOTIFY:
		addlist2 (&argvp, SHELL);
		addlist2 (&argvp, "-c");
		if (request->request->alert)
		{
			/*
			**  ES Note:
			**  In the case of a remote request (NETWORKING)
			**  we do everything as LP except for the level.
			**  This avoids the server system from having to
			**  every user on the client systems.  Levels
			**  must still be mapped properly.
			*/
#ifdef	NETWORKING
			if (strchr (request->secure->user, '!'))
			{
				procuid = Lp_Uid;
				procgid = Lp_Gid;
			}
			else
			{
				procuid = request->secure->uid;
				procgid = request->secure->gid;
			}
			proclid = request->secure->lid;
#else
			procuid = request->secure->uid;
			procgid = request->secure->gid;
			proclid = request->secure->lid;

#endif	/*  NETWORKING  */

			addlist2 (&argvp, request->request->alert);
		}
		else
		{
			/*
			**  ES Note:
			**  Keep the originators MAC-level.
			*/
			procuid = Lp_Uid;
			procgid = Lp_Gid;
			proclid = request->secure->lid;

			cp = strchr(request->secure->user, BANG_C);

			if ((request->request->actions & ACT_WRITE) &&
			    (!request->secure->system ||
			     STREQU(request->secure->system, Local_System)))
				(void) sprintf (
					argbuf,
					"%s %s || %s %s",
					BINWRITE,
					request->secure->user,
					BINMAIL,
					request->secure->user
				);
			else
				(void) sprintf (
					argbuf,
					"%s %s%s%s",
					BINMAIL,
			(cp? "" : NB(request->secure->system)),
			(cp? "" : (request->secure->system? BANG_S : "")),
					request->secure->user
				);
			addlist2 (&argvp, argbuf);

		}
		break;

	}

#ifdef	DEBUG
	if (debug & DB_EXEC)
	{
		char *	sp;
		execlog (
			"EXEC: uid %d gid %d lid %d pid %d ppid %d\n",
			procuid,
			procgid,
			proclid,
			getpid(),
			getppid()
		);
		sp = sprintlist (argvp);
		execlog ("      argv=%s\n", sp);
		Free (sp);
	}
#endif

	Fork2 ();
	/* only the child returns */

	/*
	**  Only the child returns from Fork2().
	**
	**  Now it is time to become the user.
	**  We change our level, uid, and gid which will
	**  remove everything from our priv set except MACREAD.
	**  we leave this in to be able to execute interface
	**  programs that are protected w/ SYS_PRIVATE so
	**  users do not randomly execute them.  The B2
	**  interface program is one of these.  This is
	**  not a big deal since in the LPM env.  the MACREAD
	**  will not be inherited by any of the interface
	**  programs we deliver w/ the system.  In the SUM
	**  env. the interface programs explicitly turn off
	**  all privs or at least the ones they do not use.
	**
	*/
	(void)	lvlproc (MAC_SET, &proclid);
	(void)	setgid (procgid);
	(void)	setuid (procuid);
	/*
	 * The shell doesn't allow the "trap" builtin to set a trap
	 * for a signal ignored when the shell is started. Thus, don't
	 * turn off signals in the last child!
	 */

	(void) execv (*argvp, argvp);
	Done (EXEC_EXIT_NEXEC, errno);
	/*NOTREACHED*/
}

/**
 ** addenv() - ADD A VARIABLE TO THE ENVIRONMENT
 **/

static void
#ifdef	__STDC__
addenv (
	char *			name,
	char *			value
)
#else
addenv (name, value)
	char			*name,
				*value;
#endif
{
	DEFINE_FNNAME (addenv)

	register char *		cp;

	if ((cp = makestr(name, "=", value, (char *)0)))
		(void) putenv (cp);
	return;
}

/*
 * Procedure:     Fork1
 *
 * Restrictions:
 *               mconnect: None
 *               fork(2): None
 * Notes - FORK FIRST CHILD, SET UP CONNECTION TO IT
 */

static int
#ifdef	__STDC__
Fork1 (
	EXEC *			ep
)
#else
Fork1 (ep)
	register EXEC		*ep;
#endif
{
	DEFINE_FNNAME (Fork1)

	int			pid;
	int			fds[2];


	if (pipe(fds) == -1)
		lpfail (ERROR, E_SCH_FPIPE, PERROR);
#ifdef	DEBUG
	if (debug & DB_DONE)
		execlog ("PIPE= read %d write %d\n", fds[0], fds[1]);
#endif

	ep->md = mconnect((char *)0, fds[0], fds[1]);
	(void) mlistenadd(ep->md, POLLIN);

	switch (pid = fork()) {

	case -1:
		return (-1);
	
	case 0:
		ChildMd = mconnect(NULL, fds[1], fds[1]);
		return (0);

	default:
		return (pid);
	}
}

/*
 * Procedure:     Fork2
 *
 * Restrictions:
 *               fork(2): None
 * Notes - FORK SECOND CHILD AND WAIT FOR IT
 */

static void
#ifdef	__STDC__
Fork2 ()
#else
Fork2 (ep)

EXEC *	ep;
#endif
{
	DEFINE_FNNAME (Fork2)

	switch ((ChildPid = fork())) {

	case -1:
		Done (EXEC_EXIT_NFORK, errno);
		/*NOTREACHED*/
					
	case 0:
		return;
					
	default:
		/*
		 * Delay calling "ignore_fault_signals()" as long
		 * as possible, to give the child a chance to exec
		 * the interface program and turn on traps.
		 */

#ifdef	MALLOC_3X
		/*
		 * Reset the allocated space at this point, so the
		 * the process size is reduced. This can be dangerous!
		 */
		if (! do_undial)
			/* rstalloc() */ ;
#endif

		cool_heels ();
		/*NOTREACHED*/

	}
}


/**
 ** cool_heels() - WAIT FOR CHILD TO "DIE"
 **/

static void
#ifdef	__STDC__
cool_heels (
	void
)
#else
cool_heels ()
#endif
{
	DEFINE_FNNAME (cool_heels)

	int			status;


	/*
	 * At this point our only job is to wait for the child process.
	 * If we hang out for a bit longer, that's okay.
	 * By delaying before turning off the fault signals,
	 * we increase the chance that the child process has completed
	 * its exec and has turned on the fault traps. Nonetheless,
	 * we can't guarantee a zero chance of missing a fault.
	 * (We don't want to keep trapping the signals because the
	 * interface program is likely to have a better way to handle
	 * them; this process provides only rudimentary handling.)
	 *
	 * Note that on a very busy system, or with a very fast interface
	 * program, the tables could be turned: Our sleep below (coupled
	 * with a delay in the kernel scheduling us) may cause us to
	 * detect the fault instead of the interface program.
	 *
	 * What we need is a way to synchronize with the child process.
	 */
	(void) sleep (1);
	ignore_fault_signals ();

	WaitedChildPid = 0;
	while ((WaitedChildPid = wait(&status)) != ChildPid)
		;
			
	if (
		EXITED(status) > EXEC_EXIT_USER
	     && EXITED(status) != EXEC_EXIT_FAULT
	)
		Done (EXEC_EXIT_EXIT, EXITED(status));

	done (status, 0);	/* Don't use Done() */
	/*NOTREACHED*/
}


/**
 ** trap_fault_signals() - TRAP SIGNALS THAT CAN OCCUR ON PRINTER FAULT
 ** ignore_fault_signals() - IGNORE SAME
 **/

#ifdef	__STDC__
static void
trap_fault_signals (
	void
)
#else
static void
trap_fault_signals ()
#endif
{
	DEFINE_FNNAME (trap_fault_signals)

	(void)signal (SIGHUP, sigtrap);
	(void)signal (SIGINT, sigtrap);
	(void)signal (SIGQUIT, sigtrap);
	(void)signal (SIGPIPE, sigtrap);
	return;
}

static void
#ifdef	__STDC__
ignore_fault_signals (
	void
)
#else
ignore_fault_signals ()
#endif
{
	DEFINE_FNNAME (ignore_fault_signals)

	(void)signal (SIGHUP, SIG_IGN);
	(void)signal (SIGINT, SIG_IGN);
	(void)signal (SIGQUIT, SIG_IGN);
	(void)signal (SIGPIPE, SIG_IGN);
	return;
}

/**
 ** sigtrap() - TRAP VARIOUS SIGNALS
 **/

static void
#ifdef	__STDC__
sigtrap (
	int			sig
)
#else
sigtrap (sig)
	int			sig;
#endif
{
	DEFINE_FNNAME (sigtrap)

	(void)signal (sig, SIG_IGN);
	switch (sig) {

	case SIGHUP:
		Done (EXEC_EXIT_HUP, 0);
		/*NOTREACHED*/

	case SIGQUIT:
	case SIGINT:
		Done (EXEC_EXIT_INTR, 0);
		/*NOTREACHED*/

	case SIGPIPE:
		Done (EXEC_EXIT_PIPE, 0);
		/*NOTREACHED*/

	case SIGTERM:
		/*
		 * If we were killed with SIGTERM, it should have been
		 * via the Spooler who should have killed the entire
		 * process group. We have to wait for the children,
		 * since we're their parent, but WE MAY HAVE WAITED
		 * FOR THEM ALREADY (in cool_heels()).
		 */
		if (ChildPid != WaitedChildPid) {
			register int		cpid;

			while (
				(cpid = wait((int *)0)) != ChildPid
			     && (cpid != -1 || errno != ECHILD)
			)
				;
		}

		/*
		 * We can't rely on getting SIGTERM back in the wait()
		 * above, because, for instance, some shells trap SIGTERM
		 * and exit instead. Thus we force it.
		 */
		done (SIGTERM, 0);	/* Don't use Done() */
		/*NOTREACHED*/
	}
}

/*
 * Procedure:     done
 *
 * Restrictions:
 *               mputm: None
 *               mdisconnect: None
 * Notes - TELL SPOOLER THIS CHILD IS DONE
 */

static void
#ifdef	__STDC__
done (int status, int err)
#else
done (status, err)
int	status,
	err;
#endif
{
	DEFINE_FNNAME (done)

#ifdef	DEBUG
	if (debug & (DB_EXEC|DB_DONE)) {
		char	buf[10];

		(void) sprintf (
			buf,
			(EXITED(status) == -1? "%d" : "%04o"),
			EXITED(status)
		);
		execlog (
			"DONE: pid %d exited %s killed %d errno %d\n",
			getpid(),
			buf,
			KILLED(status),
			err
		);
	}
#endif

#ifdef	NETWORKING
	if (do_undial)
		undial (1);
#endif

	/*
	 * Close port to flush output buffer before SIGTERM from spooler
	 * is received. fd will be "1" always.
	*/
	close (1);

	(void) mputm (ChildMd, S_CHILD_DONE, SpoolerKey, slot, status, err);

#ifdef	DEBUG
	if (ChildMd->mque && (debug & DB_DONE))
		execlog ("DISC! ChildMd has message queued!\n");
#endif
	(void) mdisconnect (ChildMd);

	exit (0);
	/*NOTREACHED*/
}

/**
 ** child_mallocfail()
 **/

static void
#ifdef	__STDC__
child_mallocfail (
	void
)
#else
child_mallocfail ()
#endif
{
	DEFINE_FNNAME (child_mallocfail)

	Done (EXEC_EXIT_NOMEM, ENOMEM);
}
