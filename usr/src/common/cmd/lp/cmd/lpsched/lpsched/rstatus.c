/*		copyright	"%c%" 	*/


#ident	"@(#)rstatus.c	1.3"
#ident	"$Header$"

#include "lpsched.h"

RSTATUS		*FreeList	= 0;

/**
 ** freerstatus()
 **/

void
#ifdef	__STDC__
freerstatus (
	register RSTATUS *	r
)
#else
freerstatus (r)
	register RSTATUS	*r;
#endif
{
	DEFINE_FNNAME (freerstatus)

	if (r->exec) {
		if (r->exec->pid > 0)
			terminate (r->exec);
		r->exec->ex.request = 0;
	}

	if (r->secure) {
		if (r->secure->system)
			Free (r->secure->system);
		if (r->secure->user)
			Free (r->secure->user);
		if (r->secure->req_id)
			Free (r->secure->req_id);
	}

	if (r->request)
		freerequest (r->request);

	if (r->req_file)
		Free (r->req_file);
	if (r->slow)
		Free (r->slow);
	if (r->fast)
		Free (r->fast);
	if (r->pwheel_name)
		Free (r->pwheel_name);
	if (r->printer_type)
		Free (r->printer_type);
	if (r->cpi)
		Free (r->cpi);
	if (r->lpi)
		Free (r->lpi);
	if (r->plen)
		Free (r->plen);
	if (r->pwid)
		Free (r->pwid);
	if (r->slowparm) {
		if (r->slowparm->pages)
			Free (r->slowparm->pages);
		if (r->slowparm->modes)
			Free (r->slowparm->modes);
		if (r->slowparm->type)
			Free (r->slowparm->type);
	}

	remover (r);
	r->next = FreeList;
	FreeList = r;

	return;
}

/**
 ** allocr()
 **/

#ifdef	__STDC__
RSTATUS *
allocr (void)
#else
RSTATUS *
allocr ()
#endif
{
	DEFINE_FNNAME (allocr)

	register RSTATUS	*prs;
	register REQUEST	*req;
	register SECURE		*sec;
	register SLOWPARM	*spm;
	

	if ((prs = FreeList)) {

		FreeList = prs->next;
		req	= prs->request;
		sec	= prs->secure;
		spm	= prs->slowparm;

	} else {

		prs = (RSTATUS *)Malloc (sizeof (RSTATUS));
		req = (REQUEST *)Malloc (sizeof (REQUEST));
		sec = (SECURE *)Malloc (sizeof (SECURE));
		spm = (SLOWPARM *)Malloc (sizeof (SLOWPARM));

	}

	(void) memset ((char *)prs, 0, sizeof(RSTATUS));
	(void) memset ((char *)(prs->request = req), 0, sizeof(REQUEST));
	(void) memset ((char *)(prs->secure = sec), 0, sizeof(SECURE));
	(void) memset ((char *)(prs->slowparm = spm), 0, sizeof(SLOWPARM));
	
	return (prs);
}
			
/**
 ** insertr()
 **/

void
#ifdef	__STDC__
insertr (
	RSTATUS *		r
)
#else
insertr (r)
	RSTATUS			*r;
#endif
{
	DEFINE_FNNAME (insertr)

	RSTATUS			*prs;


	if (!Request_List) {
		Request_List = r;
		return;
	}
	
	for (prs = Request_List; prs; prs = prs->next) {
		if (rsort(&r, &prs) < 0) {
			r->prev = prs->prev;
			if (r->prev)
				r->prev->next = r;
			r->next = prs;
			prs->prev = r;
			if (prs == Request_List)
				Request_List = r;
			return;
		}

		if (prs->next)
			continue;

		r->prev = prs;
		prs->next = r;
		return;
	}
}

/**
 ** remover()
 **/

void
#ifdef	__STDC__
remover (
	RSTATUS *		r
)
#else
remover (r)
	RSTATUS			*r;
#endif
{
	DEFINE_FNNAME (remover)

	if (r == Request_List)
		Request_List = r->next;
	
	if (r->next)
		r->next->prev = r->prev;
	
	if (r->prev)
		r->prev->next = r->next;
	
	r->next = 0;
	r->prev = 0;
	return;
}

/**
 ** request_by_id()
 **/

RSTATUS *
#ifdef	__STDC__
request_by_id (
	char *			id
)
#else
request_by_id (id)
	char			*id;
