/*		copyright	"%c%" 	*/


#ident	"@(#)ports.c	1.3"
#ident  "$Header$"

/*******************************************************************************
 *
 * FILENAME:    ports.c
 *
 * DESCRIPTION: Handle Printer Dialup Connections
 *
 * SCCS:	ports.c 1.3  7/7/97 at 11:00:42
 *
 * CHANGE HISTORY:
 *
 * 07-07-97  Paul Cunningham        ul96-11004
 *           Change function open_dialup() so that the "call" and "call_ext"
 *           structures are setup up correctly before calling dial().
 *
 *******************************************************************************
 */

#include <sys/types.h>
#include <priv.h>
#ifdef	NETWORKING
#include <termio.h>
#include <dial.h>
#endif
#include <unistd.h>

#include "lpsched.h"

#define WHO_AM_I	I_AM_LPSCHED
#include "oam.h"

#ifdef	__STDC__
static void		sigalrm ( int );
static int		push_module ( int , char * , char * );
#else
static void		sigalrm();
static int		push_module();
#endif

static int		SigAlrm;

/**
 ** open_dialup() - OPEN A PORT TO A ``DIAL-UP'' PRINTER
 **/

#ifdef	NETWORKING
/* ARGSUSED0 */
int
#ifdef	__STDC__
open_dialup (
	char *			ptype,	/*UNUSED*/
	PRINTER *		pp
)
#else
open_dialup (ptype, pp)
	char *			ptype;	/*UNUSED*/
	register PRINTER *	pp;
#endif
{
	DEFINE_FNNAME (open_dialup)

	static char		*baud_table[]	= {
		    "0",
		   "50",
		   "75",
		  "110",
		  "134",
		  "150",
		  "200",
		  "300",
		  "600",
		 "1200",
		 "1800",
		 "2400",
		 "4800",
		 "9600",
		"19200",
		"38400"
	};

	struct termio		tio;

	CALL			call;
	CALL_EXT		call_ext;

	int			speed,
				fd;

	char			*sspeed;

	if ((pp->speed) == NULL)
		speed = -1;
	else
		if ((speed = atoi(pp->speed)) <= 0)
			speed = -1;

	call.attr = 0;
	call.speed = speed;
	call.line = 0;
	call.device = 0;
	call.telno = pp->dial_info;

	call.baud = -1;				/* ul96-11004 (9 lines) */
	call.modem = -1;
	call.device = (char *) &call_ext;
        call.dev_len = -1;

        call_ext.service = (char *) NULL;
        call_ext.class = NULL;
        call_ext.protocol = NULL;
        call_ext.reserved1 = NULL;

	if ((fd = dial(call)) < 0)
		return (EXEC_EXIT_NDIAL | (~EXEC_EXIT_NMASK & abs(fd)));

	/*
	 * "dial()" doesn't guarantee which file descriptor
	 * it uses when it opens the port, so we probably have to
	 * move it.
	 */
	if (fd != 1) {
		(void) dup2 (fd, 1);
		(void) Close (fd);
	}
	/*
	 * The "printermgmt()" routines move out of ".stty"
	 * anything that looks like a baud rate, and puts it
	 * in ".speed", if the printer port is dialed. Thus
	 * we are saved the task of cleaning out spurious
	 * baud rates from ".stty".
	 *
	 * However, we must determine the baud rate and
	 * concatenate it onto ".stty" so that that we can
	 * override the default in the interface progam.
	 * Putting the override in ".stty" allows the user
	 * to override us (although it would be probably be
	 * silly for him or her to do so.)
	 */
	(void) ioctl (1, TCGETA, &tio);
	if ((sspeed = baud_table[(tio.c_cflag & CBAUD)])) {

		register char	*new_stty = Malloc(
			(pp->stty == NULL ? 0 : strlen(pp->stty)) + 1 + strlen(sspeed) + 1
		);

		(void) sprintf (new_stty, "%s %s", (pp->stty == NULL ? "" : pp->stty), sspeed);

		/*
		 * We can trash "pp->stty" because
		 * the parent process has the good copy.
		 */
		pp->stty = new_stty;
	}

	return (0);
}
#endif

