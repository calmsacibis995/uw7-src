#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/lpsys.c	1.6.1.1"
#endif

/* LP Subsystem Interface */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <Intrinsic.h>
#include <OpenLook.h>

#include <lp.h>
#include <msgs.h>

#include "properties.h"
#include "lpsys.h"
#include "error.h"

static void AllowSignals (Boolean);

/* LpJobs
 *
 * Get an active job.  This function is designed to be called repeatedly--
 * that is, the connection to the spooler is held open until a printer has
 * no more jobs.  The name of the printer is used on the first call, but is
 * ignored on subsequent calls until the queue is exhausted.
 * 
 */
PrintJob *
LpJobs (char *name)
{
    char		msg [MSGMAX];
    static short	status;
    static PrintJob	job;
    static Boolean	connectionOpen = False;

    /* Open connection to scheduler and ask for the jobs.  We can't cope if
     * the open fails.
     */
    if (!connectionOpen)
    {
	if (mopen () != 0)
	    return ((PrintJob *) 0);

	connectionOpen = True;
	(void) putmessage (msg, S_INQUIRE_REQUEST, "", name, "", "", "");
	if (msend (msg) != 0)
	    status = MOK;
	else
	    status = MNOINFO;
    }

    /* If status is MOK, either the previous call to LpJobs returned the
     * last item in the queue, or there was an error sending the inquire
     * request message.  In either case, we want to close the connection
     * to the spooler without receiving any more messages.
     */
    if (status == MOK || mrecv (msg, MSGMAX) == -1)
	status = MNOINFO;
    else
    {
	(void) getmessage (msg, R_INQUIRE_REQUEST, &status, &job.id,
			   &job.user, &job.size, &job.date,
			   &job.outcome, &job.printer, &job.form,
			   &job.character_set, &job.level);

	job.id = strdup (job.id);
	job.user = strdup (job.user);
	job.printer = strdup (job.printer);
	job.form = strdup (job.form);
	job.character_set = strdup (job.character_set);
    }

    if (status == MOK || status == MOKMORE)
	return (&job);
    else
    {
	mclose ();
	connectionOpen = False;
	return ((PrintJob *) 0);
    }
}	/* End of LpJobs () */

/* LpInquire
 *
 * Read a request file.  Unfortunately, normal users don't have
 * permission to read the request file unless they want to change
 * the request.  If the request has already printed or the use is
 * not allowed to change the request, return NULL.
 */
REQUEST *
LpInquire (char *id)
{
    REQUEST	*request;
    char	msg [MSGMAX];
    short	status;
    char	*name;

    request = 0;

    if (mopen () != 0)
	return (request);

    /* Start a request change to get read permission on the file. */
    (void) putmessage (msg, S_START_CHANGE_REQUEST, id);
    if (msend (msg) == 0 && mrecv (msg, MSGMAX) != -1)
    {
	(void) getmessage (msg, R_START_CHANGE_REQUEST, &status, &name);
	if (status == MOK)
	{
	    request = getrequest (name);

	    /* End the change request so the job will actually print. */
	    (void) putmessage (msg, S_END_CHANGE_REQUEST, id);
	    if (msend (msg) == 0)
		mrecv (msg, MSGMAX);
	}
    }

    mclose ();
    return (request);
}	/* End of LpInquire () */

/* LpChangeRequest
 *
 * Change a request.  Return the status of the change.  Use MTRANSMITERR for
 * open/send/receive problems.
 */
int
LpChangeRequest (char *id, REQUEST *request)
{
    char	msg [MSGMAX];
    short	status;
    char	*name;
    long	prtChk;

    status = MTRANSMITERR;

    if (mopen () != 0)
	return (status);

    (void) putmessage (msg, S_START_CHANGE_REQUEST, id);
    if (msend (msg) == 0 && mrecv (msg, MSGMAX) != -1)
    {
	(void) getmessage (msg, R_START_CHANGE_REQUEST, &status, &name);
	if (status == MOK)
	{
	    if (putrequest (name, request) == -1)
		status = MNOPERM;
	    else
	    {
		(void) putmessage (msg, S_END_CHANGE_REQUEST, id);
		if (msend (msg) != 0 || mrecv (msg, MSGMAX) == -1)
		    status = MTRANSMITERR;
		else
		    (void) getmessage(msg, R_END_CHANGE_REQUEST,
				      &status, &prtChk);
	    }
	}
    }

    mclose ();
    return (status);
}	/* End of LpChangeRequest () */

