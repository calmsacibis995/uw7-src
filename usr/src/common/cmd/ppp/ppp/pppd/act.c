#ident	"@(#)act.c	1.10"
#ident "$Header$"

#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <synch.h>
#include <thread.h>
#include <errno.h>

#include "pathnames.h"
#include "psm.h"
#include "ppp_type.h"
#include "ppp_proto.h"
#include "ppp_cfg.h"

#include "act.h"

struct psm_tab_s *psm_getpt(char *proto);

/*
 * This module contains the Active Bundle/Link management routines
 *
 * When a configuration is acivated, bundle_create is called to 
 * construct the defined bundle config. An element for the bundle
 * and each defined link is placed on the list of acive elements.
 *
 * The list of actvie link/bundles is stored on a link list, the head
 * of the list being act[type]. The list is protected by a rwlock, this
 * lock must be held in write mode if any of the next pointers
 * are to be modified (i.e. when elements are added or removed),
 * the lock must be held in at least read mode if the next pointers
 * are being traversed.
 */
rwlock_t act_rwlock;

struct act_hdr_s *act[DEF_MAX];

extern struct cfg_hdr *global[];

act_init()
{
	int i;

	rwlock_init(&act_rwlock, USYNC_PROCESS, NULL);

	for (i = 0; i < DEF_MAX; i++) {
		act[i] = NULL;
		global[i] = NULL;
	}
}


/*
 * Called when ppp is being stopped
 */
act_unload()
{
	act_hdr_t *ah, *prev;
	int ret;

	RW_RDLOCK(&act_rwlock);

	ah = act[DEF_BUNDLE];
	while (ah) {
		MUTEX_LOCK(&ah->ah_mutex);

		ASSERT(ah->ah_inuse == INUSE); /*DEBUG*/

		bundle_kill(ah);

		MUTEX_UNLOCK(&ah->ah_mutex);
		ah = ah->ah_next;
		ASSERT(ah != act[DEF_BUNDLE]);
	}
	RW_UNLOCK(&act_rwlock);

	/*
	 * Give links a chance to close, this isn't absolutely 
	 * neccasary but the kernel will want to send messages
	 * to the daemon indicating closes etc.
	 */
	nap(300);

	/* Deactivate all */
 retry:
	RW_WRLOCK(&act_rwlock);
	prev = NULL;
	ah = act[DEF_BUNDLE];
	while (ah) {
		MUTEX_LOCK(&ah->ah_mutex);

		ASSERT(ah->ah_inuse == INUSE); /*DEBUG*/

		ret = act_deactivate(ah, prev);
		switch (ret) {
		case 0:
			/* No need to unlock ah ... it's not there ! */
			RW_UNLOCK(&act_rwlock);
			goto retry;

		case EAGAIN:
			MUTEX_UNLOCK(&ah->ah_mutex);
			RW_UNLOCK(&act_rwlock);
			nap(500);
			goto retry;
		default:
			MUTEX_UNLOCK(&ah->ah_mutex);
			break;
		}

		prev = ah;
		ah = ah->ah_next;
		ASSERT(ah != act[DEF_BUNDLE]);
	}
	RW_UNLOCK(&act_rwlock);
}

/*
 * Create a protocol entry, link it into an link/bundles list.
 */
int
act_proto_alloc(char *protoId, act_hdr_t *ah)
{

	int ret;
	proto_hdr_t *ph;
	char *protocol;
	struct cfg_proto *cfg;

	ASSERT(ah->ah_inuse == INUSE);/*DEBUG*/

	ph = (proto_hdr_t *)malloc(sizeof(proto_hdr_t));
	if (!ph) {
		ppplog(MSG_WARN, ah, "act_proto_alloc: NO MEMORY !\n");
		return ENOMEM;
	}

	ph->ph_inuse = INUSE;/*DEBUG*/

	ret = ucfg_findid(protoId, DEF_PROTO, (cfg_hdr_t *)&cfg);
	if (ret) {
		ppplog(MSG_ERROR, ah,
		       "act_proto_alloc: Protocol definition not found '%s'\n",
		       protoId);
		free(ph);
		return ret;
	}
	ph->ph_cfg = (struct cfg_hdr *)cfg;

	protocol = ucfg_str(&cfg->pr_ch, cfg->pr_name);

	/* Load the protocol support and setup protocol specific stuff */

	ph->ph_psmtab = psm_getpt(protocol);
	if (!ph->ph_psmtab) {
		ppplog(MSG_ERROR, ah,
"act_proto_alloc: Failed to get support module for protocol definiton '%s'\n",
		       protoId);
		ret = EPROTONOSUPPORT;
		goto error_exit;
	}

	/* Check this is a valid protocol for this link/bundle */

	switch (ah->ah_type) {
	case DEF_BUNDLE:
		if (!(ph->ph_psmtab->pt_flags & PT_BUNDLE)) {
			ppplog(MSG_ERROR, ah,
			       "Protocol %s not for use on Bundles\n",
			       ph->ph_psmtab->pt_desc);
			ret = EINVAL;
			goto error_exit;
		}
		break;
	case DEF_LINK:
 		if (!(ph->ph_psmtab->pt_flags & PT_LINK)) {
			ppplog(MSG_ERROR, ah,
			       "Protocol %s not for use on Links\n",
			       ph->ph_psmtab->pt_desc);
			ret = EINVAL;
			goto error_exit;
		}
		break;
	}

	/* Insert this proto entry into the bundle headers proto list */
	ph->ph_parent = ah;

	ret = (*ph->ph_psmtab->pt_alloc)(ph);
	if (ret)
		goto error_exit;

	ph->ph_next = ah->ah_protos;
	ah->ah_protos = ph;

	ph->ph_flags = 0;
	ph->ph_rtbuf = NULL;
	ph->ph_rx_stats = NULL;
	ph->ph_tx_stats = NULL;

	/* Inc the config ref cnt */

	ph->ph_cfg->ch_refcnt++;
	ucfg_release();

	/* Call the protocol init routine */

	ret = (*ph->ph_psmtab->pt_init)(ah, ph);
	if (ret) {
		(*ph->ph_psmtab->pt_free)(ph);
		ph->ph_inuse = 0; /*DEBUG*/

		/* Unlink from parent */
		ah->ah_protos = ph->ph_next;
		free(ph);
	}

	return 0;

error_exit:
	ucfg_release();
	ph->ph_inuse = 0; /*DEBUG*/
	free(ph);
	return ret;
}

/*
 * Free of a protocol entry and associated data
 * (doesn't take the proto_hdr off the parents list).
 */
