#ident	"@(#)ulr.c	1.10"

#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <synch.h>
#include <thread.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <errno.h>
#include <dial.h>
#include <string.h>
#include <sys/ppp.h>
#include <sys/ppp_psm.h>

#include "pathnames.h"
#include "ulr.h"
#include "psm.h"
#include "ppp_type.h"
#include "ppp_proto.h"
#include "act.h"
#include "hist.h"

int ulr_reply(int so, ulr_msg_t *m,  int len, int err);

int
sockinit(int maxpend)
{
	int	s;
	struct sockaddr_un sin_addr;
	struct servent *serv;

	unlink(PPP_PATH);
 
	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		ppplog(MSG_FATAL, 0, "Failed on socket() call (%d)\n", errno);

	sin_addr.sun_family = AF_UNIX;
	strcpy(sin_addr.sun_path, PPP_PATH);

	if (bind(s, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0)
		ppplog(MSG_FATAL, 0, "Error binding to socket (%d)\n", errno);

	chmod(PPP_PATH, S_IWUSR);

	listen(s, maxpend);
	return s;
}

/*
 * Clear the history buffer
 */
int
ulr_hist_clear(int so, struct ulr_prim *p)
{
	hist_clear();
	ulr_reply(so, (ulr_msg_t *)p, sizeof(struct ulr_prim), 0);	
}

/*
 * User wants some call history entries
 */
int
ulr_call_hist(int so, struct ulr_callhistory *ch)
{
	struct histent_s *he;
	int index;

	index = hist_lookup_start(ch->cookie);

	he = &(ch->he);
	ch->num = 0;

	while (index != -1) {
		/* Get an entry .. put it in the output buffer ..*/
		index = hist_lookup_ent(index, he);
		ch->num++;

		/* If no room for more ... exit */
		he++;
		if ((char *)(he + 1) > (char *)ch + ULR_MAX_MSG)
			break;
	}

	hist_lookup_end();
	ch->more = (index != -1);
	ulr_reply(so, (ulr_msg_t *)ch, (char *)he - (char *)ch, 0);
}

/*
 * List ID's of the specified type.
 * Cookies are the nth occurance of an object of the specified type.
 */
int
ulr_list_links(int so, struct ulr_list_links *ll)
{
	act_hdr_t *ab;
	act_hdr_t *al;
	int *ip;
	int ret;

	ppplog(MSG_ULR, 0, "User Requests links for bundle '%s'\n",
	       ll->bundle);

	ret = act_find_id_rd(ll->bundle, DEF_BUNDLE, &ab);
	if (ret) {
		ppplog(MSG_ULR, 0, "ERROR: Bundle '%s' not known. (ENOENT)\n",
		       ll->bundle);
		ulr_reply(so, (ulr_msg_t *)ll, sizeof(ulr_msg_t), ret);
		return;
	}		

	ll->numlinks = 0;
	ip = ll->index;

	al = ab->ah_bundle.ab_links;
	while (al) {

		if ((char *)(ip + 1) >= (char *)(ll) + ULR_MAX_MSG)
			break;

		MUTEX_LOCK(&al->ah_mutex);
		*ip++ = al->ah_index;
		ll->numlinks ++;
#ifdef DEBUG_LINKS
		ppplog(MSG_DEBUG, ab, "ulr_list_links: link %d\n",
		       al->ah_index);
#endif
		MUTEX_UNLOCK(&al->ah_mutex);
		
		al = al->ah_link.al_next;
	}
	act_release(ab);
	ulr_reply(so, (ulr_msg_t *)ll, (char *)ip - (char *)ll, 0);
}

/*
 * This function is called when an incoming link indication is given.
 */
int
ulr_incoming(int so, struct ulr_incoming *ic)
{
	act_hdr_t *al;
	int ret;
	calls_t *call;
	dial_service_t serv;
	int type;
	act_hdr_t *act_incominglink(char *a, int i);

#ifdef DEBUG_ICS
	ppplog(MSG_INFO, 0, "Incoming call: ics = %s\n", ic->ics);
#endif
	/* Get the call info */

	ret = incomings(ic->fd, &call, &serv, ic->ics);
	if (ret < 0) {
		ppplog(MSG_WARN, 0,
	       "Incoming call failed: incomings() returned error %d\n", errno);

		/* Incomings will sortout freeing off the calls structure */

		undials(ic->fd, call);
		ulr_reply(so, (ulr_msg_t *)ic, sizeof(ulr_msg_t), EIO);
		hist_add(HOP_ADD, NULL, EIO, NULL, NULL, NULL);
		return;
	}

	ppplog(MSG_INFO, 0, "Incoming call: login '%s', callerid '%s'\n",
	       ic->uid, call->caller_telno);
	ppplog(MSG_INFO_LOW, 0, "Incoming call: dev '%s', speed '%d'\n", 
	       call->device_name, call->speed);

	ppplog(MSG_DEBUG, 0, "ulr_incoming: fd = %d telno '%s', serv %d\n",
	       ic->fd, call->telno, serv);
	ppplog(MSG_DEBUG, 0,
	       "ulr_incoming: sysname '%s', class '%s', protocol '%s'\n",
	       call->sysname, call->class, call->protocol);
	ppplog(MSG_DEBUG, 0,
	       "ulr_incoming: pinfo_len %d, pinfo %x, attr %x\n",
	       call->pinfo_len, call->pinfo, call->attr);

	/* Check the service type is supported */

	switch (serv) {
	case DIAL_ACU:
		type = LNK_ANALOG;
		break;
	case DIAL_ISDN_SYNC:
		type = LNK_ISDN;
		break;
	case DIAL_ISDN_ASYNC:
		type = LNK_ISDNVOC;
		break;
	case DIAL_DIRECT:
		type = LNK_STATIC;
		break;
	case DIAL_TCP:
		type = LNK_TCP;
		break;
	default:
		ppplog(MSG_WARN, 0,
		       "Incoming call failed: Unsupported link type %d\n",
		       serv);
		ulr_reply(so, (ulr_msg_t *)ic, sizeof(ulr_msg_t), ENXIO);
		hist_add(HOP_DROP, NULL, ENXIO,
			 call->device_name, ic->uid, call->caller_telno);
		undials(ic->fd, call);
		return;
	}

	/* Do we have a configuation entry for the specified link ? */

	al = act_incominglink(call->device_name, type);
	if (!al) {
		/* Don't have a suitlable active link */
		ppplog(MSG_INFO, 0,
		       "Incoming call: No Suitable active link on which to accept call\n");
		ulr_reply(so, (ulr_msg_t *)ic, sizeof(ulr_msg_t), ENOENT);
		hist_add(HOP_DROP, NULL, ENOENT,
			 call->device_name, ic->uid, call->caller_telno);

		undials(ic->fd, call);
		return;
	}

	/*
	 * Now condition the link, initialise the link structure and
	 * Administratively open LCP on the link.
	 */
	ret = link_incoming(al, ic->fd, ic->uid,  call);
	if (ret) {
		ppplog(MSG_WARN, al,
		       "Failed to initialise incoming link.\n");
		link_undial(al);
		al->ah_link.al_flags = 0;
	}

	act_release(al);
	ulr_reply(so, (ulr_msg_t *)ic, sizeof(struct ulr_prim), ret);
}

/*
 * User requests a connection to a specified system
 */
int
ulr_attach(int so, struct ulr_attach *at)
{
	int ret;
	struct cfg_out *co;
	int dstat;
	char *lp;
	act_hdr_t *ab;

	ppplog(MSG_ULR, 0, "User Requests attach for bundle '%s'\n",
	       at->bundle);

 open_more:
	ret = act_find_id(at->bundle, DEF_BUNDLE, &ab);
	if (ret) {
		ppplog(MSG_ULR, 0, "ERROR: Bundle '%s' not known. (ENOENT)\n",
		       at->bundle);
		ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
		return;
	}		

	ret = bundle_open(ab, &at->derror);
	switch (ret) {
	case 0:
		break;
	case EBUSY:
		act_release(ab);
		nap(500);
		goto open_more;
	default:
		ppplog(MSG_ULR, ab,
		       "WARNING Failed to connect bundle. (%d)\n", ret);
		break;
	}

	act_release(ab);
	ulr_reply(so, (ulr_msg_t *)at, sizeof(*at), ret);
	return;
}

/*
 * Add a link to a bundle
 */
int
ulr_linkadd(int so, struct ulr_attach *at)
{
	int ret = 0;
	act_hdr_t *ab, *al = NULL;

	ppplog(MSG_ULR, 0, "User Requests linkadd for bundle '%s'\n",
	       at->bundle);
 open_more:
	ret = act_find_id(at->bundle, DEF_BUNDLE, &ab);
	if (ret) {
		ppplog(MSG_ULR, 0, "ERROR: Bundle '%s' not known. (ENOENT)\n",
		       at->bundle);
		ulr_reply(so, (ulr_msg_t *)at, sizeof(*at), ret);
		return;
	}		

	/*
	 * If we have a link defined for addition check it exists
	 */
	if (*at->link) {
		ret = act_find_id(at->link, DEF_LINK, &al);
		if (ret) {
			ppplog(MSG_ULR, 0,
			       "ERROR: Link '%s' not known. (ENOENT)\n",
			       at->link);
			act_release(ab);
			ulr_reply(so, (ulr_msg_t *)at, sizeof(*at), ret);
			return;
		}		
		
		/* Check that the link doesn't belong to a bundle */

		if (al->ah_link.al_bundle ||
		    al->ah_link.al_flags & ALF_INUSE) {
			ppplog(MSG_ULR, 0,
			       "ERROR: Link %s already in use\n", at->link);
			act_release(al);
			act_release(ab);
			ulr_reply(so, (ulr_msg_t *)at, sizeof(*at), EINVAL);
			return;
		}

		ASSERT(!(al->ah_link.al_flags & ALF_INUSE));
		ASSERT(!(al->ah_link.al_flags & ALF_PHYS_UP));
		ASSERT(!(al->ah_link.al_flags & ALF_COND));
	}

	ppplog(MSG_DEBUG, 0, "Adding link '%s' to bundle '%s'\n",
	       al ? al->ah_cfg->ch_id : "Any?", ab->ah_cfg->ch_id);

	ret = bundle_linkadd(ab, al, NULL, LNK_ANY, &at->derror);

	if (al)
		act_release(al);
	act_release(ab);

	switch (ret) {
	case 0:
		break;

	case EBUSY:
		nap(500);
		goto open_more;
	default:
		ppplog(MSG_ULR, ab, "WARNING Failed to add link. (%d)\n", ret);
		break;
	}

	ppplog(MSG_DEBUG, 0, "ulr_linkadd: Complete .. %d\n", ret);

	ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
}

/*
 * User requests a detach from a peer
 */
int
ulr_detach(int so, struct ulr_attach *at)
{
	int ret = 0;
	act_hdr_t *ab;

	ppplog(MSG_ULR, 0, "User Requests detach for bundle '%s'\n",
	       at->bundle);

	ret = act_find_id(at->bundle, DEF_BUNDLE, &ab);
	if (ret) {
		ppplog(MSG_ULR, 0, "ERROR: Bundle '%s' not known. (ENOENT)\n",
		       at->bundle);
		ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
		return;
	}		

	if (ab->ah_bundle.ab_links)
		/* Need to take down the bundle */
		ret = bundle_close(ab, ABR_USERCLOSE);

	act_release(ab);

	ppplog(MSG_DEBUG, 0, "ulr_detach: Complete .. %d\n", ret);

	ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
}

/*
 * User requests a bundle drop a link
 */
int
ulr_linkdrop(int so, struct ulr_attach *at)
{
	int ret = 0;
	act_hdr_t *ab, *al = NULL;

	ppplog(MSG_ULR, 0, "User Requests linkdrop for bundle '%s'\n",
	       at->bundle);

	ret = act_find_id(at->bundle, DEF_BUNDLE, &ab);
	if (ret) {
		ppplog(MSG_ULR, 0, "ERROR: Bundle '%s' not known. (ENOENT)\n",
		       at->bundle);
		ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
		return;
	}		

	/* If we have a link defined for dropping check it exists */
	if (*at->link) {

		ret = act_find_id(at->link, DEF_LINK, &al);
		if (ret) {
			ppplog(MSG_ULR, 0,
			       "ERROR: Link '%s' not known. (ENOENT)\n",
			       at->link);
			act_release(ab);
			ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
			return;
		}		
		
		/* Check that the link belongs to the defined bundle */

		if (al->ah_link.al_bundle != ab) {
			ppplog(MSG_ULR, 0,
			       "ERROR: Link %s not in bundle '%s'\n",
			       at->link, at->bundle);
			act_release(al);
			act_release(ab);
			ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), EINVAL);
			return;
		}

		ASSERT(al->ah_link.al_flags & ALF_INUSE);
		ASSERT(al->ah_link.al_flags & ALF_PHYS_UP);
		ASSERT(al->ah_link.al_flags & ALF_COND);
	}

	ret = bundle_linkdrop(ab, al, ALR_USERCLOSE);

	if (al)
		act_release(al);
	act_release(ab);

	ppplog(MSG_DEBUG, 0, "ulr_linkdrop: Complete .. %d\n", ret);

	ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
}

