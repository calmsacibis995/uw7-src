/*		copyright	"%c%" 	*/


#ident	"@(#)disptab.c	1.3"
#ident  "$Header$"

#include  "dispatch.h"
#include  "debug.h"

static int	r_H(),
		r_HS();

static DISPATCH			dispatch_table[] = {
  "R_BAD_MESSAGE",		D_BADMSG,	0,
  "S_NEW_QUEUE",		D_BADMSG,	0,
  "R_NEW_QUEUE",		D_BADMSG,	0,
  "S_ALLOC_FILES",		0,		s_alloc_files,
  "R_ALLOC_FILES",		D_BADMSG,	0,
  "S_PRINT_REQUEST",		0,		s_print_request,
  "R_PRINT_REQUEST",		D_BADMSG,	0,
  "S_START_CHANGE_REQUEST",	0,		s_start_change_request,
  "R_START_CHANGE_REQUEST",	D_BADMSG,	0,
  "S_END_CHANGE_REQUEST",	0,		s_end_change_request,
  "R_END_CHANGE_REQUEST",	D_BADMSG,	0,
  "S_CANCEL_REQUEST",		0,		s_cancel_request,
  "R_CANCEL_REQUEST",		D_BADMSG,	0,
  "S_INQUIRE_REQUEST",		0,		s_inquire_request,
  "R_INQUIRE_REQUEST",		D_BADMSG,	0,
  "S_LOAD_PRINTER",		D_ADMIN,	s_load_printer,
  "R_LOAD_PRINTER",		D_BADMSG,	r_H,
  "S_UNLOAD_PRINTER",		D_ADMIN,	s_unload_printer,
  "R_UNLOAD_PRINTER",		D_BADMSG,	r_H,
  "S_INQUIRE_PRINTER_STATUS",	0,		s_inquire_printer_status,
  "R_INQUIRE_PRINTER_STATUS",	D_BADMSG,	0,
  "S_LOAD_CLASS",		D_ADMIN,	s_load_class,
  "R_LOAD_CLASS",		D_BADMSG,	r_H,
  "S_UNLOAD_CLASS",		D_ADMIN,	s_unload_class,
  "R_UNLOAD_CLASS",		D_BADMSG,	r_H,
  "S_INQUIRE_CLASS",		0,		s_inquire_class,
  "R_INQUIRE_CLASS",		D_BADMSG,	0,
  "S_MOUNT",			D_ADMIN,	s_mount,
  "R_MOUNT",			D_BADMSG,	r_H,
  "S_UNMOUNT",			D_ADMIN,	s_unmount,
  "R_UNMOUNT",			D_BADMSG,	r_H,
  "S_MOVE_REQUEST",		D_ADMIN,	s_move_request,
  "R_MOVE_REQUEST",		D_BADMSG,	r_H,
  "S_MOVE_DEST",		D_ADMIN,	s_move_dest,
  "R_MOVE_DEST",		D_BADMSG,	r_HS,
  "S_ACCEPT_DEST",		D_ADMIN,	s_accept_dest,
  "R_ACCEPT_DEST",		D_BADMSG,	r_H,
  "S_REJECT_DEST",		D_ADMIN,	s_reject_dest,
  "R_REJECT_DEST",		D_BADMSG,	r_H,
  "S_ENABLE_DEST",		0,		s_enable_dest,
  "R_ENABLE_DEST",		D_BADMSG,	r_H,
  "S_DISABLE_DEST",		0,		s_disable_dest,
  "R_DISABLE_DEST",		D_BADMSG,	r_H,
  "S_LOAD_FILTER_TABLE",	D_ADMIN,	s_load_filter_table,
  "R_LOAD_FILTER_TABLE",	D_BADMSG,	r_H,
  "S_UNLOAD_FILTER_TABLE",	D_ADMIN,	s_unload_filter_table,
  "R_UNLOAD_FILTER_TABLE",	D_BADMSG,	r_H,
  "S_LOAD_PRINTWHEEL",		D_ADMIN,	s_load_printwheel,
  "R_LOAD_PRINTWHEEL",		D_BADMSG,	r_H,
  "S_UNLOAD_PRINTWHEEL",	D_ADMIN,	s_unload_printwheel,
  "R_UNLOAD_PRINTWHEEL",	D_BADMSG,	r_H,
  "S_LOAD_USER_FILE",		D_ADMIN,	s_load_user_file,
  "R_LOAD_USER_FILE",		D_BADMSG,	r_H,
  "S_UNLOAD_USER_FILE",		D_ADMIN,	s_unload_user_file,
  "R_UNLOAD_USER_FILE",		D_BADMSG,	r_H,
  "S_LOAD_FORM",		D_ADMIN,	s_load_form,
  "R_LOAD_FORM",		D_BADMSG,	r_H,
  "S_UNLOAD_FORM",		D_ADMIN,	s_unload_form,
  "R_UNLOAD_FORM",		D_BADMSG,	r_H,
  "S_GETSTATUS",		D_ADMIN,	0,
  "R_GETSTATUS",		D_BADMSG,	0,
  "S_QUIET_ALERT",		D_ADMIN,	s_quiet_alert,
  "R_QUIET_ALERT",		D_BADMSG,	r_H,
  "S_SEND_FAULT",		0,		s_send_fault,
  "R_SEND_FAULT",		D_BADMSG,	0,
  "S_SHUTDOWN",			D_ADMIN,	s_shutdown,
  "R_SHUTDOWN",			D_BADMSG,	r_H,
  "S_GOODBYE",			D_BADMSG,	0,
  "S_CHILD_DONE",		0,		s_child_done,
  "I_GET_TYPE",			D_BADMSG,	0,
  "I_QUEUE_CHK",		D_BADMSG,	0,
  "R_CONNECT",			D_BADMSG,	0,
  "S_GET_STATUS",		D_BADMSG,	0,
  "R_GET_STATUS",		D_BADMSG,	0,
  "S_INQUIRE_REQUEST_RANK",	0,		s_inquire_request_rank,
  "R_INQUIRE_REQUEST_RANK",	D_BADMSG,	0,
  "S_CANCEL",			0,		s_cancel,
  "R_CANCEL",			D_BADMSG,	0,
  "S_NEW_CHILD",		D_BADMSG,	0,
  "R_NEW_CHILD",		D_SYSTEM,	r_new_child,
  "S_SEND_JOB",			D_BADMSG,	0,
  "R_SEND_JOB",			D_SYSTEM,	r_send_job,
  "S_JOB_COMPLETED",		0,		s_job_completed,
  "R_JOB_COMPLETED",		D_BADMSG,	0,
  "S_INQUIRE_REMOTE_PRINTER",	0,		s_inquire_remote_printer,
  "R_INQUIRE_REMOTE_PRINTER",	D_BADMSG,	0,
  "S_LOAD_SYSTEM",		D_ADMIN,	s_load_system,
  "R_LOAD_SYSTEM",		D_BADMSG,	r_H,
  "S_UNLOAD_SYSTEM",		D_ADMIN,	s_unload_system,
  "R_UNLOAD_SYSTEM",		D_BADMSG,	r_H,
  "S_MOVE_REMOTE_REQUEST",	D_ADMIN,	s_move_remote,
  "R_MOVE_REMOTE_REQUEST",	D_BADMSG,	0,
};

