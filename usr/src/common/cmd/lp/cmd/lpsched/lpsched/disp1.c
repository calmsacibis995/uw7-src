/*		copyright	"%c%" 	*/


#ident	"@(#)disp1.c	1.4"
#ident  "$Header$"

/*******************************************************************************
 *
 * FILENAME:    disp1.c
 *
 * DESCRIPTION: Handle incomming requests to the lp schedular
 *
 * SCCS:	disp1.c 1.4  9/17/97 at 14:02:02
 *
 * CHANGE HISTORY:
 *
 * 17-09-97  Paul Cunningham        us97-24635
 *           Change function s_alloc_files() so that if _alloc_files() fails
 *           because the id already exists, it loops n times to find the next
 *           free one.
 *
 *******************************************************************************
 */

#include "dispatch.h"

RSTATUS			*NewRequest;

#ifdef	__STDC__
static	char *	reqpath (char *, char **);
#else
static	char *	reqpath();
#endif

#define MAX_FILEID_SKIP 10  
	/* number of job id prefixes to check on before reporting error
	 */

/*
 * Procedure:     s_alloc_files
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_alloc_files ( char * m, MESG * md )	
#else
int
s_alloc_files ( m, md )

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_alloc_files)

	char *	file_prefix = (char*)0;
	short	status;
	ushort	count;
	int i;

	(void) getmessage (m, S_ALLOC_FILES, &count);

	/* allocate the file prefix, if it already exists skip to next
	 * and loop n times trying to get a prefix. This looping handles the
	 * case where there may still be print jobs queued with a the same
	 * id-prefix that are queued on a printer that is faultly or disabled.
	 */
	errno = EEXIST;
        for ( i = 0;
	      ((( !file_prefix) && (errno == EEXIST)) && (i < MAX_FILEID_SKIP));
	      i++)
	{
	  file_prefix = _alloc_files
			      (
			      count, (char *)0, md->uid, md->gid, md->lid
			      );
	}

	if ( file_prefix)
	{
		status = MOK;
		(void) mputm (md, R_ALLOC_FILES, status, file_prefix);
		add_flt_act(md, FLT_FILES, file_prefix, count);
	}
	else
	{
		status = MNOMEM;
		(void) mputm (md, R_ALLOC_FILES, status, "");
	}
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_print_request
 *
 * Restrictions:
 *               Chmod: None
 *               Chown: None
 *               rmsecure: None
 *               putsecure: None
 *               putrequest: None
 *               mputm: None
*/

int
#ifdef	__STDC__
s_print_request (char *m, MESG *md)	
#else
s_print_request (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_print_request)

	char *		file;
	char *		idno;
	char *		path;
	char *		req_file;
	char *		req_id	= 0;
	char *		un_used	= "";
	RSTATUS *	rp;
	REQUEST	*	r;
	SECURE	*	s;
	short		err;
	short		status;
	off_t		size;
	boolean		hold_job	= False,
			immediate_job	= False,
			priority_job	= False;

	extern
	char *	Local_System;

	(void) getmessage (m, S_PRINT_REQUEST, &file);

	/*
	 * "NewRequest" points to a request that's not yet in the
	 * request list but is to be considered with the rest of the
	 * requests (e.g. calculating # of requests awaiting a form).
	 */
	if (! (rp = NewRequest = allocr()))
	{
		status = MNOMEM;
		goto	Return;
	}
	req_file = reqpath (file, &idno);
	path = makepath (Lp_Tmp, req_file, (char *)0);
	/*
	**  ES Note:
	**  This is OK.
	**  Leave the MAC-level alone.
	*/
	(void) Chmod (path, 0600);
	(void) Chown (path, Lp_Uid, Lp_Gid);
	Free (path);

	if (! (r = Getrequest (req_file)))
	{
		status = MNOOPEN;
		goto	Return;
	}
	*(rp->request) = *r;
	rp->req_file = Strdup (req_file);
	/*
	**  Test for the presence of a secure file.
	**  If found skip sanity checks.
	**  The secure file will only exist if the request
	**  originated on a different system.  Since the
	**  request has not been validated on this side yet
	**  we remove the secure file until it is.
	**
	*/
	if (s = Getsecure (req_file))
	{
		TRACEP ("Found secure file.")
		(void)  rmsecure (req_file);
		if (s->status & SC_STATUS_ACCEPTED)
		{
			s->status &= ~SC_STATUS_ACCEPTED;
		}
		if (! NormalizeSecureUserAttributes (s))
		{
			status = MDENYDEST;
			goto	Return;
		}
		rp->request->outcome = 0;
		*(rp->secure) = *s;
		/*
		**  There are some anomallies associated w/
		**  '-1', '-2', etc. files received from other systems
		**  so even though the uid and gid will be 'lp'
		**  the mode may be incorrect.  'chfiles()' will
		**  fix this for us.
		*/
		(void)	chfiles (rp->request->file_list, Lp_Uid, Lp_Gid);
	}
	else
	{
		rp->request->outcome = 0;
		rp->secure->uid = md->uid;
		rp->secure->gid = md->gid;
		rp->secure->lid = md->lid;

		rp->secure->user = lp_uidtoname (md->uid);
		/*
		**  ES Note:
		**  These are auditable events.
		*/
		switch (rp->request->actions & ACT_SPECIAL) {
		case	0:
			/*
			**  No special actions.
			*/
			break;

		case	ACT_HOLD:
			rp->request->outcome |= RS_HELD;
			hold_job = True;
			break;

		case	ACT_IMMEDIATE:
			/*
			**  ES Note:
			**  What it was.
			**
			**  if (!md->admin)
			**  if (!SecAdvise (rp->secure, SA_WRITE, md))
			*/
			if (! ValidateAdminUser (md))
			{
				status = MNOPERM;
				goto Return;
			}
			rp->request->outcome |= RS_IMMEDIATE;
			immediate_job = True;
			break;

		case	ACT_RESUME:
			/*
			**  'RESUME' is not accepted in this context.
			*/
			/*FALLTHROUGH*/

		default:
			status = MUNKNOWN;
			goto	Return;
		}
		/*
		**  ES Note:
		**  This is OK.
		**  Leave the MAC-level alone.
		*/
		(void)	chfiles (rp->request->file_list, Lp_Uid, Lp_Gid);

		size = statfiles (rp->request->file_list, rp->secure->lid);

		if (size < 0)
		{
			status = MUNKNOWN;
			goto Return;
		}
		if (!(rp->request->outcome & RS_HELD) && size == 0)
		{
			status = MNOPERM;
			goto Return;
		}
		rp->secure->size = size;

		(void) time (&rp->secure->date);
		rp->secure->req_id = NULL;
		rp->secure->system = Strdup (Local_System);
	}
	if (rp->request->priority != -1)
	{
		priority_job = True;
	}
	if ((err = validate_request (rp, &req_id, 0)) != MOK)
	{
		status = err;
		goto	Return;
	}
	/*
	 * "req_id" will be supplied if this is from a
	 * remote system.
	 */
	if (! rp->secure->req_id)
	{
		req_id = makestr (req_id, "-", idno, (char *)0);
		rp->secure->req_id = req_id;
	}
	rp->secure->status |= SC_STATUS_ACCEPTED;

	if (putsecure (req_file, rp->secure) < 0 ||
	    putrequest (req_file, rp->request) < 0)
	{
		status = MNOMEM;
		goto	Return;
	}
	status = MOK;

	insertr (rp);
	NewRequest = 0;

	if (rp->slow)
		schedule (EV_SLOWF, rp);
	else
		schedule (EV_INTERF, rp->printer);

	del_flt_act (md, FLT_FILES);