int
act_proto_free(proto_hdr_t *ph)
{
	extern rwlock_t ucfg_rwlock;
	
	ASSERT(ph);

	/* Free any private resources */
	RW_WRLOCK(&ucfg_rwlock);

	ASSERT(ph->ph_cfg->ch_refcnt > 0);

	ph->ph_cfg->ch_refcnt--;

	if (ph->ph_rtbuf) {
		db_free(ph->ph_rtbuf);
		ph->ph_rtbuf = NULL;
	}

	if (ph->ph_rx_stats)
		free(ph->ph_rx_stats);
	if (ph->ph_tx_stats)
		free(ph->ph_tx_stats);

	(*ph->ph_psmtab->pt_free)(ph);

/* DEBUG */
	ph->ph_parent = NULL;
	ph->ph_inuse = 0;/*DEBUG*/
	free(ph);

	RW_UNLOCK(&ucfg_rwlock);
}

/*
 * Initialise the active bundle header
 */	
act_bundle_init(struct act_hdr_s *ab)
{
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;

	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/

	ab->ah_debug = cb->bn_debug;
	ab->ah_bundle.ab_flags = 0;
	ab->ah_bundle.ab_addindex = 0;
	ab->ah_bundle.ab_dropindex = 0;
	ab->ah_bundle.ab_reason = 0;
	ab->ah_bundle.ab_lastphonenum = NULL;

	ab->ah_bundle.ab_local_mrru = cb->bn_mrru;
	ab->ah_bundle.ab_local_ssn = cb->bn_ssn;
	ab->ah_bundle.ab_local_ed = cb->bn_ed;
	ab->ah_bundle.ab_peer_mrru = 0;
	ab->ah_bundle.ab_peer_ssn = 0;
	ab->ah_bundle.ab_peer_ed = 0;
	ab->ah_bundle.ab_local_ed_class = 0;
	ab->ah_bundle.ab_local_ed_len = 0;
	ab->ah_bundle.ab_numlinks = 0;
	ab->ah_bundle.ab_mtu = 0;

/* WHAT ABOUT RE-INIT */
	ab->ah_bundle.ab_open_links = 0;
	ab->ah_bundle.ab_open_ncps = 0;
	ab->ah_bundle.ab_idle_ncps = 0;
	ab->ah_bundle.ab_links = NULL;
}

/*
 * This function is called when a bundle is about to
 * have it's first link attached. We must update the bundles flags.
 */
act_bundle_up(act_hdr_t *ab, int flags)
{
	struct cfg_out *co;
	struct cfg_inc *ci;
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;

	ASSERT(ab->ah_inuse == INUSE);/*DEBUG*/
	ASSERT(!ab->ah_bundle.ab_links);

	ppplog(MSG_DEBUG, ab,
		       "act_bundle_up: Bundles max %d, min %d\n",
		       cb->bn_maxlinks, cb->bn_minlinks);

	ab->ah_bundle.ab_flags &= ~(ABF_CALLED_IN | ABF_CALLED_OUT);
	ab->ah_bundle.ab_flags |= flags;

	ppplog(MSG_DEBUG, ab, "act_bundle_up: flags = %x\n", flags);	
}

/*
 * Find a free bundle index ... starting from zero
 */
STATIC int
bundle_index_alloc()
{
	act_hdr_t *ah;
	int retry;
	int index = 0;

	RW_RDLOCK(&act_rwlock);
	
	do {
		retry = 0;
		ah = act[DEF_BUNDLE];
		while (ah) {
			MUTEX_LOCK(&ah->ah_mutex);
		
			ASSERT(ah->ah_inuse == INUSE);/*DEBUG*/

			if (ah->ah_index == index) {
				retry = 1;
				index++;
				MUTEX_UNLOCK(&ah->ah_mutex);
				break;
			}

			MUTEX_UNLOCK(&ah->ah_mutex);
			ah = ah->ah_next;
		}
	} while(retry);

	RW_UNLOCK(&act_rwlock);

	return index;
}

/*
 * Activate a bundle definition
 */
int
act_bundle_create(char *bundleId)
{
	int ret;
	act_hdr_t *ab;
	struct cfg_bundle *cb;
	char buf[MAXID], *p;
	proto_hdr_t *ph;
	extern rwlock_t ucfg_rwlock;

	/* Check not already initialised */
	ret = act_find_id(bundleId, DEF_BUNDLE, &ab);
	if (!ret) {
		ppplog(MSG_DEBUG, ab,
		       "act_bundle_create: Bundle %s already initialised !\n",
		       bundleId);
		act_release(ab);
		return 0;
	}

	/* Allocate and initialise the new bundle structure */

	ab = (act_hdr_t *)malloc(sizeof(act_hdr_t));
	if (!ab) {
		ppplog(MSG_WARN, ab, "act_bundle_create: NO MEMORY\n");
		return ENOMEM;
	}
	memset(ab, 0, sizeof(act_hdr_t));

	ab->ah_inuse = INUSE;

	ab->ah_next = NULL;
	ab->ah_refcnt = ATOMIC_INT_ALLOC();
	if (!ab->ah_refcnt) {
		ppplog(MSG_WARN, ab, "act_bundle_create: NO MEMORY\n");
		ab->ah_inuse = 0; /*DEBUG*/
		free(ab);
		return ENOMEM;
	}

	mutex_init(&ab->ah_mutex, USYNC_PROCESS, NULL);

	ab->ah_type = DEF_BUNDLE;
	ab->ah_phase = PHASE_DEAD;
      	ab->ah_protos = NULL;
	ab->ah_cfg = NULL;
	ab->ah_index = bundle_index_alloc();

	ab->ah_bundle.ab_links = NULL;

	/* Link in the config definition */
	ret = ucfg_findid(bundleId, DEF_BUNDLE, &ab->ah_cfg);
	if (ret) {
		act_bundle_free(ab);
		ppplog(MSG_DEBUG, "Bundle '%s' definition not found !\n",
		       bundleId);
		return ENOENT;
	}

	ASSERT(ab->ah_cfg->ch_refcnt == 0);

	/* Increment the ref count */

	ab->ah_cfg->ch_refcnt++;

	ucfg_release();

	/* Now construct the bundle */

	cb = (struct cfg_bundle *)ab->ah_cfg;

	if (!(cb->bn_type & (BT_OUT | BT_IN))) {
		/* Disabled */
		ppplog(MSG_DEBUG, ab,
		       "act_bundle_create: Bundle disabled\n");
		act_bundle_free(ab);
		return 0;
	}

	/* Initialise from the Config */

	act_bundle_init(ab);

	/* We get bundle_lcp for free on all MP bundles */

	ret = act_proto_alloc("\tbundle_lcp", ab);
	if (ret) {
		ppplog(MSG_DEBUG, ab,
		       "act_bundle_create: Error - exit %d\n", ret);
		act_bundle_free(ab);
		return ret;
	}

	/*
	 * Check if incoming, we have at least one auth option set
	 * caller id, authid or login
	 */
	if ((cb->bn_type & BT_IN) && !(*ucfg_str(&cb->bn_ch, cb->bn_uid) ||
	      *ucfg_str(&cb->bn_ch, cb->bn_authid) ||
	      *ucfg_str(&cb->bn_ch, cb->bn_cid))) {
		act_bundle_free(ab);
		ppplog(MSG_WARN, ab,
	       "Incoming bundles must have login, callerid or authid set\n");
		return ECONNREFUSED; /* Connections would be refused ;-> */
	}

	/* Create the list of protocols */

	p = ucfg_str(&cb->bn_ch, cb->bn_protos);

	ppplog(MSG_DEBUG, ab, "act_bundle_create: Protos = %s\n", p);

	while (*p) {
		p = ucfg_get_element(p, buf);
		ppplog(MSG_DEBUG, ab,
		       "act_bundle_create: Add proto %s\n", buf);

		ret = act_proto_alloc(buf, ab);
		if (ret) {
			ppplog(MSG_DEBUG, ab,
			       "act_bundle_create: Error - exit %d\n", ret);
			act_bundle_free(ab);
			return ret;
		}
	}

	/* Insert the bundle definition to the list of active bundles */

	RW_WRLOCK(&act_rwlock);
	MUTEX_LOCK(&ab->ah_mutex);

	ab->ah_next = act[DEF_BUNDLE];
	act[DEF_BUNDLE] = ab;

	ATOMIC_INT_INCR(ab->ah_refcnt);
	MUTEX_UNLOCK(&ab->ah_mutex);
	RW_UNLOCK(&act_rwlock);

	MUTEX_LOCK(&ab->ah_mutex);
	ATOMIC_INT_DECR(ab->ah_refcnt);

	/*
	 * If we have an outgoing definition, then check if we need
	 * to configure it's interface now ... for automatic bringup
	 */
	if ((cb->bn_type & BT_OUT) && cb->bn_bringup == OUT_AUTOMATIC)
		bundle_cfg_inf(ab);

	MUTEX_UNLOCK(&ab->ah_mutex);

	/* If any of our links are statics, bring them up */

	bundle_statics_up(bundleId);

	ppplog(MSG_DEBUG, ab, "act_bundle_create: Success\n");
	return 0;
}