/*
 * User requests a bundle kill a link
 */
int
ulr_linkkill(int so, struct ulr_attach *at)
{
	int ret = 0;
	act_hdr_t *ab, *al = NULL;

	ppplog(MSG_ULR, 0, "User Requests linkkill for bundle '%s'\n",
	       at->bundle);

	ret = act_find_id(at->bundle, DEF_BUNDLE, &ab);
	if (ret) {
		ppplog(MSG_ULR, 0, "ERROR: Bundle '%s' not known. (ENOENT)\n",
		       at->bundle);
		ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
		return;
	}		

	ret = act_find_id(at->link, DEF_LINK, &al);
	if (ret) {
		ppplog(MSG_ULR, 0,
		       "ERROR: Link '%s' not known. (ENOENT)\n",
		       at->link);
		act_release(ab);
		ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
		return;
	}		
		
	/* Check that the link belongs to the defined bundle */

	if (al->ah_link.al_bundle != ab) {
		ppplog(MSG_ULR, 0,
		       "ERROR: Link %s not in bundle '%s'\n",
		       at->link, at->bundle);
		act_release(al);
		act_release(ab);
		ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), EINVAL);
		return;
	}

	ASSERT(al->ah_link.al_flags & ALF_INUSE);
	ASSERT(al->ah_link.al_flags & ALF_PHYS_UP);
	ASSERT(al->ah_link.al_flags & ALF_COND);

	ret = bundle_linkkill(ab, al, ALR_USERCLOSE);

	act_release(al);
	act_release(ab);

	ppplog(MSG_DEBUG, 0, "ulr_linkkill: Complete .. %d\n", ret);

	ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
}