/*
 * Procedure:     dispatch
 *
 * Restrictions:
 *               mputm: None
 * Notes - DISPATCH A ROUTINE TO HANDLE A MESSAGE
 */

#ifdef	__STDC__
void
dispatch (int type, char *m, MESG *md)
#else
void
dispatch (type, m, md)

int	type;
char *	m;
MESG *	md;
#endif
{
	int	status;

	register DISPATCH	*dp	= &dispatch_table[type];

	DEFINE_FNNAME (dispatch)

	ENTRYP
	if (type <= 0 || type > LAST_MESSAGE || dp->fncp == NULL)
	{
		TRACEP ("Message out of range.")
		(void) mputm (md, R_BAD_MESSAGE);
		EXITP
		return;
	}
	if (!dp->fncp || dp->flags & D_BADMSG)
	{
		TRACEP ("Bad message.")
		(void) mputm (md, R_BAD_MESSAGE);
		EXITP
		return;
	}
	if (dp->flags & D_ADMIN && (md->credp && !ValidateAdminUser (md)))
	{
		TRACEP ("Admin message and not admin client.")
		if ((++dp)->fncp)
		{
			status = (*dp->fncp) (md, type+1);
		}
		else
			(void) mputm (md, R_BAD_MESSAGE);
		EXITP
		return;
	}
	if (dp->flags & D_SYSTEM
		&& md->type != MD_CHILD
		&& md->type != MD_BOUND)
	{
		TRACEP ("System message and incorrect client.")
		if ((++dp)->fncp)
			status = (*dp->fncp) (md, type+1);
		else
			(void) mputm (md, R_BAD_MESSAGE);
		EXITP
		return;
	}
	status = (*dp->fncp) (m, md);
	if (dp->flags & D_ADMIN && !(dp->flags & D_SYSTEM))
		CutAdminAuditRec (status ? 0 : status, md->uid, dp->namep);

	EXITP
	return;
}

/*
 * Procedure:     r_H
 *
 * Restrictions:
 *               mputm: None
 * r_H() - SEND MNOPERM RESPONSE MESSAGE
 * r_HS() - SEND MNOPERM RESPONSE MESSAGE
 */

#ifdef	__STDC__
static	int
r_H ( MESG * md, int type )
#else
static	int
r_H (md, type)
MESG	*md;
int	type;
#endif
{
	DEFINE_FNNAME (r_H)

	(void) mputm (md, type, MNOPERM);
	return	1;
}

/*
 * Procedure:     r_HS
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
static	int
r_HS (MESG * md, int type)
#else
static	int
r_HS (md, type)

MESG	*md;
int	type;
#endif
{
	DEFINE_FNNAME (r_HS)

	(void) mputm (md, type, MNOPERM, "");
	return	1;
}