/*
 * The active def list lock must be held when calling this function
 */
act_bundle_free(act_hdr_t *ab)
{
	proto_hdr_t *ph, *next;
	extern rwlock_t ucfg_rwlock;

	ASSERT(ab->ah_inuse == INUSE); /* DEBUG*/

	/* Free any allocated protos */
	ph = ab->ah_protos;
	while (ph) {

		ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/

		next = ph->ph_next;
		act_proto_free(ph);
		ph = next;
		ASSERT(ph != ab->ah_protos);
	}

	/* Decrement the config ref count */

	RW_WRLOCK(&ucfg_rwlock);

	if (ab->ah_cfg) {
		ASSERT(ab->ah_cfg->ch_refcnt > 0);
		ab->ah_cfg->ch_refcnt--;
	}

	RW_UNLOCK(&ucfg_rwlock);

	/* free the bundle */
	ab->ah_inuse = 0; /*DEBUG*/
	free(ab);
}
/*
 * Initialisae an active link header
 */
act_link_init(struct act_hdr_s *al)
{
	struct cfg_link *cl = (struct cfg_link *)al->ah_cfg;
	proto_hdr_t *ph;

	ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

	al->ah_link.al_flags = 0;
	al->ah_link.al_fd = -1;
	al->ah_index = 0;
	al->ah_debug = cl->ln_debug;
	al->ah_link.al_reason = 0;
	al->ah_link.al_uid = NULL;
	al->ah_link.al_cid = NULL;
	al->ah_link.al_type = cl->ln_type;
	al->ah_link.al_bandwidth = cl->ln_bandwidth;
	al->ah_link.al_next = NULL;
	al->ah_link.al_bundle = NULL;
	al->ah_link.al_peer_auth_name = NULL;
	al->ah_link.al_peer_ed_class = 0;
	al->ah_link.al_peer_ed_len = 0;

	al->ah_link.al_local_magic = 0;
	al->ah_link.al_local_opts = 0;
	al->ah_link.al_local_mru = 0;

	al->ah_link.al_peer_magic = 0;
	al->ah_link.al_peer_opts = 0;
	al->ah_link.al_peer_mru = 0;

	ph = al->ah_protos;
	while (ph) {
		ASSERT(ph->ph_inuse == INUSE); /*DEBUG*/

		(*ph->ph_psmtab->pt_init)(al, ph);
		ph = ph->ph_next;
	}
}

/*
 * Allocate a link discriminator, assume the act_rwlock is held ... 
 * because we will traverse all the links.
 */
STATIC ushort_t
act_ld_alloc()
{
	static ushort_t ld = 0;	/* The next value to allocate */
	act_hdr_t *al;

 retry:
	ld++;

	/* Check ld isn't being used */
	al = act[DEF_LINK];

	while (al) {
		MUTEX_LOCK(&al->ah_mutex);
		if (al->ah_link.al_local_ld == ld) {
			MUTEX_UNLOCK(&al->ah_mutex);
			goto retry;
		}

		MUTEX_UNLOCK(&al->ah_mutex);
		al = al->ah_next;
	}
	return ld;
}

/*
 * Activate a link
 */