Return:
	NewRequest = 0;
	Free (req_file);
	Free (idno);
	if (status != MOK && rp)
	{
		rmfiles (rp, 0);
		freerstatus (rp);
	}
	if (status == MOK)
	{
		char	adtbuf [64];

		if (priority_job)
		{
			(void)	sprintf (adtbuf, "PJ:%s:%d", req_id,
				(int) rp->request->priority);

			CutMiscAuditRec (0, rp->secure->user, adtbuf);
		}
		if (hold_job)
		{
			/*
			**  Since it is a new job, 'hold' is treated
			**  as a user audit event vs. an admin audit
			**  event since the user, even if an admin,
			**  is not acting in the capacity of and admin.
			**  since they own the job.
			*/
			(void)	sprintf (adtbuf, "HJ:%s", req_id);

			CutMiscAuditRec (0, rp->secure->user, adtbuf);
		}
		else
		if (immediate_job)
		{
			/*
			**  'immediate' was only accepted if the
			**  job was submitted by an admin. user.
			*/
			(void)	sprintf (adtbuf, "IJ:%s:%s",
				rp->secure->user, req_id);
			CutAdminAuditRec (0, md->uid, adtbuf);
		}
	}
	(void)	mputm (md, R_PRINT_REQUEST, status, NB(req_id),
		chkprinter_result, un_used);

	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_start_change_request
 *
 * Restrictions:
 *               Chown: None
 *               mputm: None
*/

#ifdef	__STDC__
int
s_start_change_request ( char * m, MESG * md )	
#else
int
s_start_change_request ( m, md )
char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_start_change_request)

	char		*req_id;
	char		*req_file		= "";
	short		status;
	RSTATUS		*rp;
	char		*path;

#ifdef	ALLOW_CHANGE_REMOTE

	char		*system;
	char		*cp;

#endif
	
	(void) getmessage(m, S_START_CHANGE_REQUEST, &req_id);

#ifdef	ALLOW_CHANGE_REMOTE

	if ((cp = strchr(req_id, BANG_C)) != NULL)
	{
		*cp++ = '\0';
		system = req_id;
		req_id = cp;
	}
#endif

	if (!(rp = request_by_id(req_id)))
	{
		status = MUNKNOWN;
		goto	Return;
	}
	/*
	**  ES Note:
	**  What it shall be.
	**  Do the check up here to avoid giving away any information.
	*/
	if (! SecAdvise (rp->secure, SA_WRITE, md))
	{
		if (! ValidateAdminUser (md))
		{
			status = MNOPERM;
			goto	Return;
		}
	}
	if (rp->request->outcome & RS_GONEREMOTE)
	{
		status = MGONEREMOTE;
		goto	Return;
	}
	if (rp->request->outcome & RS_DONE)
	{
		status = M2LATE;
		goto	Return;
	}
	/*
	**  ES Note:
	**  What it was.
	**
	**  if (!md->admin && md->uid != rp->secure->uid)
	**  {
	**	status = MNOPERM;
	**	goto	Return;
	**  }
	*/
	if (rp->request->outcome & RS_CHANGING)
	{
		status = MNOOPEN;
		goto	Return;
	}
	if (rp->request->outcome & RS_NOTIFYING)
	{
		status = MBUSY;
		goto	Return;
	}
	status = MOK;

	if (rp->request->outcome & RS_FILTERING &&
		!(rp->request->outcome & RS_STOPPED))
	{
		rp->request->outcome |= (RS_REFILTER|RS_STOPPED);
		terminate (rp->exec);
	}
	if (rp->request->outcome & RS_PRINTING &&
		!(rp->request->outcome & RS_STOPPED))
	{
		rp->request->outcome |= RS_STOPPED;
		terminate (rp->printer->exec);
	}
	rp->request->outcome |= RS_CHANGING;		

	/*
	 * Change the ownership of the request file to be "md->uid".
	 * Either this is identical to "rp->secure->uid", or it is
	 * "Lp_Uid" or it is root. The idea is that the
	 * person at the other end needs access, and that may not
	 * be who queued the request.
	 *
	 *  ES Note:
	 *  This is OK.  The access check has already been made (above).
	 *  Leave the MAC-level alone.
	 */
	path = makepath (Lp_Tmp, rp->req_file, (char *)0);
	(void) Chown (path, md->uid, rp->secure->gid);
	(void) lvlfile (path, MAC_SET, &(md->lid));
	Free (path);

	add_flt_act (md, FLT_CHANGE, rp);
	req_file = rp->req_file;

Return:
	(void) mputm (md, R_START_CHANGE_REQUEST, status, req_file);
	return	status == MOK ? 1 : 0;
}
/*
 * Procedure:     s_end_change_request
 *
 * Restrictions:
 *               Chmod: None
 *               Chown: None
 *               putrequest: None
 *               putsecure: None
 *               mputm: None
*/

#ifdef	__STDC__
int
s_end_change_request ( char * m, MESG * md )	
#else
int
s_end_change_request ( m, md )
char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_end_change_request)

	char		*req_id;
	RSTATUS		*rp;
	off_t		size;
	off_t		osize;
	short		err;
	short		status;
	REQUEST		*r = 0;
	REQUEST		oldr;
	int		call_schedule = 0;
	int		move_ok	= 0;
	char		*path;
	boolean		admin_user	= False,
			hold_job	= False,
			resume_job	= False,
			immediate_job	= False,
			priority_job	= False;

#ifdef	ALLOW_CHANGE_REMOTE

	char		*system = Local_System;
	char		*cp;

#endif
	
	(void) getmessage(m, S_END_CHANGE_REQUEST, &req_id);

#ifdef	ALLOW_CHANGE_REMOTE
	if ((cp = strchr(req_id, BANG_C)) != NULL)
	{
		*cp++ = '\0';
		system = req_id;
		req_id = cp;
	}
