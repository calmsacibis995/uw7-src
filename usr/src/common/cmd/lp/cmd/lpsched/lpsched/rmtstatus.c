/*		copyright	"%c%" 	*/


#ident	"@(#)rmtstatus.c	1.2"
#ident  "$Header$"

#include "stdlib.h"
#include "unistd.h"

#include "lpsched.h"

extern RSTATUS		*FreeList;	/* from rstatus.c */


/**
 ** remote_link()
 **/

static void
#ifdef	__STDC__
remote_link(
	RSTATUS *	rp
)
#else
remote_link(rp)
	RSTATUS *	rp;
#endif
{
	DEFINE_FNNAME (remote_link)
	RSTATUS *	prs;

	/* add to Remote_Request_List */
	if (!Remote_Request_List) {
		Remote_Request_List = rp;
		rp->prev = rp->next = NULL;
		return;
	}

	for (prs = Remote_Request_List; prs; prs = prs->next) {
		if (rsort(&rp, &prs) < 0) {
			rp->prev = prs->prev;
			if (rp->prev)
				rp->prev->next = rp;
			rp->next = prs;
			prs->prev = rp;
			if (prs == Remote_Request_List)
				Remote_Request_List = rp;
			return;
		}
		if (prs->next)
			continue;
		rp->prev = prs;
		prs->next = rp;
		return;
	}
}


static void
#ifdef	__STDC__
remote_unlink(
	RSTATUS *	rp
)
#else
remote_unlink(rp)
	RSTATUS *	rp;
#endif
{
	DEFINE_FNNAME (remote_unlink)

	if (rp == Remote_Request_List)
		Remote_Request_List = rp->next;
	if (rp->next)
		rp->next->prev = rp->prev;
	if (rp->prev)
		rp->prev->next = rp->next;
	rp->next = rp->prev = NULL;
}


/**
 ** request_by_remote_id()
 **/

static RSTATUS *
#ifdef	__STDC__
request_by_remote_id(
	SSTATUS *		pss,
	char *			id
)
#else
request_by_remote_id(pss, id)
	SSTATUS *		pss;
	char *			id;
#endif
{
	DEFINE_FNNAME (request_by_remote_id)

	RSTATUS			*rp;

	for (rp = Remote_Request_List; rp; rp = rp->next) {
		if (rp->printer->system != pss)
			continue;
		if (STREQU(id, rp->secure->req_id))
			return (rp);
	}
	return (0);
}

static int
#ifdef	__STDC__
update_remote_req_list(
	RSTATUS *		rp,
	SSTATUS *		pss,
	char *			req_id,
	char *			user,
	long			size,
	long			date,
	short			outcome,
	char			*printer_name,
	char			*form_name,	/* ignored */
	char			*pwheelname,
	short			rank,
	level_t			lid
)
#else
update_remote_req_list(rp, pss, req_id, user, size, date, outcome, printer_name, form_name, pwheelname, rank, lid)
	RSTATUS *		rp;
	SSTATUS	*		pss;
	char *			req_id;
	char *			user;
	long			size;
	long			date;
	short			outcome;
	char			*printer_name;
	char			*form_name;	/* ignored */
	char			*pwheelname;
	short			rank;
	level_t			lid;
#endif
{
	DEFINE_FNNAME (update_remote_req_list)

	PSTATUS	*		pps;
	short			old_rank;

	/* Use printer name (which is the remote name), to find the local
	   printer status structure */

	if (rp->printer) {
		/* Already have a printer for the request - is it still the
		 * correct one?
		 */
		pps = rp->printer;
		if (rp->printer->system == pss && STREQU(pps->remote_name, printer_name))
			goto printer_found;
	}

	for (pps=walk_ptable(1); pps; pps = walk_ptable(0)) {
		if (pps->system != pss)
			continue;
		if (!STREQU(pps->remote_name, printer_name))
			continue;
		rp->printer = pps;
		goto printer_found;
	}

	/* Couldn't find printer.  The request is probably a locally
	 * submitted request which we (somehow) forgot about - so it
	 * default to a remote request.  Just ignore it */
	/* schedlog("Unknown printer %s on system %s\n", printer_name, pss->system->name); */
	return 0;

printer_found:
	old_rank = rp->rank;
	rp->request->outcome = (outcome | RS_SENT);
	rp->request->priority = getdfltpri();
	rp->rank = rank;
	rp->status |= (RSS_RANK | RSS_REMOTE);

	if (rp->secure->req_id) {
		/* already have filled in the request structure */
		if (old_rank != rank) {
			remote_unlink(rp);
			remote_link(rp);
		}
		return 1;
	}
	rp->secure->req_id = Strdup(req_id);

	/* tell some lies... */
	rp->secure->uid = -1;
	rp->secure->gid = -1;

	if (pwheelname)
		rp->pwheel_name = Strdup(pwheelname);
	if (user) {
		/* If 'user' doesn't include a ping (!), then the request
		 * belongs to the system which has sent this data.
		 */
		char *ptr;
		int len = strlen(user);
		len++;
		if (strchr(user, '!')) {
			ptr = rp->secure->user = Malloc(len);
			if (!ptr)		/* out of mem */
				return 0;
		} else {
			int system_len = strlen(pss->system->name);
			rp->secure->user = Malloc(len+system_len+1);
			if (!rp->secure->user)
				return 0;
			strcpy(rp->secure->user, pss->system->name);
			ptr = rp->secure->user+system_len;
			*ptr++ = '!';
		}
		strcpy(ptr, user);
	}
	rp->secure->size = size;
	rp->secure->date = date;
	rp->secure->lid = lid;

	remote_link(rp);

	return 1;
}