/*
 * User requests a FAST DIRTY detach from a peer
 */
int
ulr_kill(int so, struct ulr_attach *at)
{
	int ret = 0;
	act_hdr_t *ab;

	ppplog(MSG_ULR, 0, "User Requests kill for bundle '%s'\n", at->bundle);
	ret = act_find_id(at->bundle, DEF_BUNDLE, &ab);
	if (ret) {
		ppplog(MSG_ULR, 0, "WARNING Bundle '%s' not known. (ENOENT)\n",
		       at->bundle);
		ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
		return;
	}		

	if (ab->ah_bundle.ab_links) {
		/* Need to take down the bundle */
		ret = bundle_kill(ab);
	}

	act_release(ab);

	ppplog(MSG_DEBUG, 0, "ulr_kill: Complete .. %d\n", ret);

	ulr_reply(so, (ulr_msg_t *)at, sizeof(ulr_msg_t), ret);
}

/*
 * Get the statistics info for a PSM
 */
int
ulr_psm_stats(int so, struct ulr_psm_stat *st)
{
	int ret = 0;
	act_hdr_t *ah;
	proto_hdr_t *ph;
	int i;
	struct cfg_proto *pr;

	ppplog(MSG_ULR, 0,
	       "User Requests Protocol Statistics for %s %s cookie %d\n",
	       (st->type == DEF_LINK ? "link" : "bundle"),
	       st->id, st->cookie);

	ret = act_find_id_rd(st->id, st->type, &ah);
	if (ret) {
		ppplog(MSG_ULR, 0,
		       "WARNING Bundle/Link '%s' not known. (ENOENT)\n",
		       st->id);
		ulr_reply(so, (ulr_msg_t *)st, sizeof(ulr_msg_t), ret);
		return;
	}		

	ph = ah->ah_protos;

	/* Find the proto */
	for (i = 0; i < st->cookie; i++)
		if (ph)
			ph = ph->ph_next;

	if (ph) {
		pr = (struct cfg_proto *)ph->ph_cfg;

		strcpy(st->psmid, ucfg_str(&pr->pr_ch, pr->pr_name));
		strcpy(st->cfgid, ph->ph_cfg->ch_id);
		st->more = (int)ph->ph_next;

		st->flags = 0;
		if (ph->ph_rx_stats) {
			st->flags |= PSM_RX;
			memcpy(&st->rxdata, ph->ph_rx_stats, ULR_PSM_MAX);
		}

		if (ph->ph_tx_stats) {
			st->flags |= PSM_TX;
			memcpy(&st->txdata, ph->ph_tx_stats, ULR_PSM_MAX);
		}
	} else
		ret = ENOENT;

	act_release(ah);

	ulr_reply(so, (ulr_msg_t *)st, sizeof(struct ulr_psm_stat), ret);
}