#endif

	if (! (rp = request_by_id (req_id)))
	{
		status = MUNKNOWN;
		goto	Return;
	}
	/*
	**  ES Note:
	**  Do the check up here to avoid giving away any information.
	**
	**  There is a problem here in that we need to know if priv.
	**  is being used to manipulate the job.  If so, then the
	**  user hanging on 'md' is an admin user and we need to cut
	**  an audit record for admin. use of LP.  The SecAdvice call
	**  checks for operational access.  It does not tell us
	**  if priv is being used.  That is, it could be an admin. user
	**  on 'md' but they may already own the job so any privs
	**  they may have are uneccessary.
	*/
	if (! SecAdvise (rp->secure, SA_WRITE, md))
	{
		if (ValidateAdminUser (md))
		{
			/*
			**  We'll use this later to flag cutting
			**  an admin audit record.
			*/
			admin_user = True;
		}
		else
		{
			status = MNOPERM;
			goto	Return;
		}
	}
	if (! admin_user &&
	    (md->uid != rp->secure->uid || md->lid != rp->secure->lid))
	{
		/*
		**  Privs must have been used if we got to here.
		**  All we want to know is if we should cut an
		**  admin. audit record.
		*/
		admin_user = True;
	}
	if (rp->request->outcome & RS_GONEREMOTE)
	{
		/*
		**  should never happen, but...
		*/
		status = MGONEREMOTE;
		goto	Return;
	}
	if (!(rp->request->outcome & RS_CHANGING))
	{
		status = MNOSTART;
		goto	Return;
	}
	/*
	**  ES Note:
	**  This is OK.
	*/
	path = makepath(Lp_Tmp, rp->req_file, (char *)0);
	(void) Chmod(path, 0600);
	(void) Chown(path, Lp_Uid, Lp_Gid);
	(void) lvlfile (path, MAC_SET, &(rp->secure->lid));
	Free (path);

	rp->request->outcome &= ~(RS_CHANGING);
	del_flt_act (md, FLT_CHANGE);
	/*
	 * The RS_CHANGING bit may have been the only thing preventing
	 * this request from filtering or printing, so regardless of what
	 * happens below, we must check to see if the request can proceed.
	 */
	call_schedule = 1;

	if (!(r = Getrequest (rp->req_file)))
	{
		status = MNOOPEN;
		goto	Return;
	}
	oldr = *(rp->request);
	*(rp->request) = *r;

	move_ok = STREQU(oldr.destination, r->destination);

	/*
	 * Preserve the current request status!
	 */
	rp->request->outcome = oldr.outcome;

	/*
	 * Here's an example of the dangers one meets when public
	 * flags are used for private purposes. ".actions" (indeed,
	 * anything in the REQUEST structure) is set by the person
	 * changing the job. However, lpsched uses ".actions" as
	 * place to indicate that a job came from a remote system
	 * and we must send back job completion--this is a strictly
	 * private flag that we must preserve.
	 */
	rp->request->actions |= (oldr.actions & ACT_NOTIFY);

	if ((rp->request->actions & ACT_SPECIAL) == ACT_HOLD)
	{
		rp->request->outcome |= RS_HELD;
		/*
		 * To be here means either the user owns the request
		 * or he or she is the administrator. Since we don't
		 * want to set the RS_ADMINHELD flag if the user is
		 * the administrator, the following compare will work.
		 *
		 * ES Note:
		 * This is still a safe assumption for ES.
		 */
		if (admin_user)
			rp->request->outcome |= RS_ADMINHELD;
		hold_job = True;
	}
	if ((rp->request->actions & ACT_SPECIAL) == ACT_RESUME)
	{
		/*
		**  ES Note:
		**  What it was.
		**
		**  if ((rp->request->outcome & RS_ADMINHELD) && !md->admin)
		**
		**  What it shall be.
		*/
		if ((rp->request->outcome & RS_ADMINHELD) && ! admin_user)
		{
			status = MNOPERM;
			goto Return;
		}
		rp->request->outcome &= ~(RS_ADMINHELD|RS_HELD);
		resume_job = True;
	}
	if ((rp->request->actions & ACT_SPECIAL) == ACT_IMMEDIATE)
	{
		/*
		**  ES Note:
		**  What it shall be.
		*/
		if (! (admin_user = ValidateAdminUser (md)))
		{
			status = MNOPERM;
			goto Return;
		}
		rp->request->outcome |= RS_IMMEDIATE;
		immediate_job = True;
	}
	/*
	**  ES Note:
	**  This is OK.
	**  Leave the MAC-level alone.
	*/
	(void)  chfiles (rp->request->file_list, Lp_Uid, Lp_Gid);

	size = statfiles (rp->request->file_list, rp->secure->lid);

	if (size < 0)
	{
		status = MUNKNOWN;
		goto Return;
	}
	if (! (rp->request->outcome & RS_HELD) && size == 0)
	{
		status = MNOPERM;
		goto Return;
	}

	osize = rp->secure->size;
	rp->secure->size = size;

	if ((err = validate_request(rp, (char **)0, move_ok)) != MOK)
	{
		status = err;
		rp->secure->size = osize;
		goto	Return;
	}
	status = MOK;

	if (oldr.priority != rp->request->priority)
	{
		priority_job = True;
	}
	if ((rp->request->outcome & RS_IMMEDIATE) || priority_job)
	{
		remover (rp);
		insertr (rp);
	}
	(void) time (&rp->secure->date);

	freerequest (&oldr);
	(void) putrequest (rp->req_file, rp->request);
	(void) putsecure (rp->req_file, rp->secure);

Return:
	if (status == MOK)
	{
		char	adtbuf [64];

		if (priority_job)
		{
			if (admin_user)
			{
				(void)	sprintf (adtbuf, "CJ;PJ:%s:%s:%d",
					rp->secure->user, req_id,
					(int) rp->request->priority);

				CutAdminAuditRec (0, md->uid, adtbuf);
			}
			else
			{
				(void)	sprintf (adtbuf, "CJ;PJ:%s:%d",
					req_id, (int) rp->request->priority);

				CutMiscAuditRec (0, rp->secure->user, adtbuf);
			}
		}
		if (hold_job)
		{
			if (admin_user)
			{
				(void)	sprintf (adtbuf, "CJ;HJ:%s:%s",
					rp->secure->user, req_id);

				CutAdminAuditRec (0, md->uid, adtbuf);
			}
			else
			{
				(void)	sprintf (adtbuf, "CJ;HJ:%s", req_id);

				CutMiscAuditRec (0, rp->secure->user, adtbuf);
			}
		}
		else
		if (resume_job)
		{
			if (admin_user)
			{
				(void)	sprintf (adtbuf, "CJ;RJ:%s:%s",
					rp->secure->user, req_id);

				CutAdminAuditRec (0, md->uid, adtbuf);
			}
			else
			{
				(void)	sprintf (adtbuf, "CJ;RJ:%s", req_id);

				CutMiscAuditRec (0, rp->secure->user, adtbuf);
			}
		}
		else
		if ((admin_user || immediate_job) && ! priority_job)
		{
			(void)	sprintf (adtbuf, "%s:%s:%s",
				immediate_job ? "CJ;IJ" : "CJ",
				rp->secure->user, req_id);

			CutAdminAuditRec (0, md->uid, adtbuf);
		}
		else
		{
			(void)	sprintf (adtbuf, "CJ:%s", req_id);

			CutMiscAuditRec (0, rp->secure->user, adtbuf);
		}
	}
	if (status != MOK && rp)
	{
		if (r)
		{
			freerequest(r);
			*(rp->request) = oldr;
		}
		if (status != MNOSTART)
			(void) putrequest (rp->req_file, rp->request);
	}
	if (call_schedule)
		maybe_schedule(rp);

	(void) mputm(md, R_END_CHANGE_REQUEST, status, chkprinter_result);
	return	status == MOK ? 1 : 0;
}

