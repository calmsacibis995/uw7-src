/*		copyright	"%c%" 	*/


#ident	"@(#)cancel.c	1.3"
#ident	"$Header$"

#include "lpsched.h"

/**
 ** cancel() - CANCEL A REQUEST
 **/

int
#ifdef	__STDC__
cancel (
	register RSTATUS *	prs,
	int			spool
)
#else
cancel (prs, spool)
	register RSTATUS	*prs;
	int			spool;
#endif
{
	DEFINE_FNNAME (cancel)

	ENTRYP

	if (prs->request->outcome & RS_DONE)
		return (0);

	prs->request->outcome |= RS_CANCELLED;

	if (spool || (prs->request->actions & (ACT_MAIL|ACT_WRITE|ACT_NOTIFY)))
		prs->request->outcome |= RS_NOTIFY;

	(void) putrequest (prs->req_file, prs->request);

	/*
	 * If the printer for this request is on a remote system,
	 * send a cancellation note across. HOWEVER, this isn't
	 * necessary if the request hasn't yet been sent!
	 */
	if (prs->printer == NULL)
		return(1);
	else
	if (prs->printer->status & PS_REMOTE &&
	    prs->request->outcome & (RS_SENT | RS_SENDING))
	{
		/*
		 * Mark this request as needing sending, then
		 * schedule the send in case the connection to
		 * the remote system is idle.  If there isn't a 
		 * response from the remote SVR4+ system, manually cancel
	    	 * the job and notify the user.
		 */
		prs->status |= RSS_SENDREMOTE;
 		if (prs->printer->system->system->protocol == S5_PROTO &&
			(prs->printer->system->exec->flags &
 			(EXF_WAITJOB|EXF_WAITCHILD))) {
 			    notify(prs,(char*)0,0,0,0);
 			    check_request(prs);
 		}
		else {
			/* We perform this check quite late, but it does
			 * belong here.
			 * Basically, we can't cancel a request (on
			 * a NUC server) which we didn't submit.
			 */
			if (prs->printer->system->system->protocol == NUC_PROTO
			    && (prs->status & RSS_REMOTE)) {
				prs->request->outcome |= RS_CANCELLED;
				return 0;
			}
			schedule (EV_SYSTEM, prs->printer->system);
		}
	}
	else
	if (prs->request->outcome & RS_PRINTING)
		terminate (prs->printer->exec);
	else
	if (prs->request->outcome & RS_FILTERING)
		terminate (prs->exec);
	else
	if (prs->request->outcome & RS_NOTIFY)
		notify (prs, (char *)0, 0, 0, 0);
	else
		check_request (prs);

	return	1;
}
