#ident	"@(#)pred1_rt.c	1.2"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <synch.h>
#include <sys/ppp.h>
#include <sys/ppp_psm.h>
#include <sys/pred1comp.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "psm.h"
#include "act.h"
#include "lcp_hdr.h"
#include "ppp_proto.h"
#include "cfg.h"


#pragma pack(1)
struct co_pred1_s {
        struct co_s h; /* type and length bytes */
};

struct pred1_reset_req_s {
        struct lcp_hdr_s rr_hdr;
};


#pragma pack(4)

/*
 * Option number for Predictor Compression
 */
#define ALG_PRED1 1 
#define PRED1_MOD "pred1" /* Kernel PSM name */

STATIC int pred1_modid;

	
/*
 * Called on receipt of a CCP Configure Request containing
 * the Predictor Compression option. It configures the link for compression.
 */
STATIC int
rcv_pred1(proto_hdr_t *ph, int action, db_t *db, db_t *ndb, int state)
{
	struct co_pred1_s *pred1, *npred1;
	alg_hdr_t *ag, *palg;

	/*
	 * Are we configured for this link/bundle ?
	 *
	 * Get a pointer to our private data, if we don't find any
	 * then we aren't configured (Reject if not Naking)
	 */
	ag = psm_find_alg(ph, ALG_PRED1, PSM_TX);
	if (!ag) {
		if (action == CFG_DEFAULT) {
			psm_log(MSG_INFO_MED, ph,
			"(Predictor-1) Default - No Stack Compression on Tx\n");
			return CFG_ACK;
		}
		psm_log(MSG_INFO_MED, ph,
			"(Predictor-1) Not configured for Tx (Rej)\n");
		return (state == CFG_NAK) ? state : CFG_REJ;
	}


	ag->ag_flags &= ~AGF_NEG;

	/*
	 * If the TX primary is set, and it's not us .. then
	 * configure reject this algoritm
	 */
	palg = psm_get_alg(ph, PSM_TX);

	if (palg && palg->ag_id != ALG_PRED1) {
		if (action == CFG_DEFAULT) {
			psm_log(MSG_INFO_MED, ph,
			"(Predictor-1) Default - No Stack Compression on Tx\n");
			return CFG_ACK;
		}
		psm_log(MSG_INFO_MED, ph,
	"(Predictor-1) Not required for Tx (Already have primary) (Rej)\n");
		return (state == CFG_NAK) ? state : CFG_REJ;
	}

	switch (action) {
	case CFG_CHECK:

		if (state == CFG_REJ)
			return CFG_ACK;

		psm_log(MSG_INFO_MED, ph, "(Predictor-1) Peer Requests check (Ack)\n");
		ag->ag_flags |= AGF_NEG;

		/* Set Predictor-1 as the primary algorithm */
		psm_set_alg(ph, PSM_TX, ag);
		return CFG_ACK;

	case CFG_MKNAK:
		goto nak;

	case CFG_DEFAULT:
		psm_log(MSG_INFO_MED, ph,
			"(Predictor-1) Default - No Stack Compression on Tx\n");
		break;
	}

	return CFG_ACK;

nak:
	psm_log(MSG_INFO_MED, ph, "(Predictor-1) Nak'ing \n");
	npred1 = (struct co_pred1_s *)ndb->db_wptr;
	npred1->h.co_type = ALG_PRED1;
	npred1->h.co_len = sizeof(struct co_pred1_s);
	ndb->db_wptr += sizeof(struct co_pred1_s);
	return CFG_NAK;
}

/*
 * This routine is called when we want to request a configuration
 * of Predictor 1 Compression algorithm.
 */
STATIC int
snd_pred1(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_pred1_s *pred1 = (struct co_pred1_s *)db->db_wptr;
	alg_hdr_t *ag;
	struct cfg_pred1 *cb;

	/*
	 * Are we configured for this link/bundle ?
	 *
	 * Get a pointer to our private data, if we don't find any
	 * then we aren't configured (Reject if not Naking)
	 */
	ag = psm_find_alg(ph, ALG_PRED1, PSM_RX);
	if (!ag) {
		psm_log(MSG_INFO_MED, ph,
			"(Predictor-1) Not configured for Rx\n");
		return;
	}

	ag->ag_flags &= ~AGF_NEG;
	pred1->h.co_type = ALG_PRED1;
        pred1->h.co_len = sizeof(struct co_pred1_s);

	switch(state) {
	case CFG_REJ:
		psm_log(MSG_INFO_MED, ph,
			"(Predictor-1) Was Rejected\n");		
		return;

 	case CFG_NAK:

	 	psm_log(MSG_INFO_MED, ph,
			"(Predictor-1) Was NAK'ed\n");
		break;

	case CFG_ACK:
		cb = (struct cfg_pred1 *)ag->ag_cfg;
		psm_log(MSG_INFO_MED, ph,
			"(Predictor-1) Request Ack\n");
		break;
	}
	
	/*
	 * All algorithms should claim to be the
	 * primary receive algorithm. The last alg to claim to be
	 * primary will be it.... 
	 */
	psm_set_alg(ph, PSM_RX, ag);
	ag->ag_flags |= AGF_NEG;
	db->db_wptr += sizeof(struct co_pred1_s);
}