int
act_link_create(char *linkId)
{
	act_hdr_t *al;
	int ret;
	struct cfg_link *cl;
	char buf[MAXID], *p;
	proto_hdr_t *ph;

	/* Check if already active */

	ret = act_find_id(linkId, DEF_LINK, &al);
	if (!ret) {
		ppplog(MSG_DEBUG, al,
		       "act_link_create: Link %s already initialised !\n",
		       linkId);
		act_release(al);
		return 0;
	}

	/* Allocate and initialise the new bundle structure */

	al = (act_hdr_t *)malloc(sizeof(act_hdr_t));
	if (!al) {
		ppplog(MSG_WARN, al, "act_link_create: NO MEMORY\n");
		return ENOMEM;
	}
	memset(al, 0, sizeof(act_hdr_t));

	al->ah_inuse = INUSE;/* DEBUG*/

	al->ah_next = NULL;
	al->ah_refcnt = ATOMIC_INT_ALLOC();
	if (!al->ah_refcnt) {
		ppplog(MSG_WARN, al, "act_link_create: NO MEMORY\n");
		al->ah_inuse = 0;
		free(al);
		return ENOMEM;
	}
	mutex_init(&al->ah_mutex, USYNC_PROCESS, NULL);
	al->ah_type = DEF_LINK;
	al->ah_phase = PHASE_DEAD;
      	al->ah_protos = NULL;
	al->ah_cfg = NULL;
	al->ah_link.al_lcp = NULL;

	al->ah_link.al_uid = NULL;
	al->ah_link.al_cid = NULL;
	al->ah_link.al_bundle = NULL;
	al->ah_link.al_peer_auth_name = NULL;
	al->ah_link.al_local_ld = act_ld_alloc();

	/* Link in the config definition */
	ret = ucfg_findid(linkId, DEF_LINK, &al->ah_cfg);
	if (ret) {
		ppplog(MSG_DEBUG, al, "act_link_create: Config not found %s\n",
		       linkId);
		act_link_free(al);
		return ENOENT;
	}

	ASSERT(al->ah_cfg->ch_refcnt == 0);

	/* Increment the ref count */
	al->ah_cfg->ch_refcnt++;

	ucfg_release();

	/* Now construct the link */

	cl = (struct cfg_link *)al->ah_cfg;

	act_link_init(al);

	/* Create the list of protocols */

	p = ucfg_str(&cl->ln_ch, cl->ln_protocols);

	ppplog(MSG_DEBUG, al, "act_link_create: Protos = %s\n", p);

	while (*p) {
		p = ucfg_get_element(p, buf);
		ppplog(MSG_DEBUG, al, "act_link_create: Add proto %s\n", buf);

		ret = act_proto_alloc(buf, al);
		if (ret)
			goto error_exit;
	}

	/* Ensure LCP has been defined */

	if (al->ah_link.al_lcp == NULL) {
		ppplog(MSG_ERROR, al,
		       "act_link_create: LCP must be defined for links.\n");
		ret = EPROTO;
		goto error_exit;
	}

	/* We get pap and chap for free on all links */

	ret = act_proto_alloc("pap", al);
	if (ret)
		goto error_exit;

	ret = act_proto_alloc("chap", al);
	if (ret)
		goto error_exit;

	RW_WRLOCK(&act_rwlock);

	/* Insert the bundle definition to the list of active bundles */

	al->ah_next = act[DEF_LINK];
	act[DEF_LINK] = al;

	/* Free the active definition lock */
	RW_UNLOCK(&act_rwlock);
	return 0;

error_exit:
	ppplog(MSG_DEBUG, al, "act_link_create: Error exit %d\n", ret);
	act_link_free(al);
	return ret;
}

/*
 * Free a links resources and it's elements resources
 */
int
act_link_free(act_hdr_t *al)
{
	proto_hdr_t *ph, *next;
	extern rwlock_t ucfg_rwlock;

	ASSERT(al->ah_inuse == INUSE);
	/* Free any allocated protos */
	ph = al->ah_protos;
	while (ph) {
		ASSERT(ph->ph_inuse == INUSE);
		next = ph->ph_next;
		act_proto_free(ph);
		ph = next;
		ASSERT(ph != al->ah_protos);
	}
/* DEBUG */
al->ah_protos = NULL;

	/* Decrement the config ref count */

	RW_WRLOCK(&ucfg_rwlock);

	if (al->ah_cfg) {
		ASSERT(al->ah_cfg->ch_refcnt > 0);
		al->ah_cfg->ch_refcnt--;
	}
	
/* DEBUG */
al->ah_cfg = NULL;

	RW_UNLOCK(&ucfg_rwlock);

	/* Free the active link header */
	if (al->ah_link.al_uid)
		free(al->ah_link.al_uid);

	if (al->ah_link.al_cid)
		free(al->ah_link.al_cid);

	if (al->ah_link.al_peer_auth_name)
		free(al->ah_link.al_peer_auth_name);

	al->ah_inuse = 0;
	free(al);
}

/*
 * Given a link/bundle id ... remove it from the active list
 * and return a pointer to it
 */
int
act_destroy(char *id, int type)
{
	act_hdr_t *ah, *prev;
	proto_hdr_t *ph, *phnext;
	extern rwlock_t ucfg_rwlock;
	int ret;

retry:
	prev = NULL;
	RW_WRLOCK(&act_rwlock);
	
	ah = act[type];
	while (ah) {
		MUTEX_LOCK(&ah->ah_mutex);
		
		ASSERT(ah->ah_inuse == INUSE); /*DEBUG*/

		if (strcmp(ah->ah_cfg->ch_id , id) == 0)
			break;

		MUTEX_UNLOCK(&ah->ah_mutex);
		prev = ah;
		ah = ah->ah_next;
		ASSERT(ah != act[type]);
	}

	if (!ah) {
		RW_UNLOCK(&act_rwlock);
		return ENOENT;
	}

	ret = act_deactivate(ah, prev);
	switch (ret) {
	case 0:
		/* No need to unlock ah ... it's not there ! */
		break;
	case EAGAIN:
		nap(500);
		MUTEX_UNLOCK(&ah->ah_mutex);
		RW_UNLOCK(&act_rwlock);
		goto retry;
	default:
		MUTEX_UNLOCK(&ah->ah_mutex);
		break;
	}

	RW_UNLOCK(&act_rwlock);
	return ret;
}


int
act_deactivate(act_hdr_t *ah, act_hdr_t *prev)
{
	int ret;

	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));

	/* Check that it is okay to remove this element */

	switch(ah->ah_type) {
	case DEF_LINK:
		if (ah->ah_link.al_bundle) {
			ASSERT(ah->ah_link.al_bundle->ah_inuse == INUSE); /*DEBUG*/
			ppplog(MSG_DEBUG, ah,
		       "act_deactivate: Cannot destroy link .. in a bundle\n");

			return EBUSY;
		}
		break;
	case DEF_BUNDLE:
		if (ah->ah_bundle.ab_links) {
			ASSERT(ah->ah_bundle.ab_links->ah_inuse == INUSE); /*DEBUG*/
			ppplog(MSG_DEBUG, ah,
		       "act_deactivate: Bundle is busy .. active links\n");
			return EBUSY;
		}
		break;
	}

	/* Check that we are the only ones acessing the element */

	if (ATOMIC_INT_READ(ah->ah_refcnt) > 0)
		return EAGAIN;

	/*
	 * If we have a bundle with an automatic outgoing definition
	 * then we need to tear down any interfaces associated with it.
	 */
	if (ah->ah_type == DEF_BUNDLE) {
		struct cfg_bundle *bn = (struct cfg_bundle *)ah->ah_cfg;

		if ((bn->bn_type & BT_OUT)
		    && bn->bn_bringup == OUT_AUTOMATIC) {
			ppplog(MSG_DEBUG, ah,
			       "act_deactivate: Automatic bringup\n");
			bundle_decfg_inf(ah);
		}
	}

	/* Remove the element from the active list */

	if (prev) {
		MUTEX_LOCK(&prev->ah_mutex);
		prev->ah_next = ah->ah_next;
		MUTEX_UNLOCK(&prev->ah_mutex);
	} else
		act[ah->ah_type] = ah->ah_next;

	MUTEX_UNLOCK(&ah->ah_mutex);

	/* Free resources */

	ATOMIC_INT_DEALLOC(ah->ah_refcnt);

	switch (ah->ah_type) {
	case DEF_LINK:
		act_link_free(ah);
		break;
	case DEF_BUNDLE:
		act_bundle_free(ah);
		break;
	}
	return 0;
}

