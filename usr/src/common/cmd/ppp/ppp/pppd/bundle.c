#ident	"@(#)bundle.c	1.9"

#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <synch.h>
#include <thread.h>
#include <errno.h>
#include <dlfcn.h>
#include "dial.h"

#include "pathnames.h"
#include "psm.h"
#include "ppp_type.h"
#include "ppp_proto.h"
#include "ppp_cfg.h"
#include "act.h"

/*
 * This module contains the Active Bundle/Link management routines
 *
 * When a configuration is acivated, bundle_create is called to 
 * construct the defined bundle config. An element for the bundle
 * and each defined link is placed on the list of acive elements.
 *
 */

act_hdr_t * act_join_bundle(act_hdr_t * al);
#define CALL_IN "incoming"
#define CALL_OUT "outgoing"

bundle_audit(act_hdr_t *ab)
{
	time_t t;
	struct bl_stats_s *bs;
	uint_t h, m, s, d;
	struct cfg_hdr *ch;
	char *dir;
	struct tm tmtime;
	char tok[255];

	bs = &ab->ah_bundle.ab_stats;
	ch = ab->ah_cfg;

	if (ab->ah_bundle.ab_flags & ABF_CALLED_IN)
		dir = CALL_IN;
	else
		dir = CALL_OUT;

	if (ab->ah_bundle.ab_open_links > 0) {

		localtime_r(&bs->bs_time, &tmtime);
		asctime_r(&tmtime, tok);

		ppplog(MSG_AUDIT, ab,
		       "%s %s. Connected %s", dir, ch->ch_id, tok);

	} else {

		/* It's not open now ... log connect time */

		t = bs->bs_time - bs->bs_contime;
		s = t % 60;
		m = (t / 60) % 60;
		h = t / 3600;
	
		d = (bs->bs_inoctets + bs->bs_outoctets) / 1024;


		ppplog(MSG_AUDIT, ab,
"%s %s. Disconnected. Total connection time %d:%2.2d:%2.2d. Data %d Kbytes\n",
		       dir, ch->ch_id, h, m, s, d);
	}
}

/*
 * Called when a link wishes to join a bundle, the link will
 * be in the AUTH -> NETWORK phase transition.
 */
act_hdr_t *
bundle_incoming_join(act_hdr_t *al)
{
	act_hdr_t *ab;
	struct cfg_bundle *cb;
	int ret;

	ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

	ATOMIC_INT_INCR(al->ah_refcnt);
	MUTEX_UNLOCK(&al->ah_mutex);

	/* Find out which bundle the link could join */
	ab = act_join_bundle(al);
	if (!ab) {
		MUTEX_LOCK(&al->ah_mutex);
		ATOMIC_INT_DECR(al->ah_refcnt);
		return NULL;
	}

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));

	cb = (struct cfg_bundle *)ab->ah_cfg;

	if (!ab->ah_bundle.ab_links)
		act_bundle_up(ab, ABF_CALLED_IN);

	/* Will adding this link exceed maxlinks */
	if (ab->ah_bundle.ab_numlinks >= cb->bn_maxlinks) {
		ppplog(MSG_DEBUG, ab,
	       "Request for link addition failed. Maxlinks reached.\n");
		MUTEX_UNLOCK(&ab->ah_mutex);
		MUTEX_LOCK(&al->ah_mutex);
		ATOMIC_INT_DECR(al->ah_refcnt);
		return NULL;
	}

	MUTEX_LOCK(&al->ah_mutex);
	ATOMIC_INT_DECR(al->ah_refcnt);

	ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

	/* Add the link to the bundle */

	ppplog(MSG_INFO_LOW, ab,
	       "Adding link %s to the bundle\n", al->ah_cfg->ch_id);

	al->ah_link.al_next = ab->ah_bundle.ab_links;
	ab->ah_bundle.ab_links = al;
	ab->ah_bundle.ab_addindex = al->ah_link.al_cindex;

	al->ah_link.al_bundle = ab;
	al->ah_link.al_flags |= ALF_INUSE;
	ab->ah_bundle.ab_numlinks++;

	ppplog(MSG_DEBUG, al, "bundle_incoming_join: numlinks now %d\n",
	       ab->ah_bundle.ab_numlinks);

	/* Tell the kernel that this link belongs to the bundle */

	ret = cd_add_link2bundle(al, ab, 0);
	if (ret) {
		ppplog(MSG_WARN, ab,
		       "Failed to add link %s to bundle (error %d)\n",
		       al->ah_cfg->ch_id, ret);

		/* Remove from the bundle .. */
		ab->ah_bundle.ab_links = al->ah_link.al_next;
		al->ah_link.al_next = NULL;

		al->ah_link.al_bundle = NULL;
		al->ah_link.al_flags &= ~ALF_INUSE;
		ab->ah_bundle.ab_numlinks--;

		/* Need to unlock the bundle */
		ATOMIC_INT_INCR(al->ah_refcnt);
		MUTEX_UNLOCK(&al->ah_mutex);
		MUTEX_UNLOCK(&ab->ah_mutex);
		MUTEX_LOCK(&al->ah_mutex);
		ATOMIC_INT_DECR(al->ah_refcnt);
		return NULL;
	}

	/* Both the bundle and link are now locked */
	return ab;
}

/*
 * Check that a link is suitable for a bundle.
 *
 * Things to check include ed, auth name and if incoming, callerid and login.
 *
 * Both the link and bundle passed in must be locked.
 */
