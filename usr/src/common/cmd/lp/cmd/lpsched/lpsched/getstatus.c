/*		copyright	"%c%" 	*/

#ident	"@(#)getstatus.c	1.4"
#ident  "$Header$"

/*******************************************************************************
 *
 * FILENAME:    getstatus.c
 *
 * DESCRIPTION: Handle the getting of a status of printer(s) on a remote print
 *              server. Note: the request may be suspended while we wait for
 *              the remote end to respond.
 *
 * SCCS:	getstatus.c 1.4  7/21/97 at 14:35:07
 *
 * CHANGE HISTORY:
 *
 * 21-07-97  Paul Cunningham        ul97-19724
 *           Change function waitforstatus so that it sets  up and starts a
 *           to timeout out a remote askforstatus() request. Also add a new 
 *           function scanSuspList() which scans down the Suspend_List looking
 *           for items that have been timed out and executes them
 *
 *******************************************************************************
 */

#include "stdlib.h"
#include "unistd.h"

#include "lpsched.h"

int			Redispatch	= 0;

RSTATUS *		Status_List	= 0;

static SUSPENDED	*Suspend_List	= 0;

#ifdef	__STDC__
RSTATUS *	mkreq ( SSTATUS *, PSTATUS * );
#else
RSTATUS *	mkreq ();
#endif

#define SHOULD_NOTIFY(PRS) 	((PRS)->request->actions & \
				 (ACT_MAIL|ACT_WRITE|ACT_NOTIFY) || \
				 (PRS)->request->alert)

/**
 ** mesgdup()
 **/

static char *
#ifdef	__STDC__
mesgdup (
	char *			m
)
#else
mesgdup (m)
	char *			m;
#endif
{
	DEFINE_FNNAME (mesgdup)

	char *			p;

	unsigned long		size	= msize(m);

	p = Malloc(size);
	(void) memcpy (p, m, size);
	return (p);
}


/**
 ** askforstatus()
 **/

void
#ifdef	__STDC__
askforstatus (
	SSTATUS *		pss,
	MESG *			md,
	PSTATUS *		pps,
	int			remote_printer_flag,
	int			check_timer
)
#else
askforstatus (pss, md, pps, remote_printer_flag, check_timer)
	SSTATUS *		pss;
	MESG *			md;
	PSTATUS *		pps;
	int			remote_printer_flag;
	int			check_timer;
#endif
{
	DEFINE_FNNAME (askforstatus)

	WAITING *		w;
	time_t			now;


	/*
	 * If wait is -1, the user has been through all of this once
	 * already and should not be kept waiting. This remedies the
	 * situation where the "md" is waiting for 2 or more systems
	 * and the response from one system comes more than
	 * USER_STATUS_EXPIRED seconds after another has reported back
	 * (i.e., while waiting for one system, the other expired again).
	 * Without this check, the <md> could deadlock always waiting
	 * for the status from one more system.
	 */
	if (md->wait == -1) {
		schedlog ("Already waited for status once, don't wait again.\n");
		return;
	}

	if (pss->status & SS_RETRYING) {
		/* Problems connecting to the remote system.
		 * This func is only called to satisfy a users
		 * request (its not core), so we'll simply avoid
		 * updating the remote requests for this system.
		 * The user will see the old status of remote requests.
		 */
		schedlog("Avoiding system %s for remote request - it might be down\n", pss->system->name);
		return;
	}

	now = time((time_t *)0);
	if (check_timer) {
		if ((now - pss->laststat) <= USER_STATUS_EXPIRED)
			check_timer=0;
		if (pps && (now - pps->laststat) <= USER_STATUS_EXPIRED)
			check_timer=0;
		if (check_timer)
			schedlog ("Timer has not expired yet.\n");
	}
	if (!check_timer) {
		RSTATUS *prs;

		pss->laststat = now;
		if (pps)
			pps->laststat = now;
		w = (WAITING *)Malloc(sizeof(WAITING));
		w->md = md;
		w->printer = pps;
		if (remote_printer_flag) {
			w->next = pss->ps_waiting;
			pss->ps_waiting = w;
		} else {
			w->next = pss->waiting;
			pss->waiting = w;
		}
		md->wait++;
		schedlog ("Scheduling status call to %s\n", pss->system->name);
		prs = mkreq (pss, pps);
		if (remote_printer_flag)
			prs->status |= RSS_PRINTSTAT;
		/* mark the request as a user status request so we can fail
		 * it quickly if the remote system is down. */
		prs->status |= RSS_FOR_USER;
		schedule (EV_SYSTEM, pss);
	}

	return;
}