/**
 ** _cancel()
 **/

#ifdef	__STDC__
static char *
_cancel ( MESG * md, char * dest, char * user, char * req_id)		
#else
static char *
_cancel ( md, dest, user, req_id )

MESG	*md;
char	*dest;
char	*user;
char	*req_id;
#endif
{
	DEFINE_FNNAME (_cancel)

	static RSTATUS		*rp1;
	static RSTATUS		*rp2;
	static char		*s_dest;
	static char		*s_user;
	static char		*s_req_id;
	static int		current;
	RSTATUS			*crp;
	char			*creq_id;
	char			*creq_user = NULL;

	errno = MOK;

	if (dest || user || req_id)
	{
		s_dest = dest;
		s_user = user;
		s_req_id = req_id;
		rp1 = Request_List;
		rp2 = Remote_Request_List;
		current = 0;
		if (STREQU(s_req_id, CURRENT_REQ))
		{
			current = 1;
			s_req_id = NULL;
		}
	}


	/*
	 * Search through both lists;  Request_List
	 * and Remote_Request_List
	 */
	while (1) {
		crp = rp1;
		if (crp == NULL) {
			crp = rp2;
			if (crp == NULL)
				break;
			rp2 = rp2->next;
		} else
			rp1 = rp1->next;
		
		if (*s_dest 
			&& !STREQU(s_dest, crp->request->destination)
			&& !STREQU(s_dest, crp->printer->printer->name))
			continue;

		if (current && !(crp->request->outcome & RS_PRINTING))
			continue;

		if (s_req_id && *s_req_id &&
			!STREQU(s_req_id, crp->secure->req_id))
			continue;

		if (*s_user && ! bangequ (s_user, crp->secure->user))
			continue;

		/*
		**  What it was.
		**
		if (!md->admin && md->uid != crp->secure->uid)
		*/
		/*
		**  What it shall be.
		*/
		if (!SecAdvise (crp->secure, SA_WRITE, md))
		{
			if (IsMacInstalled())
				errno = MDENYDEST;
			else
				errno = MNOPERM;
			return  Strdup (crp->secure->req_id);
		}

		crp->reason = MOK;
		creq_id = Strdup (crp->secure->req_id);
		creq_user = Strdup (crp->secure->user);

		/*
		**  ES Note:
		**  This just denotes that if the user cancelling the
		**  job is not the original owner then the original owner
		**  must be notified.
		*/
		if (cancel (crp, (md->uid != crp->secure->uid)))
		{
			CutCancelAuditRec (0, md->uid, creq_id, creq_user);
			errno = MOK;
		}
		else
			errno = M2LATE;

		Free (creq_user);
		return	creq_id;
	}

	errno = MUNKNOWN;
	return  NULL;
}

/*
 * Procedure:     s_cancel_request
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_cancel_request ( char * m, MESG * md )		
#else
int
s_cancel_request ( m, md )

char		*m;
MESG		*md;
#endif
{
	DEFINE_FNNAME (s_cancel_request)

	char	*req_id;
	char	*rid;
	short	status = MUNKNOWN;
	int	found = 0;
	int	loop = 0;

	(void) getmessage(m, S_CANCEL_REQUEST, &req_id);

	if (!request_by_id(req_id)) {
		PSTATUS *pps;
		PSTATUS *plist = (PSTATUS *)1;
		RSTATUS	*rp;
		/* The request could be on a remote system.
		 * First check the remote requests we already know about,
		 * before asking all the remote systems.
		 */
retry:
		for (rp = Remote_Request_List; rp; rp = rp->next) {
			if (!STREQU(req_id, rp->secure->req_id))
				continue;
			found++;
			goto jmp_remotes;
		}
		if (loop++)
			goto jmp_remotes;
		if (Redispatch)
			goto jmp_remotes;

		/* Need to question remote systems */

		/* Be over careful, and ensure the temporary chain is clear */
		for (pps=walk_ptable(1); pps; pps = walk_ptable(0))
			pps->next_list = NULL;

		for (pps = walk_ptable(1); pps; pps = walk_ptable(0)) {
			if (!(pps->status & PS_REMOTE))
				continue;
			if (!pps->next_list) {
				pps->next_list = plist;
				plist = pps;
			}
		}
		if (plist == (PSTATUS *)1)
			goto jmp_remotes;
		while (plist != (PSTATUS *)1) {
			SSTATUS *pss;
			pps = plist;
			pss = pps->system;
			if (pss->system->protocol == S5_PROTO) {
				/* See disp1.c/s_inquire_request_rank() */
				PSTATUS **pplist = &plist->next_list;
				PSTATUS *tmp;
				while (*pplist != (PSTATUS *)1) {
					if ((*pplist)->system == pss) {
						tmp = (*pplist)->next_list;
						(*pplist)->next_list = NULL;
						*pplist = tmp;
						continue;
					}
					pplist = &(*pplist)->next_list;
				}
				pps = NULL;
			}
			plist = plist->next_list;
			askforstatus(pss, md, pps, 0, 0);
			if (pps)
				pps->next_list = NULL;
		}
		if (waitforstatus(m, md) == 0)
			return  0;
		goto retry;
	} else
		found++;

jmp_remotes:
	if (found) {
		if ((rid = _cancel(md, "", "", req_id)) != NULL)
			Free(rid);
		status = (short)errno;
	}
	(void) mputm(md, R_CANCEL_REQUEST, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_cancel
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_cancel ( char * m, MESG * md )		
#else
int
s_cancel ( m, md )

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_cancel)

	int	nerrno;
	int	oerrno;
	char	*req_id;
	char	*user;
	char	*destination;
	char	*rid;
	char	*nrid;
	char	*unused;
	char	*bang;
	short	status;
	int	dest_all = 0;
	PSTATUS *pps;
	PSTATUS *plist = (PSTATUS *)1;
	RSTATUS	*rp;

	(void) getmessage(m, S_CANCEL, &destination, &user, &req_id, &unused);