int
bundle_check_linkid(act_hdr_t *ab, act_hdr_t *al)
{
	act_hdr_t *e_al;	/* Existing al */
	int ret = 0;

	e_al = ab->ah_bundle.ab_links;

	while (e_al) {
		if (e_al != al) {
			/* Check it's state !! Need an LCP OPENed link */
			MUTEX_LOCK(&e_al->ah_mutex);
			if (e_al->ah_link.al_lcp->ph_state.ps_state == OPENED)
				break;
			MUTEX_UNLOCK(&e_al->ah_mutex);
		} 

		e_al = e_al->ah_link.al_next;
	}

	if (!e_al) {
		ppplog(MSG_DEBUG, ab, "First link about to join\n");
		return 1;
	}
	
	/* We have a link to compare with */

	ppplog(MSG_DEBUG, e_al, "Checking new link against this ..\n");

	/* Compare ... */

	if (!(e_al->ah_link.al_local_opts & ALO_MRRU)) {
		ppplog(MSG_DEBUG, ab,
		       "Local,  Multilink not negotiated - no more links\n");
		goto exit;
	}

	if (!(e_al->ah_link.al_peer_opts & ALO_MRRU)) {
		ppplog(MSG_DEBUG, ab,
		       "Peer, Multilink not negotiated - no more links\n");
		goto exit;
	}

	if (e_al->ah_link.al_peer_opts & ALO_ED) {
		/* Compare ED */

		if (e_al->ah_link.al_peer_ed_class !=
		    al->ah_link.al_peer_ed_class) {
			ppplog(MSG_DEBUG, ab,
			       "Endpoint Discriminator mismatch (Class)\n");
			goto exit;
		}

		if (e_al->ah_link.al_peer_ed_len !=
		    al->ah_link.al_peer_ed_len) {
			ppplog(MSG_DEBUG, ab,
			       "Endpoint Discriminator mismatch (Len)\n");
			goto exit;
		}

		if (memcmp(e_al->ah_link.al_peer_ed, 
			   al->ah_link.al_peer_ed,
			   e_al->ah_link.al_peer_ed_len) != 0) {
			ppplog(MSG_DEBUG, ab,
			       "Endpoint Discriminator mismatch (Data)\n");
			goto exit;
		}

		/*
		 * Check that we aren't loopbacked
		 * If classes are equal ensure data not equal
		 */
		if ((ab->ah_bundle.ab_local_ed_class == 
		    al->ah_link.al_peer_ed_class) &&
		    (ab->ah_bundle.ab_local_ed_len == 
		     al->ah_link.al_peer_ed_len) &&
		    (memcmp(ab->ah_bundle.ab_local_ed_addr,
			   al->ah_link.al_peer_ed,
			   al->ah_link.al_peer_ed_len) == 0)) {
			ppplog(MSG_WARN, ab,
			       "Endpoint Discriminator Indicates Loopback.\n");
			goto exit;
		}

	}

	/* Check same auth algorithm */

	if ((e_al->ah_link.al_flags & (ALO_PAP | ALO_CHAP)) !=
	    (al->ah_link.al_flags & (ALO_PAP | ALO_CHAP))) {
		ppplog(MSG_DEBUG, ab, "Auth protcol mis-match.\n");
		goto exit;
	}

	/* Now ensure that the peerauthname matches */

	if (e_al->ah_link.al_peer_auth_name) {

		if (!al->ah_link.al_peer_auth_name) {
			ppplog(MSG_DEBUG, ab, "Auth name expected - not found\n");
			goto exit;
		}

		if (strcmp((char *)e_al->ah_link.al_peer_auth_name,
			   (char *)al->ah_link.al_peer_auth_name) != 0) {

			ppplog(MSG_DEBUG, ab,
			       "Auth names differ '%s', '%s'.\n",
			       (char *)e_al->ah_link.al_peer_auth_name,
			       (char *)al->ah_link.al_peer_auth_name);

			goto exit;
		}

	} else if (al->ah_link.al_peer_auth_name) {
		ppplog(MSG_DEBUG, ab, "Auth name mis-match - shouldn't have one\n");
		goto exit;
	}

	/*
	 * Check that the link discriminator is unique and we have
	 * one if other links in the bundle have one
	 */
/*TODO*/
	ret = 1;

 exit:
	MUTEX_UNLOCK(&e_al->ah_mutex);
	ppplog(MSG_DEBUG, ab, "bundle_check_linkid: return %d\n", ret);
	
	return ret;
}

/*
 * Check that ML attributes (SSN and MRRU Match)
 * Both the link and bundle passed in must be locked.
 */