/**
 ** waitforstatus()
 **/

int
#ifdef	__STDC__
waitforstatus (
	char *			m,
	MESG *			md
)
#else
waitforstatus (m, md)
	char *			m;
	MESG *			md;
#endif
{
	DEFINE_FNNAME (waitforstatus)

	SUSPENDED *		s;
	

	if (md->wait <= 0) {
		md->wait = 0;
		schedlog ("No requests to wait for.\n");
		return (-1);
	}

	s = (SUSPENDED *)Malloc(sizeof(SUSPENDED));
	s->message = mesgdup(m);
	s->md = md;

	/* timeout in n seconds but also cause alarm in n x 2 secs in case 
	 * the lpsched is polling so that we get back into the main lpsched
	 * loop to check on the Suspend_List (scanSuspList()).
	 */
	s->timeoutAt = (time((time_t *)0)) + TIMEOUT_GSTAT;
	(void)alarm( (unsigned)(2*TIMEOUT_GSTAT));

	s->next = Suspend_List;
	Suspend_List = s;

	schedlog ("Suspend %lu for status\n", md);

	return (0);
}

/*
 * Procedure:     load_bsd_stat
 *
 * Restrictions:
 *               open_lpfile: None
 *               Unlink: None
 *               fgets: None
 *               close_lpfile: None
*/

void
#ifdef	__STDC__
load_bsd_stat (
	SSTATUS *		pss,
	PSTATUS *		pps
)
#else
load_bsd_stat (pss, pps)
	SSTATUS *		pss;
	PSTATUS *		pps;