/*
 * Get the status info for a PSM
 */
int
ulr_psm_status(int so, struct ulr_psm_stat *st)
{
	int ret = 0;
	act_hdr_t *ah;
	proto_hdr_t *ph;
	int i;
	struct cfg_proto *pr;

	ppplog(MSG_ULR, 0,
	       "User Requests Protocol Status for %s %s cookie %d\n",
	       (st->type == DEF_LINK ? "link" : "bundle"),
	       st->id, st->cookie);

	ret = act_find_id_rd(st->id, st->type, &ah);
	if (ret) {
		ppplog(MSG_ULR, 0,
		       "WARNING Bundle/Link '%s' not known. (ENOENT)\n",
		       st->id);
		ulr_reply(so, (ulr_msg_t *)st, sizeof(ulr_msg_t), ret);
		return;
	}		

	ph = ah->ah_protos;

	/* Find the proto */
	for (i = 0; i < st->cookie; i++)
		if (ph)
			ph = ph->ph_next;

	if (ph) {
		pr = (struct cfg_proto *)ph->ph_cfg;

		strcpy(st->psmid, ucfg_str(&pr->pr_ch, pr->pr_name));
		strcpy(st->cfgid, ph->ph_cfg->ch_id);
		st->more = (int)ph->ph_next;

		if (ph->ph_psmtab->pt_status)
			(*ph->ph_psmtab->pt_status)(ph, st->data);
	} else
		ret = ENOENT;

	act_release(ah);
	ulr_reply(so, (ulr_msg_t *)st, sizeof(struct ulr_psm_stat), ret);
}

/*
 * Get tne status info for a link or bundle
 */