/*
 * Procedure:     open_direct
 *
 * Restrictions:
 *               tidbit: None
 *               stat(2): None
 *               devstat(2): None
 *               lvlproc(2): None
 *               lvlfile(2): None
 *               access(2): None
 *               open(2): None
 *               dup2: None
 *               isastream: None
 *               ioctl(2): None
 *
 * Notes - OPEN A PORT TO A DIRECTLY CONNECTED PRINTER
 */

int
#ifdef	__STDC__
open_direct (
	char *			ptype,
	PRINTER *		pp
)
#else
open_direct (ptype, pp)
	char *			ptype;
	register PRINTER *	pp;
#endif
{
	DEFINE_FNNAME (open_direct)

	int			open_mode,
				fd;
	short			bufsz	    = -1,
				cps	    = -1;
	struct stat		statbuf;
	struct devstat		devbuf;

	register unsigned int	oldalarm,
				newalarm    = 0;
	register void		(*oldsig)() = signal(SIGALRM, sigalrm);


	ENTRYP
	/*
	 * Set an alarm to wake us from trying to open the port.
	 * We'll try at least 60 seconds, or more if the printer
	 * has a huge buffer that, in the worst case, would take
	 * a long time to drain.
	 */
	(void) tidbit (ptype, "bufsz", &bufsz);
	(void) tidbit (ptype, "cps", &cps);
	if (bufsz > 0 && cps > 0)
		newalarm = (((long)bufsz * 1100) / cps) / 1000;
	if (newalarm < 60)
		newalarm = 60;
	oldalarm = alarm (newalarm);
	/*
	**  ES Note:
	**  All our privs should still be on but our uid, gid and lid
	**  should be the user.  Now if the file/device is owned by
	**  LP then it is an LP managed device.  Therefore, we use privilege
	**  to open the device.
	**
	**  We don't want to change the file/device owner/group to the
	**  user or he will then control it for the duration of his print
	**  job.  Which means, he could corrupt his own job and interfere
	**  with things like markings on the output.
	**  We do want to set the lid of the device to match the lid
	**  of the user.
	**  Except for '/dev/null'.
	*/
	if (stat (pp->device, &statbuf) < 0)
	{
		TRACEP ("stat() of device failed.")
		TRACEd (errno)
		EXITP
		return	EXEC_EXIT_NPORT;
	}
	if ((statbuf.st_mode & S_IFMT) == S_IFCHR)
	{
		TRACEP ("char-special file.")
		if (devstat (pp->device, DEV_GET, &devbuf) < 0)
		{
			TRACEP ("devstat() failed.")
			TRACEd (errno)
			if (errno != ENOPKG)
			{
				EXITP
				return	EXEC_EXIT_NPORT;
			}
		}
		else
		{
			TRACEP ("devstat() of device succeeded.")
			if (devbuf.dev_relflag != DEV_SYSTEM)
			{
				level_t	lid;

				TRACEP ("non-DEV_SYSTEM device.")
				if (lvlproc (MAC_GET, &lid) < 0)
				{
					if (errno == ENOPKG)
						goto	_procprivl;
					EXITP
					return	EXEC_EXIT_NPORT;
				}
				TRACEP ("lvlproc succeeded.")
				if (lvlfile (pp->device, MAC_SET, &lid) < 0)
				{
					if (errno == ENOSYS)
						goto _procprivl;
					EXITP
					return  EXEC_EXIT_NPORT;
				}
				TRACEP ("lvlfile succeeded.")
			}
		}
	}
_procprivl:
	(void)	procprivl (CLRPRV, ALLPRIVS_W, (priv_t)0);
	/*
	**  Only use privs if it is an LP managed device.
	*/
	if (statbuf.st_uid == Lp_Uid)
	{
		TRACEP ("Device is owned by LP.")
		TRACEP ("Using dacread & dacwrite to open.")
		(void)	procprivl (SETPRV, DACREAD_W, DACWRITE_W,
			(priv_t)0);
	}
	/*
	 * The following open must be interruptable.
	 * O_APPEND is set in case the ``port'' is a file.
	 * O_RDWR is set in case the interface program wants
	 * to get input from the printer. Don't fail, though,
	 * just because we can't get read access.
	 */
	open_mode = O_WRONLY;
	if (access (pp->device, R_OK) == 0)
		open_mode = O_RDWR;
	open_mode |= O_APPEND;

	SigAlrm = 0;
	while ((fd = open (pp->device, open_mode, 0)) < 0)
	{
		TRACEP ("open() on device failed.")
		TRACEd (errno)
		if (errno == EACCES)
		{
			(void)	procprivl (SETPRV, ALLPRIVS_W, (priv_t)0);
			EXITP
			return	EXEC_EXIT_ACCESS;
		}
		if (errno != EINTR)
		{
			(void)	procprivl (SETPRV, ALLPRIVS_W, (priv_t)0);
			EXITP
			return	EXEC_EXIT_NPORT;
		}
		if (SigAlrm)
		{
			(void)	procprivl (SETPRV, ALLPRIVS_W, (priv_t)0);
			EXITP
			return	EXEC_EXIT_TMOUT;
		}
	}
	TRACEP ("open() on device succeeded.")
	(void)	procprivl (SETPRV, ALLPRIVS_W, (priv_t)0);

	(void) alarm (oldalarm);
	(void) signal (SIGALRM, oldsig);

	/*
	 * We should get the correct channel number (1), but just
	 * in case....
	 */
	if (fd != 1)
	{
		(void) dup2 (fd, 1);
		(void) Close (fd);
	}

	/*
	 * Handle streams modules:
	 */
	if (isastream(1))
	{
		/*
		 * First, pop all current modules off, unless
		 * instructed not to.
		 */
#ifdef	CAN_DO_MODULES
		if (emptylist (pp->modules) ||
		    ! STREQU(pp->modules[0], NAME_KEEP))
#endif
			while (ioctl(1, I_POP, 0) == 0)
			{
				continue;
			}
		/*
		 * Now push either the administrator specified modules
		 * or the standard modules, unless instructed to push
		 * nothing.
		 */
#ifdef	CAN_DO_MODULES
		if (emptylist (pp->modules) ||
		    STREQU(NAME_NONE, pp->modules[0]) ||
		    STREQU(NAME_KEEP, pp->modules[0]))
		{
			/*EMPTY*/;
		}
		else
		if (!STREQU(NAME_DEFAULT, pp->modules[0]))
		{
			char **	pm = pp->modules;

			while (*pm)
			{
				if (push_module (1, pp->device, *pm++) < 0)
				{
					EXITP
					return (EXEC_EXIT_NPUSH);
				}
			}
		}
		else
#endif
		{
			char **	pm = getlist (DEFMODULES, LP_WS, LP_SEP);

			while (*pm)
			{
				if (push_module (1, pp->device, *pm++) < 0)
				{
					EXITP
					return (EXEC_EXIT_NPUSH);
				}
			}
		}
	}
	EXITP
	return (0);
}