/*
	if (STREQU(user, NAME_ALL))
		user = "";
*/
	if (STREQU(destination, NAME_ALL)) {
		destination = "";
		dest_all++;
	}

	if (STREQU(req_id, NAME_ALL))
		req_id = "";

	if (Redispatch)
		goto Search;

	/* Be over careful, and ensure the temporary chain is clear */
	for (pps=walk_ptable(1); pps; pps = walk_ptable(0))
		pps->next_list = NULL;

	/* S_CANCEL is only passed under two conditions;
	 *	1) From cancel(1) when the '-u' option is used.
	 *	   This only causes the 'destination' and 'user' fields
	 *	   to be set.
	 *	2) From a remote system, where all the fields are set.
	 *
	 * NOTE: Just have 'req_id' set does not currently happen! (The code
	 * always calls s_cancel_request()).
	 *
	 * Therefore, to ensure the Remote_Request_List is upto date for any
	 * of the above searches in _cancel(), we need to;
	 * 	1) If the user has a '!' then fetch requests from the host
	 *	   which is referenced (without a '!' we assume the request(s)
	 *	   to be cancel are local).
	 *	2) If destination is set, then fetch requests for the
	 *	   the destination.
	 *	3) If can ignore the reqid in deciding what to fetch as
	 *	   we _do not_ support it being NAME_ALL!
	 */
	if ((*user && (bang=strchr(user, '!'))) || (*destination || dest_all)) {
		int	host_all = 0;

		if (bang) {
			*bang = '\0';
			if (STREQU(NAME_ALL, user))
				host_all++;
		}
		for (pps = walk_ptable(1); pps; pps = walk_ptable(0)) {
			if (!(pps->status & PS_REMOTE))
				continue;
			if (bang)
				if (!host_all && !STREQU(pps->system->system->name, user))
					continue;
			if (!dest_all && (*destination && !STREQU(pps->printer->name, destination)))
				continue;

			if (!pps->next_list) {
				pps->next_list = plist;
				plist = pps;
			}
		}
		if (bang)
			*bang = '!';
	}

	
	if (plist != (PSTATUS *)1) {
		/* We have a chain of (remote) printers to question... */
		while (plist != (PSTATUS *)1) {
			SSTATUS *pss;

			pps = plist;
			pss = pps->system;
			if (pss->system->protocol == S5_PROTO) {
				PSTATUS **pplist = &plist->next_list;
				PSTATUS *tmp;
				/* See s_inquire_request_rank() */
				while (*pplist != (PSTATUS *)1) {
					if ((*pplist)->system == pss) {
						tmp = (*pplist)->next_list;
						(*pplist)->next_list = NULL;
						*pplist = tmp;
						continue;
					}
					pplist = &(*pplist)->next_list;
				}
				pps = NULL;
			}
			plist = plist->next_list;
			askforstatus(pss, md, pps, 0, 0);
			if (pps)
				pps->next_list = NULL;
		}
		
		if (waitforstatus(m, md) == 0)
			return	0;
	}

Search:
	if (rid = _cancel(md, destination, user, req_id)) {
		oerrno = errno;

		while ((nrid = _cancel(md, NULL, NULL, NULL)) != NULL)
		{
			nerrno = errno;
			(void) mputm(md, R_CANCEL, MOKMORE, oerrno, rid);
			Free(rid);
			rid = nrid;
			oerrno = nerrno;
		}
		(void) mputm(md, R_CANCEL, status = MOK, oerrno, rid);
		Free(rid);
		return	status == MOK ? 1 : 0;
	}

	(void) mputm(md, R_CANCEL, status = MOK, MUNKNOWN, req_id);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_inquire_request
 *
 * Restrictions:
 *               mputm: None
 */

#ifdef	__STDC__
int
s_inquire_request ( char * m, MESG * md )		
#else
int
s_inquire_request ( m, md )

char		*m;
MESG		*md;
#endif
{
	DEFINE_FNNAME (s_inquire_request)

	char		*form;
	char		*dest;
	char		*pwheel;
	char		*user;
	char		*req_id;
	short		status;
	RSTATUS		*rp;
	RSTATUS		*found = (RSTATUS *)0;

	ENTRYP
	(void) getmessage(m, S_INQUIRE_REQUEST,&form,&dest,
		&req_id,&user,&pwheel);

	TRACEs (form)
	TRACEs (dest)
	TRACEs (req_id)
	TRACEs (user)
	TRACEs (pwheel)

	for (rp = Request_List; rp != NULL; rp = rp->next)
	{
		/*
		**  ES Note:
		**  Lets do the security check up front before we do
		**  any other work.
		*/
		if (!SecAdvise (rp->secure, SA_READ, md))
			continue;

		if (*form && !SAME(form, rp->request->form))
			continue;

		if (*dest 
			&& !STREQU(dest, rp->request->destination)
			&& !STREQU(dest, rp->printer->printer->name))
			continue;
		
		if (*req_id && !STREQU(req_id, rp->secure->req_id))
			continue;

		if (*user && ! bangequ (user, rp->secure->user))
			continue;

		if (*pwheel && !SAME(pwheel, rp->pwheel_name))
			continue;
		
		if (found)
			(void) mputm(md, R_INQUIRE_REQUEST,
				 MOKMORE,
				 found->secure->req_id,
				 found->secure->user,
				 found->secure->size,
				 found->secure->date,
				 found->request->outcome,
				 found->printer->printer->name,
				 (found->form? found->form->form->name : ""),
				 NB(found->pwheel_name),
				 found->secure->lid
			);
		found = rp;
	}
	if (found)
		(void) mputm(md, R_INQUIRE_REQUEST,
			 status = MOK,
			 found->secure->req_id,
			 found->secure->user,
			 found->secure->size,
			 found->secure->date,
			 found->request->outcome,
			 found->printer->printer->name,
			 (found->form? found->form->form->name : ""),
			 NB(found->pwheel_name),
			 found->secure->lid
		);
	else
		(void) mputm(md, R_INQUIRE_REQUEST, status = MNOINFO, "", "",
				0L, 0L, 0, "", "", "", 0L);

	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_inquire_request_rank
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_inquire_request_rank ( char * m, MESG * md )		
#else
int
s_inquire_request_rank ( m, md )

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_inquire_request_rank)

	int	found_rank = 0;
	int	display_remote = 0;
	int	req_id_found = 0;
	int	index;
	int	len;
	int	all_flag = 0;
	time_t	last_time = (time_t) 0;
	char	*form;
	char	*dest;
	char	*pwheel;
	char	*user;
	char	*req_id;
	PSTATUS *plist = (PSTATUS *)1;
	char	*bang;
	short	prop;
	short	status;
	RSTATUS	*rp;
	RSTATUS *rp1;
	RSTATUS *rp2;
	RSTATUS *rp1_next;
	RSTATUS *rp2_next;
	RSTATUS	*found;
	PSTATUS	*pps;

	found = (RSTATUS *)0;

	(void) getmessage(m, S_INQUIRE_REQUEST_RANK,&prop, 
		   &form,&dest,&req_id,&user,&pwheel);

	if (Redispatch || !prop)
		goto SendBackStatus;

	/* Be overly careful, and ensure the temporary chain is clear */
	for (pps=walk_ptable(1); pps; pps = walk_ptable(0))
		pps->next_list = NULL;

	for (rp = Request_List; rp != NULL; rp = rp->next)
	{
		/*
		**  ES Note:
		**  Lets do the security check up front before we do
		**  any other work.
		*/
		if (!SecAdvise (rp->secure, SA_READ, md))
				continue;

		if (*req_id && !STREQU(req_id, rp->secure->req_id))
			continue;
		req_id_found++;

		if (*form && !SAME(form, rp->request->form))
			continue;

		if (*dest 
			&& !STREQU(dest, rp->request->destination)
			&& !STREQU(dest, rp->printer->printer->name))
			continue;


		if (*user && ! bangequ (user, rp->secure->user))
			continue;

		if (*pwheel && !SAME(pwheel, rp->pwheel_name))
			continue;
		
		if (rp->printer->status & PS_REMOTE
		    && rp->request->outcome & (RS_SENT | RS_SENDING)
		    && !(rp->request->outcome & RS_DONE)) {

			if (!rp->printer->next_list) {
				rp->printer->next_list = plist;
				plist = rp->printer;
			}
		}
	}