int
bundle_check_linkattr(act_hdr_t *ab, act_hdr_t *al)
{
	act_hdr_t *e_al;	/* Existing al */
	int ret = 0;

	e_al = ab->ah_bundle.ab_links;

	while (e_al) {
		if (e_al != al) {
			/* Check it's state !! Need an LCP OPENed link */
			MUTEX_LOCK(&e_al->ah_mutex);
			if (e_al->ah_link.al_lcp->ph_state.ps_state == OPENED)
				break;
			MUTEX_UNLOCK(&e_al->ah_mutex);
		} 

		e_al = e_al->ah_link.al_next;
	}

	if (!e_al) {
		ppplog(MSG_DEBUG, ab, "First link about to join\n");
		return 1;
	}
	
	/* We have a link to compare with */
	ppplog(MSG_DEBUG, e_al, "Checking new link against this ..\n");

	/* Check the peer end ML negotiated options match */
	ppplog(MSG_DEBUG, al, "Peer opts %x, %x\n", 
	       (e_al->ah_link.al_peer_opts & ALO_MLOPTS),
	       (al->ah_link.al_peer_opts & ALO_MLOPTS));

	if ((e_al->ah_link.al_peer_opts & ALO_MLOPTS) != 
	    (al->ah_link.al_peer_opts & ALO_MLOPTS)) {
		ppplog(MSG_DEBUG, ab, "Peers ML options didn't match\n");
		goto exit;
	}

	/* Check the local end ML negotiated options match */
	ppplog(MSG_DEBUG, al, "Local opts %x, %x\n", 
	       (e_al->ah_link.al_local_opts & ALO_MLOPTS),
	       (al->ah_link.al_local_opts & ALO_MLOPTS));

	if ((e_al->ah_link.al_local_opts & ALO_MLOPTS) != 
	    (al->ah_link.al_local_opts & ALO_MLOPTS)) {
		ppplog(MSG_DEBUG, ab, "Local ML options didn't match\n");
		goto exit;
	}

	/* Check Mrru is the same */

	ppplog(MSG_DEBUG, al, "Local peer mrru %d, %d\n",
	       e_al->ah_link.al_peer_mrru, al->ah_link.al_peer_mrru);

	if (e_al->ah_link.al_peer_mrru != al->ah_link.al_peer_mrru) {
		ppplog(MSG_DEBUG, ab, "Peers MRRU didn't match\n");
		goto exit;
	}

	/*
	 * If the peer has LD's ... ensure they are unique
	 */
/*TODO*/	
	ret = 1;

 exit:
	MUTEX_UNLOCK(&e_al->ah_mutex);
	ppplog(MSG_DEBUG, ab, "bundle_check_linkattr: return %d\n", ret);
	
	return ret;
}

/*
 * Set a links multilink attributes to those contained in the
 * bundle
 */
void
bundle_set_linkattr(act_hdr_t *ab, act_link_t *al)
{
	al->al_local_mrru = ab->ah_bundle.ab_local_mrru;
	al->al_peer_mrru = ab->ah_bundle.ab_peer_mrru;

	al->al_peer_opts &= ~(ALO_SSN | ALO_ED);
	al->al_local_opts &= ~(ALO_SSN | ALO_ED);

	if (ab->ah_bundle.ab_peer_ssn)
		al->al_peer_opts |= ALO_SSN;

	if (ab->ah_bundle.ab_local_ssn)
		al->al_local_opts |= ALO_SSN;

	if (ab->ah_bundle.ab_local_ed)
		al->al_local_opts |= ALO_ED;

	if (ab->ah_bundle.ab_peer_ed)
		al->al_peer_opts |= ALO_ED;

	al->al_peer_opts |= ALO_MRRU;
	al->al_local_opts |= ALO_MRRU;

	if (ab->ah_bundle.ab_flags & ABF_LDS) {
		al->al_local_opts |= ALO_LD;
		al->al_peer_opts |= ALO_LD;
	}
}

/*
 * Selects an outgoing link for a call based on service type
 * Finds a link with matching service type that appears not to be in use.
 * - Use links in their order of definition.
 *
 * Returns 0 	- success,
 * EAGAIN	- no links available
 * ENXIO	- no links defined
 */
int
select_outgoing(act_hdr_t *ab, act_hdr_t **alp, int service)
{
	act_hdr_t *al;
	struct cfg_bundle *cb;
	char *links, buf[MAXID], *sys;
	int i, ret;

	ASSERT(ab->ah_inuse == INUSE); /*DEBUG*/

	/* First find an appropriate configured link to use */

	cb = (struct cfg_bundle *)ab->ah_cfg;
	links = ucfg_str(&cb->bn_ch, cb->bn_links);
	if (!*links) {
		ppplog(MSG_INFO_MED, ab,
		       "No Outgoing/Links defined for bundle\n");
		return ENXIO;
	}

	ppplog(MSG_DEBUG, ab, "select_outgoing: Selecting from %s\n", links);
	ab->ah_bundle.ab_addindex = 0;

trynext:
	links = ucfg_get_element(links, buf);
	ab->ah_bundle.ab_addindex++;

	if (!*buf) {
		ppplog(MSG_DEBUG, ab, "select_outgoing: Tried all links\n");
		return EAGAIN; /* Resource temp unavailable */
	}

	ppplog(MSG_DEBUG, ab, "select_outgoing: Link to try is %s\n", buf);

	/* Is it in use ? Is it the correct service type ? */

	/*
	 * While scanning the list of links, just use read access ..
	 * if a link looks like we could use it then we re-aquire in r/w
	 */
	ret = act_find_id_rd(buf, DEF_LINK, &al);
	if (ret) {
		ppplog(MSG_DEBUG, ab,
		       "select_outgoing: Link %s not found in active list !\n",
		       buf);
		goto trynext;
	}

	if ((al->ah_link.al_flags & ALF_INUSE) ||
	    ATOMIC_INT_READ(al->ah_refcnt) > 1 ) {
		ppplog(MSG_DEBUG, ab,
		       "select_outgoing: Link %s is in use\n", buf);
		act_release(al);
		goto trynext;
	}

	ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

	/* Get the link for r/w */
	act_release(al);

	ret = act_find_id(buf, DEF_LINK, &al);
	if (ret) {
		ppplog(MSG_DEBUG, ab,
		       "select_outgoing: Link %s not found in active list !\n",
		       buf);
		goto trynext;
	}

	if (al->ah_link.al_flags & ALF_INUSE) {
		ppplog(MSG_DEBUG, ab,
		       "select_outgoing: Link %s is in use\n", buf);
		act_release(al);
		goto trynext;
	}

	/*
	 * Check the service type, if not correct goto retry.
	 */
/*TODO*/

	/*
	 * Check if the cs thinks the device is available
	 */
/*TODO*/

	*alp = al;
	return 0;
}