/*
 * Get Read style access to the definition
 */
int
act_find_id_rd(char *Id, int type, act_hdr_t **ahp)
{
	act_hdr_t *ah;

	RW_RDLOCK(&act_rwlock);

	ah = act[type];
	while (ah) {

		MUTEX_LOCK(&ah->ah_mutex);

		ASSERT(ah->ah_inuse == INUSE); /*DEBUG*/

		if (strcmp(ah->ah_cfg->ch_id, Id) == 0)
				break;

		MUTEX_UNLOCK(&ah->ah_mutex);
		ah = ah->ah_next;
		ASSERT(ah != act[type]);
	}

	if (!ah) {
		RW_UNLOCK(&act_rwlock);
		return ENOENT;
	}

	ATOMIC_INT_INCR(ah->ah_refcnt);
	MUTEX_UNLOCK(&ah->ah_mutex);
	RW_UNLOCK(&act_rwlock);

	MUTEX_LOCK(&ah->ah_mutex);
	ATOMIC_INT_DECR(ah->ah_refcnt);

	*ahp = ah;
	return 0;
}

/*
 * act_find_id()
 *
 * Find the link/bundle requested .. by ID
 */
int
act_find_id(char *Id, int type, act_hdr_t **ahp)
{
	act_hdr_t *ah;

	/*ppplog(MSG_DEBUG, 0, "act_find_id: getting act_rwlock\n");*/
retry:
	RW_RDLOCK(&act_rwlock);

	ah = act[type];
	while (ah) {
		MUTEX_LOCK(&ah->ah_mutex);

		ASSERT(ah->ah_inuse == INUSE); /*DEBUG*/

		if (strcmp(ah->ah_cfg->ch_id, Id) == 0)
				break;

		MUTEX_UNLOCK(&ah->ah_mutex);
		ah = ah->ah_next;
		ASSERT(ah != act[type]);
	}

	if (!ah) {
		RW_UNLOCK(&act_rwlock);
		return ENOENT;
	}
	
	ATOMIC_INT_INCR(ah->ah_refcnt);
	MUTEX_UNLOCK(&ah->ah_mutex);
	RW_UNLOCK(&act_rwlock);

	if (ATOMIC_INT_READ(ah->ah_refcnt) > 1) {
		ATOMIC_INT_DECR(ah->ah_refcnt);
		nap(100);
		goto retry;
	}

	MUTEX_LOCK(&ah->ah_mutex);
	ATOMIC_INT_DECR(ah->ah_refcnt);

	*ahp = ah;
	return 0;
}

/*
 * Release the active elements lock
 */
int
act_release(act_hdr_t *ah)
{
	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));
	MUTEX_UNLOCK(&ah->ah_mutex);
}

/*
 * Given a bundle index, find the bundle
 */
int
act_find_bundle(int index, act_hdr_t **ah)
{
	act_hdr_t *ab;

retry:
	RW_RDLOCK(&act_rwlock);

	ab = act[DEF_BUNDLE];
	while (ab) {
		MUTEX_LOCK(&ab->ah_mutex);

		ASSERT(ab->ah_inuse == INUSE); /*DEBUG*/

		if (index == ab->ah_index) {
		        ATOMIC_INT_INCR(ab->ah_refcnt);
			MUTEX_UNLOCK(&ab->ah_mutex);
			RW_UNLOCK(&act_rwlock);

			if (ATOMIC_INT_READ(ab->ah_refcnt) > 1) {
				ATOMIC_INT_DECR(ab->ah_refcnt);
				nap(100);
				goto retry;
			}

			MUTEX_LOCK(&ab->ah_mutex);
			ATOMIC_INT_DECR(ab->ah_refcnt);
			*ah = ab;
			return 0;
		}

		MUTEX_UNLOCK(&ab->ah_mutex);
		ab = ab->ah_next;
	}

	RW_UNLOCK(&act_rwlock);
	ppplog(MSG_DEBUG, 0, "act_find_bundle: Index %d - Not found\n", index);
	return(ENOENT);

}

/*
 * Given a link index, find the bundle
 */
int
act_find_link(int index, act_hdr_t **ah)
{
	act_hdr_t *al;

retry:
	RW_RDLOCK(&act_rwlock);

	al = act[DEF_LINK];
	while (al) {
		MUTEX_LOCK(&al->ah_mutex);

		ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

		if (index == al->ah_index) {
		        ATOMIC_INT_INCR(al->ah_refcnt);
			MUTEX_UNLOCK(&al->ah_mutex);
			RW_UNLOCK(&act_rwlock);

			if (ATOMIC_INT_READ(al->ah_refcnt) > 1) {
				ATOMIC_INT_DECR(al->ah_refcnt);
				nap(100);
				goto retry;
			}

			MUTEX_LOCK(&al->ah_mutex);
			ATOMIC_INT_DECR(al->ah_refcnt);
			*ah = al;
			return 0;
		}

		MUTEX_UNLOCK(&al->ah_mutex);
		al = al->ah_next;
	}

	RW_UNLOCK(&act_rwlock);
	return(ENOENT);
}

int
act_find_link_rd(int index, act_hdr_t **ah)
{
	act_hdr_t *al;

retry:
	RW_RDLOCK(&act_rwlock);

	al = act[DEF_LINK];
	while (al) {
		MUTEX_LOCK(&al->ah_mutex);

		ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

		if (index == al->ah_index) {
		        ATOMIC_INT_INCR(al->ah_refcnt);
			MUTEX_UNLOCK(&al->ah_mutex);
			RW_UNLOCK(&act_rwlock);

			MUTEX_LOCK(&al->ah_mutex);
			ATOMIC_INT_DECR(al->ah_refcnt);
			*ah = al;
			return 0;
		}

		MUTEX_UNLOCK(&al->ah_mutex);
		al = al->ah_next;
	}

	RW_UNLOCK(&act_rwlock);
	return(ENOENT);
}

