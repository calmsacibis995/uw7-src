/*		copyright	"%c%" 	*/


#ident	"@(#)disp5.c	1.3"
#ident  "$Header$"

#include "stdlib.h"
#include "unistd.h"

#include "dispatch.h"
 
extern int		Net_fd;

extern MESG *		Net_md;

#define	WHO_AM_I	I_AM_LPSCHED
#include "oam.h"

/**
 ** s_child_done()
 **/

int
#ifdef	__STDC__
s_child_done (char *m, MESG *md)
#else
s_child_done (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_child_done)

	long			key;
	short			slot;
	short			status;
	short			err;


	(void) getmessage (m, S_CHILD_DONE, &key, &slot, &status, &err);

	if (slot < 0
		|| slot >= ET_Size
		|| Exec_Table[slot].key != key
		|| Exec_Table[slot].md != md)
	{
#ifdef	DEBUG
		if (debug & (DB_EXEC|DB_DONE))
		{
			execlog (
				"FAKE! slot %d pid ??? status %d err %d\n",
				slot, status, err);
		}
#endif
		return	0;
	}


#ifdef	DEBUG
	if (debug & (DB_EXEC|DB_DONE))
	{
		EXEC *ep = &Exec_Table[slot];

		execlog (
			"OKAY: slot %d pid %d status %d err %d\n",
			slot, ep->pid, status, err);
		execlog ("%e", ep);
	}
#endif
	/*
	 * Remove the message descriptor from the listen
	 * table, then forget about it; we don't want to
	 * accidently match this exec-slot to a future,
	 * unrelated child.
	 */
	DROP_MD (Exec_Table[slot].md) /* EMPTY */ ;
	Exec_Table[slot].md = 0;

	Exec_Table[slot].pid = -99;
	Exec_Table[slot].status = status;
	Exec_Table[slot].errno = err;
	DoneChildren++;

	if (Exec_Table[slot].type == EX_INTERF)
		CutEndJobAuditRec ((int) status, 1,
			Exec_Table[slot].ex.printer->request->secure->user,
			Exec_Table[slot].ex.printer->request->secure->req_id);
	else
	if (Exec_Table[slot].type == EX_SLOWF)
		CutEndJobAuditRec ((int) status, 2,
			Exec_Table[slot].ex.request->secure->user,
			Exec_Table[slot].ex.request->secure->req_id);

	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     r_new_child
 *
 * Restrictions:
 *               ioctl(2): None
 *               mconnect: None
 *               flvlfile(2): None
 *               mputm: None
 *               mdisconnect: None
*/