#endif
{
	DEFINE_FNNAME (load_bsd_stat)

	FILE *			fp;

	char			buf[BUFSIZ];
	char			mbuf[MSGMAX];

	char *			file;
	char *			rmesg	= NULL;
	char *			dmesg	= NULL;
	char *			req	= "";
	char *			cp;
	char *			job_id;
	char *			user;
	char *			rank_ptr;
	char *			host;
	char *			size_ptr;

	RSTATUS *		prs;

	time_t			now;

	short			status	= 0;

	short			rank;
	int			req_flag = 0;
	int			size;


	file = pps->alert->msgfile;
	if ((fp = open_lpfile(file, "r", MODE_NOREAD)) == NULL)
		return;
	(void) Unlink (file);
	
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) - 1] = '\0';
	
		schedlog (">>>%s\n", buf);

		switch(*buf) {
		case '%':
			/*
			 * MORE: add code to fetch old status and restore
			 * it
			 */
			break;
		    
		case '-':
			if (strstr(buf + 2, "queue")) {
				schedlog ("Added to reject reason\n");
				status |= PS_REJECTED;
				(void) addstring (&rmesg, buf + 2);
				(void) addstring (&rmesg, "\n");
			} else {
				schedlog ("Added to disable reason\n");
				status |= PS_DISABLED;
				(void) addstring (&dmesg, buf + 2);
				(void) addstring (&dmesg, "\n");
			}
			break;
		    
		default:
			/*
			 * Message format:
			 *
			 *	user:rank:jobid:host:size
			 */
			size = 0;
			user = buf;
			cp = strchr(user, ':');
			if (!(cp))
				break;
			*cp++ = '\0';
			rank_ptr = cp;
			cp = strchr(cp, ':');
			if (!(cp))
				break;
			*cp++ = '\0';
			rank = strtol(rank_ptr, NULL, 10);
			/* fix for when talking to old (buggy) SVR4 bsd
			 * lpNets (where the job-id also includes the
			 * printer name).
			 */
			if (!(job_id=strchr(cp, '-')))
				job_id =  cp;
			else
				job_id++;
			cp = strchr(cp, ':');
			if (!(cp))
				break;
			*cp++ = '\0';
			host = cp;
			cp = strchr(cp, ':');
			if (cp) {
				*cp++ = '\0';
				size_ptr = cp;
				/* test for next ':' - future expansion */
				if ((cp = strchr(cp, ':')))
					*cp = '\0';
				size = strtol(size_ptr, NULL, 10);
			}
			prs = request_by_jobid (pps->printer->name, job_id,
				pss->system->protocol);
			if (!prs) {
				size_t	len;
				char 	*req_id;
				char	*full_user;
				len = strlen(pps->printer->name) + strlen(job_id) + 2;
				req_id = Malloc(len);

				if (!req_id)
					continue;
				sprintf(req_id, "%s-%s", pps->printer->name, job_id);
				len = strlen(user)+strlen(host)+2;
				full_user = Malloc(len);
				if (!full_user)
					continue;
				sprintf(full_user, "%s!%s", host, user);
				len = update_remote_req(pss, req_id, full_user, size, 0, 0, pps->remote_name, NULL, NULL, rank, 0);
				Free(full_user);
				if (len || rank) {
					Free(req_id);
					continue;
				}
				status |= PS_BUSY;
				if (req_flag++)
					free(req);	/* shouldn't happen */
				req = req_id;
				continue;
			}
			schedlog ("Saving a rank of %d\n", rank);
			prs->status |= (RSS_MARK|RSS_RANK);
			if ((prs->rank = rank) == 0) {
				if (req_flag) {
					/* shouldn't happen */
					free(req);
					req_flag = 0;
				}
				status |= PS_BUSY;
				req = prs->secure->req_id;
			}
		}
	}

	schedlog ("Cleaning up old requests\n");
	purge_remote(pss, pps, 0);
	BEGIN_WALK_BY_PRINTER_LOOP (prs, pps)
		if (!(prs->request->outcome & RS_SENT))
			continue;
		if (prs->status & RSS_MARK) {
			prs->status &= ~RSS_MARK;
			continue;
		}
		schedlog ("Completed \"%s\"\n", prs->secure->req_id);
		prs->request->outcome &= ~RS_ACTIVE;
		prs->request->outcome |= RS_PRINTED;
		if (SHOULD_NOTIFY(prs))
			prs->request->outcome |= RS_NOTIFY;
		notify (prs, (char *)0, 0, 0, 0);
		prs->printer->request = 0;
		check_request(prs);
	END_WALK_LOOP

	now = time((time_t *)0);
	schedlog ("Saving printer status\n");

	pps->rmt_status = status;
	pps->rmt_dis_date = now;
	pps->rmt_rej_date = now;
	if (pps->rmt_form) {
		Free(pps->rmt_form);
		pps->rmt_form = NULL;
	}
	if (pps->rmt_pwheel) {
		Free(pps->rmt_pwheel);
		pps->rmt_pwheel = NULL;
	}
	if (pps->rmt_dis_reason) {
		Free(pps->rmt_dis_reason);
		pps->rmt_dis_reason = NULL;
	}
	if (pps->rmt_rej_reason) {
		Free(pps->rmt_rej_reason);
		pps->rmt_rej_reason = NULL;
	}
	if (pps->rmt_req_id) {
		Free(pps->rmt_req_id);
		pps->rmt_req_id = NULL;
	}

	if (dmesg)
		pps->rmt_dis_reason = Strdup(dmesg);
	if (rmesg)
		pps->rmt_rej_reason = Strdup(rmesg);
	if (req) {
		if (req_flag)
			pps->rmt_req_id = req;
		else
			pps->rmt_req_id = Strdup(req);
	}

	(void) close_lpfile(fp);
	return;
}

/**
 ** update_req()
 **/

int
#ifdef	__STDC__
update_req (
	char *			req_id,
	long			rank
)
#else
update_req (req_id, rank)
	char *			req_id;
	long			rank;
#endif
{
	DEFINE_FNNAME (update_req)

	RSTATUS		*prs;


	if (!(prs = request_by_id(req_id)))
		return 0;
	
	prs->status |= RSS_RANK;
	prs->rank = rank;

	return 1;
}

/**
 ** md_wakeup()
 **/

void
#ifdef	__STDC__
md_wakeup (
	SSTATUS *		pss,
	PSTATUS *		pps,
	int			remote_printer_flag
)
#else
md_wakeup (pss, pps, remote_printer_flag)
	SSTATUS *		pss;
	PSTATUS *		pps;
	int			remote_printer_flag;