/*
 * Procedure:     CheckPrinter
 *
 * Restrictions:
 *               open(2): None
 *               fdevstat(2): None
 *               flvlfile(2): None
 *		 getprinter: None
 *  Notes:
 *
 *  Check access to the device and open it.
 *  Used by 'enable ()'
 *
 *  return:
 *		0 for failure.
 *		1 for success without range change
 *		2 for success but the range on the device has changed.
*/

#ifdef	__STDC__
int
CheckPrinter (PSTATUS *psp)
#else
int
CheckPrinter (psp)

PSTATUS *psp;
#endif
{
	DEFINE_FNNAME (CheckPrinter)

	int		fd, n, returnCode = 0;
	uint		oldalarm;
	level_t		lid;
	PRINTER *	pp;
	PRINTER	*	checkpp;
	struct devstat	devbuf;

	void	(*oldsig)();

	TRACEx (psp)
	if (! psp)
		return	0;

	pp = psp->printer;

	TRACEx (psp->status)
	if (psp->status & PS_REMOTE) {
		while (!(checkpp = getprinter(pp->name)) && errno == EINTR)
			;
		if (!checkpp)
			return 0;
		if (pp->hilevel == checkpp->hilevel &&
	    	    pp->lolevel == checkpp->lolevel)
		{
			return 1;
		}
		else
		{
			pp->hilevel = checkpp->hilevel;
			pp->lolevel = checkpp->lolevel;
			return 2;
		}
	}

	if (pp->dial_info)
		return 1;

	TRACEs (pp->device)
	if (! pp->device)
		return	0;

	oldsig = signal (SIGALRM, sigalrm);
	SigAlrm = 0;
	oldalarm = alarm ((uint) 2);

	fd = open (pp->device, O_WRONLY | O_APPEND | O_NOCTTY, 0);

	if (SigAlrm)
	{
		oldalarm--;
		oldalarm--;
		if (oldalarm == 0)
			oldalarm++;
	}
	(void)	alarm ((oldalarm < 0 ? 0 : oldalarm));
	(void)	signal (SIGALRM, oldsig);

	if (fd < 0)
		return	0;

	while ((n = fdevstat (fd, DEV_GET, &devbuf)) < 0 && errno == EINTR)
		continue;
	if (n < 0)
	{
		if (errno == ENODEV || errno == ENOPKG)
		{
			(void)  close (fd);
			return	1;
		}
		(void)  close (fd);
		return	0;
	}
	/*
	**  We cannot work with PRIVATE or DYNAMIC, or devices
	**  that have other processes using them.
	*/
	if (devbuf.dev_state != DEV_PUBLIC ||
	    devbuf.dev_mode  != DEV_STATIC 
/*          can't do this with the device already open.  abs s21
**	    || (devbuf.dev_usecount > 1 && 	
**	    devbuf.dev_relflag  != DEV_SYSTEM)
*/
	   )
	{
		(void)	close (fd);
		errno = EINVAL;
		return	0;
	}
	/*
	**  DEV_SYSTEM devices have their hilevel and lolevel
	**  set to 0, so use the level on the file object to
	**  be the lolevel of the device.  The hilevel of the
	**  device is already set to PR_DEFAULT_HILEVEL.
	**
	**  '/dev/null' is such a device.
	*/
	if (devbuf.dev_relflag == DEV_SYSTEM)
	{
		if (flvlfile (fd, MAC_GET, &lid) < 0)
		{
			if (errno != ENOPKG) {
				(void)	close (fd);
				return	0;
			}
		}
		if (pp->lolevel == lid)
		{
			returnCode = 1;
		}
		else
		{
			pp->lolevel = lid;
			returnCode = 2;
		}
	}
	else
	{
		if (pp->hilevel == devbuf.dev_hilevel &&
	    	    pp->lolevel == devbuf.dev_lolevel)
		{
			returnCode = 1;
		}
		else
		{
			pp->hilevel = devbuf.dev_hilevel;
			pp->lolevel = devbuf.dev_lolevel;
			returnCode = 2;
		}
	}
	(void)	close (fd);
	return	returnCode;
}

/**
 ** sigalrm()
 **/

/* ARGSUSED0 */
#ifdef	__STDC__
static void
sigalrm (int ignore /*UNUSED*/)
#else
static void
sigalrm (ignore)

int	ignore;	/*UNUSED*/
#endif
{
	DEFINE_FNNAME (sigalrm)

	(void) signal (SIGALRM, SIG_IGN);
	SigAlrm = 1;
	return;
}


/*
 * Procedure:     push_module
 *
 * Restrictions:
 *               ioctl(2): None
*/

static int
#ifdef	__STDC__
push_module (
	int			fd,
	char *			device,
	char *			module
)
#else
push_module (fd, device, module)
	int			fd;
	char *			device;
	char *			module;
#endif
{
	DEFINE_FNNAME (push_module)

	int			ret	= ioctl(fd, I_PUSH, module);

	if (ret == -1)
		lpnote (ERROR, E_SCH_PUSH, module, device, PERROR);
	return (ret);
}