/* LpAllocFiles
 *
 * Get files for print jobs.  This function leaves the connection to the
 * spooler open.  It returns the file prefix if successful and NULL if not.
 */
char *
LpAllocFiles (int cnt)
{
    char	msg [MSGMAX];
    char	*prefix;
    short	status;

    /* Open connection to scheduler.  We can't cope if the open fails. */
    if (mopen () != 0)
	return ((char *) 0);

    (void) putmessage (msg, S_ALLOC_FILES, cnt);
    if (msend (msg) != 0 || mrecv (msg, MSGMAX) == -1)
    {
	mclose ();
	return ((char *) 0);
    }

    (void) getmessage (msg, R_ALLOC_FILES, &status, &prefix);
    if (status == MOK)
	return (strdup (prefix));
    else
    {
	mclose ();
	return ((char *) 0);
    }
}	/* End of LpAllocFiles () */

/* LpRequest
 *
 * Submit a print job to the spooler.  The connection to the spooler is
 * assumed to be open.
 */
int
LpRequest (char *requestName, char **pRqId, long *pPrtChk)
{
    char	msg [MSGMAX];
    short	status;
    char	*rqId;
    int 	junk;

    (void) putmessage (msg, S_PRINT_REQUEST, requestName);
    if (msend (msg) != 0 || mrecv (msg, MSGMAX) == -1)
	status = MNOOPEN;	/* not really the problem, but 'twill serve */
    else
    {
	(void) getmessage (msg, R_PRINT_REQUEST, &status, &rqId, pPrtChk, &junk);
	*pRqId = (status == MOK) ? strdup (rqId) : (char *) 0;
    }
    mclose ();
    return (status);
}	/* End of LpRequest () */

/* LpCancel
 *
 * Cancel a job for a printer.  Return the status from the spooler cancel
 * request.  If unable to send requests to the spooler, return MTRANSMITERR.
 * Leave the connection to the spooler open between invocations; close the
 * connection when a null request name is passed in.
 */
int
LpCancel (char *rqId)
{
    char	msg [MSGMAX];
    short	status;
    static Boolean	connectionOpen;

    if (!rqId)
    {
	if (connectionOpen)
	{
	    connectionOpen = False;
	    mclose ();
	}
	return (MOK);
    }

    /* Open connection to scheduler.  We can't cope if the open fails. */
    if (!connectionOpen)
	if (mopen () != 0)
	    return (MTRANSMITERR);
	else
	    connectionOpen = True;

    /* cancel the job. */
    (void) putmessage (msg, S_CANCEL_REQUEST, rqId);
    if (msend (msg) != 0 || mrecv (msg, MSGMAX) == -1 ||
	getmessage (msg, R_CANCEL_REQUEST, &status) == -1)
    {
	return (MTRANSMITERR);
    }
    return (status);
}	/* End of LpCancel () */


/* AllowSignals
 *
 * Allow or disallow signals SIGHUP, SIGINT, SIGQUIT, SIGTERM.  When
 * disallowed, the signals are held until a subsequent call allows them.
 */
static void
AllowSignals (Boolean allow)
{
    if (allow)
    {
	(void) sigrelse (SIGHUP);
	(void) sigrelse (SIGINT);
	(void) sigrelse (SIGQUIT);
	(void) sigrelse (SIGTERM);
    }
    else
    {
	(void) sighold (SIGHUP);
	(void) sighold (SIGINT);
	(void) sighold (SIGQUIT);
	(void) sighold (SIGTERM);
    }
} /* End of AllowSignals () */