/*
 * Find an entry (prefer DEF_LINK then DEF_BUNDLE) that has a correspoding
 * link/bundle index.
 *
 * If we have find a link entry and it also has a bundle entry then return the
 * link entry .. but leave both the link and bundle entries locked.
 * If we return the link entry as the active header, then we also return a
 * pointer to the locked bundle header .. so the caller can manage the locks
 *
 */
int
act_findtrans(int lindex, int bindex, act_hdr_t **alp, act_hdr_t **abp)
{
	act_hdr_t *ab, *al;

retry:
	RW_RDLOCK(&act_rwlock);

	/*
	 * Find the element
	 */
	ab = act[DEF_BUNDLE];
	while (ab) {
		MUTEX_LOCK(&ab->ah_mutex);

		ASSERT(ab->ah_inuse == INUSE); /*DEBUG*/

		if (lindex < 0) {
			if (bindex == ab->ah_index) {
				ATOMIC_INT_INCR(ab->ah_refcnt);
				MUTEX_UNLOCK(&ab->ah_mutex);
				RW_UNLOCK(&act_rwlock);
				
				MUTEX_LOCK(&ab->ah_mutex);
				ATOMIC_INT_DECR(ab->ah_refcnt);
				*abp = ab;
				*alp = NULL;
				return 0;
			}
		} else {
			al = ab->ah_bundle.ab_links;
			while (al) {
				MUTEX_LOCK(&al->ah_mutex);

				ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

				if (lindex == al->ah_index) {

					/* Get the locking right */
					ATOMIC_INT_INCR(ab->ah_refcnt);
					ATOMIC_INT_INCR(al->ah_refcnt);
					MUTEX_UNLOCK(&al->ah_mutex);
					MUTEX_UNLOCK(&ab->ah_mutex);
					RW_UNLOCK(&act_rwlock);

					MUTEX_LOCK(&ab->ah_mutex);
					ATOMIC_INT_DECR(ab->ah_refcnt);
					MUTEX_LOCK(&al->ah_mutex);
					ATOMIC_INT_DECR(al->ah_refcnt);

					*alp = al;
					*abp = ab;
					return 0;
				}
				MUTEX_UNLOCK(&al->ah_mutex);
				al = al->ah_link.al_next;
				ASSERT(al != ab->ah_bundle.ab_links);
			}
		}
		MUTEX_UNLOCK(&ab->ah_mutex);
		ab = ab->ah_next;
	}
	/*
	 * Not found on an active bundle .. how about incoming ...
	 * just check active links ... they may not belong to a bundle
	 */
	if (bindex == -1) {
		al = act[DEF_LINK];
		while (al) {
			MUTEX_LOCK(&al->ah_mutex);

			ASSERT(al->ah_inuse == INUSE); /*DEBUG*/

			if (lindex == al->ah_index) {
				/* Get the locking right */
				ATOMIC_INT_INCR(al->ah_refcnt);
				MUTEX_UNLOCK(&al->ah_mutex);
				RW_UNLOCK(&act_rwlock);

				ab = al->ah_link.al_bundle;
				if (ab)
					MUTEX_LOCK(&ab->ah_mutex);
				MUTEX_LOCK(&al->ah_mutex);
				ATOMIC_INT_DECR(al->ah_refcnt);
ppplog(MSG_DEBUG, al, "act_findtrans: al_bundle %x\n", al->ah_link.al_bundle);
				*alp = al;
				*abp = ab;
				return 0;
			}
			MUTEX_UNLOCK(&al->ah_mutex);
			al = al->ah_next;
		}
	}
	*abp = NULL;
	RW_UNLOCK(&act_rwlock);
	return(ENOENT);
}


/*
 * Return the proto header of the specified protocol
 */
struct proto_hdr_s *
act_findproto(act_hdr_t *ah, ushort_t proto, int datagram)
{
	proto_hdr_t *ph;
	int i;
	act_hdr_t *ab;

	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));
	ASSERT(ah->ah_inuse == INUSE); /*DEBUG*/

	ph = ah->ah_protos;
	while (ph) {

		ASSERT(ph->ph_inuse == INUSE); /*DEBUG*/

		if (ph->ph_psmtab->pt_proto == proto)
			break;

		if (datagram) {
			for (i = 0;
			     ph->ph_psmtab->pt_netproto &&
			     ph->ph_psmtab->pt_netproto[i] &&
				     ph->ph_psmtab->pt_netproto[i] != proto;
			     i++)
			;

			if (ph->ph_psmtab->pt_netproto &&
			    ph->ph_psmtab->pt_netproto[i])
				break;
		}
		ph = ph->ph_next;
	}

	/*
	 * If the active element has a parent, check its proto list
	 */
	if (!ph && ah->ah_type == DEF_LINK && (ab = ah->ah_link.al_bundle)) {
		ASSERT(MUTEX_LOCKED(&ab->ah_mutex));
		return act_findproto(ab, proto, datagram);
	}

	return ph;
}


/* DEBUG */

act_display_proto(proto_hdr_t *ph)
{
	int i;

	psm_log(MSG_DEBUG, ph,
		"    protocol %s at %x\n", ph->ph_cfg->ch_id, ph);
#ifdef NOT
	psm_log(MSG_DEBUG, ph,
		"    parent  ... %x\n", ph->ph_parent);
	psm_log(MSG_DEBUG, ph,
		"    prototab .. %x\n", ph->ph_psmtab);
	psm_log(MSG_DEBUG, ph,
		"    priv .... %x\n", ph->ph_priv);
	psm_log(MSG_DEBUG, ph,
		"    next .... %x\n", ph->ph_next);
#endif
	psm_log(MSG_DEBUG, ph, "    proto = 0x%4.4x\n",
		ph->ph_psmtab->pt_proto);
	
	for (i = 0; ph->ph_psmtab->pt_netproto && ph->ph_psmtab->pt_netproto[i]; i++)
		psm_log(MSG_DEBUG, ph,
			"    netproto = 0x%4.4x\n",
			ph->ph_psmtab->pt_netproto[i]);
}

