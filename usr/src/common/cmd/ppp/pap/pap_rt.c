#ident	"@(#)pap_rt.c	1.13"

#include <stdio.h>
#include <assert.h>
#include <dial.h>
#include <errno.h>
#include <sys/conf.h>
#include <synch.h>
#include <sys/ppp.h>
#include <sys/byteorder.h>

#include "ppp_cfg.h"
#include "ppp_type.h"
#include "psm.h"
#include "fsm.h"
#include "act.h"
#include "ppp_proto.h"
#include "pap.h"
#include "auth.h"

#define LCP_SIZE DEFAULT_MRU
#define min(a, b) ((a) < (b) ? (a) : (b))

void auth_open(act_hdr_t *);
STATIC int pap_req_tmout(proto_hdr_t *ph, act_hdr_t *ah);
STATIC void pap_snd_req(proto_hdr_t *);


STATIC int
pap_load()
{
	ppplog(MSG_INFO, 0, "PAP Loaded\n");
	return 0;
}

STATIC int
pap_alloc(proto_hdr_t *ph)
{
	psm_log(MSG_DEBUG, ph, "Alloc\n");

	ph->ph_priv = (void *)malloc(sizeof(struct pap_s));
	if (!ph->ph_priv)
		return ENOMEM;

	return 0;
}

STATIC int
pap_free(proto_hdr_t *ph)
{
	/*printf("pap_free: %x\n", ph);*/
	free(ph->ph_priv);
	ph->ph_priv = NULL;
}

STATIC int
pap_init(struct act_hdr_s *ah, proto_hdr_t *ph)
{
	struct pap_s *pap = (struct pap_s *)ph->ph_priv;

	ASSERT(ah->ah_type == DEF_LINK);

	pap->pap_local_status = PAP_INITIAL;
	pap->pap_peer_status = PAP_INITIAL;
	pap->pap_local_fail = 0;
	pap->pap_peer_fail = 0;

	pap->pap_peer_name[0] = 0;

	return 0;
}

STATIC void
pap_start(proto_hdr_t *ph)
{
	struct pap_s *pap = (struct pap_s *)ph->ph_priv;
	act_hdr_t *ah = ph->ph_parent;

	psm_log(MSG_INFO_LOW, ph, "Started\n");

	pap->pap_local_auth_cnt = ah->ah_link.al_auth_tmout;
	pap->pap_local_auth_tmid = 0;
	pap->pap_peer_auth_cnt = ah->ah_link.al_auth_tmout;
	pap->pap_peer_auth_tmid = 0;

	pap->pap_local_auth_id = 0;
	pap->pap_peer_auth_id = 0;

	pap->pap_local_status = PAP_INITIAL;
	pap->pap_peer_status = PAP_INITIAL;

	pap->pap_peer_name[0] = 0;

	if (ah->ah_link.al_local_opts & ALO_PAP) {
		/* Start the 'waiting for PAP request timer */
		pap->pap_local_auth_tmid =
			timeout(ah->ah_link.al_auth_tmout * HZ,
				pap_req_tmout, (caddr_t)ph,
				(caddr_t)ph->ph_parent->ah_link.al_bundle);

	}

	if (ah->ah_link.al_peer_opts & ALO_PAP) {
		/* Peer requires PAP. Send a PAP Request */
		pap_snd_req(ph);
	}
}

STATIC void
pap_term(proto_hdr_t *ph)
{
	struct pap_s *pap = (struct pap_s *)ph->ph_priv;
	act_hdr_t *ah = ph->ph_parent;

	psm_log(MSG_INFO_LOW, ph, "Terminated\n");

	if (pap->pap_local_auth_tmid) {
		untimeout(pap->pap_local_auth_tmid);
		pap->pap_local_auth_tmid = 0;
	}

	if (pap->pap_peer_auth_tmid) {
		untimeout(pap->pap_peer_auth_tmid);
		pap->pap_peer_auth_tmid = 0;
	}

	/* Check PAP is complete */

	if (ah->ah_link.al_local_opts & ALO_PAP) {
		if (pap->pap_local_status == PAP_INITIAL) {
			pap->pap_local_status = PAP_FAILED;
			pap->pap_local_fail++;
		}
	}

	if (ah->ah_link.al_peer_opts & ALO_PAP) {
		if (pap->pap_peer_status == PAP_INITIAL) {
			pap->pap_peer_status = PAP_FAILED;
			pap->pap_peer_fail++;
		}
	}

}