/* ARGSUSED0 */
#ifdef	__STDC__
int
r_new_child (char *m, MESG *md)
#else
int
r_new_child (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (r_new_child)

	int			were_waiting	= 0;
    	int			cred_size;
	char *			name;
	char *			originator_name;
	short			status;

	MESG *			new_md;
	MESG *			drop_md;
	MESG *			hold_md;
	SSTATUS *		pss;
	SSTATUS *		default_pss;
	short			protocol;
	PSTATUS *		pps;

	struct strrecvfd	recvfd;
    	struct s_strrecvfd *	recvbufp;


	(void) getmessage (m, R_NEW_CHILD, &name, &originator_name, &protocol, &status);
	schedlog (
		"Received R_NEW_CHILD for system %s (type = %s) requested by %s\n",
		name, (protocol == S5_PROTO) ? "s5" : "BSD",
		originator_name
	);

	if (!(pss = search_stable(name)))
	{
		if (!(default_pss = default_system(protocol)))
		{
			schedlog ("%s is an unknown system\n", name);
			recvfd.fd = -1;	/* So as not to clobber someone else */
			(void) ioctl (Net_fd, I_RECVFD, &recvfd);
			(void) close (recvfd.fd);
			return 0;
		}
		/*
		**  Manufacture a SSTATUS structure.
		*/
		pss = (SSTATUS *)Calloc(1, sizeof(SSTATUS));
		pss->system = (SYSTEM *)Calloc(1, sizeof(SYSTEM));
		pss->exec = (EXEC *)Calloc(1, sizeof(EXEC));

		*(pss->system) = *(default_pss->system);

		pss->system->name = strdup(name);

		addone ((void ***)&SStatus, pss, &ST_Size, &ST_Count);
		auto_putsystem (name, protocol);
	}


	switch (status) {

	case MOK:
		break;

	case MUNKNOWN:
		/*
		 * The network manager doesn't know about this system.
		 * While strictly speaking this ought not occur, it can
		 * because we can't prevent someone from mucking with
		 * the system table. So if this happens we disable the
		 * printer(s) that go to this system.
		 */
		for (pps = walk_ptable(1); pps; pps = walk_ptable(0))
			if (pps->system == pss)
				(void)disable (pps, CUZ_NOREMOTE, DISABLE_STOP);
		return 0;

	case MNOSTART:
		/*
		 * We get this only in response to our request for a
		 * connection. However, between the time we had asked
		 * and the receipt of this response we may have received
		 * another R_NEW_CHILD originated remotely. So, we try
		 * again later only if we still need to.
		 */
		if (!pss->exec->md) {
			schedlog (
				"Failed contact with %s, retry in %d min.\n",
				name,
				WHEN_NOSTART / MINUTE
			);
			resend_remote (pss, WHEN_NOSTART);
		}
		return 0;

	default:
		schedlog (
			"Strange status (%d) in R_NEW_CHILD for %s.\n",
			status,
			name
		);
		return 0;
	}
	/*
	**	Retrieve the file descriptor
	*/
	if ((cred_size = secadvise (0, SA_SUBSIZE, 0)) < 0)
	{
		/*
		**  Errno is already set.
		*/
		return	0;
	}
	recvbufp = (struct s_strrecvfd *)
        		Calloc(1, sizeof (struct s_strrecvfd) +
			cred_size - sizeof (struct sub_attr));

	if (!recvbufp)
	{
		errno = ENOMEM;
		return	0;
	}
	if (ioctl (Net_fd, I_S_RECVFD, recvbufp) < 0)
	{
		switch (errno) {
		case EBADMSG:
			schedlog ("No file descriptor passed.\n");
			break;

		case ENXIO:
			schedlog ("System server terminated early.\n");
			break;

		case EMFILE:
			schedlog ("Too many open files!\n");
			break;
		}
		Free (recvbufp);
		return	0;
	}

	new_md = mconnect (NULL, recvbufp->fd, recvbufp->fd);
	if (! new_md)
		mallocfail ();

	new_md->credp = (struct sub_attr *) Calloc (1, cred_size);

	if (!new_md->credp)
	{
	    	Free (recvbufp);
	    	Free (new_md);
		errno = ENOMEM;
		return	0;
	}
	new_md->gid = recvbufp->gid;
	new_md->uid = recvbufp->uid;
	if (flvlfile (new_md->readfd, MAC_GET, &(new_md->lid)) < 0)
	{
		if (errno != ENOPKG) {
			int	save = errno;
			Free (recvbufp);
			Free (new_md->credp);
			Free (new_md);
			errno = save;
			return	0;
		}
	}
	(void)	memcpy (new_md->credp, &(recvbufp->s_attrs), cred_size);

	Free (recvbufp);
	/*
	 * Save this flag, because in the hustle and bustle below
	 * we may lose the original information.
	 */
	were_waiting = (pss->exec->flags & EXF_WAITCHILD);


	/*
	 * Check for a collision with another system trying to contact us:
	 *
	 *	* We had asked for a connection to this system (i.e. had
	 *	  sent a S_NEW_CHILD message), but this isn't the response
	 *	  to that message.
	 *
	 *	* We already have a connection.
	 *
	 * These cases are handled separately below, but the same
	 * arbitration is used: The system with the name that comes
	 * ``first'' in collating order gets to keep the connection
	 * it originated.
	 */
	if (were_waiting)
	{
		if (STREQU(Local_System, originator_name))
		{
			/*
			 * This is the usual case.
			 */
			schedlog ("Making new connection to %s\n", name);
			hold_md = new_md;
			drop_md = 0;

		}
		else
		{
			/*
			 * We have a pending collision, since we
			 * are still waiting for a response to our
			 * connection request (this isn't it). Resolve
			 * the collision now, by either accepting
			 * this response (we'll have to refuse our
			 * real response later) or by refusing this
			 * response.
			 */
			schedlog (
				"Potential collision between %s and %s\n",
				Local_System,
				name);
			if (strcmp(Local_System, name) < 0)
			{
				schedlog ("Take no connection.\n");
				hold_md = 0;
				drop_md = new_md;
			}
			else
			{
				schedlog ("Drop this connection.\n");
				hold_md = new_md;
				drop_md = 0;
			}
		}

	}
	else 
	if (pss->exec->md) {
		MESG *			my_md;
		MESG *			his_md;

		schedlog (
			"Collision between %s and %s!\n",
			Local_System,
			name);

		/*
		 * The message descriptor we got last time
		 * MAY NOT be for the connection we originated.
		 * We have to check the "originator_name" to be sure.
		 */
		if (STREQU(Local_System, originator_name))
		{
			my_md = new_md;
			his_md = pss->exec->md;
		}
		else
		{
			my_md = pss->exec->md;
			his_md = new_md;
		}

		/*
		 * (First means < 0, right?)
		 */
		if (strcmp(Local_System, name) < 0)
		{
			schedlog ("I win!\n");
			drop_md = his_md;
			hold_md = my_md;
		}
		else
		{
			schedlog ("He wins.\n");
			drop_md = my_md;
			hold_md = his_md;
		}

	}
	else
	{
		schedlog ("Accepting unsolicited connection.\n");
		hold_md = new_md;
		drop_md = 0;
	}
	if (drop_md)
	{
		if (drop_md == pss->exec->md)
		{
			schedlog ("Dropping fd %d from listen table\n",
				drop_md->readfd);
			DROP_MD (drop_md) /* EMPTY */ ;

			/*
			 * We are probably waiting on a response
			 * to an S_SEND_JOB from the network child
			 * on the other end of the connection we
			 * just dropped. If so, we have to resend the
			 * job through the new channel...yes, we know
			 * we have a new channel, as the only way to
			 * get here is if we're dropping the exising
			 * channel, and we do that only if we have to
			 * pick between it and the new channel.
			 */
			if (pss->exec->flags & EXF_WAITJOB)
			{
				resend_remote (pss, -1);
				were_waiting = 1;
			}
		}
		else
		{
			schedlog (
				"Sending S_CHILD_SYNC M2LATE on %x\n",
				drop_md);
			drop_md->type = MD_CHILD;
			(void) mputm (drop_md, S_CHILD_SYNC, M2LATE);
			(void) mdisconnect (drop_md);
		}
	}
	if (hold_md)
	{
		if (hold_md != pss->exec->md)
		{
			schedlog (
				"Sending S_CHILD_SYNC MOK on %x\n",
				hold_md);
			hold_md->type = MD_CHILD;
			(void) mputm (hold_md, S_CHILD_SYNC, MOK);
			pss->exec->md = hold_md;
			if (mlistenadd(pss->exec->md, POLLIN) == -1)
				mallocfail ();
		}
		pss->exec->flags &= ~EXF_WAITCHILD;
	}

	/*
	 * If we still have a connection to the remote system,
	 * and we had been waiting for the connection, (re)send
	 * the job.
	 */
	if (pss->exec->md && were_waiting)
		schedule (EV_SYSTEM, pss);

	return	1;
}

