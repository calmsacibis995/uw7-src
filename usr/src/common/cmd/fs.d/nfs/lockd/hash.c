/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)hash.c	1.2"
#ident	"$Header$"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * hash.c
 * rotuines handle insertion, deletion of hashed monitor, file entries
 */

#include <unistd.h>
#include "prot_lock.h"

#define	MAX_HASHSIZE	100

extern int debug;

typedef struct lm_vnode cache_fp;
typedef struct lm_vnode cache_me;

cache_me *table_me[MAX_HASHSIZE];

void 	add_req_to_me();
void 	upgrade_req_in_me();
void 	remove_req_in_me();
void 	add_req_to_list();
struct reclock *del_req_from_list();

extern int	merge_lock();

/*
 * find_me returns the cached entry;
 * it returns NULL if not found;
 */
struct lm_vnode *
find_me(svr)
	char *svr;
{
	cache_me *cp;

	cp = table_me[hash((unsigned char *) svr)];
	while (cp != NULL) {
		if (strcmp(cp->svr, svr) == 0) {
			/* found */
			return (cp);
		}
		cp = cp->next;
	}
	return (NULL);
}

/*
 * add_req_to_me() -- add lock request req to the monitor list
 */
void
add_req_to_me(req, status)
	struct reclock *req;
{
	struct lm_vnode *me;

	if (debug)
		printf("enter add_req_to_me (reclock %x status %d) ...\n",
			req, status);

	/* find the monitor entry for this server, it should be found	*/
	/* as the entry is created before the lock request is sent out	*/
	/* to the server.						*/
	if ((me = find_me(req->lck.server_name)) == NULL) {
		syslog(LOG_INFO,
		       gettxt(":113", "monitor entry not found for %s, request is not monitored."),
		       req->lck.server_name);
		return;
	}

	switch (status) {
				/* add request to the appropriate list	*/
				/* exclusive or shared if the request	*/
				/* is granted				*/
	case nlm_granted: 	if (req->exclusive)
					(void) merge_lock(req, &me->exclusive);
				else
					(void) merge_lock(req, &me->shared);
				break;

				/* if request is queued at server, add	*/
				/* it to the pending list		*/
	case nlm_blocked: 	add_req_to_list(req, &me->pending);
				break;
	}
	return;
}

/*
 * upgrade_req_in_me() -- will move the lock request req that's in the
 *			  monitor's pending list to exclusive/shared list
 */
void
upgrade_req_in_me(req)
	struct reclock *req;
{
	struct lm_vnode *me;

	if (debug)
		printf("enter upgrade_req_in_me (reclock %x) ...\n", req);

	/* find the monitor entry for this server, it should be found	*/
	/* as the entry is created before the lock request is sent out	*/
	/* to the server.						*/
	if ((me = find_me(req->lck.server_name)) == NULL) {
		syslog(LOG_INFO,
		       gettxt(":113", "monitor entry not found for %s, request is not monitored."),
		       req->lck.server_name);
		return;
	}

	/* remove the request from the pending list */
	(void) del_req_from_list(&req->lck.lox, &me->pending);

	/* add the request to the exclusive/shared list */
	if (req->exclusive)
		(void) merge_lock(req, &me->exclusive);
	else
		(void) merge_lock(req, &me->shared);
	return;
}

/*
 * remove_req_in_me() -- will remove the lock request req from the monitor's
 *			 pending, exclusive or shared list.
 *			 del_req() is used to free the memory of the lock
 *			 request.
 */
void
remove_req_in_me(req)
	struct reclock *req;
{
	struct lm_vnode *me;

	if (debug)
		printf("enter remove_req_in_me (reclock %x) ...\n", req);

	/* find the monitor entry for this server, it should be found	*/
	/* as the entry is created before the lock request is sent out	*/
	/* to the server.						*/
	if ((me = find_me(req->lck.server_name)) == NULL) {
		syslog(LOG_INFO,
		       gettxt(":113", "monitor entry not found for %s, request is not monitored."),
		       req->lck.server_name);
		return;
	}

	/* since we don't know which of the monitor list this request reside */
	/* we'll try pending, and then exclusive or shared list. */
	/* the memory of this lock request req will be freed by del_req() */
	if (del_req(req, &me->pending))
		return;
	else {
		lm_unlock_region(req, &me->exclusive);
		lm_unlock_region(req, &me->shared);
	}
	return;
}