/*
 * Given a bundle and link, this routine makes a call on the
 * specified link, to the "outgoing" system. The link is then
 * added to the bundle and LCP started.
 *
 * Bundle and link must be locked
 *
 * Returns 0	- success
 * errors from select_outgoing
 * EIO		- for dial errors
 */
STATIC int
add_link2bundle(act_hdr_t *ab, act_hdr_t *al, char *phone, int *dial_error)
{
	int ret = 0;
	char *sys;
	char *nullstr = "";
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
	ASSERT(MUTEX_LOCKED(&al->ah_mutex));

	ASSERT(ab->ah_inuse == INUSE); /*DEBUG*/
	ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

	if (!ab->ah_bundle.ab_links)
		act_bundle_up(ab, ABF_CALLED_OUT);

	/* Add the link to the bundle */

	al->ah_link.al_next = ab->ah_bundle.ab_links;
	ab->ah_bundle.ab_links = al;
	al->ah_link.al_bundle = ab;
	al->ah_link.al_flags |= ALF_INUSE;
	al->ah_link.al_flags |= ALF_DIAL;
	al->ah_link.al_cindex = ab->ah_bundle.ab_addindex;
	ab->ah_bundle.ab_numlinks++;

	ppplog(MSG_DEBUG, ab, "add_link2bundle: numlinks now %d\n",
	       ab->ah_bundle.ab_numlinks);

	/* Connect the link */
	sys = ucfg_str(&cb->bn_ch, cb->bn_remote);
	if (!*sys && (!phone || !*phone)) {
		ret = EIO;
		*dial_error = BAD_SYS;
		goto exit;
	}
	
	/*
	 * Drop the locks for a while ... this may take some time
	 */
	ATOMIC_INT_INCR(al->ah_refcnt);
	MUTEX_UNLOCK(&al->ah_mutex);
	ATOMIC_INT_INCR(ab->ah_refcnt);
	MUTEX_UNLOCK(&ab->ah_mutex);

	ret = link_dial(al, sys, phone, dial_error);

	MUTEX_LOCK(&ab->ah_mutex);
	ATOMIC_INT_DECR(ab->ah_refcnt);
	MUTEX_LOCK(&al->ah_mutex);
	ATOMIC_INT_DECR(al->ah_refcnt);

	/* Stuff to do here ... if it was busy ..
	 * try the next line ? .. retry the same line ???
	 */
	if (ret || *dial_error) {
		if (!ret)
			ret = EIO; /* Indicates dial error is set */
		ppplog(MSG_INFO_LOW, ab, "Failed to dial on link '%s'\n",
		       al->ah_cfg->ch_id);
		goto exit;
	}

	/* Condition the line */

	ret = link_condition(al);
	if (ret)
		goto exit;
	
	/* Tell the kernel that this link belongs to the bundle */

	ppplog(MSG_INFO_MED, ab,
	       "Adding link %s to the bundle\n", al->ah_cfg->ch_id);

	ret = cd_add_link2bundle(al, ab, 0);
	if (ret) {
		ppplog(MSG_WARN, ab,
		       "Failed to add link %s to bundle (error %d)\n",
		       al->ah_cfg->ch_id, ret);
		goto exit;
	}

	/* Start LCP */
	al->ah_link.al_flags &= ~ALF_DIAL;

	fsm_state(al->ah_link.al_lcp, OPEN, 0);

exit:
	if (ret) {
		ab->ah_bundle.ab_links = al->ah_link.al_next;
		act_link_init(al);
		ab->ah_bundle.ab_numlinks--;
	}
	return ret;
}

bundle_statics_up(char *bundleId)
{
	struct cfg_bundle *cb;
	char *links, buf[MAXID];
	act_hdr_t *al, *ab;
	int ret, dummy, more;

	do {
		more = 0;

		/* Check not already initialised */
		ret = act_find_id(bundleId, DEF_BUNDLE, &ab);
		if (ret) {
			ppplog(MSG_DEBUG, ab,
			       "bundle_statics_up: Bundle not found !\n",
			       bundleId);
			return ret;
		}

		ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

		cb = (struct cfg_bundle *)ab->ah_cfg;

		if (!(cb->bn_type & BT_OUT)) {
			act_release(ab);
			continue;
		}

		links = ucfg_str(&cb->bn_ch, cb->bn_links);
		if (!*links) {
			act_release(ab);
			return;
		}

		if (ab->ah_bundle.ab_open_links < 1 &&
		    ab->ah_bundle.ab_numlinks != ab->ah_bundle.ab_open_links) {
			ppplog(MSG_DEBUG, ab,
			       "Links still in progress ... wait \n");
			act_release(ab);
			nap(500);
			continue;
		}

		while (links && *links) {
			links = ucfg_get_element(links, buf);

			ret = act_find_id_rd(buf, DEF_LINK, &al);
			if (ret) {
				ppplog(MSG_INFO_MED, ab,	
				       "Defined link '%s' is not active\n",   
				       buf);
				continue;
			}

			ASSERT(al->ah_inuse == INUSE);/*DEBUG*/

			if (al->ah_link.al_flags & ALF_INUSE ||
			    al->ah_link.al_type != LNK_STATIC) {
				act_release(al);
				continue;
			}

			/* Found a static link */
			ppplog(MSG_INFO_MED, ab,
			       "Bringing up STATIC link '%s'\n",
			       al->ah_cfg->ch_id);

			/* Need to have the link for r/w */

			act_release(al);

			ret = act_find_id(buf, DEF_LINK, &al);
			if (ret) {
				ppplog(MSG_INFO_MED, ab,	
				       "Defined link '%s' is not active\n",   
				       buf);
				continue;
			}

			ASSERT(al->ah_inuse == INUSE);/*DEBUG*/

			/* Double check */

			if (al->ah_link.al_flags & ALF_INUSE ||
			    al->ah_link.al_type != LNK_STATIC) {
				act_release(al);
				continue;
			}

			ret = add_link2bundle(ab, al, NULL, &dummy);

			act_release(al);

			if (ret) {
				ppplog(MSG_DEBUG, ab,
				 	"Failed to add STATIC link.\n");
				act_release(ab);
				return EIO;
			}

			more = 1;
			break;
		}

		act_release(ab);

		if (more) {
			ppplog(MSG_DEBUG, 0,
			       "bundle_statics_up: Napping ... 500\n");
			nap(500);
		}
			
	} while (more);

	return 0;
}