#if	0
	if (!*form && !*dest && !*req_id && !*user && !*pwheel) {
		/* kludge for the handling of the special 'all' case
		 * (which is passed as all fields being empty)
		 */
		all_flag++;
	}
#endif

	if (*req_id && !req_id_found) {
		/*
		 * This handles the case where a request id has been given,
		 * which wasn't found locally (above).  Need to check if
		 * the id is remote.
		 * First check the remote ids we already know about...
		 */
		for (rp = Remote_Request_List; rp != NULL; rp = rp->next) {
			if (!STREQU(req_id, rp->secure->req_id))
				continue;
			/* found */
			break;
		}
		if (rp) {
			/* The request-id could have been used by another
			 * system, but we'll first ask the system which used
			 * it last.
			 */
			if (!rp->printer->next_list) {
				rp->printer->next_list = plist;
				plist = rp->printer;
			}
			display_remote++;
		} else {
			/* We've never seen the id - need to request all
			 * known systems
			 */
			for (pps=walk_ptable(1); pps; pps = walk_ptable(0)) {
				if (!(pps->status & PS_REMOTE))
					continue;
				if (!pps->next_list) {
					pps->next_list = plist;
					plist = pps;
				}
				display_remote++;
			}
		}
	}

	bang = NULL;
	if ((*user && (bang=strchr(user, '!'))) || *dest || all_flag) {
		/*
		 * Either 'user' given in format which includes a host,
		 * or a destination given.  May need to check remotes.
		 */
		PSTATUS *	pps;
		int		flag_all=0;

		if (bang) {
			*bang = '\0';
			if (STREQU(NAME_ALL, user))
				flag_all++;
		}
		for (pps=walk_ptable(1); pps; pps = walk_ptable(0)) {
			if (!(pps->status & PS_REMOTE))
				continue;
			if (bang)
				if (!flag_all && !STREQU(pps->system->system->name, user))
					continue;
			if (*dest && !STREQU(dest, pps->printer->name))
				continue;

			if (!pps->next_list) {
				pps->next_list = plist;
				plist = pps;
			}
			display_remote++;
		}
		if (bang)
			*bang = '!';
	}


	if (plist != (PSTATUS *)1) {
		/* We have a chain of (remote) printers to question... */
		while (plist != (PSTATUS *)1) {
			SSTATUS *pss;

			pps = plist;
			pss = pps->system;
			if (pss->system->protocol == S5_PROTO) {
				PSTATUS **pplist = &plist->next_list;
				PSTATUS *tmp;
				/* Ugly!  For BSD systems, we need to make
				 * a request on a per-printer basis, but for
				 * SVR4 we need to request all printers on
				 * a system.  Why?  Because of; the design of
				 * the callback func (r_send_job()), the
				 * 'pmlist' associated with a system - not
				 * a printer, and the return from an
				 * R_INQUIRE_REQUEST_RANK does not contain
				 * any useful info for us to determine the
				 * requested SVR4 printer if the printer
				 * has no queued requests.  Oh, and
				 * md_wakeup() doesn't help either!
				 */
				/* remove any other requests to the system
				 * from our printer list. */
				while (*pplist != (PSTATUS *)1) {
					if ((*pplist)->system == pss) {
						tmp = (*pplist)->next_list;
						(*pplist)->next_list = NULL;
						*pplist = tmp;
						continue;
					}
					pplist = &(*pplist)->next_list;
				}
				pps = NULL;
			}
			plist = plist->next_list;
			askforstatus(pss, md, pps, 0, 0);
			if (pps)
				pps->next_list = NULL;
		}
		
		if (waitforstatus(m, md) == 0)
			return	0;
	}