#endif
{
	DEFINE_FNNAME (md_wakeup)

	WAITING **		wpp;

	int			wakeup	= 0;

	SUSPENDED *		susp;
	SUSPENDED *		newlist	= 0;


	/* Which waiting queue? */
	if (remote_printer_flag)
		wpp = &(pss->ps_waiting);
	else
		wpp = &(pss->waiting);
	while (*wpp) {
		if (!pps || (*wpp)->printer == pps) {
			WAITING	*w = *wpp;

			if (--(w->md->wait) <= 0)
				wakeup = 1;
			w->md = NULL;
			w->printer = NULL;
			*wpp = w->next;
			Free(w);
			continue;
		}
		wpp = &((*wpp)->next);
	}

	if (wakeup) {
		while (Suspend_List) {
			susp = Suspend_List;
			Suspend_List = susp->next;
			if (susp->md->wait <= 0) {
				susp->md->wait = -1;
				Redispatch = 1;
				dispatch (mtype(susp->message), susp->message, susp->md);
				Redispatch = 0;
				susp->md = (MESG *)0;
				Free (susp->message);
				Free (susp);
			} else {
				susp->next = newlist;
				newlist = susp;
			}
		}
		Suspend_List = newlist;
	}
}


/**/
/*******************************************************************************
 *
 * FUNCTION:     scanSuspList()
 *
 * DESCRIPTION:  Scan the suspended Request Status list to see if any of the
 *               requests have timed out. A request would normally only time
 *               out if the remote end is not responding. If the request has
 *               timed out continue the suspended function and free the item
 *               from the Suspend_List.
 *
 * PARAMETERS:   None
 *
 * RESULT CODES: None
 *
 *******************************************************************************
 */

void
#ifdef	__STDC__
scanSuspList()
#else
scanSuspList( void);
#endif
{
  DEFINE_FNNAME (scanSuspList)

  SUSPENDED *p_nextOnList = (SUSPENDED*)NULL;
  SUSPENDED *p_prevOnList = (SUSPENDED*)NULL;
  SUSPENDED *p_timedOut = (SUSPENDED*)NULL;
  time_t timeNow = 0;

  /* -------- */

  p_nextOnList = Suspend_List;
  while ( p_nextOnList)
  {
    timeNow = time((time_t *)0); /* the time now is */

    if ( p_nextOnList->timeoutAt <= timeNow)
    {
      schedlog( "scanSuspList: handle timed out item on Suspend_List\n");
      p_timedOut = p_nextOnList;
      p_timedOut->md->wait = -1;
      Redispatch = 1;
      dispatch
         (
         mtype( p_timedOut->message), p_timedOut->message, p_timedOut->md
         );
      Redispatch = 0;

      /* remove the timed out item from the list and
       * then free it
       */
      if ( p_prevOnList == (SUSPENDED*)NULL)
      {
        /* removing first item on list */
        Suspend_List = p_nextOnList->next;
      }
      else
      {
        p_prevOnList->next = p_nextOnList->next;
      }
      p_nextOnList = p_nextOnList->next;

      Free( p_timedOut->message);
      Free( p_timedOut);
    }

    else
    {
      /* get next item on list */
      p_prevOnList = p_nextOnList;
      p_nextOnList = p_nextOnList->next;
    }
  } /* while */

} /* scanSuspList */


/**
 ** mkreq()
 **/

RSTATUS *
#ifdef	__STDC__
mkreq (
	SSTATUS *		pss,
	PSTATUS	*		pps
)
#else
mkreq (pss, pps)
	SSTATUS *		pss;
	PSTATUS *		pps;
#endif
{
	DEFINE_FNNAME (mkreq)

	char			idno[STRSIZE(BIGGEST_REQID_S) + 1];

	RSTATUS *		prs;
	RSTATUS *		r;


	/*
	 * Create a fake request with enough information to
	 * fool the various request handling routines.
	 */
	prs = allocr();
	(void) sprintf (idno, "%ld", _alloc_req_id());
	prs->secure->req_id = makestr("(fake)", "-", idno, (char *)0);
	prs->system = pss;
	if (pps)
		prs->printer = pps;

	if (Status_List) {
		for (r = Status_List; r->next; r = r->next)
			;
		r->next = prs;
		prs->prev = r;
	} else
		Status_List = prs;

	return	prs;
}

/**
 ** rmreq()
 **/

void
#ifdef	__STDC__
rmreq (
	RSTATUS *		prs
)
#else
rmreq (prs)
	RSTATUS *		prs;
#endif
{
	DEFINE_FNNAME (rmreq)

	RSTATUS *		old_Request_List;


	/*
	 * UGLY: Rename the "Request_List" to "Status_List",
	 * so that "freerstatus()" (actually "remove()") will
	 * unlink the correct list.
	 */
	old_Request_List = Request_List;
	Request_List = Status_List;
	freerstatus (prs);
	Status_List = Request_List;
	Request_List = old_Request_List;

	return;
}