/*
 * If any static links are in the STOPPED/CLOSED state, re-start them
 *
 * Returns the number kicked.
 */
bundle_kick_statics(act_hdr_t *ab)
{
	act_hdr_t *al;
	int state;
	int count = 0;

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	al = ab->ah_bundle.ab_links;

	while (al) {
		MUTEX_LOCK(&al->ah_mutex);
		ASSERT(al->ah_inuse == INUSE);/*DEBUG*/
		
		/* If static ... and Stopped or Closed ... Give it a kick */
		if (al->ah_link.al_type == LNK_STATIC) {
			state = al->ah_link.al_lcp->ph_state.ps_state;
			if (state == CLOSED || state == STOPPED) {
				ppplog(MSG_INFO_LOW, al,
				       "Restart STATIC link\n");
				fsm_state(al->ah_link.al_lcp, DOWN, NULL);
				count++;
			}
		}

		MUTEX_UNLOCK(&al->ah_mutex);
		al = al->ah_link.al_next;
	}
	return count;
}

/*
 * Check if the bundle has at least one link .. not
 * in the DEAD or TERMINATE phase.
 */
int
bundle_transition(act_hdr_t *ab)
{
	act_hdr_t *al;
	int trans = 0;

	al = ab->ah_bundle.ab_links;

	while (al) {
		MUTEX_LOCK(&al->ah_mutex);
		ASSERT(al->ah_inuse == INUSE);/*DEBUG*/
		
		if (al->ah_phase != PHASE_DEAD && al->ah_phase != PHASE_TERM) {
			ppplog(MSG_DEBUG, al,
			       "bundle_transition: Transitioning\n");
			trans++;
		}
		MUTEX_UNLOCK(&al->ah_mutex);
		al = al->ah_link.al_next;
	}
	return 	trans;
}

/*
 * This routine attempts to open a bundle. 
 *
 * Returns	- 0 success
 * EBUSY	- Indicates that the bundle is busy and we should retry later
 */
int
bundle_open(struct act_hdr_s *ab, int *dial_error)
{
	int  ret;
	act_hdr_t *al;
	char *nullstr = "";
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;
	int numlinks;

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	*dial_error = 0;

	/* Check we have an outgoing definition */

	if (!(cb->bn_type & BT_OUT)) {
		ppplog(MSG_DEBUG, ab,
		       "bundle_open: NOT defined for Outgoing calls\n");
		return EINVAL;
	}

	/* Check that the bundle is not connected */

	if (ab->ah_bundle.ab_numlinks >= cb->bn_minlinks) {
		ppplog(MSG_DEBUG, ab,
		       "bundle_open: Check if any statics need a kick\n");

		ASSERT(ab->ah_bundle.ab_links != NULL);

		/* Check the links have LCP up */
		if (bundle_kick_statics(ab))
			return 0;

		if (bundle_transition(ab))
			return 0;
	}

	ab->ah_bundle.ab_reason = 0;

	for (numlinks = ab->ah_bundle.ab_numlinks;
	     numlinks < cb->bn_minlinks; numlinks++) {

		/*
		 * If we aren't the first link, check that the fist link
		 * is established
		 */
		if (ab->ah_bundle.ab_open_links < 1 &&
		    ab->ah_bundle.ab_numlinks != ab->ah_bundle.ab_open_links) {
			ppplog(MSG_DEBUG, ab,
			       "Links still in progress ... wait \n");
			return EBUSY; /* Come back later ... */;
		}

		/* Find an appropriate configured link to use */

		ret = select_outgoing(ab, &al, LNK_ANY);
		if (ret) {
			ppplog(MSG_DEBUG, ab,
			       "bundle_open: select_outgoing returns %d\n",
			       ret);
			break;
		}

		ret = add_link2bundle(ab, al,
				      ucfg_str(&cb->bn_ch, cb->bn_phone),
				      dial_error);

		act_release(al);

		cd_d2k_adm_bundle(ab, CMD_ADDBW, ret);

		if (ret)
			break;
	}

	return ret;
}

/*
 * Add a link to an open bundle. 
 *
 *	ab - bundle being added to
 *	al - link to add (may be null)
 *	phone - number to dial (may be null)
 *	service - type of service required
 *	dial_error - returned dial error
 *
 * Will return 0 will dial_error 0 if successful
 *
 * This routine is called in three instances
 * 1. Manual outgoing. Phone is NULL.
 *     We use the outgoing definition to obtain the remote system name,
 *     phone number and service type
 * 2. Automatic outgoing. Phone is NULL.
 *     We use the outgoing definition to obtain the remote system name,
 *     phone number and service type
 * 3. The peer initiated the bundle, but we are required to
 *     bringup an additional link (due to BAP). Phone and service will
 *     be valid.
 *
 * Returns	- 0 success
 * EBUSY	- Indicates that the bundle is busy and we should retry later
 * ENODEV	- Maxlinks reached

 */