/*
 * These functions implement the CHAP and PAP protocols
 */
STATIC int
pap_rcv(proto_hdr_t *pr, db_t *db)
{
	struct pap_hdr_s *ph = (struct pap_hdr_s *)db->db_rptr;
	struct pap_response_s *papr;
	ushort_t len;
	int ret = 0;
	db_t *ndb;
	struct pap_hdr_s *nph;
	uchar_t *id, *passwd, idlen, passwdlen, *secret;
	act_hdr_t *ah = pr->ph_parent;
	struct pap_s *pap = (struct pap_s *)pr->ph_priv;
	char *msg = NULL;
	extern pppd_debug;

	ASSERT(ah);
	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));
	ASSERT(ah->ah_type == DEF_LINK);

	/* Check we are in the auth phase */
	if (ah->ah_phase != PHASE_AUTH && ah->ah_phase != PHASE_NETWORK) {
		psm_log(MSG_WARN, pr,
		 "Received when not in AUTH or NETWORK phase. Discard\n");
		db_free(db);
		return;
	}

	/* Check the length is sensible */
	
	if (db->db_wptr - db->db_rptr < sizeof(struct pap_hdr_s)) {
		psm_log(MSG_WARN, pr, "Received frame too short - dumped\n");
		pap->pap_local_status = PAP_BADFMT;
		pap->pap_local_fail++;
		db_free(db);
		return;
	}

	len = ntohs(ph->pap_len);

	if (db->db_wptr - db->db_rptr < len) {
		psm_log(MSG_WARN, pr, "Received frame too short - dumped\n");
		pap->pap_local_status = PAP_BADFMT;
		pap->pap_local_fail++;
		db_free(db);
		return;
	}

	/*
	 * We use the pap_len in preference to the length of the packet
	 * - there may have been padding ... which we don't want to see
	 */
	db->db_wptr = db->db_rptr + len;

	switch (ph->pap_code) {
	case PAP_REQ:
		psm_log(MSG_INFO_LOW, pr, "Received Request\n");

		/* Check we said they could used PAP */

		if (!(ah->ah_link.al_local_opts & ALO_PAP)) {
			/*
			 * We got an unsolicited PAP request, just
			 * send an ACK
			 */
			psm_log(MSG_WARN, pr,
			"Received request when not PAP not negotiated\n");

			if (pppd_debug & DEBUG_ANVL) {
				db_free(db);
				return;
			}

			ret = 0;
			msg = "PAP not negotiated";
			goto reply;
		}

		/* Check the login/password */

		if (len < sizeof(struct pap_hdr_s) + 2) {
		bad_pap_format:
			psm_log(MSG_WARN, pr, "Bad Request packet format\n");
			pap->pap_local_status = PAP_BADFMT;
			pap->pap_local_fail++;
			break;
		}

		/*
		 * IF we have already seen success or failure ... 
		 * reply with what we already decided
		 */
		switch (pap->pap_local_status) {
		case PAP_SUCCESS:
			ret = 0;
			msg = "Authentication successful (not first PAP request)";
			goto reply;
		case PAP_FAILED:
			ret = 1;
			msg = "Authentication failed (not first PAP request)";
			goto reply;
		}

		db->db_rptr += sizeof(struct pap_hdr_s);
		idlen = *db->db_rptr++;
		id = db->db_rptr;
		db->db_rptr += idlen;

		if (db->db_rptr >= db->db_wptr)
			goto bad_pap_format;

		passwdlen = *db->db_rptr++;
		passwd = db->db_rptr;
		db->db_rptr += passwdlen;		

		if (db->db_rptr > db->db_wptr)
			goto bad_pap_format;

		if (pap->pap_local_auth_tmid)
			untimeout(pap->pap_local_auth_tmid);

		sprintf(pap->pap_peer_name, "%*.*s",
			min(idlen, PAP_PNAMELEN),
			min(idlen, PAP_PNAMELEN), id);

		psm_log(MSG_INFO_MED, pr, "Name '%*.*s', Secret '%*.*s'\n",
		       idlen, idlen, id, passwdlen, passwdlen, passwd);

		/*
		 * Save the name our peer used 
		 */
		id = (uchar_t *)auth_peername(pr, id, idlen);
		if (!id) {
			psm_log(MSG_WARN, pr, "Out of memory - dropped\n");
			pap->pap_local_status = PAP_NORES;
			pap->pap_local_fail++;
			break;
		}
		idlen = strlen(id);

		ah->ah_link.al_peer_auth_name = id;
		ah->ah_link.al_peer_auth_namelen = idlen;

		/* Given peername get the password associated, localsecret */

		secret = (uchar_t *)auth_get_secret(pr, LOCALSEC, id, idlen);
		if (!secret) {
			psm_log(MSG_WARN, pr,
				"No localsecret for the name '%s'\n",
				id);
			secret = (uchar_t *)strdup("");
			if (!secret) {
				psm_log(MSG_WARN, pr, "No memory - dropped\n");
				break;
			}				
		}

		if (strlen((char *)secret) == passwdlen &&
		    memcmp(secret, passwd, passwdlen) == 0) {
			ret = 0;
			msg = "Authentication successful";
		} else {
			ret = 1;
			msg = "Authentication failed";
		}

		free(secret);

	reply:
		/* Reply NAK or ACK */

		ndb = db_alloc(sizeof(struct pap_response_s) + 2 + strlen(msg));
		if (!ndb) {
			psm_log(MSG_WARN, pr, "No memory - frame dropped\n");
			pap->pap_local_status = PAP_NORES;
			pap->pap_local_fail++;
			break;
		}

		psm_add_pf(pr, ndb);

		papr = (struct pap_response_s *)ndb->db_wptr;
		papr->pap_hdr.pap_id = ph->pap_id;
		papr->pap_hdr.pap_len = htons(sizeof(struct pap_response_s)
					      + strlen(msg));
		papr->pap_mlen = strlen(msg);
		ndb->db_wptr += sizeof(struct pap_response_s);

		if (strlen(msg) > 0) {
			memcpy(ndb->db_wptr, msg, strlen(msg));
			ndb->db_wptr += strlen(msg);
		}

		if (ret) {
			papr->pap_hdr.pap_code = PAP_NAK;
			pap->pap_local_status = PAP_FAILED;
			pap->pap_local_fail++;
			psm_log(MSG_INFO_LOW, pr,
			"Peer failed PAP authentication (Send Nack)\n"); 
			psm_snd(pr, ndb);
			psm_close_link(ah, ALR_AUTHFAIL);
		} else {
			papr->pap_hdr.pap_code = PAP_ACK;
			psm_log(MSG_INFO_LOW, pr,
			"Peer succeeded PAP authentication (Send Ack)\n"); 
			psm_snd(pr, ndb);
			ah->ah_link.al_flags |= ALF_LOCAL_AUTH;
			pap->pap_local_status = PAP_SUCCESS;
			auth_open(ah);
		}
		break;

	case PAP_ACK:
		papr = (struct pap_response_s *)db->db_rptr;

		psm_log(MSG_INFO_LOW, pr, "Received Ack\n");

		if (len < sizeof(struct pap_response_s)) {
			psm_log(MSG_WARN, pr,
				"Received frame too short - dumped\n");
			pap->pap_local_status = PAP_BADFMT;
			pap->pap_local_fail++;
			break;
		}

		/* Skip to the message field */
		db->db_rptr += sizeof(struct pap_response_s);

		if (papr->pap_mlen > db->db_wptr - db->db_rptr) {
			psm_log(MSG_WARN, pr,
				"Received frame too short (msg) - dumped\n");
			pap->pap_local_status = PAP_BADFMT;
			pap->pap_local_fail++;
			break;
		}

		/* Strict mode for ANVL ... message lengths must be exact */
		if ((pppd_debug & DEBUG_ANVL) &&
		    papr->pap_mlen != db->db_wptr - db->db_rptr) {
			psm_log(MSG_WARN, pr,
				"Received frame has bad length - dumped\n");
			pap->pap_local_status = PAP_BADFMT;
			pap->pap_local_fail++;
			break;
		}

		/* Check the ID matches our Request */
		if (pap->pap_peer_auth_id != papr->pap_hdr.pap_id) {
			psm_log(MSG_WARN, pr,
				 "Discard PAP_ACK with id mis-match\n");
			pap->pap_peer_status = PAP_BADFMT;
			pap->pap_peer_fail++;
			break;
		}

		if (pap->pap_peer_auth_tmid)
			untimeout(pap->pap_peer_auth_tmid);

		/* We have has successfully PAP authenticated */
		ah->ah_link.al_flags |= ALF_PEER_AUTH;
		pap->pap_peer_status = PAP_SUCCESS;

		if (papr->pap_mlen > 0) {
			psm_log(MSG_INFO_LOW, pr, "Message '%*.*s'\n",
				papr->pap_mlen, papr->pap_mlen, db->db_rptr);
		}

		psm_log(MSG_INFO_LOW, pr,
			 "Authentication with Peer Succeeded\n");
		auth_open(ah);
		break;

	case PAP_NAK:
		papr = (struct pap_response_s *)db->db_rptr;

		psm_log(MSG_INFO_LOW, pr, "Received Nak\n");

		if (len < sizeof(struct pap_response_s)) {
			psm_log(MSG_WARN, pr,
				"Received frame too short - dumped\n");
			pap->pap_local_status = PAP_BADFMT;
			pap->pap_local_fail++;
			break;
		}

		/* Skip to the message field */
		db->db_rptr += sizeof(struct pap_response_s);

		if (papr->pap_mlen > db->db_wptr - db->db_rptr) {
			psm_log(MSG_WARN, pr,
				"Received frame too short (msg) - dumped\n");
			pap->pap_local_status = PAP_BADFMT;
			pap->pap_local_fail++;
			break;
		}

		/* Strict mode for ANVL ... message lengths must be exact */
		if ((pppd_debug & DEBUG_ANVL) &&
		    papr->pap_mlen != db->db_wptr - db->db_rptr) {
			psm_log(MSG_WARN, pr,
				"Received frame has bad length - dumped\n");
			pap->pap_local_status = PAP_BADFMT;
			pap->pap_local_fail++;
			break;
		}

		/* Check the ID matches our Request */

		if (pap->pap_peer_auth_id != papr->pap_hdr.pap_id) {
			psm_log(MSG_WARN, pr,
				 "Discard PAP_NAK with id mis-match\n");
			pap->pap_peer_status = PAP_BADFMT;
			pap->pap_peer_fail++;
			break;
		}

		if (pap->pap_peer_auth_tmid)
			untimeout(pap->pap_peer_auth_tmid);

		/* Oops we have failed.  Terminate the link */
		pap->pap_peer_status = PAP_FAILED;
		pap->pap_peer_fail++;
		if (papr->pap_mlen > 0) {
			psm_log(MSG_INFO_LOW, pr, "Message '%*.*s'\n",
				papr->pap_mlen, papr->pap_mlen, db->db_rptr);
		}
		psm_log(MSG_INFO_LOW, pr,
			"Failed to authenticate with Peer\n");
		psm_close_link(ah, ALR_AUTHFAIL);
		break;

	default:
		psm_log(MSG_WARN, pr, "Unknown PAP code (%d)\n",
			 ph->pap_code);
		break;
	}
	db_free(db);

}

