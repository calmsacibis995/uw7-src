#ident	"@(#)chap_rt.c	1.10"

#include <stdio.h>
#include <assert.h>
#include <dial.h>
#include <errno.h>
#include <sys/conf.h>
#include <synch.h>
#include <sys/byteorder.h>
#include <sys/ppp.h>
#include <sys/types.h>
#include <time.h>

#include "ppp_cfg.h"
#include "ppp_type.h"
#include "fsm.h"
#include "psm.h"
#include "act.h"
#include "ppp_proto.h"
#include "chap.h"
#include "auth.h"

#define LCP_SIZE DEFAULT_MRU

void auth_open(act_hdr_t *);
uchar_t *auth_peername(proto_hdr_t *ph, uchar_t *name, int namelen);

STATIC void chap_snd_challenge(proto_hdr_t *ph);
STATIC int chap_chall_tmout(proto_hdr_t *ph, act_hdr_t *ah);

#define CHAP_RESP_LEN 16
#define min(a, b) ((a) < (b) ? (a) : (b))

STATIC int
chap_load()
{
	ppplog(MSG_INFO, 0, "CHAP Loaded\n");
	return 0;
}

STATIC int
chap_alloc(proto_hdr_t *ph)
{
	ph->ph_priv = (void *)malloc(sizeof(struct chap_s));
	if (!ph->ph_priv)
		return ENOMEM;
	return 0;
}

STATIC int
chap_free(proto_hdr_t *ph)
{
	free(ph->ph_priv);
	ph->ph_priv = NULL;
}

chap_init(struct act_hdr_s *ah, proto_hdr_t *ph)
{
	struct chap_s *chap = (struct chap_s *)ph->ph_priv;

	ASSERT(ah->ah_type == DEF_LINK);

	chap->chap_local_status = CHAPS_INITIAL;
	chap->chap_peer_status = CHAPS_INITIAL;
	chap->chap_local_fail = 0;
	chap->chap_peer_fail = 0;
	chap->chap_peer_name[0] = 0;

	return 0;
}

STATIC mkchallenge(ulong challenge[])
{
	int i;
	ulong j;

	j = challenge[0];

	for (i = 0; i < 4; i++) {
		j = (j * 125621) + 3;
		challenge[i] = j;
	}
}


STATIC void
chap_start(proto_hdr_t *ph)
{
	struct chap_s *chap = (struct chap_s *)ph->ph_priv;
	act_hdr_t *ah = ph->ph_parent;

	psm_log(MSG_INFO_LOW, ph, "Started\n");

	psm_log(MSG_DEBUG, ph, "Auth timeout = %d\n",
		chap->chap_local_auth_cnt);

	chap->chap_local_auth_cnt = ah->ah_link.al_auth_tmout;
	chap->chap_local_auth_tmid = 0;
	chap->chap_peer_auth_cnt = ah->ah_link.al_auth_tmout;
	chap->chap_peer_auth_tmid = 0;

	chap->chap_local_auth_id = 0;
	chap->chap_peer_auth_id = 0;

	chap->chap_local_status = CHAPS_INITIAL;
	chap->chap_peer_status = CHAPS_INITIAL;
	chap->chap_peer_name[0] = 0;

	chap->chap_challenge[0] = (ulong)time(NULL);

	if (ah->ah_link.al_local_opts & ALO_CHAP)
		/* We require CHAP */
		chap_snd_challenge(ph);

	if (ah->ah_link.al_peer_opts & ALO_CHAP)
		/* Peer requires CHAP.  */
		chap->chap_peer_auth_tmid =
			timeout(ah->ah_link.al_auth_tmout * HZ,
				chap_chall_tmout, (caddr_t)ph,
				(caddr_t)ph->ph_parent->ah_link.al_bundle);
}