act_display_bundle(act_hdr_t *ab)
{
	proto_hdr_t *ph;
	act_hdr_t *al;

	ppplog(MSG_DEBUG, ab,  "\n");
	ppplog(MSG_DEBUG, ab, "Bundle %x is %s (index = %d)\n",
	       ab, ab->ah_cfg->ch_id, ab->ah_index);

	ph = ab->ah_protos;
	ppplog(MSG_DEBUG, ab, "Protos :\n");
	while (ph) {
		ppplog(MSG_DEBUG, ab, "  %x : %s\n", ph, ph->ph_cfg->ch_id);
		act_display_proto(ph);
		ph = ph->ph_next;
		ASSERT(ph != ab->ah_protos);
	}

	ppplog(MSG_DEBUG, ab, "Active links : \n");
	al = ab->ah_bundle.ab_links;
	while (al) {
		ppplog(MSG_DEBUG, ab, "  %x : %s (index %d)\n",
		       al, al->ah_cfg->ch_id, al->ah_index);
		
		al = al->ah_link.al_next;
		ASSERT(al != ab->ah_bundle.ab_links);
	}	
	ppplog(MSG_DEBUG, ab, "Phase ... %d\n", ab->ah_phase);
	ppplog(MSG_DEBUG, ab, "Links open  %d\n", ab->ah_bundle.ab_open_links);
	ppplog(MSG_DEBUG, ab, "Numlinks  %d\n", ab->ah_bundle.ab_numlinks);
	ppplog(MSG_DEBUG, ab, "Links NCP's  %d\n", ab->ah_bundle.ab_open_ncps);
	ppplog(MSG_DEBUG, ab, "Links Idle NCP's  %d\n",
	       ab->ah_bundle.ab_idle_ncps);
}

act_display_link(act_hdr_t *al)
{
	proto_hdr_t *ph;

	ppplog(MSG_DEBUG, al, "\n");
	ppplog(MSG_DEBUG, al, "Link %x is %s (index = %d)\n",
	       al, al->ah_cfg->ch_id, al->ah_index);

	ppplog(MSG_DEBUG, al, "Flags 0x%x\n", al->ah_link.al_flags);
	ppplog(MSG_DEBUG, al, "Fd %d\n", al->ah_link.al_fd);
	ppplog(MSG_DEBUG, al, "uid '%s'\n", al->ah_link.al_uid ? al->ah_link.al_uid : "Non set");
	ppplog(MSG_DEBUG, al, "cid '%s'\n", al->ah_link.al_cid ? al->ah_link.al_cid : "Non set");
	ppplog(MSG_DEBUG, al, "type %d\n", al->ah_link.al_type);	
	ppplog(MSG_DEBUG, al, "Bandwidth %d\n", al->ah_link.al_bandwidth);
	ppplog(MSG_DEBUG, al, "Auth Tmout %d\n", al->ah_link.al_auth_tmout);

	ppplog(MSG_DEBUG, al, "Local Opts 0x%x\n", al->ah_link.al_local_opts);
	ppplog(MSG_DEBUG, al, "Local Mru %d\n", al->ah_link.al_local_mru);
	ppplog(MSG_DEBUG, al, "Local Accm 0x%x \n", al->ah_link.al_local_accm);
	ppplog(MSG_DEBUG, al, "Local Magic 0x%x \n", al->ah_link.al_local_magic);
	ppplog(MSG_DEBUG, al, "Local Mrru %d\n", al->ah_link.al_local_mrru);

	ppplog(MSG_DEBUG, al, "Peer Opts 0x%x\n", al->ah_link.al_peer_opts);
	ppplog(MSG_DEBUG, al, "Peer Mru %d\n", al->ah_link.al_peer_mru);
	ppplog(MSG_DEBUG, al, "Peer Accm 0x%x \n", al->ah_link.al_peer_accm);
	ppplog(MSG_DEBUG, al, "Peer Magic 0x%x \n", al->ah_link.al_peer_magic);
	ppplog(MSG_DEBUG, al, "Peer Mrru %d\n", al->ah_link.al_peer_mrru);

	ppplog(MSG_DEBUG, al, "Peer Auth Name '%s'\n", ((char *)(al->ah_link.al_peer_auth_name) ? (char*)(al->ah_link.al_peer_auth_name) : "Non set"));
	ppplog(MSG_DEBUG, al, "Peer Ed class %d\n", al->ah_link.al_peer_ed_class);
#ifdef NOT
	ppplog(MSG_DEBUG, al, "Parent bundle .. %x\n", al->ah_link.al_bundle);
	ppplog(MSG_DEBUG, al, "Next active link .. %x\n", al->ah_link.al_next);
	ppplog(MSG_DEBUG, al, "Short cut to lcp .. %x\n", al->ah_link.al_lcp);
#endif
	ppplog(MSG_DEBUG, al, "Phase ... %d\n", al->ah_phase);

	ph = al->ah_protos;
	ppplog(MSG_DEBUG, al, "Protos :\n");
	while (ph) {
		ppplog(MSG_DEBUG, al, "  %x : %s\n", ph, ph->ph_cfg->ch_id);
		act_display_proto(ph);
		ph = ph->ph_next;
	}
	
}

act_display(act_hdr_t *ah)
{
	switch (ah->ah_type) {
	case DEF_BUNDLE:
		act_display_bundle(ah);
		break;
	case DEF_LINK:
		act_display_link(ah);
		break;
	}
}

/*
 * This is used for incoming call -> link definition mapping.
 * We must find an active link which provides the service type requested
 */
act_hdr_t *
act_incominglink(char *dev, int service)
{
	act_hdr_t *ah;
	int ret;
	struct cfg_link *cl;
	
	if (!dev)
		dev = "";

	ppplog(MSG_DEBUG, 0,
	       "act_incominglink: dev = %s, serv = %d\n", dev, service);
	RW_RDLOCK(&act_rwlock);

	ah = act[DEF_LINK];
	while (ah) {

		MUTEX_LOCK(&ah->ah_mutex);

		ASSERT(ah->ah_inuse == INUSE); /*DEBUG*/

		if (ah->ah_link.al_flags & ALF_INUSE)
			goto next;

		cl = (struct cfg_link *)ah->ah_cfg;

		ppplog(MSG_DEBUG, 0,
		       "act_incominglink: link ... %s, serv %d\n",
		       ucfg_str(&cl->ln_ch, cl->ln_dev), cl->ln_type);

		if (cl->ln_type != service)
			goto next;

		if (strcmp(ucfg_str(&cl->ln_ch, cl->ln_dev), dev) == 0)
			break;

	next:
		MUTEX_UNLOCK(&ah->ah_mutex);
		ah = ah->ah_next;
	}

	if (ah) {
		ATOMIC_INT_INCR(ah->ah_refcnt);
		MUTEX_UNLOCK(&ah->ah_mutex);
	}

	RW_UNLOCK(&act_rwlock);

	if (ah) {
		ah->ah_phase = PHASE_DEAD;
		MUTEX_LOCK(&ah->ah_mutex);
		ATOMIC_INT_DECR(ah->ah_refcnt);
	}

	ppplog(MSG_DEBUG, 0, "act_incominglink: return %d\n", ah);
	return ah;
}