int
ulr_status(int so, struct ulr_status *st)
{
	int ret = 0;
	act_hdr_t *ah;

	if (st->index >= 0) {
		switch(st->type) {
		case DEF_LINK:
			ppplog(MSG_ULR, 0,
			       "User Requests status for link index %d\n",
			       st->index);       
			ret = act_find_link_rd(st->index, &ah);
			if (ret) {
				ppplog(MSG_ULR, 0,
				       "WARNING Link %d not known. (ENOENT)\n",
				       st->index);
				ulr_reply(so, (ulr_msg_t *)st,
					  sizeof(ulr_msg_t), ret);
				return;
			}
			strcpy(st->id, ah->ah_cfg->ch_id);
			break;
		case DEF_BUNDLE:
			ppplog(MSG_ULR, 0,
			       "User Requests status for link index %d\n",
			       st->index);
		default:

			ppplog(MSG_ERROR, 0, "ulr_status: Not supported\n");
			ulr_reply(so, (ulr_msg_t *)st,
				  sizeof(ulr_msg_t), EINVAL);
			return;
		}
	} else {
		ppplog(MSG_ULR, 0, "User Requests status for %s '%s'\n",
		       st->type == DEF_LINK ? "link" : "bundle",
		       st->id);

		ret = act_find_id_rd(st->id, st->type, &ah);
		if (ret) {
			ppplog(MSG_ULR, 0,
			       "WARNING %s '%s' not known. (ENOENT)\n",
			       st->type == DEF_LINK ? "link" : "bundle",
			       st->id);
			ulr_reply(so, (ulr_msg_t *)st, sizeof(ulr_msg_t), ret);
			return;
		}		
	}

	memcpy(&st->ah, ah, sizeof(act_hdr_t));
	act_release(ah);

	ulr_reply(so, (ulr_msg_t *)st, sizeof(struct ulr_status), ret);
}

/*
 * Set link or bundle debug levels
 */
int
ulr_debug(int so, struct ulr_debug *dbg)
{
	int ret = 0;
	act_hdr_t *ah;

	ppplog(MSG_ULR, 0, "User Requests Debug %d for %s '%s'\n",
	       dbg->level,
	       dbg->type == DEF_LINK ? "link" : "bundle", dbg->id);

	ret = act_find_id_rd(dbg->id, dbg->type, &ah);
	if (ret) {
		ppplog(MSG_ULR, 0, "WARNING %s '%s' not known. (ENOENT)\n",
		       dbg->type == DEF_LINK ? "link" : "bundle", dbg->id);
		ulr_reply(so, (ulr_msg_t *)dbg, sizeof(ulr_msg_t), ret);
		return;
	}		

	ah->ah_debug = dbg->level;
	switch(ah->ah_type) {
	case DEF_LINK:
		cd_d2k_cfg_link(ah);
		break;
	case DEF_BUNDLE:
		cd_d2k_cfg_bundle(ah);
		break;
	}

	act_release(ah);
	ulr_reply(so, (ulr_msg_t *)dbg, sizeof(ulr_msg_t), 0);
}

int
ulr_stats(int so, struct ulr_stats *st)
{
	int ret = 0;
	act_hdr_t *ah;

	ppplog(MSG_ULR, 0, "User Requests statistics for %s '%s'\n",
	       st->type == DEF_LINK ? "link" : "bundle", st->id);

	ret = act_find_id_rd(st->id, st->type, &ah);
	if (ret) {
		ppplog(MSG_ULR, 0, "%s '%s' not known. (ENOENT)\n",
		       st->type == DEF_LINK ? "link" : "bundle",
		       st->id);
		ulr_reply(so, (ulr_msg_t *)st, sizeof(ulr_msg_t), ret);
		return;
	}		

	switch (st->type) {
	case DEF_LINK:
		if (ah->ah_link.al_flags & ALF_PHYS_UP)
			cd_d2k_adm_link(ah, CMD_STATS);
		else
			ret = ENODEV; /* No such link open */
		break;
	case DEF_BUNDLE:
		cd_d2k_adm_bundle(ah, CMD_STATS);
		break;

	}

	if (!ret) {
		act_release(ah);
/* YUK - need to time stamp stats */

		nap(500);

		ret = act_find_id_rd(st->id, st->type, &ah);
		if (ret) {
			ppplog(MSG_ULR, 0, "%s '%s' not known. (ENOENT)\n",
			       st->type == DEF_LINK ? "link" : "bundle",
			       st->id);
			ulr_reply(so, (ulr_msg_t *)st, sizeof(ulr_msg_t), ret);
			return;
		}		
	} else
		ret = 0;

	switch (st->type) {
	case DEF_LINK:
		memcpy(&st->spc, &ah->ah_link.al_stats,
		       sizeof(struct pc_stats_s));
		break;
	case DEF_BUNDLE:
		memcpy(&st->spc, &ah->ah_bundle.ab_stats,
		       sizeof(struct bl_stats_s));
		break;
	}

	act_release(ah);
	ulr_reply(so, (ulr_msg_t *)st, sizeof(struct ulr_stats), ret);
}

/*
 * Send an error reply to user, free the message when complete
 */