STATIC void
chap_term(proto_hdr_t *ph)
{
	struct chap_s *prv = (struct chap_s *)ph->ph_priv;
	act_hdr_t *ah = ph->ph_parent;

	psm_log(MSG_INFO_LOW, ph, "Terminated\n");

	if (prv->chap_local_auth_tmid) {
		untimeout(prv->chap_local_auth_tmid);
		prv->chap_local_auth_tmid = 0;
	}

	if (prv->chap_peer_auth_tmid) {
		untimeout(prv->chap_peer_auth_tmid);
		prv->chap_peer_auth_tmid = 0;
	}

	/* If we have CHAP configured, ensure it has completed */

	if (ah->ah_link.al_local_opts & ALO_CHAP) {
		if (prv->chap_local_status == CHAPS_INITIAL) {
			prv->chap_local_status = CHAPS_FAILED;
			prv->chap_local_fail++;
		}
	}

	if (ah->ah_link.al_peer_opts & ALO_CHAP) {
		if (prv->chap_peer_status == CHAPS_INITIAL) {
			prv->chap_peer_status = CHAPS_FAILED;
			prv->chap_peer_fail++;
		}
	}
}

/*
 * chap_rcv
 *
 * Called to handle incoming CHAP protocol packets
 */
STATIC int
chap_rcv(proto_hdr_t *ph, db_t *db)
{
	struct chap_hdr_s *chap = (struct chap_hdr_s *)db->db_rptr;
	ushort_t len;
	int ret = 0, i;
	db_t *ndb;
	struct chap_hdr_s *nchap;
	uchar_t valsize, *val, namelen, *name;
	uchar_t buf[64], buflen;
	struct chap_hdr_s *resp;
	uchar_t *secret, expect[CHAP_RESP_LEN];
	struct chap_s *prv = (struct chap_s *)ph->ph_priv;
	act_hdr_t *ah = ph->ph_parent;
	char *msg;

	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));
	ASSERT(ah->ah_type == DEF_LINK);

	/* Check that we are in AUTH or NETWORK phases */

	if (ah->ah_phase != PHASE_AUTH && ah->ah_phase != PHASE_NETWORK) {
		psm_log(MSG_WARN, ph, 
			 "Received when not in AUTH or NETWORK phase.\n");
		db_free(db);
		return;
	}

	/* Check the length is sensible */
	
	if (db->db_wptr - db->db_rptr < sizeof(struct chap_hdr_s)) {
		psm_log(MSG_WARN, ph, "Received Frame too short - dumped\n");
		db_free(db);
		return;
	}

	len = ntohs(chap->chap_len);

	if (db->db_wptr - db->db_rptr < len) {
		psm_log(MSG_WARN, ph, "Received Frame too short - dumped\n");
		db_free(db);
		return;
	}

	/*
	 * We use the chap_len in preference to the length of the packet
	 * - there may have been padding ... which we don't want to see
	 */
	db->db_wptr = db->db_rptr + len;

	switch (chap->chap_code) {
	case CHAP_CHALLENGE:
		psm_log(MSG_INFO_LOW, ph, "Received Challenge\n");

		if (!(ah->ah_link.al_peer_opts & ALO_CHAP)) {
			/*
			 * We got an unsolicited CHAP challenge - log this
			 */
			psm_log(MSG_WARN, ph,
			"Received Challenge when CHAP not negotiated\n");
			prv->chap_peer_status = CHAPS_BADFMT;
			prv->chap_peer_fail++;
			break;
		}

		if (len < sizeof(struct chap_hdr_s) + 1) {
			psm_log(MSG_DEBUG, ph, "Bad length (a)\n");
		  bad_chap_chall_format:
			psm_log(MSG_WARN, ph, "Bad Challenge packet format\n");
			prv->chap_peer_status = CHAPS_BADFMT;
			prv->chap_peer_fail++;
			break;
		}

		db->db_rptr += sizeof(struct chap_hdr_s);

		valsize = (uchar_t)*db->db_rptr++;
		val = (uchar_t *)db->db_rptr;
		db->db_rptr += valsize;

		if (db->db_rptr > db->db_wptr) {
			psm_log(MSG_DEBUG, ph,
				"Bad length (b) valsize = %d\n", valsize);
			goto bad_chap_chall_format;
		}

		name = db->db_rptr;
		namelen = db->db_wptr - db->db_rptr;

		psm_log(MSG_INFO_MED, ph, "Peer Name '%*.*s'\n",
			 namelen, namelen, name);

		sprintf(prv->chap_peer_name, "%*.*s",
			min(namelen, CHAP_PNAMELEN),
			min(namelen, CHAP_PNAMELEN), name);

		/*
		 * Store the challenge ID so we can match up a 
		 * fail/success packet
		 */
		prv->chap_peer_auth_id = chap->chap_id; 

		/* Check for peerauthname override */

		name = auth_peername(ph, name, namelen);
		if (!name) {
			psm_log(MSG_WARN, ph, "Out of memory - dropped\n");
			prv->chap_peer_status = CHAPS_NORES;
			prv->chap_peer_fail++;
			break;
		}

		namelen = strlen(name);

		/* We have the peer name, lookup the peers secret */

		secret = (uchar_t *)auth_get_secret(ph, PEERSEC, name, namelen);
		if (!secret) {
			psm_log(MSG_WARN, ph, "No secret for the name '%*.*s'\n",
				namelen, namelen, name);
			secret = (uchar_t*)strdup("");
			if (!secret) {
				psm_log(MSG_WARN, ph, "No memory, dropped.\n");
				prv->chap_peer_status = CHAPS_NORES;
				prv->chap_peer_fail++;
				free(name);
				break;
			}
		}

		/*
		 * Calculate the expected response 
		 *
		 * Calculate tha hashed value based upon a string which
		 * consisits of the ID concatenated with the secret followed
		 * by the Challenge value.
		 */
		sprintf((char *)buf, "%c%s", chap->chap_id, secret);
		buflen = 1 + strlen((char *)(buf + 1));

		free(secret);	/* It was strdup()ed when created */

		memcpy((char *)(buf + buflen), val, valsize);

		buflen += valsize;

		MD5String_r(buf, buflen, expect);

#ifdef DEBUG_CHAP
		psm_log(MSG_DEBUG, ph, "Calculated response\n");
		psm_loghex(MSG_DEBUG, ph, (char*)expect, CHAP_RESP_LEN);
		psm_log(MSG_DEBUG, ph, "From buffer\n");
		psm_loghex(MSG_DEBUG, ph, (char*)buf, buflen);
#endif
		/* Send a chap response */

		ndb = db_alloc(LCP_SIZE);
		if (!ndb) {
			psm_log(MSG_WARN, ph, "Out of memory - dropped\n");
			free(name);
			prv->chap_peer_status = CHAPS_NORES;
			prv->chap_peer_fail++;
			break;
		}

		psm_add_pf(ph, ndb);

		resp = (struct chap_hdr_s *)ndb->db_wptr;

		resp->chap_code = CHAP_RESPONSE;
		resp->chap_id = chap->chap_id;

		ndb->db_wptr += sizeof(struct chap_hdr_s);

		/* Copy in the challenge response value */

		*ndb->db_wptr++ = CHAP_RESP_LEN;
		memcpy(ndb->db_wptr, expect, CHAP_RESP_LEN);
		ndb->db_wptr += CHAP_RESP_LEN;

		/* And now our name value */
		name = (uchar_t *)auth_localname(ph);

		memcpy(ndb->db_wptr, name, strlen((char *)name));
		ndb->db_wptr += strlen((char *)name);

		psm_log(MSG_INFO_MED, ph, 
			 "Sending response. Local Name '%s'\n", name);

		free(name);

		resp->chap_len = htons(ndb->db_wptr - (uchar_t *)resp);

		/* Send the reponse */

		psm_snd(ph, ndb);

		/* Set timer waiting for Success/Fail ??*/

		break;

	case CHAP_RESPONSE:
		psm_log(MSG_INFO_LOW, ph, "Received Response\n");

		if (prv->chap_local_auth_id != chap->chap_id) {
			psm_log(MSG_WARN, ph,
				 "Discard Frame with id mis-match\n");
			prv->chap_local_status = CHAPS_BADFMT;
			prv->chap_local_fail++;
			break;
		}

		if (len < sizeof(struct chap_hdr_s) + 1) {
		  bad_chap_resp_format:
			psm_log(MSG_WARN, ph, "Bad Response packet format\n");
			prv->chap_local_status = CHAPS_BADFMT;
			prv->chap_local_fail++;
			break;
		}

		db->db_rptr += sizeof(struct chap_hdr_s);

		valsize = (uchar_t)*db->db_rptr++;
		val = (uchar_t *)db->db_rptr;
		db->db_rptr += valsize;

		if (db->db_rptr >= db->db_wptr)
			goto bad_chap_resp_format;

		/*
		 * If we have already sent/success failure for this id
		 * we must resend the same success failure ... prevents
		 * successive retries with the same response id from
		 * discovering the name/secrets
		 */
		switch (prv->chap_local_status) {
		case CHAPS_SUCCESS:
			ret = 0;
			msg = "Authentication successful (not first CHAP response)";
			goto reply;
		case CHAPS_FAILED:
			ret = 1;
			msg = "Authentication failed (not first CHAP response)";
			goto reply;
		}

		name = db->db_rptr;
		namelen = db->db_wptr - db->db_rptr;
		
		psm_log(MSG_INFO_MED, ph, "Peer Name '%*.*s'\n", 
			 namelen, namelen, name);

		sprintf(prv->chap_peer_name, "%*.*s",
			min(namelen, CHAP_PNAMELEN),
			min(namelen, CHAP_PNAMELEN), name);

		/*
		 * Save the peers name 
		 */
		name = (uchar_t *)auth_peername(ph, name, namelen);
		if (!name) {
			psm_log(MSG_WARN, ph, "Out of memory - dropped\n");
			prv->chap_local_status = CHAPS_NORES;
			prv->chap_local_fail++;
			break;
		}
		namelen = strlen(name);

		ah->ah_link.al_peer_auth_name = name;
		ah->ah_link.al_peer_auth_namelen = namelen;

		/*
		 * We have a valid response, cancel the 'waiting for
		 * response' timeout
		 */
		if (prv->chap_local_auth_tmid)
			untimeout(prv->chap_local_auth_tmid);

		/* We have the name, so lookup the secret */

		secret = (uchar_t*)auth_get_secret(ph, LOCALSEC, name, namelen);
		if (!secret) {
			psm_log(MSG_WARN, ph, "No secret for the name '%s'\n",
			       name);
			ret = 1;
			prv->chap_local_status = CHAPS_NOSEC;
			msg = "Authentication failed";
			prv->chap_local_fail++;
			goto reply;
		}

		/*
		 * Calculate the expected response 
		 *
		 * Calculate tha hashed value based upon a string which
		 * consisits of the ID concatenated with the secret followed
		 * by the Challenge value.
		 */
		sprintf((char *)buf, "%c%s", chap->chap_id, secret);
		buflen = 1 + strlen((char *)(buf + 1));

		free(secret);	/* It was strdup()ed when created */

		memcpy((char *)&buf[buflen], &prv->chap_challenge,
		       sizeof(prv->chap_challenge));

		buflen += sizeof(prv->chap_challenge);

		MD5String_r(buf, buflen, expect);

		/* Compare the expected and actual response */

		if (valsize != CHAP_RESP_LEN ||
		    memcmp(expect, val, valsize) != 0) {
			ret = 1;
			msg = "Authentication failed";
		} else {
			ret = 0;
			msg = "Authentication successful";
		}

	reply:
		ndb = db_alloc(sizeof(struct chap_hdr_s) + 2 + strlen(msg));
		if (!ndb) {
			psm_log(MSG_WARN, ph, "Out of memory - dropped\n");
			prv->chap_local_status = CHAPS_NORES;
			prv->chap_local_fail++;
			break;
		}

		psm_add_pf(ph, ndb);

		resp = (struct chap_hdr_s *)ndb->db_wptr;
		resp->chap_id = chap->chap_id;
		resp->chap_len = htons(sizeof(struct chap_hdr_s)
				       + strlen(msg));
		ndb->db_wptr += sizeof(struct chap_hdr_s);

		if (strlen(msg) > 0) {
			memcpy(ndb->db_wptr, msg, strlen(msg));
			ndb->db_wptr += strlen(msg);
		}

		if (ret) {

			/* Fail */
			psm_log(MSG_INFO_LOW, ph,
			       "Received invalid Response (Send Fail)\n");
			resp->chap_code = CHAP_FAIL;
			psm_snd(ph, ndb);
			prv->chap_local_status = CHAPS_FAILED;
			prv->chap_local_fail++;

			/* Administratively close the link */
			psm_close_link(ah, ALR_AUTHFAIL);

		} else {
			/* Succeed */
			psm_log(MSG_INFO_LOW, ph,
			       "Received Valid Response (Send Success)\n");

			resp->chap_code = CHAP_SUCCESS;
			psm_snd(ph, ndb);
			prv->chap_local_status = CHAPS_SUCCESS;
			ah->ah_link.al_flags |= ALF_LOCAL_AUTH;
			auth_open(ah);
		}
		break;

	case CHAP_SUCCESS:
		psm_log(MSG_INFO_LOW, ph, "Received SUCCESS\n");

		/* Check the ID matches our Request */

		if (prv->chap_peer_auth_id != chap->chap_id) {
			psm_log(MSG_WARN, ph, 
				 "Discard Frame with id mis-match\n");
			prv->chap_peer_status = CHAPS_BADFMT;
			prv->chap_peer_fail++;
			break;
		}

		if (prv->chap_peer_auth_tmid)
			untimeout(prv->chap_peer_auth_tmid);
		prv->chap_peer_status = CHAPS_SUCCESS;
		ah->ah_link.al_flags |= ALF_PEER_AUTH;

		len = ntohs(chap->chap_len);
		if (len > sizeof(struct chap_hdr_s)) {
			db->db_rptr += sizeof(struct chap_hdr_s);
			len -= sizeof(struct chap_hdr_s);
			if (len > 0)
				psm_log(MSG_INFO_LOW, ph,
					"Message '%*.*s'\n",
					len, len, db->db_rptr);
		}

		psm_log(MSG_INFO_LOW, ph,
			 "Authentication with Peer Succeeded\n");
		auth_open(ah);
		break;

	case CHAP_FAIL:
		/* Check the ID matches our Request */

		if (prv->chap_peer_auth_id != chap->chap_id) {
			psm_log(MSG_WARN, ph,
				 "Discard Frame with id mis-match\n");
			prv->chap_peer_status = CHAPS_BADFMT;
			prv->chap_peer_fail++;
			break;
		}

		if (prv->chap_peer_auth_tmid)
			untimeout(prv->chap_peer_auth_tmid);
		prv->chap_peer_status = CHAPS_FAILED;
		prv->chap_peer_fail++;

		len = ntohs(chap->chap_len);
		if (len > sizeof(struct chap_hdr_s)) {
			db->db_rptr += sizeof(struct chap_hdr_s);
			len -= sizeof(struct chap_hdr_s);
			if (len > 0)
				psm_log(MSG_INFO_LOW, ph,
					"Message '%*.*s'\n",
					len, len, db->db_rptr);
		}

		psm_log(MSG_INFO_LOW, ph,
			"Failed to authenticate with Peer\n");
		psm_close_link(ah, ALR_AUTHFAIL);
		break;

	default:
		psm_log(MSG_WARN, ph,
		       "Unknown CHAP code (%d)\n", chap->chap_code);
		break;
	}

	db_free(db);
}