STATIC int
rcv_reset_req(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct pred1_reset_req_s *rr = (struct pred1_reset_req_s *)db->db_rptr;
	alg_hdr_t *ag;
	struct pred1_msg_s pm;

	psm_log(MSG_INFO_MED, ph, "(Predictor-1) Received Reset Request\n");

	/* Validate the packet */
        if (ntohs(rr->rr_hdr.lcp_len) != sizeof(struct pred1_reset_req_s)) {
                psm_log(MSG_WARN, ph,
                "(Predictor-1) Reset Request - Frame bad size\n");
                return CFG_DISCARD;
        }

	ag = psm_find_alg(ph, ALG_PRED1, PSM_TX);
	if (!ag) {
		psm_log(MSG_WARN, ph,
			"(Predictor-1) Not configured for Tx\n");
		return;
	}

#define CCP_RESET_REQ 14
#define CCP_RESET_ACK 15

	/*
	 * Tell the Kernel out PSM needs to reset its 
	 * Compression Dictionary 
	 */
	pm.pm_cmd = PRED1_RESET_CMD;

	cd_d2k_psm_msg(ph->ph_parent, PROTO_CCPDG, PSM_TX,
		       &pm, sizeof(struct pred1_msg_s));

	/* 
	 *Generate an Ack ... steal the request ... 
	 */
	*ndb = db_dup(db);
	(*ndb)->db_rptr -= 2; /* Get back to the Protocol Field */
	rr->rr_hdr.lcp_code = CCP_RESET_ACK;

	psm_log(MSG_INFO_MED, ph, "(Predictor-1) Sending Reset Ack\n");

	return CFG_DISCARD;
}

STATIC void
snd_reset_req(proto_hdr_t *ph, ushort_t proto)
{
	db_t *db;
	struct pred1_reset_req_s *rr;
	ushort_t *pf;

	db = db_alloc(PSM_MTU(ph->ph_parent));
	if (!db) {
		psm_log(MSG_WARN, ph, "(Predictor-1) No memory\n");
		return;
	}

	psm_set_pri(ph, db);

	/* Insert the protocol field */
	ASSERT(proto == PROTO_CCPDG || proto == PROTO_ILCCPDG);
	pf = (ushort_t *)db->db_wptr;
	*pf = htons(proto | 0x8000);

	db->db_wptr += sizeof(ushort_t);

	rr = (struct pred1_reset_req_s *)db->db_wptr;
	rr->rr_hdr.lcp_code = CCP_RESET_REQ;
	rr->rr_hdr.lcp_id = ++ph->ph_state.ps_lastid;
	rr->rr_hdr.lcp_len = htons(sizeof(struct pred1_reset_req_s));
	db->db_wptr += sizeof(struct pred1_reset_req_s);

	psm_log(MSG_INFO_MED, ph, "(Predictor-1) Sending Reset Request\n");
	psm_snd(ph, db);
}


STATIC void
rcv_reset_ack(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct pred1_reset_req_s *ra = (struct pred1_reset_req_s *)db->db_rptr;
	alg_hdr_t *ag;
	struct pred1_msg_s pm;

	psm_log(MSG_INFO_MED, ph,
		"(Predictor-1) Received Reset Ack \n");

	/* Tell the kernel to reset the dictionary */

	pm.pm_cmd = PRED1_RESET_CMD;

	cd_d2k_psm_msg(ph->ph_parent, PROTO_CCPDG, PSM_RX,
		       &pm, sizeof(struct pred1_msg_s));
	
}


STATIC int
pred1_load()
{
	/* modload the kernel support */
	pred1_modid = modload(PRED1_MOD);
	if (pred1_modid < 0) {
		ppplog(MSG_ERROR, 0,
		       "Predictor-1 Compression Failed to load kernel support, errno %d\n",
		       errno);
		return -1;
	}

	psm_log(MSG_INFO, 0, "Predictor-1 Compression Loaded\n");
	return 0;
}

STATIC int
pred1_unload()
{
	int ret;

	/* moduload the kernel support */

	if (pred1_modid >= 0) {
		do { 
			ret = moduload(pred1_modid);
			if (ret >= 0) {
				errno = 0;
				break;
			}
			switch (errno) {
			case EBUSY:
				nap(500);
				break;
			default:
				ppplog(MSG_ERROR, 0,
				       "Predictor-1 Compression Failed to unload kernel support, errno %d\n",
				       errno);
				break;
			}
		} while (errno == EBUSY);
	}

	psm_log(MSG_INFO, 0, "Predictor-1 Compression Unloaded\n");

}

STATIC int
pred1_alloc(proto_hdr_t *ph, struct alg_hdr_s *ag)
{
	/* no config data */
	ag->ag_priv = NULL;
	return 0;
}

STATIC int
pred1_free(proto_hdr_t *ph, struct alg_hdr_s *ag)
{
	ag->ag_priv = NULL;
}