/*
 * add_req_to_list() -- to add a lock request req to the monitor list listp
 */
void
add_req_to_list(req, list)
	struct reclock *req;
	struct reclock **list;
{
	struct reclock *tmp_p = *list;

	if (debug)
		printf("enter add_req_to_list (reclock %x list %x) ...\n",
			req, *list);

	/* if list is empty */
	if (tmp_p == NULL) {
		*list = req->prev = req->next = req;
		return;
	}

	/* make sure the request is not in the list already, this is possible */
	/* due to retransmissions. */
	for (tmp_p = *list; tmp_p; tmp_p = tmp_p->next) {
		if ((tmp_p == req) || (same_lock(req, &(tmp_p->lck.lox)) &&
		     obj_cmp(&(req->lck.fh), &(tmp_p->lck.fh))))
			return;
		if (tmp_p->next == *list)
			break;
	}

	/* set up the double linked list to add req to the end of the listp */
	tmp_p = *list;
	tmp_p->prev->next = req;
	req->next = tmp_p;
	req->prev = tmp_p->prev;
	tmp_p->prev = req;
	return;
}

/*
 * del_req_from_list() -- will remove the request of the data lock l from the
 *			  doubly linked list listp
 *			  similar to del_req() except it doesn't free reclock
 *			  + arg is data_lock instead of reclock.
 *			  if the req is not found, return NULL; else return the
 *			  reclock.
 */
struct reclock *
del_req_from_list(l, list)
	struct data_lock *l;
	struct reclock **list;
{
	struct reclock *tmp_p = *list;

	if (debug)
		printf("enter del_req_from_list (lox %x list %x) ...\n",
			l, *list);

	/* if list is empty */
	if (tmp_p == NULL)	return (NULL);

	/* make sure we find the req */
	while (&tmp_p->lck.lox != l) {
		if (tmp_p->next == *list)
			return (NULL);
		else
			tmp_p = tmp_p->next;
	}
	/* re-set the doubly linked list w/ this req removed */
	tmp_p->prev->next = tmp_p->next;
	tmp_p->next->prev = tmp_p->prev;

	if (tmp_p == *list) {
		if (tmp_p->next == tmp_p)
			*list = NULL;
		else
			*list = tmp_p->next;
	}
	return (tmp_p);
}


/*
 * del_req() -- similar to del_req_from_list() in that it will remove the
 *		request req from the linked list listp.  difference in that
 *		del_req() will free the memory of req.
 *		difference is arg is reclock instead of data lock.
 *		if the req is not found, return FALSE; else TRUE.
 */
bool_t
del_req(req, list)
	struct reclock *req;
	struct reclock **list;
{
	struct reclock *tmp_p = *list;

	if (debug)
		printf("enter del_req (reclock %x list %x) ...\n",
			req, *list);

	/* if list is empty */
	if (tmp_p == NULL)	return (FALSE);

	/* search for the request, return FALSE if not found */
	while (!((WITHIN(&(tmp_p->lck.lox), &(req->lck.lox)) ||
		  same_bound(&(req->lck.lox), &(tmp_p->lck.lox))) &&
		  SAMEOWNER(&(req->lck.lox), &(tmp_p->lck.lox)) &&
		  obj_cmp(&(req->lck.fh), &(tmp_p->lck.fh)))) {
		if (tmp_p->next == *list)
			return (FALSE);
		else
			tmp_p = tmp_p->next;
	}

	/* re-set the linked list w/ req removed */
	tmp_p->prev->next = tmp_p->next;
	tmp_p->next->prev = tmp_p->prev;

	if (tmp_p == *list) {
		if (tmp_p->next == tmp_p)
			*list = NULL;
		else
			*list = tmp_p->next;
	}

	/* free the lock request req */
	tmp_p->rel = 1;
	release_reclock(tmp_p, "del_req");

	return (TRUE);
}

/*
 * insert_me() -- add monitor entry to the hash table
 */
void
insert_me(mp)
	struct lm_vnode *mp;
{
	int h;

	h = hash((unsigned char *) mp->svr);
	mp->next = table_me[h];
	table_me[h] = mp;
}