/**
 ** remove_remote
 **/
static void
#ifdef	__STDC__
remove_remote(
	RSTATUS			*rp
)
#else
remove_remote(rp)
	RSTATUS			*rp;
#endif
{
	DEFINE_FNNAME (remove_remote)

	/* Free request, and unlink it from the
	 * remote request chain */
	if (rp->secure) {
		if (rp->secure->user)
			Free (rp->secure->user);
		if (rp->secure->req_id)
			Free (rp->secure->req_id);
	}

	if (rp->request)
		freerequest (rp->request);
	if (rp->pwheel_name)
		Free (rp->pwheel_name);

	remote_unlink(rp);

	rp->next = FreeList;
	FreeList = rp;

	return;
}



/**
 ** purge_remote
 **/

void
#ifdef	__STDC__
purge_remote(
	SSTATUS *		pss,	/* system */
	PSTATUS *		pps,
	int			force_flag
)
#else
purge_remote(pss, pps, force_flag)
	SSTATUS *		pss;
	PSTATUS *		pps;
	int			force_flag;
#endif
{
	DEFINE_FNNAME (purge_remote)

	RSTATUS *		rp;
	RSTATUS *		next;

	/* remove all request for system (and printer, if given),
	 * that haven't been marked. */
	for (rp = Remote_Request_List; rp; rp = next) {
		next = rp->next;
		if (rp->printer->system != pss)
			continue;
		if (pps && rp->printer != pps)
			continue;
			
		if (rp->status & RSS_MARK) {
			rp->status &= ~RSS_MARK;
			continue;
		}
		if (!force_flag && (rp->status & RSS_SENDREMOTE) &&
			(rp->request->outcome & RS_CANCELLED)) {
			continue;
		}
			
		/* Request has disappeared - canceled, printed, etc */
		remove_remote(rp);
	}

	return;
}


/**
 ** update_remote_req
 **/

int
#ifdef	__STDC__
update_remote_req (
	SSTATUS			*pss,
	char *			req_id,
	char *			user,
	long			size,
	long			date,
	short			outcome,
	char			*printer_name,
	char			*form_name,
	char			*pwheelname,
	short			rank,
	level_t			lid
)
#else
update_remote_req (pss, req_id, user, size, date, outcome, printer_name, form_name, pwheelname, rank, lid)
	SSTATUS			*pss;
	char *			req_id;
	char *			user;
	long			size;
	long			date;
	short			outcome;
	char			*printer_name;
	char			*form_name;
	char			*pwheelname;
	short			rank;
	level_t			lid;
#endif
{
	DEFINE_FNNAME (update_remote_req)

	RSTATUS		*rp;

	/* schedlog("update_remote_req(): req_id:%s user:%s printer_name:%s pwheelname:%s\n", req_id, user, printer_name, pwheelname); */

	if (!(rp = request_by_remote_id(pss, req_id))) {
		/* Don't have this request */
		if (!(rp = allocr())) {
			/* Will just have to ignore it */
			return 1;
		}
	}

	if (!update_remote_req_list(rp, pss, req_id, user, size, date, outcome,
			 printer_name, form_name, pwheelname, rank,
			 lid)) {
		remove_remote(rp);
		return 1;
	}

	rp->status |= RSS_MARK;

	return 0;
}