int
ulr_reply(int so, ulr_msg_t *m, int len, int err)
{
	int ret;

	m->prim.error = err;

	ret = send(so, (addr_t)m, len, 0);

	if (ret != len) {
		ppplog(MSG_WARN, 0,
		       "ulr_reply: Error sending reply to user, %d, %d\n",
		       ret, errno);
	}
}

/*
 * User has defined a configuration ... store it
 */
int
ulr_set_cfg(int so, struct ulr_cfg *uc)
{
	int ret;
	act_hdr_t *ah;

	ppplog(MSG_ULR, 0, "Set config for %s (type %d)\n",
	       uc->ucid, uc->type);

	uc->ucch.ch_flags = 0;

	ret = ucfg_set(uc->type, &uc->ucch);
	if (ret) {
		ulr_reply(so, (ulr_msg_t *)uc, sizeof(ulr_msg_t), ret);
		return;
	}

	/* Activate links/bundles .. if not already active */
	if (act_find_id_rd(uc->ucid, uc->type, &ah) == 0) {
		act_release(ah);
	} else { 
		/* Enoent */
		switch(uc->type) {
		case DEF_BUNDLE:
			ppplog(MSG_ULR, 0, "Activate bundle %s\n", uc->ucid);
			ret = act_bundle_create(uc->ucid);
			break;
		case DEF_LINK:
			ppplog(MSG_ULR, 0, "Activate link %s\n", uc->ucid);
			ret = act_link_create(uc->ucid);
			break;
		}
	}
	ulr_reply(so, (ulr_msg_t *)uc, sizeof(struct ulr_prim), ret);
}

/*
 * User has enquired about a configured element ... retreive to
 * configuration for the user
 */
int
ulr_get_cfg(int so, struct ulr_cfg *uc)
{
	cfg_hdr_t *c;
	int ret;

	ret = ucfg_findid(uc->ucid, uc->type, &c);
	if (ret) {
		ulr_reply(so, (ulr_msg_t *)uc, sizeof(ulr_msg_t), ret);
		return;
	}

	memcpy(&uc->ucch, c, c->ch_len);
	ucfg_release();

	ulr_reply(so, (ulr_msg_t *)uc, uc->uclen + sizeof(struct ulr_cfg), 0);
}

/*
 * List ID's of the specified type.
 * Cookies are the nth occurance of an object of the specified type.
 */
int
ulr_list_cfg(int so, struct ulr_list_cfg *m)
{
	struct list_ent *le = &m->entry;
	int start_cookie = (int)le->le_cookie;
	int i, num = 0;
	cfg_hdr_t *c;
	extern rwlock_t ucfg_rwlock;
	extern struct cfg_hdr *ucfg[];

	/* Lock the definition list */

	RW_RDLOCK(&ucfg_rwlock);

	/* Scan to the starting position */

	c = ucfg[m->type];

	i = 0;
	while (i < start_cookie + 1 && c) {
		if (i < start_cookie && c)
			c = c->ch_next;
		if (!(c->ch_flags & CHF_HIDE))
			i++;
	}

	if (!c) {
		RW_UNLOCK(&ucfg_rwlock);
		m->num = 0;
		ulr_reply(so, (ulr_msg_t *)m, sizeof(struct ulr_list_cfg), 0);
		return;
	}

	/*
	 * Copy entries into the message until the message is full
	 * or we run out of definitions
	 */
	while (c) {
		if (c->ch_flags & CHF_HIDE) {
			c = c->ch_next;
			continue;
		}

		/* Copy this entry to the return message */
		le->le_cookie = (void *)++start_cookie;
		le->le_refs = c->ch_refcnt;
		strcpy(le->le_id, c->ch_id);

		le++;
		num++;

		/* Find the next entry */
		c = c->ch_next;

		/*Check we can fit another entry in the message */
		if ((char *)(le + 1) > (char *)m + ULR_MAX_MSG)
			break;
	}
	RW_UNLOCK(&ucfg_rwlock);
	m->num = num;
	ulr_reply(so, (ulr_msg_t *)m, (char *)le - (char *)m, 0);
}

/*
 * User wished to remove a configuration
 */