SendBackStatus:
	if (Redispatch || !prop) {
		/* Ugly check 'cause we can't store enough state in
		 * the message.
		 * Need to find if the remote request list should be
		 * used. */
#if	0
		if (!*form && !*dest && !*req_id && !*user && !*pwheel) {
			/* kludge for the handling of the special 'all' case
		 	* (which is passed as all fields being empty)
		 	*/
			display_remote++;
		}
#endif
		if (!display_remote && *req_id) {
			for (rp = Remote_Request_List; rp != NULL; rp = rp->next) {
				if (!STREQU(req_id, rp->secure->req_id))
					continue;
				/* found */
				display_remote++;
				break;
			}
		}
		bang = NULL;
		if (!display_remote && (*user && (bang=strchr(user, '!'))) || *dest) {
			PSTATUS *	pps;
			int		flag_all=0;
			if (bang) {
				*bang = '\0';
				if (STREQU(NAME_ALL, user))
					flag_all++;
			}
			for (pps=walk_ptable(1); pps; pps = walk_ptable(0)) {
				if (!(pps->status & PS_REMOTE))
					continue;
				if (bang)
					if (!flag_all && !STREQU(pps->system->system->name, user))
						continue;
				if (*dest && !STREQU(dest, pps->printer->name))
					continue;
				display_remote++;
				break;
			}
			if (bang)
				*bang = '!';
		}
	}

	for (pps = walk_ptable(1); pps; pps = walk_ptable(0))
		pps->nrequests = 0;

	rp1 = Request_List;
	rp1_next = rp1;
	rp2 = NULL;
	if (display_remote)
		rp2 = Remote_Request_List;
	rp2_next = rp2;
	for ( ; rp1 || rp2; rp1 = rp1_next, rp2 = rp2_next) {
		if (rp1 && (!rp2 || rsort(&rp1, &rp2) < 0)) {
			/* use rp1 */
			rp = rp1;
			rp1_next = rp1->next;
		} else {
			/* use rp2 */
			rp = rp2;
			rp2_next = rp2->next;
		}

		/*
		**  ES Note:
		**  Lets do the security check up front before we do
		**  any other work.
		*/
		if (!SecAdvise (rp->secure, SA_READ, md))
				continue;

		if (rp->printer && !(rp->request->outcome & RS_DONE))
			rp->printer->nrequests++;

		if (*req_id && !STREQU(req_id, rp->secure->req_id))
			continue;

		if (*form && !SAME(form, rp->request->form))
			continue;

		if (*dest 
			&& !STREQU(dest, rp->request->destination)
			&& !STREQU(dest, rp->printer->printer->name))
			continue;

		if (*user && ! bangequ (user, rp->secure->user))
			continue;

		if (*pwheel && !SAME(pwheel, rp->pwheel_name))
			continue;

		if (found) {
			if (found->secure->date)
				last_time = found->secure->date;
			else if (!last_time)
				last_time = time((time_t *)NULL);
			(void) mputm(md, R_INQUIRE_REQUEST_RANK,
				  status = MOKMORE,
				  found->secure->req_id,
				  found->secure->user,
				  found->secure->size,
				  last_time,
				  found->request->outcome,
				  found->printer->printer->name,
				  (found->form? found->form->form->name : ""),
				  NB(found->pwheel_name),
				  (found->status & RSS_RANK) ?
					found->rank : found_rank,
				  found->secure->lid);
		}
		found = rp;
		found_rank = found->printer->nrequests;
	}

	if (found) {
		if (found->secure->date)
			last_time = found->secure->date;
		else if (!last_time)
			last_time = time((time_t *)NULL);
		(void) mputm(md, R_INQUIRE_REQUEST_RANK,
			 status = MOK,
			 found->secure->req_id,
			 found->secure->user,
			 found->secure->size,
			 last_time,
			 found->request->outcome,
			 found->printer->printer->name,
			 (found->form? found->form->form->name : ""),
			 NB(found->pwheel_name),
			 (found->status & RSS_RANK) ?
				found->rank : found_rank,
			 found->secure->lid);
	} else
		(void) mputm(md, R_INQUIRE_REQUEST_RANK, status = MNOINFO,
			"", "", 0L, 0L, 0, "", "", "", 0, 0L);

	return	status == MOK ? 1 : 0;
}



#ifdef	__STDC__
static int
_move_request(char *m, MESG *md, int mtype, char *req_id, char *dest)
#else
static int
_move_request(m, md, mtype, req_id, dest)
char		*m;
MESG		*md;
int		mtype;
char		*req_id;
char		*dest;
#endif
{
	DEFINE_FNNAME (_move_request)

	PSTATUS		*prp = 0;
	RSTATUS		*rp;
	char		*olddest;
	short		err;
	short		status = MUNKNOWN;
	long		l1 = 0;
	EXEC		*oldexec;
	PSTATUS		*oldprinter;

	if (!(prp = search_ptable(dest)) && !( search_ctable(dest))) {
		status = MUNKNOWN;
		goto status_return;
	}

	rp = request_by_id(req_id);

	if (Redispatch && req_id) {
		status = rp->new_dest_status;
		goto status_return;
	}

	if (rp) {
		if (STREQU(rp->request->destination, dest)
			|| STREQU(rp->printer->printer->name, dest))
		{
			/*
			**  ES Note:
			**  This just says that it is already bound for
			**  the requested destination.
			*/
			status = MOK;
			goto status_return;
		}
		if (rp->request->outcome & (RS_DONE|RS_NOTIFYING))
		{
			status = M2LATE;
			goto status_return;
		}
		if (rp->request->outcome & RS_CHANGING)
		{
			status = MBUSY;
			goto status_return;
		}
		if (rp->status & RSS_MOVE) {
			status = MBUSY;
			goto status_return;
		}
		if (rp->request->outcome & RS_GONEREMOTE) {
			/* Request already gone to the remote.
			 * Need to pass this move request to the remote.
			 */

			/*
			 * Only allow remote requests to be moved to
			 * a different print queue on the same system
			 */
			if (prp->system != rp->printer->system) {
				status = MGONEREMOTE;
				goto status_return;
			}

			/*
			 * No support for remote moving of requests
			 * under NUC.
			if (prp->system->protocol == NUC_PROTO) {
				status = MGONEREMOTE;
				goto status_return;
			}

			/* We don't move the queue here, but need to
			 * temporary change the request so that we can validate
			 * the new destination. */
			olddest = rp->request->destination;
			oldprinter = rp->printer;
			if (!STREQU(olddest, NAME_ANY))
				rp->request->destination = Strdup(dest);
			else
				rp->request->destination = Strdup(olddest);
			rp->printer = prp;

			err = validate_request(rp, (char **)0, 1);

			Free(rp->request->destination);
			rp->request->destination = olddest;
			rp->printer = oldprinter;
			if (err != MOK) {
				status = err;
				l1 = chkprinter_result;
				goto status_return;
			}

			/* The move request looks OK from this end,
			 * send it onto the remote */
			rp->status |= (RSS_MOVE|RSS_SENDREMOTE);
			rp->new_dest = prp;

			schedule(EV_SYSTEM, rp->printer->system);
			if (rp->status & RSS_MOVE) {
				/*
				 * Response hasn't come back straight
				 * away, we need to link in a callback
				 * into the request.
				 */
				SUSPENDED *s;
				unsigned long size;
				s = Malloc(sizeof(SUSPENDED));
				size = msize(m);
				s->message = Malloc(size);
				memcpy(s->message, m, size);
				s->md = md;
				rp->move_suspend = s;
				return 0;
			}
			status = rp->new_dest_status;
			goto status_return;
		}

		oldexec = rp->printer->exec;
		olddest = rp->request->destination;
		oldprinter = rp->printer;
		if (!STREQU(olddest, NAME_ANY))
			rp->request->destination = Strdup(dest);
		else
			rp->request->destination = Strdup(olddest);
		rp->printer = prp;
		/*
		**  ES Note:
		**  'validate_request' will check to see if the user can
		**  print on the requested destination.
		*/
		if ((err = validate_request(rp, (char **)0, 1)) == MOK)
		{
			Free(olddest);
			(void) putrequest(rp->req_file, rp->request);
			(void) mputm(md, mtype, MOK, mtype==R_MOVE_REQUEST?0L:req_id);

			/*
			 * If the request was being filtered or was printing,
			 * it would have been stopped in "validate_request()",
			 * but only if it has to be refiltered. Thus, the
			 * filtering has been stopped if it has to be stopped,
			 * but the printing may still be going.
			 */
			if (rp->request->outcome & RS_PRINTING
				&& !(rp->request->outcome & RS_STOPPED))
			{
				rp->request->outcome |= RS_STOPPED;
				terminate (oldexec);
			}
			maybe_schedule(rp);

			return	1;
		}
		status = err;
		l1 = chkprinter_result;
		Free(rp->request->destination);
		rp->request->destination = olddest;
		rp->printer = oldprinter;
	}

status_return:
	if (mtype==R_MOVE_REQUEST)
		(void) mputm(md, R_MOVE_REQUEST, status, l1);
	else
		(void) mputm(md, R_MOVE_REMOTE_REQUEST, status, req_id);
	return	status == MOK ? 1 : 0;
}