STATIC int
chap_status(proto_hdr_t *ph, struct chap_s *chap)
{
	memcpy(chap, ph->ph_priv, sizeof(struct chap_s));
}

/*
 * The table of PSM entry points
 */
psm_tab_t psm_entry = {
	PSM_API_VERSION,
	"CHAP",
	PROTO_CHAP,
	PT_AUTH | PT_LINK, /* flags */
	PT_PRI_MED,

	NULL,

	chap_load,	/* load */
	NULL,		/* unload */
	chap_alloc,
	chap_free,
        chap_init,

	chap_rcv,
	NULL,	/* send */
	NULL,	/* log */
	chap_status,

	/* Non-FSM specific fields .. */
	chap_start,
	chap_term,
};

/*
 * Called when authenication with the Peer has not completed in the
 * specified time.
 */
STATIC int
chap_chall_tmout(proto_hdr_t *ph, act_hdr_t *ab)
{
	struct chap_s *prv = (struct chap_s *)ph->ph_priv;

	if (ab)
		MUTEX_LOCK(&ab->ah_mutex);
	MUTEX_LOCK(&ph->ph_parent->ah_mutex);

	if (prv->chap_peer_auth_tmid) {
		psm_log(MSG_DEBUG, ph, "chap_chall_tmout: id = %x\n", 
		       prv->chap_peer_auth_tmid);

		/* Assume that the CHAP_SUCCESS was lost !! Just proceed */

		psm_log(MSG_INFO_LOW, ph,
	"Timed out waiting for SUCCESS - Assuming CHAP_SUCCESS lost\n");
		ph->ph_parent->ah_link.al_flags |= ALF_PEER_AUTH;
		prv->chap_peer_status = CHAPS_TIMEOUT;
		prv->chap_peer_fail++;
		auth_open(ph->ph_parent);
	}
	MUTEX_UNLOCK(&ph->ph_parent->ah_mutex);
	if(ab)
		MUTEX_UNLOCK(&ab->ah_mutex);
}