bundle_linkadd(struct act_hdr_s *ab, struct act_hdr_s *al,
	       char *phone, int service, int *dial_error)
{
	int ours = 0;
	act_hdr_t *alp = al;
	int ret;
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;
	extern int pppd_debug;

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));

	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	if (phone) {
		ppplog(MSG_DEBUG, ab,
		       "bundle_linkadd: Phone number supplied .. need work\n");
		abort();
	}

	if (!(cb->bn_type & BT_OUT)) {
		ppplog(MSG_DEBUG, ab,
       "Request for link addition failed. Not defined for  outgoing calls.\n");
		ret = ENODEV;
		goto exit;
	}

	if (ab->ah_bundle.ab_numlinks >= cb->bn_maxlinks) {
		ppplog(MSG_DEBUG, ab,
	       "Request for link addition failed. Maxlinks reached.\n");
		ret = ENODEV;
		goto exit;
	}

	/*
	 * If we aren't the first link, check that the fist link
	 * is established
	 */
	if (ab->ah_bundle.ab_open_links < 1 &&
	    ab->ah_bundle.ab_numlinks != ab->ah_bundle.ab_open_links) {
		ppplog(MSG_DEBUG, ab,
		       "Links still in progress ... wait \n");
		if (pppd_debug & DEBUG_ANVL)
			ppplog(MSG_DEBUG, ab,
		       "ANVL Code path taken (allowing subseqent add)\n");
		else
			return EBUSY; /* Come back later ... */;
	}

	if (!alp) {
		ret = select_outgoing(ab, &alp, service);
		if (ret) {
			ppplog(MSG_DEBUG, ab,
			       "bundle_linkadd: select_outgoing returns %d\n",
			       ret);
			goto exit;
		}
	}

	ret = add_link2bundle(ab, alp, phone, dial_error);
	if (!al)
		act_release(alp);

 exit:
	/* Tell the kernel that bandwidth has been added */
	cd_d2k_adm_bundle(ab, CMD_ADDBW, ret);
	return ret;
}

/*
 * Drop a link from a bundle.
 * If a link is not specified then one will be selected (prefer
 * non-static low bandwidth)
 *
 * If a link to drop is NOT specified,  then we will select one
 * based on the following
 *	a) NOT a static link
 *	b) NOT an incoming link
 * then from the remaining links,
 *	c) the lowest bandwidth link.
 */
bundle_linkdrop(act_hdr_t *ab, act_hdr_t *al, int l_reason)
{
	int selected = 0;
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;
	int ret = 0;
	uint_t min_bw;
	act_hdr_t *min_bw_al;

	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	if (!al) {

		if (ab->ah_bundle.ab_numlinks <= cb->bn_minlinks) {
			ppplog(MSG_DEBUG, ab,
		       "Request for link drop failed. Minlinks reached.\n");
			ret = ENODEV;
			goto exit;
		}

		/*
		 * Need to select a link to drop.
		 */ 

		al = ab->ah_bundle.ab_links;
		min_bw_al = NULL;
		min_bw = 0xffffffff;

		while (al) {
			MUTEX_LOCK(&al->ah_mutex);

			ASSERT(al->ah_inuse == INUSE);/*DEBUG*/

			/* Don't drop STATIC links */

			if (al->ah_link.al_type == LNK_STATIC)
				goto next;
		
			/* Don't drop INCOMING links */

			if (al->ah_link.al_flags & ALF_INCOMING)
				goto next;

			/*
			 * Is this a lower bandwidth than those
			 * already checked ?
			 */
			if (al->ah_link.al_bandwidth < min_bw) {
				min_bw = al->ah_link.al_bandwidth;
				min_bw_al = al;
			}
			
		next:
			MUTEX_UNLOCK(&al->ah_mutex);
			al = al->ah_link.al_next;
		}

		if (!min_bw_al) {
			ppplog(MSG_DEBUG, ab,
			       "bundle_linkdrop: No suitable link to drop.\n");
			ret = ENODEV;
			goto exit;
		} 

		al = min_bw_al;
		MUTEX_LOCK(&al->ah_mutex);

		ASSERT(al->ah_link.al_flags & ALF_INUSE);
		ASSERT(al->ah_link.al_flags & ALF_PHYS_UP);
		ASSERT(al->ah_link.al_flags & ALF_COND);

		selected = 1;
	}

	ppplog(MSG_DEBUG, ab, "bundle_linkdrop: Terminate link %s, bw %d\n",
	       al->ah_cfg->ch_id, al->ah_link.al_bandwidth);

	al->ah_link.al_reason |= l_reason;

	/* Close all protocols */

	fsm_state(al->ah_link.al_lcp, CLOSE, 0);
	
	if (al->ah_link.al_type == LNK_STATIC &&
	    al->ah_link.al_lcp->ph_state.ps_state == CLOSED) {
		ppplog(MSG_DEBUG, al, "STATIC link CLOSED\n");
		bundle_link_finished(al);
	}

	if (selected)
		MUTEX_UNLOCK(&al->ah_mutex);

 exit:
	/* Tell the kernel that bandwidth could be added when desired */
	cd_d2k_adm_bundle(ab, CMD_REMBW, ret);

	return ret;
}

/*
 * Kill a link from a bundle.
 */
bundle_linkkill(act_hdr_t *ab, act_hdr_t *al, int l_reason)
{
	int selected = 0;
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;
	uint_t min_bw;
	act_hdr_t *min_bw_al;

	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/


	ppplog(MSG_DEBUG, ab, "bundle_linkkill: Terminate link %s, bw %d\n",
	       al->ah_cfg->ch_id, al->ah_link.al_bandwidth);

	al->ah_link.al_reason |= l_reason;

	/* Close all protocols */

	bundle_link_finished(al);
	cd_d2k_adm_bundle(ab, CMD_REMBW, 0);
	return 0;
}

