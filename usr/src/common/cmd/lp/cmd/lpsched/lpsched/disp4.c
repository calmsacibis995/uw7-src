/*		copyright	"%c%" 	*/


#ident	"@(#)disp4.c	1.3"
#ident  "$Header$"

#include "time.h"

#include "dispatch.h"


#define PRINTER_ON_SYSTEM(PPS,PSS) \
	(((PPS)->status & PS_REMOTE) && (PPS)->system == (PSS))

/*
 * Procedure:     s_job_completed
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_accept_dest (char *m, MESG *md)
#else
int
s_accept_dest (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_accept_dest)

	char	*destination;
	ushort	status;

	register PSTATUS	*pps;
	register CSTATUS	*pcs;


	(void) getmessage (m, S_ACCEPT_DEST, &destination);

	/*
	 * Have we seen this destination as a printer?
	 */
	if ((pps = search_ptable(destination)))
	{
		if ((pps->status & PS_REJECTED) == 0)
			status = MERRDEST;
		else
		{
			pps->status &= ~PS_REJECTED;
			(void) time (&pps->rej_date);
			dump_pstatus ();
			status = MOK;
		}
		goto	Return;
	}
	/*
	 * Have we seen this destination as a class?
	 */
	if ((pcs = search_ctable(destination)))
	{
		if ((pcs->status & CS_REJECTED) == 0)
			status = MERRDEST;
		else {
			pcs->status &= ~CS_REJECTED;
			(void) time (&pcs->rej_date);
			dump_cstatus ();
			status = MOK;
		}
		goto	Return;
	}
	status = MNODEST;

Return:
	(void) mputm (md, R_ACCEPT_DEST, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_reject_dest
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_reject_dest (char *m, MESG *md)
#else
int
s_reject_dest (m, md)
char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_reject_dest)

	char			*destination,
				*reason;
	ushort			status;

	register PSTATUS	*pps;
	register CSTATUS	*pcs;


	(void) getmessage (m, S_REJECT_DEST, &destination, &reason);

	/*
	 * Have we seen this destination as a printer?
	 */
	if ((pps = search_ptable(destination)))
	{
		if (pps->status & PS_REJECTED)
			status = MERRDEST;
		else {
			pps->status |= PS_REJECTED;
			(void) time (&pps->rej_date);
			load_str (&pps->rej_reason, reason);
			dump_pstatus ();
			status = MOK;
		}
		goto	Return;
	}
	/*
	 * Have we seen this destination as a class?
	 */
	if ((pcs = search_ctable(destination)))
	{
		if (pcs->status & CS_REJECTED)
			status = MERRDEST;
		else {
			pcs->status |= CS_REJECTED;
			(void) time (&pcs->rej_date);
			load_str (&pcs->rej_reason, reason);
			dump_cstatus ();
			status = MOK;
		}
		goto	Return;
	}
	status = MNODEST;