#endif
{
	DEFINE_FNNAME (request_by_id)

	register RSTATUS	*prs;
	
	for (prs = Request_List; prs; prs = prs->next)
		if (STREQU(id, prs->secure->req_id))
			return (prs);
	return (0);
}

/**
 ** request_by_jobid()
 **/

RSTATUS *
#ifdef	__STDC__
request_by_jobid (char *printer, char *jobid, int protocol)
#else
request_by_jobid (printer, jobid, protocol)

char *	printer;
char *	jobid;
int	protocol;
#endif
{
	ENTRY ("request_by_jobid")

	RSTATUS *	prs;

	/*
	**  Strip out leading zeros except the last zero.
	*/
	while (*jobid == '0' && *(jobid+1) != '\0')
	{
		jobid++;
	}
	for (prs = Request_List; prs; prs = prs->next)
	{
		/*
		 * If the protocol is NUC then we've been passed a remote
		 * jobid.
		 */
		if (protocol == NUC_PROTO) {
			if (STREQU(printer, prs->printer->printer->name) &&
			    STREQU(jobid, prs->secure->rem_reqid))
			{
				return	prs;
			}
		}
		else
			if (STREQU(printer, prs->printer->printer->name) &&
			    STREQU(jobid, getreqno(prs->secure->req_id)))
			{
				return	prs;
			}
	}
	return	(RSTATUS *) 0;
}

/**
 ** rsort()
 **/

#ifdef	__STDC__
static int		later ( RSTATUS * , RSTATUS * );
#else
static int		later();
#endif

int
#ifdef	__STDC__
rsort (
	register RSTATUS **	p1,
	register RSTATUS **	p2
)
#else
rsort (p1, p2)
	register RSTATUS	**p1,
				**p2;
#endif
{
	DEFINE_FNNAME (rsort)

	/*
	 * Of two requests needing immediate handling, the first
	 * will be the request with the LATER date. In case of a tie,
	 * the first is the one with the larger request ID (i.e. the
	 * one that came in last).
	 */
	if ((*p1)->request->outcome & RS_IMMEDIATE)
		if ((*p2)->request->outcome & RS_IMMEDIATE)
			if (!later(*p1, *p2))
				return (-1);
			else
				return (1);
		else
			return (-1);

	else if ((*p2)->request->outcome & RS_IMMEDIATE)
		return (1);

	/*
	 * Of two requests not needing immediate handling, the first
	 * will be the request with the highest priority. If both have
	 * the same priority, the first is the one with the EARLIER date.
	 * In case of a tie, the first is the one with the smaller ID
	 * (i.e. the one that came in first).
	 */
	else if ((*p1)->request->priority == (*p2)->request->priority)
		if (!later(*p1, *p2))
			return (-1);
		else
			return (1);

	else
		return ((*p1)->request->priority - (*p2)->request->priority);
	/*NOTREACHED*/
}

static int
#ifdef	__STDC__
later (
	register RSTATUS *	prs1,
	register RSTATUS *	prs2
)
#else
later (prs1, prs2)
	register RSTATUS	*prs1,
				*prs2;
#endif
{
	DEFINE_FNNAME (later)

	if (prs1->secure->date && prs2->secure->date) {
		if (prs1->secure->date > prs2->secure->date)
			return (1);

		if (prs1->secure->date < prs2->secure->date)
			return (0);
	}
	if (prs1->status & prs2->status & RSS_RANK) {
		/* Ranks should never be equal, but something may have
		 * messed up... */
		if (prs1->rank > prs2->rank)
			return 1;
		if (prs1->rank < prs2->rank)
			return 0;
	}


	if (prs1->secure->date && !prs2->secure->date)
			return 0;

	if (prs2->secure->date && !prs1->secure->date)
			return 1;

	/*
	 * The dates are the same, so compare the request IDs.
	 * One problem with comparing request IDs is that the order
	 * of two IDs may be reversed if the IDs wrapped around. This
	 * is a very unlikely problem, because the cycle should take
	 * more than one second to wrap!
	 */
	{
		register int		len1 = strlen(prs1->req_file),
					len2 = strlen(prs2->req_file);

		/*
		 * Use the request file name (ID-0) for comparison,
		 * because the real request ID (DEST-ID) won't compare
		 * properly because of the destination prefix.
		 * The strlen() comparison is necessary, otherwise
		 * IDs like "99-0" and "100-0" will compare wrong.
		 */
		if (len1 > len2)
			return (1);
		else if (len1 < len2)
			return (0);
		else
			return (strcmp(prs1->req_file, prs2->req_file) > 0);
	}
	/*NOTREACHED*/
}