/*
 * Called when an NCP has detected it's idle
 */
void
bundle_ncp_idle(act_hdr_t *ab)
{
	act_hdr_t *al;

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));

	ab->ah_bundle.ab_idle_ncps++;

	if (ab->ah_bundle.ab_idle_ncps != ab->ah_bundle.ab_open_ncps)
		return;

	/*
	 * All NCP's are idle ... close the &m.m_stat_act.st_ah, args);
	 * bundle ... unless we have static links. 
	 */
	al = ab->ah_bundle.ab_links;
	while (al) {
		MUTEX_LOCK(&al->ah_mutex);
		if (al->ah_link.al_type == LNK_STATIC) {
			MUTEX_UNLOCK(&al->ah_mutex);
			return;
		}
		MUTEX_UNLOCK(&al->ah_mutex);
		al = al->ah_link.al_next;
	}

	ppplog(MSG_INFO_LOW, ab, "Inactivity has cause the bundle to close\n");
	bundle_close(ab, ABR_IDLE);
}

/*
 * Called when an idle NCP has detected it's again active 
 */
void
bundle_ncp_active(act_hdr_t *ab)
{
	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));

	ab->ah_bundle.ab_idle_ncps--;
}

/*
 * Called from LCP Finish when a link is to be removed from a bundle
 *
 * The link is closed and removed from the parent bundles chain
 * of open bundles.
 */
bundle_link_finished(struct act_hdr_s *al)
{
	act_hdr_t *ab = al->ah_link.al_bundle;
	act_hdr_t *curr, *prev = NULL;

	ASSERT(MUTEX_LOCKED(&al->ah_mutex));
	ASSERT(al->ah_type == DEF_LINK);
	ASSERT(ab ? MUTEX_LOCKED(&ab->ah_mutex) : 1);
	ASSERT(al->ah_inuse == INUSE);/*DEBUG*/
	ASSERT(ab ? ab->ah_inuse == INUSE : 1);/*DEBUG*/

	if (!(al->ah_link.al_flags & ALF_INUSE))
		return;

	al->ah_link.al_flags |= ALF_HANGUP;

	ppplog(MSG_DEBUG, al,
	       "Remove link index %d from bundle\n", al->ah_index);

	/* Undial to ensure no more traffic */

	link_undial(al);

	if (ab) {
		ppplog(MSG_DEBUG, al, "Removing link from bundle\n");

		/* Remove from the bundles list */

		curr = ab->ah_bundle.ab_links;
		while (curr != al) {
			ASSERT(curr->ah_inuse == INUSE);/*DEBUG*/
			prev = curr;
			curr = curr->ah_link.al_next;
		}

		if (prev)
			prev->ah_link.al_next = al->ah_link.al_next;
		else
			ab->ah_bundle.ab_links = al->ah_link.al_next;

		al->ah_link.al_bundle = NULL;
		ab->ah_bundle.ab_numlinks--;

		if (al->ah_link.al_flags & ALF_OPEN)
			ab->ah_bundle.ab_open_links--;
	}

	al->ah_link.al_flags = 0;

	ppplog(MSG_DEBUG, al, "bundle_link_finished: Done.\n");
}

/*
 * Called from LCP to indicate that the protocol has gone down.
 * Take down all other protocols on the link.
 * If this is the last link in a bundle, indicate this event to the higher
 * layer protocols
 */
bundle_link_down(act_hdr_t *al)
{
	act_hdr_t *ab = al->ah_link.al_bundle;
	proto_hdr_t *ph = al->ah_protos;

	ASSERT(MUTEX_LOCKED(&al->ah_mutex));
	ASSERT(al->ah_inuse == INUSE);/*DEBUG*/

	ppplog(MSG_DEBUG, al, "bundle_link_down: Link going down\n");

	if (ab && (al->ah_link.al_flags & ALF_OPEN))
		ab->ah_bundle.ab_open_links--;

	al->ah_link.al_flags &= ~ALF_OPEN;

	while (ph) {
		ASSERT(ph->ph_inuse == INUSE);
		if ((ph->ph_psmtab->pt_flags & (PT_FSM|PT_LCP)) == PT_FSM) {

			ppplog(MSG_DEBUG, al,
			       "bundle_link_down: Signal down for %s\n",
			       ph->ph_cfg->ch_id);

			if (ph->ph_state.ps_state != INITIAL && 
			    ph->ph_state.ps_state != STARTING)
				fsm_state(ph, DOWN, 0);

			fsm_state(ph, CLOSE, 0);

		} else  if (!(ph->ph_psmtab->pt_flags & (PT_FSM))) {
			/* Call the non-FSM terminate routines */
			if (ph->ph_psmtab->pt_term)
				(*ph->ph_psmtab->pt_term)(ph);
		}
		ph = ph->ph_next;
	}

	if (!ab)
		return;

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	/*
	 * When we reduce a bundle to one link, then our bundles
	 * MTU needs to revert the the links MTU (this stops
	 * Network Layer protocols from stending us fragments
	 * which we would later fragment to the links MTU
	 */
	if (ab->ah_bundle.ab_open_links == 1) {
		/* Bundles MTU is now the last links peer mru */
		ab->ah_bundle.ab_mtu =
			ab->ah_bundle.ab_links->ah_link.al_peer_mru;

		/* Tell the kernel */
		cd_d2k_cfg_bundle(ab);
	}

	if (ab->ah_bundle.ab_open_links > 0)
		return;

	/*
	 * If the bundle is DEAD, then no work to do
	 */
	if (ab->ah_phase == PHASE_DEAD)
		return;

	ppplog(MSG_DEBUG, ab,
	       "Last link in bundle .. inform bundle protos DOWN\n");

	ph = ab->ah_protos;
	while (ph) {
		ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/
		if ((ph->ph_psmtab->pt_flags & (PT_FSM | PT_LCP)) == PT_FSM) {
			ppplog(MSG_DEBUG, ab, 
			       "bundle_link_down: Signal down for %s\n",
			       ph->ph_cfg->ch_id);
			if (ph->ph_state.ps_state != INITIAL && 
			    ph->ph_state.ps_state != STARTING)
				fsm_state(ph, DOWN, 0);
			fsm_state(ph, CLOSE, 0);
		}
		ph = ph->ph_next;
	}
	ppp_phase(ab, PHASE_DEAD);
}