STATIC int
chap_no_resp_tmout(proto_hdr_t *ph, act_hdr_t *ab)
{
	struct chap_s *prv = (struct chap_s *)ph->ph_priv;

	if (ab)
		MUTEX_LOCK(&ab->ah_mutex);
	MUTEX_LOCK(&ph->ph_parent->ah_mutex);

	if (prv->chap_local_auth_tmid) {
		psm_log(MSG_DEBUG, ph, "chap_no_resp_tmout: id = %x\n",
		       prv->chap_local_auth_tmid);

		/* Peer failed to CHAP Authenticate */

		if (prv->chap_local_auth_cnt == 0) {
			psm_log(MSG_DEBUG, ph,
		"chap_no_resp_tmout: Timed out .. no ACK or NAK received\n");
			prv->chap_local_status = CHAPS_TIMEOUT;
			prv->chap_local_fail++;
			psm_close_link(ph->ph_parent, ALR_AUTHTMOUT);
		} else
			chap_snd_challenge(ph);
	}

	MUTEX_UNLOCK(&ph->ph_parent->ah_mutex);	
	if(ab)
		MUTEX_UNLOCK(&ab->ah_mutex);
}

/*
 * Called to send a CHAP challenge to the peer (and we require CHAP).
 */
STATIC void
chap_snd_challenge(proto_hdr_t *ph)
{
	db_t *db;
	struct chap_hdr_s *chap;
	uchar_t *name;
	struct chap_s *prv = (struct chap_s *)ph->ph_priv;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	ASSERT(ph->ph_parent->ah_type == DEF_LINK);

	prv->chap_local_status = CHAPS_INITIAL;

	db = db_alloc(LCP_SIZE);
	if (!db) {
		psm_log(MSG_WARN, ph,
			 "Out of memory, could not respond to peer\n");
		prv->chap_local_status = CHAPS_NORES;
		prv->chap_local_fail++;
		return;
	}

	/* Construct a chap challenge */

	psm_add_pf(ph, db);

	chap = (struct chap_hdr_s *)db->db_wptr;
	chap->chap_code = CHAP_CHALLENGE;
	chap->chap_id = ++prv->chap_local_auth_id;

	db->db_wptr += sizeof(struct chap_hdr_s);

	/* Insert the challenge */
	mkchallenge(prv->chap_challenge);

	*db->db_wptr++ = sizeof(prv->chap_challenge);
	memcpy(db->db_wptr, &prv->chap_challenge,
	      sizeof(prv->chap_challenge));
	db->db_wptr += sizeof(prv->chap_challenge);

	/* Insert the name */

	name = (uchar_t *)auth_localname(ph);

	memcpy((char *)db->db_wptr, name, strlen((char *)name));
	db->db_wptr += strlen((char *)name);

	psm_log(MSG_INFO_LOW, ph, "Send Challenge. Name '%s'\n", name);

	free(name);

	chap->chap_len = htons(db->db_wptr - (uchar_t *)chap);

	prv->chap_local_auth_cnt--;

	psm_snd(ph, db);

	/* Setup time out */

	prv->chap_local_auth_tmid = timeout(HZ, chap_no_resp_tmout,
					    (caddr_t)ph,
					    (caddr_t)ph->ph_parent->ah_link.al_bundle);
}