#ifdef	__STDC__
int
s_move_remote ( char * m, MESG * md )		
#else
int
s_move_remote ( m, md )
char		*m;
MESG		*md;
#endif
{
	DEFINE_FNNAME (s_move_remote)

	char		*req_id;
	char		*dest;
	char	*s1;
	char	*s2;

	(void) getmessage(m, S_MOVE_REMOTE_REQUEST, &s1, &s2, &dest, &req_id);

	return _move_request(m, md, R_MOVE_REMOTE_REQUEST, req_id, dest);
}



/*
 * Procedure:     s_move_request
 *
 * Restrictions:
 *               mputm: None
 *               putrequest: None
*/
#ifdef	__STDC__
int
s_move_request ( char * m, MESG * md )		
#else
int
s_move_request ( m, md )
char		*m;
MESG		*md;
#endif
{
	DEFINE_FNNAME (s_move_request)

	char		*req_id;
	char		*dest;


	(void) getmessage(m, S_MOVE_REQUEST, &req_id, &dest);

	return _move_request(m, md, R_MOVE_REQUEST, req_id, dest);
}



/*
 * Procedure:     s_move_dest
 *
 * Restrictions:
 *               mputm: None
 *               putrequest: None
*/

#ifdef	__STDC__
int
s_move_dest  ( char * m, MESG * md )		
#else
int
s_move_dest ( m, md )

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_move_dest)

	char	*dest;
	char	*fromdest;
	char	*olddest;
	char	*found = (char *)0;
	short	num_ok = 0;
	short	status;
	RSTATUS	*rp;
	EXEC	*oldexec;

	(void) getmessage(m, S_MOVE_DEST, &fromdest, &dest);

	if (!search_ptable(fromdest) && !search_ctable(fromdest))
	{
		(void) mputm(md, R_MOVE_DEST, MNODEST, fromdest, 0);
		return	0;
	}

	if (!(search_ptable(dest)) && !(search_ctable(dest)))
	{
		(void) mputm(md, R_MOVE_DEST, MNODEST, dest, 0);
		return	0;
	}

	if (STREQU(dest, fromdest))
	{
			/*
			**  ES Note:
			**  This just means that the 'to' and 'from' destinations
		**  are the same.
			*/
		(void) mputm(md, R_MOVE_DEST, MOK, "", 0);
		return	1;
	}

	for (rp = Request_List; rp != NULL; rp = rp->next)
	{

		if (!STREQU(fromdest, rp->request->destination)
			&& !STREQU(fromdest, rp->printer->printer->name))
			continue;
		
		if (!(rp->request->outcome &
			(RS_DONE|RS_CHANGING|RS_NOTIFYING|RS_GONEREMOTE)))
		{
			oldexec = rp->printer->exec;
			olddest = rp->request->destination;
			rp->request->destination = Strdup(dest);
			/*
			**  ES Note:
			**  'validate_request' will check to see if the user can
			**  print on the requested destination.
			*/
			if (validate_request(rp, (char **)0, 1) == MOK)
			{
				num_ok++;
				Free(olddest);
				(void) putrequest(rp->req_file, rp->request);
	
				/*
			 	* If the request was being filtered or was printing,
			 	* it would have been stopped in "validate_request()",
			 	* but only if it has to be refiltered. Thus, the
			 	* filtering has been stopped if it has to be stopped,
			 	* but the printing may still be going.
			 	*/
				if (rp->request->outcome & RS_PRINTING
					&& !(rp->request->outcome & RS_STOPPED))
				{
					rp->request->outcome |= RS_STOPPED;
					terminate (oldexec);
				}
	
				maybe_schedule(rp);
	
				continue;
			}
			Free(rp->request->destination);
			rp->request->destination = olddest;
		}
	if (found)
		(void) mputm(md, R_MOVE_DEST, MMORERR, found, 0);

	found = rp->secure->req_id;
	}

	if (found)
		(void) mputm(md, R_MOVE_DEST, status = MERRDEST, found, num_ok);
	else
		(void) mputm(md, R_MOVE_DEST, status = MOK, "", num_ok);
	return	status == MOK ? 1 : 0;
}

/**
 ** reqpath
 **/

#ifdef	__STDC__
static char *
reqpath ( char * file, char ** idnumber )		
#else
static char *
reqpath ( file, idnumber )

char		*file;
char		**idnumber;
#endif
{
	DEFINE_FNNAME (reqpath)

	char	*path;
	char	*cp;
	char	*cp2;
	
	/*
	**	/var/spool/lp/tmp/machine/123-0
	**	/var/spool/lp/temp/123-0
	**	/usr/spool/lp/temp/123-0
	**	/usr/spool/lp/tmp/machine/123-0
	**	123-0
	**	machine/123-0
	**
	**	/var/spool/lp/tmp/machine/123-0 + 123
	*/
	if (*file == '/')
	{
		if (STRNEQU(file, Lp_Spooldir, strlen(Lp_Spooldir)))
			cp = file + strlen(Lp_Spooldir) + 1;
		else
			if(STRNEQU(file, "/usr/spool/lp", 13))
				cp = file + strlen("/usr/spool/lp") + 1;
			else
			{
				*idnumber = NULL;
				return(NULL);
			}

		if (STRNEQU(cp, "temp", 4))
		{
			cp += 5;
			path = makepath(Local_System, cp, NULL);
		}
		else
			path = Strdup(cp);
	}
	else
	{
		if (strchr(file, '/'))
			path = makepath(file, NULL);
		else
			path = makepath(Local_System, file, NULL);
	}

	cp = strrchr(path, '/');
	cp++;
	if ((cp2 = strchr(cp, '-')) == NULL)
		*idnumber = Strdup(cp);
	else
	{
		*cp2 = '\0';
		*idnumber = Strdup(cp);
		*cp2 = '-';
	}

	return	path;
}