int
ulr_del_cfg(int so, struct ulr_cfg *uc)
{
	int ret = 0;
	act_hdr_t *al, *ab;
	uint_t t;

	ppplog(MSG_ULR, 0, "Deleting configuration for %s (type %d)\n",
	       uc->ucid, uc->type);

	switch(uc->type) {
	case DEF_BUNDLE:
		ppplog(MSG_ULR, 0, "Delete bundle %s\n", uc->ucid);
		t = 500; /* Initial nap time */
	more_bundle:
		ret = act_find_id(uc->ucid, uc->type, &ab);
		if (!ret) {
			/* It's active */
			if (ab->ah_bundle.ab_numlinks > 0) {
				bundle_close(ab);
				act_release(ab);
				nap(t);
				t <<= 1;
				goto more_bundle;
			}

			act_release(ab);
			ret = act_destroy(uc->ucid, uc->type);
			if (ret)
				ppplog(MSG_WARN, 0,
				       "Error deactivating bundle %s, %d\n",
				       uc->ucid, ret);
		}
		
		break;
	case DEF_LINK:
		ppplog(MSG_ULR, 0, "Delete link %s\n", uc->ucid);
		t = 500; /* Initial nap time */
	more_link:
		ret = act_find_id(uc->ucid, uc->type, &al);
		if (!ret) {
			/* It's active */

			if (al->ah_phase != PHASE_DEAD ||
			    al->ah_link.al_bundle) {
				ab = al->ah_link.al_bundle;
				ASSERT(ab);

				ATOMIC_INT_INCR(al->ah_refcnt);
				act_release(al);			

				MUTEX_LOCK(&ab->ah_mutex);
				MUTEX_LOCK(&al->ah_mutex);
				ATOMIC_INT_DECR(al->ah_refcnt);

				bundle_linkdrop(ab, al, ALR_USERCLOSE);

				MUTEX_UNLOCK(&al->ah_mutex);
				MUTEX_UNLOCK(&ab->ah_mutex);

				nap(t);
				t <<= 1;
				goto more_link;
			} else
				act_release(al);			

			ret = act_destroy(uc->ucid, uc->type);
			if (ret)
				ppplog(MSG_WARN, 0,
				       "Error deactivating link %s, %d\n",
				       uc->ucid, ret);
		}
		break;
	default:
		break;
	}

	/*
	 * If we had ENOENT, then the link/bundle was not
	 * active ... so delete it anyway
	 */
	if (ret == ENOENT)
		ret = 0;

	if (!ret)
		ret = ucfg_del(uc->ucid, uc->type);

	ulr_reply(so, (ulr_msg_t *)uc, sizeof(struct ulr_prim), ret);
}

/*
 * Reset a configuration .. 
 *
 * Kill any active connection ... deactivate then reactivate
 */
int
ulr_reset_cfg(int so, struct ulr_cfg *uc)
{
	int ret;
	act_hdr_t *al, *ab;
	uint_t t = 500;
	char *bundle_id = NULL;

	switch(uc->type) {
	case DEF_BUNDLE:
		ppplog(MSG_ULR, 0, "Reset bundle %s\n", uc->ucid);
	more_bundle:
		ret = act_find_id(uc->ucid, uc->type, &ab);
		if (!ret) {
			/* It's active */
			if (ab->ah_bundle.ab_numlinks > 0) {
				bundle_close(ab, ABR_USERCLOSE);
				act_release(ab);
				nap(t);
				t <<= 1;
				goto more_bundle;
			}

			act_release(ab);
			ret = act_destroy(uc->ucid, uc->type);
			if (ret)
				ppplog(MSG_WARN, 0,
				       "Error deactivating bundle %s, %d\n",
				       uc->ucid, ret);
		}
		
		ret = act_bundle_create(uc->ucid);
		break;
	case DEF_LINK:
		ppplog(MSG_ULR, 0, "Reset link %s\n", uc->ucid);
	more_link:
		ret = act_find_id(uc->ucid, uc->type, &al);
		if (!ret) {
			/* It's active */

			if (al->ah_phase != PHASE_DEAD ||
			    al->ah_link.al_bundle) {
				ab = al->ah_link.al_bundle;
				ASSERT(ab);

				ATOMIC_INT_INCR(al->ah_refcnt);
				act_release(al);			

				MUTEX_LOCK(&ab->ah_mutex);
				MUTEX_LOCK(&al->ah_mutex);
				ATOMIC_INT_DECR(al->ah_refcnt);

				/* Remember the bundle the link was in */
				if (!bundle_id)
					bundle_id = strdup(ab->ah_cfg->ch_id);

				bundle_linkdrop(ab, al, ALR_USERCLOSE);

				MUTEX_UNLOCK(&al->ah_mutex);
				MUTEX_UNLOCK(&ab->ah_mutex);

				nap(t);
				t <<= 1;
				goto more_link;
			} else
				act_release(al);			

			ret = act_destroy(uc->ucid, uc->type);
			if (ret)
				ppplog(MSG_WARN, 0,
				       "Error deactivating link %s, %d\n",
				       uc->ucid, ret);
		}

		ret = act_link_create(uc->ucid);

		if (bundle_id) {
			/* If the link is a static it will now restart */
			if (!ret)
				bundle_statics_up(bundle_id);
			free(bundle_id);
		}
		break;
	default:
		ppplog(MSG_WARN, 0, "ulr_reset_cfg: Unexpected type %d\n",
		       uc->type);
		ret = EINVAL;
		break;
	}
	ulr_reply(so, (ulr_msg_t *)uc, sizeof(struct ulr_prim), ret);
}

int
ulr_end(int so, ulr_msg_t *m, int *quit)
{
	*quit = 1;
}

/*
 * Shutdown the daemon
 */