Return:
	(void) mputm (md, R_REJECT_DEST, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_enable_dest
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_enable_dest (char *m, MESG *md)
#else
int
s_enable_dest (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_enable_dest)

	char	*printer;
	ushort	status;

	register PSTATUS	*pps;


	if (! ValidateEnableUser (md))
	{
		/*
		**  ES note:
		**  To the user the printer does not exist so
		**  we use MNODEST versus MNOPERM.
		*/
		status = MNODEST;
		goto	_return;
	}
	(void) getmessage (m, S_ENABLE_DEST, &printer);

	/*
	 * Have we seen this printer before?
	 */
	if ((pps = search_ptable(printer)))
	{
		if (enable (pps, md) < 0)
		switch (errno) {
		case	EINVAL:
			status = MERRDEST;
			break;

		case	EBUSY:
			status = MNOOPEN;
			break;

		case	ENOENT:
			status = MNODEST;
			break;
		default:
			status = MERRDEST;
			break;
		}
		else
			status = MOK;
	}
	else
		status = MNODEST;

_return:
	(void) mputm (md, R_ENABLE_DEST, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_disable_dest
 *
 * Restrictions:
 *               mputm: None
*/
#ifdef	__STDC__
int
s_disable_dest (char *m, MESG *md)
#else
int
s_disable_dest (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_disable_dest)

	char			*destination,
				*reason,
				*req_id		= 0;
	ushort			when,
				status;

	register PSTATUS	*pps;


	if (! ValidateEnableUser (md))
	{
		/*
		**  ES note:
		**  To the user the printer does not exist so
		**  we use MNODEST versus MNOPERM.
		*/
		status = MNODEST;
		goto	_return;
	}
	(void) getmessage (m, S_DISABLE_DEST, &destination, &reason, &when);

	/*
	 * Have we seen this printer before?
	 */
	if ((pps = search_ptable(destination)))
	{
		if (! ValidatePrinterUser (pps->printer, md) &&
		    ! ValidateAdminUser (md))
		{
			/*
			**  ES note:
			**  For 'enable/disable' only an admin user
			**  is OK regardless of whether the admin can
			**  print on that printer or not.
			**  To the user the printer does not exist so
			**  we use MNODEST versus MNOPERM.
			*/
			status = MNODEST;
			goto	_return;
		}
		/*
		 * If we are to cancel a currently printing request,
		 * we will send back the request's ID.
		 * Save a copy of the ID before calling "disable()",
		 * in case the disabling loses it (e.g. the request
		 * might get attached to another printer). (Actually,
		 * the current implementation won't DETACH the request
		 * from this printer until the child process responds,
		 * but a future implementation might.)
		 */
		if (pps->request && when == 2)
			req_id = Strdup(pps->request->secure->req_id);

		if (disable(pps, reason, (int)when) == -1) {
			if (req_id) {
				Free (req_id);
				req_id = 0;
			}
			status = MERRDEST;
		} else
			status = MOK;

	} else
		status = MNODEST;

_return:
	(void) mputm (md, R_DISABLE_DEST, status, NB(req_id));
	if (req_id)
		Free (req_id);

	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_load_filter_table
 *
 * Restrictions:
 *               mputm: None
*/

/* ARGSUSED0 */
#ifdef	__STDC__
int
s_load_filter_table (char *m, MESG *md)
#else
int
s_load_filter_table (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_load_filter_table)

	ushort			status;

	trash_filters ();
	if (Loadfilters((char *)0) == -1)
		status = MNOOPEN;
	else {

		/*
		 * This is what makes changing filters expensive!
		 */
		queue_check (qchk_filter);

		status = MOK;
	}

	(void) mputm (md, R_LOAD_FILTER_TABLE, status);
	return	status == MOK ? 1 : 0;
}


/*
 * Procedure:     s_unload_filter_table
 *
 * Restrictions:
 *               mputm: None
*/

/* ARGSUSED0 */
#ifdef	__STDC__
int
s_unload_filter_table (char *m, MESG *md)
#else
int
s_unload_filter_table (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_unload_filter_table)

	trash_filters ();

	/*
	 * This is what makes changing filters expensive!
	 */
	queue_check (qchk_filter);

	(void) mputm (md, R_UNLOAD_FILTER_TABLE, MOK);
	return	1;
}

/*
 * Procedure:     s_load_user_file
 *
 * Restrictions:
 *               mputm: None
*/
/* ARGSUSED0 */
#ifdef	__STDC__
int
s_load_user_file (char *m, MESG *md)
#else
int
s_load_user_file (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_load_user_file)

	/*
	 * The first call to "getuser()" will load the whole file.
	 */
	trashusers ();

	(void) mputm (md, R_LOAD_USER_FILE, MOK);
	return	1;
}

/*
 * Procedure:     s_unload_user_file
 *
 * Restrictions:
 *               mputm: None
*/

/* ARGSUSED0 */
#ifdef	__STDC__
int
s_unload_user_file (char *m, MESG *md)
#else
int
s_unload_user_file (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_unload_user_file)

	trashusers ();	/* THIS WON'T DO TRUE UNLOAD, SORRY! */

	(void) mputm (md, R_UNLOAD_USER_FILE, MOK);
	return	1;
}

/*
 * Procedure:     s_load_system
 *
 * Restrictions:
 *               mputm: None
*/

#define BUSY_SYSTEM(PSS) \
	((PSS)->exec->md || ((PSS)->exec->flags & EXF_WAITCHILD))

