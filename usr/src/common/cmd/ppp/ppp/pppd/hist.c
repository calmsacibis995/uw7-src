#ident	"@(#)hist.c	1.2"
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
#include "hist.h"
#include "act.h"

#define HIST_SIZE 21

/*
 * This file contains the functions used to maintain and
 * interogate a call history.
 *
 * The call history is held in a circular list.
 */
struct histent_s hist[HIST_SIZE];
int hist_head, hist_tail;
mutex_t hist_mutex;	/* lock list and head/tail pointers */

void
hist_init()
{
	mutex_init(&hist_mutex, USYNC_PROCESS, NULL);
	hist_head = 0;
	hist_tail = 0;
}

void
hist_clear()
{
	MUTEX_LOCK(&hist_mutex);
	hist_head = 0;
	hist_tail = 0;
	MUTEX_UNLOCK(&hist_mutex);
}

/*
 * This function adds a hist entry to the list.
 *
 * The uid and cid arguments are only used when 
 * the al (act_hdr_t) is NULL ... ie no associated link def.
 */
void
hist_add(int op, act_hdr_t *ah, int error, char *dev, char *uid, char *cid)
{
	struct histent_s *he;
	act_link_t *al;

	MUTEX_LOCK(&hist_mutex);

	ppplog(MSG_DEBUG, ah, "hist_add: op %d, error %d\n", op, error);

	if (!ah)
		ppplog(MSG_DEBUG, ah, "hist_add: dev %s, uid %s, cid %s\n",
		       dev, uid, cid);

	/* Tail points to next entry to be filled */
	he = &hist[hist_tail];

	/* Initialise the entry .. */
	he->he_alflags = 0;
	he->he_alreason = 0;
	he->he_abreason = 0;
	he->he_linkid[0] = 0;
	he->he_bundleid[0] = 0;
	he->he_dev[0] = 0;
	he->he_uid[0] = 0;
	he->he_aid[0] = 0;
	he->he_cid[0] = 0;
	he->he_sys[0] = 0;

	/* Fill the entry */
	he->he_op = op;
	he->he_error = error;
	he->he_time = time(NULL);

	if (ah) {
		struct cfg_link *cl = (struct cfg_link *)ah->ah_cfg;

		al = &ah->ah_link;

		he->he_alflags = al->al_flags;
		he->he_alreason = al->al_reason;

		strncpy(he->he_linkid, ah->ah_cfg->ch_id, MAXID);

		if (al->al_uid) {
			strncpy(he->he_uid, al->al_uid, MAXUIDLEN);
			he->he_uid[MAXUIDLEN] = 0;
		}

		if (al->al_cid) {
			strncpy(he->he_cid, al->al_cid, MAXID);
			he->he_cid[MAXID] = 0;
		}

		if (al->al_peer_auth_name) {
			strncpy(he->he_aid, al->al_peer_auth_name, MAXID);
			he->he_aid[MAXID] = 0;
		}
		
		strncpy(he->he_dev, ucfg_str(&cl->ln_ch, cl->ln_dev), MAXID);
		he->he_dev[MAXID] = 0;

	} else {
		if (cid) {
			strncpy(he->he_cid, cid, MAXID);
			he->he_cid[MAXID] = 0;
		}

		if (uid) {
			strncpy(he->he_uid, uid, MAXUIDLEN);
			he->he_uid[MAXUIDLEN] = 0;
		}

		if (dev) {
			strncpy(he->he_dev, dev, MAXID);
			he->he_dev[MAXID] = 0;
		}
	}

	if (al->al_bundle) {
		act_hdr_t *ab = al->al_bundle;

		he->he_abreason = ab->ah_bundle.ab_reason;
		strncpy(he->he_bundleid, ab->ah_cfg->ch_id, MAXID);
	}

	/* Move tail onwards ... */
	hist_tail++;
	if (hist_tail == HIST_SIZE)
		hist_tail = 0;
	/*
	 * If we have pushed the tail to the head ..
	 * move the head onwards
	 */
	if (hist_tail == hist_head)
		hist_head++;
	if (hist_head == HIST_SIZE)
		hist_head = 0;

	MUTEX_UNLOCK(&hist_mutex);
}

/*
 * Called to start a lookup of hist entries.
 * Return index for entry specified by cookie ... or -1 if not found
 */
int
hist_lookup_start(int cookie)
{
	int i, index;

	MUTEX_LOCK(&hist_mutex);

	if (hist_tail == hist_head)
		return -1;

	index = hist_tail - 1;
	if (index == -1)
		index = HIST_SIZE - 1;

	/* Find the first entry */
	for (i = 0; i < cookie && index != hist_head; i++) {

		if (index == hist_head)
			return -1;

		index--;
		if (index == -1)
			index = HIST_SIZE - 1;
	}
	return index;
}

/*
 * Get the history entry specified by index. Return the index of the
 * next element (-1 if there isn't one).
 */
int
hist_lookup_ent(int index, struct histent_s *he)
{
	*he = hist[index];

	if (index == hist_head)
		return -1;

	index--;
	if (index == -1)
		index = HIST_SIZE - 1;
	return index;
}

void
hist_lookup_end()
{
	MUTEX_UNLOCK(&hist_mutex);	
}