STATIC int
pap_status(proto_hdr_t *ph, struct pap_s *pap)
{
	memcpy(pap, ph->ph_priv, sizeof(struct pap_s));
}

/*
 * Table of PSM entry points
 */
psm_tab_t psm_entry = {
	PSM_API_VERSION,
	"PAP",
	PROTO_PAP,
	PT_AUTH | PT_LINK, /* Flags */
	PT_PRI_LOW,

	NULL,

	pap_load,	/* Load */
	NULL,		/* Unload */
	pap_alloc,
	pap_free,
	pap_init,

	pap_rcv,
	NULL,	/* Snd */
	NULL,	/* Log */
	pap_status,

	/* Non-FSM specific fields .. */
	pap_start,
	pap_term,
};

/*
 * This function is called if we don't receive a PAP ACK/NAK 
 *in the specified time
 */
STATIC int
pap_no_ack_tmout(proto_hdr_t *ph, act_hdr_t *ab)
{
	struct pap_s *pap = (struct pap_s *)ph->ph_priv;

	if (ab)
		MUTEX_LOCK(&ab->ah_mutex);
	MUTEX_LOCK(&ph->ph_parent->ah_mutex);

	if (pap->pap_peer_auth_tmid) {

		/* Peer failed to PAP Authenticate */

		if (pap->pap_peer_auth_cnt == 0) {
			pap->pap_peer_status = PAP_TIMEOUT;
			pap->pap_peer_fail++;
			psm_close_link(ph->ph_parent, ALR_AUTHFAIL);
		} else
			pap_snd_req(ph);
	}

	MUTEX_UNLOCK(&ph->ph_parent->ah_mutex);	
	if (ab)
		MUTEX_UNLOCK(&ab->ah_mutex);
}