#ifdef	__STDC__
int
s_load_system (char *m, MESG *md)
#else
int
s_load_system (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_load_system)

	char	*system;
	ushort	status;

	register SYSTEM		*ps;
	register SSTATUS	*pss;
	register PSTATUS	*pps;


	(void) getmessage (m, S_LOAD_SYSTEM, &system);

	if (!*system)
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * Strange or missing system?
	 */
	if (!(ps = Getsystem(system)))
	{
		switch (errno) {
		case EBADF:
			status = MERRDEST;
			break;
		case ENOENT:
		default:
			status = MNODEST;
			break;
		}
		goto	Return;
	}
	/*
	 * Have we seen this system before?
	 */
	if ((pss = search_stable(system)))
	{
		/*
		 * Check that the new information won't mess us up.
		 *
		 *	- Different protocol can be trouble if we have
		 *	  already started talking with the other side.
		 *
		 * WARNING: We don't pass the new information on to
		 * lpNet, and currently it doesn't check for it on
		 * active connections (it will pick up new systems).
		 */
		if (pss->system->protocol != ps->protocol && BUSY_SYSTEM(pss))
			status = M2LATE;

		/*
		 * So far other changes don't require doing anything
		 * more than just copying the new information.
		 */
		else
		{
			freesystem (pss->system);
			*(pss->system) = *ps;
			status = MOK;
		}
		goto	Return;
	}
	/*
	 * Add new system.
	 */
	pss = (SSTATUS *)Calloc(1, sizeof(SSTATUS));
	pss->system = (SYSTEM *)Calloc(1, sizeof(SYSTEM));
	pss->exec = (EXEC *)Calloc(1, sizeof(EXEC));

	*(pss->system) = *ps;

	addone ((void ***)&SStatus, pss, &ST_Size, &ST_Count);

	/*
	 * Try re-initializing the ``remoteness'' of orphan
	 * printers (those marked remote with no system).
	 * Properly behaving user-level commands (or better
	 * self-protection on our part) would make this moot
	 * But here we are....
	 */
	for (pps = walk_ptable(1); pps; pps = walk_ptable(0))
		if (PRINTER_ON_SYSTEM(pps, (SSTATUS *)0))
			init_remote_printer (pps, pps->printer);

	status = MOK;

Return:
	(void) mputm (md, R_LOAD_SYSTEM, status);
	return  status == MOK ? 1 : 0;
}

/**
 ** s_unload_system()
 **/
#ifdef	__STDC__
static void
_unload_system (SSTATUS *pss)
#else
static void
_unload_system (pss)

SSTATUS	*pss;
#endif
{
	DEFINE_FNNAME (_unload_system)

	WAITING	*w;

	/* Release any 'waiting nodes' for the unloading system.
	 * There could be requests waiting for replies, and
	 * will hence need waking up to tell them no reply will
	 * be coming (as the system is being unloaded).
	 * Unfortunately, there is currently no way of passing
	 * a "no-reply" status into the waiters.
	 * Shouldn't cause too many problems, but would be
	 * better if we did the RightThing(tm).
	 */
	w = pss->waiting;
	while (w) {
		w->md = NULL;
		w->printer = NULL;
		w = w->next;
		Free(pss->waiting);
	}
	w = pss->ps_waiting;
	while (w) {
		w->md = NULL;
		w->printer = NULL;
		w = w->next;
		Free(pss->ps_waiting);
	}
	pss->waiting = pss->ps_waiting = NULL;

	Free (pss->exec);
	freesystem (pss->system);
	Free (pss->system);
	delone ((void ***)&SStatus, pss, &ST_Size, &ST_Count);
	Free (pss);

	return;
}

/*
 * Procedure:     s_unload_system
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_unload_system (char *m, MESG *md)
#else
int
s_unload_system (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_unload_system)

	char *	system;
	ushort	status;

	register SSTATUS	*pss;
	register PSTATUS	*pps;
	register int		i;

	(void) getmessage (m, S_UNLOAD_SYSTEM, &system);

	/*
	 * Unload ALL systems?
	 */
	if (!*system || STREQU(system, NAME_ALL))
	{
		/*
		 * Satisfy the request only if NO system has a printer.
		 */
		status = MOK;
		for (i = 0; (pss = SStatus[i]); i++)
			for (pps = walk_ptable(1); pps; pps = walk_ptable(0))
				if (
					BUSY_SYSTEM(pss)
				     || PRINTER_ON_SYSTEM(pps, pss)
				) {
					status = MBUSY;
					break;
				}
		if (status == MOK)
			/*
			 * DELONE DEPENDENT:
			 * This requires knowing how "delone()" works,
			 * sorry.
			 */
			while ((pss = SStatus[0]))
				_unload_system (pss);
		goto	Return;
	}
	/*
	 * Have we seen this system before?
	 */
	if (!(pss = search_stable(system)))
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * Any printers on this system?
	 */
	status = MOK;
	for (pps = walk_ptable(1); pps; pps = walk_ptable(0))
		if (
			BUSY_SYSTEM(pss)
		     || PRINTER_ON_SYSTEM(pps, pss)
		) {
			status = MBUSY;
			break;
		}
	if (status == MOK)
		_unload_system (pss);