/*
 * Configure interfaces for a bundle
 */
bundle_cfg_inf(act_hdr_t *ab)
{
	proto_hdr_t *ph;
	int ret;

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	ppplog(MSG_DEBUG, ab, "Configure interfaces for bundle %s\n",
	       ab->ah_cfg->ch_id);

	/*
	 * For each NCP call it's interface configuration routine
	 */
	ph = ab->ah_protos;
	while (ph) {
		ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/
		if (ph->ph_psmtab->pt_flags & PT_NCP) {
			ph->ph_flags |= PHF_AUTO;
			ret = (*ph->ph_psmtab->pt_cfg)(ph);
			if (ret) {
				ppplog(MSG_WARN, ab,
	       "Failed to configure interface for %s (%s) - error %d\n",
				       ph->ph_cfg->ch_id,
				       ph->ph_psmtab->pt_desc, ret);
			}
		}
		ph = ph->ph_next;
	}
}

/*
 * Deconfigure interfaces for a bundle
 */
bundle_decfg_inf(act_hdr_t *ab)
{
	proto_hdr_t *ph;
	int ret;

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	ppplog(MSG_DEBUG, ab, "De-Configure interfaces for bundle %s\n",
	       ab->ah_cfg->ch_id);

	/*
	 * For each NCP call it's interface configuration routine
	 */
	ph = ab->ah_protos;
	while (ph) {
		ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/
		if (ph->ph_psmtab->pt_flags & PT_NCP) {
			ret = (*ph->ph_psmtab->pt_decfg)(ph);
			if (ret) {
				ppplog(MSG_WARN, ab,
	       "Failed to de-configure interface for %s (%s) - error %d\n",
				       ph->ph_cfg->ch_id,
				       ph->ph_psmtab->pt_desc, ret);
			}
		}
		ph = ph->ph_next;
	}
}

/*
 * Take a bundle down ... remove all active links
 * This is called from ulr_detach ... and is an 
 * administrative close of a bundle
 */
bundle_close(struct act_hdr_s *ab, int b_reason)
{
	act_hdr_t *al, *alnext;

	ppplog(MSG_DEBUG, ab,
	       "bundle_close: Bundle %s, reason %d\n",
	       ab->ah_cfg->ch_id, b_reason);

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	ab->ah_bundle.ab_reason |= b_reason;

	al = ab->ah_bundle.ab_links;
	while (al) {

		MUTEX_LOCK(&al->ah_mutex);

		ASSERT(al->ah_inuse == INUSE);/*DEBUG*/

		alnext = al->ah_link.al_next;
		ASSERT(alnext != ab->ah_bundle.ab_links);

		ppplog(MSG_DEBUG, ab,
		       "bundle_close: Terminate link %s\n", al->ah_cfg->ch_id);

		/* Close all protocols */

		fsm_state(al->ah_link.al_lcp, CLOSE, 0);

		if (al->ah_link.al_type == LNK_STATIC &&
		    al->ah_link.al_lcp->ph_state.ps_state == CLOSED) {
			ppplog(MSG_DEBUG, al, "STATIC link CLOSED\n");
			bundle_link_finished(al);
		}

		MUTEX_UNLOCK(&al->ah_mutex);
		al = alnext;
	}
	ab->ah_bundle.ab_idle_ncps = 0;
	return 0;
}

/*
 * Take a bundle down.. FAST ... remove all active links
 * This is called from ulr_kill ... and is a DIRTY administrative
 * close of a bundle
 */
bundle_kill(struct act_hdr_s *ab)
{
	act_hdr_t *al, *alnext;

	ppplog(MSG_DEBUG, ab,
	       "bundle_kill: Bundle %s\n", ab->ah_cfg->ch_id);

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	al = ab->ah_bundle.ab_links;
	while (al) {

		MUTEX_LOCK(&al->ah_mutex);

		ASSERT(al->ah_inuse == INUSE);/*DEBUG*/

		alnext = al->ah_link.al_next;
		ASSERT(alnext != ab->ah_bundle.ab_links);

		ppplog(MSG_DEBUG, ab,
		       "bundle_kill: Terminate link %s\n", al->ah_cfg->ch_id);

		/* Close all protocols .. YUK */

		bundle_link_finished(al);

		MUTEX_UNLOCK(&al->ah_mutex);
		al = alnext;
	}
	ab->ah_bundle.ab_reason |= ABR_USERCLOSE;
	return 0;
}

int
bundle_number(act_hdr_t *ah)
{
	ASSERT(ah->ah_type == DEF_BUNDLE);
	ASSERT(ah->ah_inuse == INUSE);/*DEBUG*/
	return(ah->ah_index);
}