int
ulr_stop(int so, ulr_msg_t *m, int *quit)
{
	ppplog(MSG_INFO, 0, "Stop.\n");
	pppstop();
	ulr_reply(so, m, sizeof(ulr_msg_t), 0);
	ppplog(MSG_INFO, 0, "PPP Stopped.\n");
	exit(0);
}

/*
 * Place the specifed text in the log file
 */
int
ulr_log(int so, struct ulr_log *m, int *quit)
{
	ppplog(MSG_INFO, 0, "Log : %s\n", m->msg);
	ulr_reply(so, (ulr_msg_t *)m, sizeof(struct ulr_prim), 0);
}

/*
 * Set or get the configuration state
 */
int
ulr_cfgstate(int so, struct ulr_state *state, int *quit)
{
	extern int ucfg_state, pppd_debug;

	switch (state->cmd) {
	case UDC_GET:
		state->cfgstate = ucfg_state;
		state->debug = pppd_debug;
		break;
	case UDC_SET:
		ucfg_state = state->cfgstate;
		break;
	}
	ulr_reply(so, (ulr_msg_t *)state, sizeof(ulr_msg_t), 0);
}

int
ulr_unexpected(int so, ulr_msg_t *m, int *quit)
{
	ppplog(MSG_WARN, 0,
	       "ulr_unexpected: Unexpected message type received %d\n",
	       m->prim);
	ulr_reply(so, m, sizeof(ulr_msg_t), EINVAL);
}

/*
 * Message value to handler function mapping table
 */
int (*ulr_table[])() = {
	ulr_incoming,
	ulr_attach,
	ulr_detach,
	ulr_kill,
	ulr_status,
	ulr_set_cfg,
	ulr_get_cfg,
	ulr_list_cfg,
	ulr_del_cfg,
	ulr_reset_cfg,
	ulr_stop,
	ulr_log,
	ulr_linkadd,
	ulr_linkdrop,
	ulr_linkkill,
	ulr_stats,
	ulr_debug,
	ulr_list_links,
	ulr_psm_stats,
	ulr_psm_status,
	ulr_cfgstate,
	ulr_call_hist,
	ulr_hist_clear,
	ulr_end,	/* MUST BE LAST */
};

ulr_read_msg(int so, ulr_msg_t *msg, int *len)
{
	struct msghdr msgh;
	struct iovec iov;
	int passfd = -1;
	int ret;

	iov.iov_base = (char *)msg;
	iov.iov_len = ULR_MAX_MSG;
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;

	msgh.msg_accrights = (caddr_t)&passfd;
	msgh.msg_accrightslen = sizeof(passfd);

	ret = recvmsg(so, &msgh, 0);

	if (ret > 0 && msg->prim.prim == ULR_INCOMING)
		msg->incoming.fd = passfd;

	if (ret < 0)
		return ret;
	else if (ret == 0)
		return -1;

	*len = ret;
	return 0;
}

void *
ulr_process(void *argp)
{
	struct ulr_prim *msg;
	int so = *(int *)argp;
	int quit = 0;
	int len;

	msg = (struct ulr_prim *)malloc(ULR_MAX_MSG);
	if (!msg) {
		ppplog(MSG_WARN, 0, "ulr_process: Out of memory!\n");
		goto exit;
	}

	while (!quit) {

		/* Read request */
		if (ulr_read_msg(so, (ulr_msg_t *)msg, &len))
			goto exit;
		/*
		 * The function called to  handle the message
		 * is NOT expected to free the message 
		 */
		if (msg->prim >= 0 && msg->prim <= ULR_END)
			(*ulr_table[msg->prim])(so, msg, &quit);
		else
			ulr_unexpected(so, (ulr_msg_t *)msg, &quit);
	}

exit:
	if (msg)	
		free(msg);

	close(so);
	free(argp);
	thr_exit(0);
}

/*
 * User-level request thread.
 *
 * This waits for a connection from appp utility and then creates a thread
 * to deal with it.
 */
void *
ulr_thread()
{
	int so, msg_sock;
	struct sockaddr sin;
	int waitmax = 5;
	int sinlen;
	int *argp;

	so = sockinit(waitmax);

 	for (;;) {
		
		sinlen = sizeof(sin);
		if ((msg_sock = accept(so, (struct sockaddr *)&sin,
				       (size_t *)&sinlen)) == -1) {

			if (errno == EINTR) {
				continue;
			}
			ppplog(MSG_ERROR, 0, "ulr_thread: accept error, %d\n",
			       errno);
			sleep(10); /* Don't spin too fast !*/
			continue;
		}

		argp = (int *)malloc(sizeof(int));
		if (!argp) {
			close(msg_sock);
			ppplog(MSG_ERROR, 0, "ulr_thread: Out of memory !\n");
			sleep(10); /* Don't spin too fast !*/
			continue;
		}

		*argp = msg_sock;

		thr_create(NULL, 0, ulr_process, (void *)argp,
			   THR_DETACHED | THR_BOUND, NULL);
	}
}