STATIC int
pred1_init(proto_hdr_t *ph, struct alg_hdr_s *ag)
{
	psm_log(MSG_DEBUG, ph, "pred1_init:\n");
	ag->ag_flags &= ~(AGF_NEG);
	return 0;
}

STATIC int
pred1_rcv(proto_hdr_t *ph, db_t *db)
{
	psm_log(MSG_DEBUG, ph, "Predictor-1 Compression pred1_rcv:\n");	
	return CFG_ACK;
}

STATIC int
pred1_snd(proto_hdr_t *ph, db_t *db)
{
	psm_log(MSG_DEBUG, ph, "Predictor-1 Compression pred1_snd:\n");	
}

STATIC void
pred1_up(proto_hdr_t *ph, alg_hdr_t *ag, int flags)
{
        act_hdr_t *ah = ph->ph_parent;

	switch (flags) {
	case PSM_TX:
		psm_log(MSG_INFO_LOW, ph,
			"Predictor-1 Compression Configured for Tx\n");

		/* Tell the kernel to start transmitting compressed */
		cd_d2k_bind_psm(ah, PROTO_PRED1, PROTO_CCPDG,
				PSM_TX, NULL, 0);
/* Need to handle link/bundle compression */
		break;

	case PSM_RX:
		psm_log(MSG_INFO_LOW, ph,
			"Predictor-1 Compression Configured for Rx\n");

		/* Tell the kernel to start receiving compressed */
		cd_d2k_bind_psm(ah, PROTO_PRED1, PROTO_CCPDG,
				PSM_RX, NULL, 0);
/* Need to handle link/bundle compression */
		break;
	}
}

STATIC void
pred1_down(proto_hdr_t *ph, alg_hdr_t *ag, int flags)
{
	act_hdr_t *ah = ph->ph_parent;

	switch (flags) {
	case PSM_TX:
		psm_log(MSG_INFO_LOW, ph,
			"Predictor-1 Compression De-Configured for Tx\n");
		/* Tell the kernel to stop transmitting compressed */
		cd_d2k_unbind_psm(ah, PROTO_PRED1, PROTO_CCPDG, PSM_TX);
/* Need to handle link/bundle compression */
		break;

	case PSM_RX:
		psm_log(MSG_INFO_LOW, ph,
			"Predictor-1 Compression De-Configured for Rx\n");
		/* Tell the kernel to stop receiving compressed */
		cd_d2k_unbind_psm(ah, PROTO_PRED1, PROTO_CCPDG, PSM_RX);
/* Need to handle link/bundle compression */
		break;
	}

}

STATIC void
k2d_msg(proto_hdr_t *ph, ushort_t proto, ushort_t flags,
	void *arg, ushort_t argsz)
{

	struct pred1_msg_s *pm = (struct pred1_msg_s *)arg;

	psm_log(MSG_DEBUG, ph, "(Predictor-1 Compression) Got k2d\n");
	ASSERT(argsz == sizeof(struct pred1_msg_s));

	switch (flags) {
	case PSM_TX:
		psm_log(MSG_ERROR, ph,
			"(Predictor-1) Unexpected PSM_TX kernel message\n");
		break;
	case PSM_RX:
		switch (pm->pm_cmd) {
                case PRED1_RESET_CMD:
			psm_log(MSG_INFO_MED, ph,
		"(Predictor-1) Received PRED1_RESET_CMD from kernel\n");
			snd_reset_req(ph, proto);
			break;
		case PRED1_CONFIGREQ_CMD:
			/* rfc 1978 says to send a config request upon error */
			psm_log(MSG_INFO_MED, ph,
		"(Predictor-1) Received PRED1_CONFIGREQ_CMD from kernel\n");
			fsm_state(ph, OPEN, 0);
			break;
		default:
			psm_log(MSG_INFO_MED, ph,
		"(Predictor-1) Unexpected PSM_RX Command (%d) from kernel\n", pm->pm_cmd);
			break;
		}
		break;

	}
}

#ifdef NOT
STATIC ushort_t pred1_protos[] = {
	PROTO_PRED1,
	0,
};
#endif

STATIC struct psm_opt_tab_s pred1_opt[] = {
	rcv_pred1,
	snd_pred1,
	OP_LEN_EXACT,
	2,
	"Predictor-1",
};

struct psm_tab_s psm_entry = {
	PSM_API_VERSION,
	"Predictor-1",
	0,
	PT_COMP,
	PT_PRI_HI,

	NULL, 		/* Proto list */

	pred1_load,	/* load */
	pred1_unload,	/* Unload */
	pred1_alloc,
	pred1_free,
	pred1_init,

	NULL,
	NULL,
	NULL,		/* log */
	NULL,		/* status */

	(void (*)()) rcv_reset_req,
	rcv_reset_ack,
	pred1_up,
	pred1_down,
	(void (*)())k2d_msg,

	NULL,	/*Pad*/

	pred1_opt,		/* pred1_opt */
	ALG_PRED1,
};