Return:
	(void) mputm (md, R_UNLOAD_SYSTEM, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_shutdown
 *
 * Restrictions:
 *               mputm: None
*/
#ifdef	__STDC__
int
s_shutdown (char *m, MESG *md)
#else
int
s_shutdown (m, md)

char	*m;
MESG	*md;
#endif
{
	int			i;
	ushort			immediate;
	SSTATUS *		pss;

	DEFINE_FNNAME (s_shutdown)

	(void) getmessage (m, S_SHUTDOWN, &immediate);

	switch (md->type) {

	case MD_UNKNOWN:	/* Huh? */
	case MD_BOUND:		/* MORE: Not sure about this one */
	case MD_MASTER:		/* This is us. */
		schedlog ("Received S_SHUTDOWN on a type %d connection\n",
		md->type);
		break;

	case MD_STREAM:
	case MD_SYS_FIFO:
	case MD_USR_FIFO:
		(void) mputm (md, R_SHUTDOWN, MOK);
		CutAdminAuditRec (0, md->uid, "S_SHUTDOWN");
		lpshut (immediate);
		/*NOTREACHED*/

	case MD_CHILD:
		/*
		 * A S_SHUTDOWN from a network child means that IT has
		 * shut down, not that WE are to shut down.
		 *
		 * We have to clear the message descriptor
		 * so we don't accidently try using it in the future.
		 * Unfortunately, this requires looking through the
		 * system table to see which network child died.
		 */		
		DROP_MD (md) /* EMPTY */ ;
		if (SStatus) {
			for (i = 0; (pss = SStatus[i]); i++)
				if (pss->exec->md == md)
					break;
			if (pss) {
				schedlog (
				"Trying the connection again (request %s)\n",
				(pss->exec->ex.request?
				pss->exec->ex.request->secure->req_id :
				"<none>"));
				pss->exec->md = 0;
				resend_remote (pss, 0);
			}
		}
		break;

	}
	return	1;
}

/*
 * Procedure:     s_quiet_alert
 *
 * Restrictions:
 *               mputm: None
*/
#ifdef	__STDC__
int
s_quiet_alert (char *m, MESG *md)
#else
int
s_quiet_alert (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_quiet_alert)

	char			*name;
	ushort			type,
				status;

	register FSTATUS	*pfs;
	register PSTATUS	*pps;
	register PWSTATUS	*ppws;


	/*
	 * We quiet an alert by cancelling it with "cancel_alert()"
	 * and then resetting the active flag. This effectively just
	 * terminates the process running the alert but tricks the
	 * rest of the Spooler into thinking it is still active.
	 * The alert will be reactivated only AFTER "cancel_alert()"
	 * has been called (to clear the active flag) and then "alert()"
	 * is called again. Thus:
	 *
	 * For printer faults the alert will be reactivated when:
	 *	- a fault is found after the current fault has been
	 *	  cleared (i.e. after successful print or after manually
	 *	  enabled).
	 *
	 * For forms/print-wheels the alert will be reactivated when:
	 *	- the form/print-wheel becomes mounted and then unmounted
	 *	  again, with too many requests still pending;
	 *	- the number of requests falls below the threshold and
	 *	  then rises above it again.
	 */

	(void) getmessage (m, S_QUIET_ALERT, &name, &type);

	if (!*name)
	{
		status = MNODEST;
		goto	Return;
	}
	switch (type) {
	case QA_FORM:
		if (!(pfs = search_ftable(name)))
			status = MNODEST;

		else if (!pfs->alert->active)
			status = MERRDEST;

		else {
			cancel_alert (A_FORM, pfs);
			pfs->alert->active = 1;
			status = MOK;
		}
		break;
		
	case QA_PRINTER:
		if (!(pps = search_ptable(name)))
			status = MNODEST;

		else if (!pps->alert->active)
			status = MERRDEST;

		else {
			cancel_alert (A_PRINTER, pps);
			pps->alert->active = 1;
			status = MOK;
		}
		break;
		
	case QA_PRINTWHEEL:
		if (!(ppws = search_pwtable(name)))
			status = MNODEST;

		else if (!ppws->alert->active)
			status = MERRDEST;

		else {
			cancel_alert (A_PWHEEL, ppws);
			ppws->alert->active = 1;
			status = MOK;
		}
		break;
	}

Return:
	(void) mputm (md, R_QUIET_ALERT, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_send_fault
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_send_fault (char *m, MESG *md)
#else
int
s_send_fault (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_send_fault)

	long			key;
	char			*printer,
				*alert_text;
	ushort			status;

	register PSTATUS	*pps;


	(void) getmessage (m, S_SEND_FAULT, &printer, &key, &alert_text);

	if (
		!(pps = search_ptable(printer))
	     || !pps->exec
	     || pps->exec->key != key
	     || !pps->request
	)
		status = MERRDEST;

	else {
		printer_fault (pps, pps->request, alert_text, 0);
		status = MOK;
	}

	(void) mputm (md, R_SEND_FAULT, status);
	return	status == MOK ? 1 : 0;
}