/*
 * Test if the specified link matches an incoming definition
 * ... the link must be defined, caller id and login must match.
 */
int
act_try_incoming(act_hdr_t *ab, act_hdr_t *al)
{
	struct cfg_bundle *cb;
	char *uid, *links, *cid, *aid;

	ppplog(MSG_DEBUG, ab, "act_try_incoming: link %s\n",
	       al->ah_cfg->ch_id);

	/* Is this link listed ? */
	cb = (struct cfg_bundle *)ab->ah_cfg;
	links = ucfg_str(&cb->bn_ch, cb->bn_links);
	uid = ucfg_str(&cb->bn_ch, cb->bn_uid);
	cid = ucfg_str(&cb->bn_ch, cb->bn_cid);
	aid = ucfg_str(&cb->bn_ch, cb->bn_authid);

	al->ah_link.al_cindex =
		ucfg_find_element(al->ah_cfg->ch_id, links);

	if (!al->ah_link.al_cindex) {
		ppplog(MSG_INFO_MED, al,
		       "Link not in bundle %s\n", ab->ah_cfg->ch_id);
		return 0;
	}
	ppplog(MSG_DEBUG, al, "act_try_incoming: link matched\n");

	/*
	 * Must have at least one non-null authentication parameter
	 */
	if (!*uid && !*cid && !*aid) {
		ppplog(MSG_INFO_MED, al,
	       "No login, callerid or authid .. can't join bundle %s\n",
		       ab->ah_cfg->ch_id);
		return 0;
	}

	/* Does the login ID match */	

	if (*uid) {
		/* One has been defined ... must match */
		if (!(al->ah_link.al_uid && *al->ah_link.al_uid)) {
			ppplog(MSG_INFO_MED, al,
		       "login '%s' required .. can't join bundle %s\n",
			       uid, ab->ah_cfg->ch_id);
			return 0;
		}

		if (strcmp(uid, "*") == 0) {
			ppplog(MSG_INFO_MED, al,
			       "Allowing any non-null login\n");
		} else if (!ucfg_find_element(al->ah_link.al_uid, uid)) {
			ppplog(MSG_INFO_MED, al,
       "login mismatch (require %s, found %s) .. can't join bundle %s\n",
			       uid, al->ah_link.al_uid, ab->ah_cfg->ch_id);
			return 0;
		}

		/*
		 * If we are at login time ... and not auto detected
		 * then just go with a user id match for now.
		 */
		if (al->ah_phase <= PHASE_ESTAB && 
		    !(al->ah_link.al_flags & ALF_AUTO)) {
			ppplog(MSG_DEBUG, al,
		       "act_try_incoming: Allowing to join .. login\n");
			return 1;
		}
		ppplog(MSG_DEBUG, al, "act_try_incoming: uid matched\n");
	}

	/* Check Caller ID matches */

	if (*cid) {
		/* One has been defined ... must match */
		if (!(al->ah_link.al_cid && *al->ah_link.al_cid)) {
			ppplog(MSG_INFO_MED, al,
		       "callerid '%s' required .. can't join bundle %s\n",
			       cid, ab->ah_cfg->ch_id);
			return 0;
		}

		if (!ucfg_find_element(al->ah_link.al_cid, cid)) {
			ppplog(MSG_INFO_MED, al,
       "callerid mismatch (require %s, found %s) .. can't join bundle %s\n",
			       cid, al->ah_link.al_cid, ab->ah_cfg->ch_id);
			return 0;
		}
		ppplog(MSG_DEBUG, al, "act_try_incoming: cid matched\n");
	}

	/* Check Auth Id Matches */

	if (*aid) {
		/* One has been defined ... must match */
		if (!(al->ah_link.al_peer_auth_name &&
		      *al->ah_link.al_peer_auth_name)) {
			ppplog(MSG_INFO_MED, al,
		       "Authid '%s' required .. can't join bundle %s\n",
			       aid, ab->ah_cfg->ch_id);
			return 0;
		}

		if (strcmp(aid, "*") == 0) {
			ppplog(MSG_INFO_MED, al,
			       "Allowing any non-null authid\n");
		} else if (!ucfg_find_element((char *)al->ah_link.al_peer_auth_name, aid)) {
			ppplog(MSG_INFO_MED, al,
       "Authid mismatch (require %s, found %s) .. can't join bundle %s\n",
			       aid, al->ah_link.al_peer_auth_name,
			       ab->ah_cfg->ch_id);
			return 0;
		}
		ppplog(MSG_DEBUG, al, "act_try_incoming: aid matched\n");
	}

	ppplog(MSG_DEBUG, al, "act_try_incoming: Okay\n");
	return 1;
}

/*
 * Search the all defined bundles, if the link specifed matches an
 * incoming definition and the auth, ed, login and caller id all
 * match ... we have found the bundle to join.
 *
 * If a link is a secondary the things that must match are:
 * 	Login Id, Caller Id, Endpoint Discriminator and Auth name
 */
act_hdr_t *
act_join_bundle(act_hdr_t *al)
{
	act_hdr_t *ab;
	struct cfg_bundle *cb;
	RW_RDLOCK(&act_rwlock);
	ab = act[DEF_BUNDLE];

	while (ab) {
		MUTEX_LOCK(&ab->ah_mutex);

		ASSERT(ab->ah_inuse == INUSE); /*DEBUG*/

		cb = (struct cfg_bundle *)ab->ah_cfg;
		if (!(cb->bn_type & BT_IN))
			goto next;

		ppplog(MSG_DEBUG, al, "act_join_bundle: Check bundle %s\n",
		       ab->ah_cfg->ch_id);

		if (ab->ah_bundle.ab_open_links > 0) {
			/*
			 * May be joining an existing bundle, so check the
			 * Endpoint-discriminator and authentication match
			 */
			ppplog(MSG_DEBUG, al,
	        "act_join_bundle: Try Adding secondary link to bundle %s\n",
			       ab->ah_cfg->ch_id);
			if (!bundle_check_linkid(ab, al))
				goto next;
				
		}

		/* Must match current bundles incoming profile */
		if (act_try_incoming(ab, al))
			break;

	next:
		MUTEX_UNLOCK(&ab->ah_mutex);
		ab = ab->ah_next;
	}

	if (ab) {
		ATOMIC_INT_INCR(ab->ah_refcnt);
		MUTEX_UNLOCK(&ab->ah_mutex);
	}

	RW_UNLOCK(&act_rwlock);

	if (ab) {
		MUTEX_LOCK(&ab->ah_mutex);
		ATOMIC_INT_DECR(ab->ah_refcnt);
	}

	return ab;
}