/*
 * The remote peer requires us to use PAP to authenticate
 */
STATIC void
pap_snd_req(proto_hdr_t *ph)
{
	struct pap_s *prv = (struct pap_s *)ph->ph_priv;
	db_t *db;
	struct pap_hdr_s *pap;
	uchar_t *id, *passwd;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	ASSERT(ph->ph_parent->ah_type == DEF_LINK);

	db = db_alloc(LCP_SIZE);
	if (!db) {
		psm_log(MSG_WARN, ph,
			 "No memory - cannot create Request\n");
		prv->pap_peer_status = PAP_NORES;
		prv->pap_peer_fail++;
		return;
	}

	/* Construct a pap request */

	psm_add_pf(ph, db);

	pap = (struct pap_hdr_s *)db->db_wptr;

	pap->pap_code = PAP_REQ;
	pap->pap_id = ++prv->pap_peer_auth_id;
	db->db_wptr += sizeof(struct pap_hdr_s);

	/* Get the local name/secret pair */

	/* If the peername is set, lookup secret for peername
	 * else
	 * If localname is set lookup secret for localname
	 * else 
	 * use default local name
	 */
	id = (uchar_t *)auth_peername(ph, "", 0);
	if (*id) {
		passwd = (uchar_t *)auth_get_secret(ph, PEERSEC,
						    id, strlen(id));
	} else {
		id = (uchar_t *)auth_localname(ph);
		passwd = (uchar_t *)auth_get_secret(ph, PEERSEC,
						    id, strlen(id));
	}

	if (!passwd) {
		psm_log(MSG_WARN, ph, "No peersecret found for name '%s'.\n",
			id);
		passwd = (uchar_t *)strdup("");
	}

	free(id);

	/*
	 * Now get the name that we will insert in the PAP request,
	 * that is, our name
	 */

	id = (uchar_t *)auth_localname(ph);

	*db->db_wptr++ = (char)strlen((char *)id);
	memcpy(db->db_wptr, id, strlen((char *)id));
	db->db_wptr += strlen((char *)id);


	*db->db_wptr++ = (char)strlen((char *)passwd);
	memcpy(db->db_wptr, passwd, strlen((char *)passwd));
	db->db_wptr += strlen((char *)passwd);
	free(passwd);

	pap->pap_len = htons(db->db_wptr - (uchar_t *)pap);

	prv->pap_peer_auth_cnt--;

	if (prv->pap_peer_auth_tmid)
		untimeout(prv->pap_peer_auth_tmid);

	psm_log(MSG_INFO_LOW, ph, "Sending Request. Name '%s', Secret '%s'\n",
	       id, passwd);

	free(id);

	prv->pap_peer_auth_tmid = timeout(HZ, pap_no_ack_tmout,
					  (caddr_t)ph, 
					  (caddr_t)ph->ph_parent->ah_link.al_bundle);

	/* Send the request */

	psm_snd(ph, db);
}

/*
 * This function is called when a PAP request has not been received
 * in the timeout period
 */
STATIC int
pap_req_tmout(proto_hdr_t *ph, act_hdr_t *ab)
{
	struct pap_s *pap = (struct pap_s *)ph->ph_priv;

	if (ab)
		MUTEX_LOCK(&ab->ah_mutex);
	MUTEX_LOCK(&ph->ph_parent->ah_mutex);

	if (pap->pap_local_auth_tmid) {
		psm_log(MSG_INFO_MED, ph, "Timed out. Closing link.\n");

		/* Peer failed to PAP Authenticate */
		psm_close_link(ph->ph_parent, ALR_AUTHTMOUT);
		pap->pap_local_status = PAP_TIMEOUT;
		pap->pap_local_fail++;
	}
	MUTEX_UNLOCK(&ph->ph_parent->ah_mutex);
	if (ab)
		MUTEX_UNLOCK(&ab->ah_mutex);
}