/*
 * Procedure:     r_send_job
 *
 * Restrictions:
 *               putrequest: None
*/
  
/* ARGSUSED */
#ifdef	__STDC__
int
r_send_job (char *m, MESG *md)
#else
int
r_send_job (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (r_send_job)

	char			buf[MSGMAX];

	char *			name;
	char *			sent_msg;
	char *			req_id;
	char *			remote_req_id;
	char *			remote_name;
	char *			s1;
	char *			s2;
	char *			s3;
	char *			s4;
	char *			s5;

	short			status;
	short			sent_size;
	short			rank;
	short			h1;

	long			lstatus;
	long			l1;
	long			l2;
	level_t			lid;

	SSTATUS *		pss;

	RSTATUS *		prs;

	PSTATUS *		pps;

#ifdef	SEND_CANCEL_RESPONSE
	MESG *			user_md;
#endif

	int			not_found = 1;

	(void)	getmessage (m, R_SEND_JOB, &name, &status,
		&sent_size, &sent_msg);
	schedlog ("Received R_SEND_JOB from system %s.\n", name);

	if (!(pss = search_stable(name)))
	{
		schedlog ("%s is an unknown system\n", name);
		return	0;
	}

	/* assume success, and clear any retrying flag */
	pss->status &= ~SS_RETRYING;

	prs = pss->exec->ex.request;

	if (!(prs->request->outcome & RS_SENDING))
	{
		schedlog ("Unexpected R_SEND_JOB--no request sent!\n");
		return	0;
	}
	if (!(pss->exec->flags & EXF_WAITJOB))
	{
		schedlog ("Unexpected R_SEND_JOB--not waiting!\n");
		return	0;
	}
	switch (status) {
	case MOK:
	case MOKMORE:
		break;

	case MRETRYING:
		schedlog("Received MRETRYING from %s\n", name);
		pss->status |= SS_RETRYING;
		for (prs = Status_List; prs; prs = prs->next) {
			if (prs->system != pss)
				continue;
			if (!(prs->status & RSS_FOR_USER))
				continue;
			/* Need to kludge a reply - but still be prepared
			   for the real reply to come in */
			if (pss->exec && (pss->exec->flags & EXF_WAITCHILD)) {
				/* waiting for a connection to be established,
				   this is most common case, and the only
				   one we handle */
				int printer_flag = 0;
				if (prs->status & RSS_PRINTSTAT)
					printer_flag++;
				if (pss->system->protocol == S5_PROTO) {
					md_wakeup(pss, NULL, printer_flag);
				} else if (pss->system->protocol == BSD_PROTO)
					md_wakeup(pss, prs->printer, printer_flag);
			}
		}
		return	0;

	case MTRANSMITERR:
		schedlog ("Received MTRANSMITERR from %s, retrying.\n", name);
		resend_remote (pss, 0);
		pss->status |= SS_RETRYING;
		return	0;

	default:
		schedlog ("Odd status in R_SEND_JOB, %d!\n", status);
		return	0;

	}
	switch (mtype(sent_msg)) {

	case R_PRINT_REQUEST:
		/*
		 * Clear the waiting-for-R_SEND_JOB flag now that
		 * we know this message isn't bogus.
		 */
		pss->exec->flags &= ~EXF_WAITJOB;

		(void) getmessage (
			sent_msg,
			R_PRINT_REQUEST,
			&status,
			&req_id,
			&chkprinter_result,
			&remote_req_id
		);

		prs->request->outcome &= ~RS_SENDING;
		prs->printer->status &= ~PS_BUSY;

		if (status == MOK)
		{
			char	*reqno;
			char	*path;
			schedlog("S_SEND_JOB succeeded\n");

			/*
			 * Put the in memory copy of 'secure request'
			 * structure in sync with disk copy which
			 * has just been updated by the child.
			 *
			 * remote_reqid should be "" for non NUC machines
			 */

			if (prs->secure) {
				prs->secure->rem_reqid =
					strdup(remote_req_id);
			}
			prs->request->outcome |= RS_SENT;
                        /*
                         * Adding print request to printer list, allow user
                         * to issue 'cancel <printer>'
                         */
			/*prs->printer->request = prs;*/
				/* Backing out change to allow user to issue
				 * 'cancel <printer>'
				 * This is not the correct location to add
				 * the request to the remote printer list.
				 * There may be an appropriate location and
				 * corresponding duration for the request to
				 * be on the printer, but determining that
				 * will wait for now. cjh on MR ul92-08611
				 */
			/*
			 * Record the fact that we've sent this job,
			 * to avoid sending it again if we restart.
			 */
			(void) putrequest (prs->req_file, prs->request);
			/*
			 * Remove the notify/error file to allow
			 * returning file from remote system
			 */
			if (prs->secure && prs->secure->req_id) {
				reqno = getreqno (prs->secure->req_id);
				path = makepath (Lp_Temp, reqno, (char *)0);
				(void) Unlink (path);
				Free(path);
			}
			if (pss->system->protocol == BSD_PROTO ||
			    pss->system->protocol == NUC_PROTO)
				schedule (EV_LATER, WHEN_POLLBSD,
					EV_POLLPRINTER, prs->printer);
		}
		else
		{
			schedlog ("S_SEND_JOB had failed, status was %d!\n",
				status);
			/*
			 * This is very much like what happens if the
			 * print service configuration changes and causes
			 * a local job to be no longer printable.
			 */
			prs->reason = status;
			(void) cancel (prs, 1);
		}
		break;

	case R_GET_STATUS:
		/*
		 * Were we expecting this?
		 */
		if (!(prs->status & RSS_RECVSTATUS))
		{
			schedlog ("Unexpected GET_STATUS from system: %s\n",
				pss->system->name);
			break;
		}
		if (status != MOKMORE)
			rmreq (prs);

		/*
		 * Is the protocol correct?
		 */
		if (pss->system->protocol == S5_PROTO) {
			schedlog(
			"Protocol mismatch: system %s, got BSD, expected S5\n",
				pss->system->name);
			break;
		}

		(void) getmessage (sent_msg, R_GET_STATUS, &status, &remote_name);

		for (pps = walk_ptable(1); pps; pps = walk_ptable(0)) {
			if (pps->system != pss)
				continue;
			if (STREQU(pps->remote_name, remote_name)) {

				not_found = 0;

				load_bsd_stat (pss, pps);

				if (status != MOKMORE) {
					/*
					* Processed the last printer for pss,
					* so clear the waiting-for-R_SEND_JOB flag.
					*/
					pss->exec->flags &= ~EXF_WAITJOB;
					md_wakeup (pss, pps, 0);
					md_wakeup (pss, pps, 1);
				}
			}
		}
		if (not_found) {
			schedlog (
			"Received GET_STATUS on unknown printer %s\n",
			remote_name);
			break;
		}
		break;

	case R_INQUIRE_PRINTER_STATUS:
		/*
		 * Expecting this?
		 */
		if (!(prs->status & (RSS_RECVSTATUS|RSS_PRINTSTAT)))
		{
			schedlog (
			"Unexpected INQUIRE_PRINTER_STATUS from system %s\n",
				pss->system->name);
			break;
		}
		if (status != MOKMORE)
			rmreq (prs);

		/*
		 * Protocol ok?
		 */
		if (pss->system->protocol == BSD_PROTO)
		{
			schedlog (
			"Protocol mismatch:  system %s is BSD got S5\n",
			pss->system->name);
			break;
		}
		(void) getmessage (
			sent_msg,
			R_INQUIRE_PRINTER_STATUS,
			&status,
			&remote_name,
			&s1, &s2, &s3, &s4, &h1, &s5, &l1, &l2
		);
		for (pps = walk_ptable(1); pps; pps = walk_ptable(0)) {
			if (pps->system != pss)
				continue;
			if (!STREQU(pps->remote_name, remote_name))
				continue;

			/* Updates this printer's status fields */
			pps->rmt_status = h1;
			pps->rmt_dis_date = l1;
			pps->rmt_rej_date = l2;
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
			if (s1)
				pps->rmt_form = Strdup(s1);
			if (s2)
				pps->rmt_pwheel = Strdup(s2);
			if (s3)
				pps->rmt_dis_reason = Strdup(s3);
			if (s4)
				pps->rmt_rej_reason = Strdup(s4);
			if (s5)
				pps->rmt_req_id = Strdup(s5);
			not_found = 0;
			break;
		}
#if	0
		/* Because of limitations in lpsched, we always ask
		 * for all the printer statuses on a remote system.
		 * However, we might not known about all the printers
		 * the remote system has.
		 * Therefore, if we receive a printer we do not know it
		 * is simply ignored, not even an error message is logged
		 * (might worry an admin).
		 */
		if (not_found) {
			schedlog (
			"Received INQUIRE_PRINTER_STATUS for unknown printer %s\n", remote_name);
		}
#endif
		if (status != MOKMORE) {
			/*
			 * Last of status received from the system, so
			 * clear the waiting-for-R_SEND_JOB flag and wake
			 * up all that were wanting the status from the
			 * remote system.
			 */
			pss->exec->flags &= ~EXF_WAITJOB;
			md_wakeup (pss, NULL, 1);
		}
		break;


	case R_INQUIRE_REQUEST_RANK:
		/*
		 * Expecting this?
		 */
		if (!(prs->status & RSS_RECVSTATUS)) {
			schedlog ("Unexpected INQUIRE_REQUEST_RANK from system %s\n",
				pss->system->name);
			break;
		}
		if (status != MOKMORE)
			rmreq (prs);

		/*
		 * Protocol ok?
		 */
		if (pss->system->protocol == BSD_PROTO) {
			schedlog ("Protocol mismatch: system %s is BSD got S5\n",
				pss->system->name);
			break;
		}

		(void) getmessage (
			sent_msg,
			R_INQUIRE_REQUEST_RANK,
			&status,
			&req_id,
			&s1, &l1, &l2, &h1, &s2, &s3, &s4,
			&rank,
			&lid
		);

		/* schedlog ("R_INQUIRE_REQUEST_RANK - %d - %s\n", status, req_id); */
		/* for svr4 protcol we get the requests on all printers on
		   the remote system */
		pps = NULL;
		if (status == MOKMORE || status == MOK)
			if (!update_req (req_id, rank)) {
				/* don't know the request locally */
				update_remote_req(pss, req_id, s1, l1, l2,
						  h1, s2, s3, s4, rank, lid);
			}

		if (status != MOKMORE) {
			/*
			 * Last of status received from the system, so
			 * clear the waiting-for-R_SEND_JOB flag.
			 */
			purge_remote(pss, NULL, 0);
			pss->exec->flags &= ~EXF_WAITJOB;
			md_wakeup (pss, NULL, 0);
		}

		break;

	case R_CANCEL:
		schedlog ("Received R_CANCEL\n");
		if (! (prs->request->outcome & RS_CANCELLED))
		{
			schedlog ("Unexpected R_CANCEL (not canceled)!\n");
			break;
		}
		(void) getmessage (sent_msg, R_CANCEL, &status,
			&lstatus, &req_id);

		if (pss->system->protocol == NUC_PROTO)
		{
			if (!STREQU(prs->secure->rem_reqid, req_id))
			{
			    schedlog (
				"Out of sync on R_CANCEL: wanted %s, got %s\n",
					prs->secure->rem_reqid, req_id);
			    break;
			}
		}
		else
		{
			if (!STREQU(prs->secure->req_id, req_id))
			{
			    schedlog (
				"Out of sync on R_CANCEL: wanted %s, got %s\n",
					prs->secure->req_id, req_id);
			    break;
			}
		}
		/*
		 * Clear the waiting-for-R_SEND_JOB flag now that
		 * we know this message isn't bogus.
		 */
		pss->exec->flags &= ~EXF_WAITJOB;
		prs->request->outcome &= ~RS_SENDING;
		/*
		**  For SVR4+ systems:
		**    The S_CANCEL that we sent will cause notification of
		**    the job completion to be sent back to us.
		**    s_job_completed() will called, and it will do the
		**    following:
		**	dowait_remote (EX_NOTIFY, prs, 0, (char *)0);
		**  HOWEVER:
		**    for BSD and NUC systems there is no notification and so
		**    we must do the call here.
		*/
		if (pss->system->protocol == BSD_PROTO ||
		    pss->system->protocol == NUC_PROTO)
		{
		/*
		 * 'lstatus' is also set to MNOPERM or M2LATE.
		 * This is good information but unfortunately,
		 * nothing is ever done with it.
		 * A table could be built to allow more accurate
		 * notifications to the user. Entries could be 
		 * supplied to notify() as errbuf.
		 */
			switch (lstatus) {
			case MOK:
				if (status == MOKMORE) {
					prs->status |= RSS_SENDREMOTE;
					break;
				}
			case MNOINFO:
			default:
			    schedlog (
				"For R_CANCEL: request id %s, status %d, lstatus %d\n",
					prs->secure->req_id,status,lstatus);
				notify (prs, (char *)0, 0, 0, 0);
				prs->printer->request = 0;
/*
			dowait_remote (EX_NOTIFY, prs, 0, (char *)0);
*/
			}
		}
		if (pss->system->protocol == S5_PROTO) {
			/* If we were not the system that originated the print
			 * request, then we will not receive a notification.
			 * But that doesn't matter as we do not have a request
			 * file to update.
			 * However, we do want to if there are more responses
			 * on the way...
			 */
			if (prs->status & RSS_REMOTE) {
				if(lstatus == MOK && status == MOKMORE)
					prs->status |= RSS_SENDREMOTE;
			}
		}
		break;

	case R_JOB_COMPLETED:
		if (!(prs->request->outcome & RS_DONE)) {
			schedlog ("Unexpected R_JOB_COMPLETED (not done)!\n");
			return 0;
		}

		schedlog ("Received R_JOB_COMPLETED, request %s\n", prs->secure->req_id);

		/*
		 * Clear the waiting-for-R_SEND_JOB flag now that
		 * we know this message isn't bogus.
		 */
		pss->exec->flags &= ~EXF_WAITJOB;

		(void) getmessage (sent_msg, R_JOB_COMPLETED, &status);
		if (status == MUNKNOWN)
  		     lpnote (INFO, E_SCH_REFREQ,
		        	name,
				prs->secure->req_id
			);

		prs->request->outcome &= ~RS_SENDING;
		dowait_remote (EX_NOTIFY, prs, 0, (char *)0);

		break;

	case R_MOVE_REMOTE_REQUEST:
		if (!(prs->status | RSS_MOVE)) {
			schedlog("Unexpected R_MOVE_REMOTE_REQUEST (ignoring)\n");
			break;
		}

		/*
		 * Protocol ok?
		 */
		if (pss->system->protocol == NUC_PROTO) {
			schedlog("Protocol mismatch: system %s is NUC got S5/BSD\n", pss->system->name);
			break;
		}

		/*
		 * Clear the waiting-for-R_SEND_JOB flag now that
		 * we know this message isn't bogus.
		 */
		pss->exec->flags &= ~EXF_WAITJOB;
		(void) getmessage (sent_msg, R_MOVE_REMOTE_REQUEST, &status, &req_id);

		/* Now for the trickly bit.
		 * Need to move the request onto the new destinations
		 * print queue.  The permission check for this was
		 * done beforew the request was sent to the remote.
		 */

		if (status == MOK) {
			char	*dest;
			char	*olddest;
			PSTATUS	*prp;
			PSTATUS	*oldprinter;
			prp = prs->new_dest;
			dest = prp->printer->name;

			olddest = prs->request->destination;
			oldprinter = prs->printer;

			if (!STREQU(olddest, NAME_ANY))
				prs->request->destination = Strdup(dest);
			else
				prs->request->destination = Strdup(olddest);

			prs->printer = prp;
			Free(olddest);
			(void) putrequest(prs->req_file, prs->request);
		}

		prs->status &= ~RSS_MOVE;
		if (prs->new_dest) {
			prs->new_dest = NULL;
		} else
			schedlog ("Strange R_MOVE_REMOTE_REQUEST-  Req %s doesn't have new-dest to free.\n", prs->secure->req_id);

		prs->new_dest_status = status;

		/* We now need to wake up an S_MOVE_REQUEST call which was
		 * waiting for this. */
		if (prs->move_suspend) {
			SUSPENDED	*susp;
			susp = prs->move_suspend;
			prs->move_suspend = NULL;
			Redispatch = 1;
			dispatch(mtype(susp->message), susp->message, susp->md);
			Redispatch = 0;
			Free(susp->message);
			Free(susp);
		}
		break;
	}

	/*
	 * There may be another request waiting to go out over
	 * the network.
	 */
	schedule (EV_SYSTEM, pss);

	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_job_completed
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_job_completed (char *m, MESG *md)
#else
int
s_job_completed (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_job_completed)

	char *		req_id;
	char *		errfile;
	short		outcome;
	RSTATUS *	prs;


	(void) getmessage (m, S_JOB_COMPLETED, &outcome, &req_id, &errfile);

	if (!(prs = request_by_id(req_id))) {
		schedlog ("Got S_JOB_COMPLETED for unknown request %s\n",
			req_id);
		(void) mputm (md, R_JOB_COMPLETED, MUNKNOWN);
		return	0;
	}

	(void) mputm (md, R_JOB_COMPLETED, MOK);
	dowait_remote (EX_INTERF, prs, outcome, errfile);

	return	1;
}

