#ident	"@(#)fsm.c	1.8"

#include <synch.h>
#include <sys/ppp.h>

#include "ppp_type.h"
#include "fsm.h"
#include "psm.h"
#include "act.h"
#include "ppp_proto.h"
#include "lcp_hdr.h"
#include "ppp_cfg.h"
#include "state_table.h"

/*
 *	FSM states.
 */
char *fsm_states[] = {
	"INITIAL",
	"STARTING",
	"CLOSED",
	"STOPPED",
	"CLOSING",
	"STOPPING",
	"REQSENT",
	"ACKRCVD",
	"ACKSENT",
	"OPENED",
};

/*
 * States in which a restart timer may be running
 */
STATIC uchar_t fsm_timer[] = {
	0,	/* initial */
	0,	/* starting */
	0,	/* closed */
	0,	/* stopped */
	1,	/* closing */
	1,	/* stopping */
	1,	/* req-sent */
	1,	/* ack-rcvd */
	1,	/* ack-sent */
	0,	/* opened */
};

/*
 * FSM events.
 */
STATIC char *fsm_events[] = {
	"Up",
	"Down",
	"Administrative Open",
	"Administrative Close",
	"TimeOut (Count non-zero)",
	"TimeOut (Count Zero)",
	"Receive Configure Request (Acceptable)",
	"Receive Configure Request (Not acceptable)",
	"Receive Configure Ack",
	"Receive Configure Nak/Reject",
	"Receive Terminate Request",
	"Receive Terminate Ack",
	"Receive Unknown Code",
	"Receive Protocol/Code Reject (Acceptable)",
	"Receive Protocol/Code Reject (Unacceptable)",
	"Receive Echo Request/Reply or Discard",
};

/*
 * Load the FSM counters using the configured values
 *
 * This should be called from Control Protocols Start routines.
 */
int
fsm_load_counters(proto_hdr_t *ph)
{
	struct cfg_proto *cp = (struct cfg_proto *)ph->ph_cfg;
	proto_state_t *ps = &ph->ph_state;

	psm_log(MSG_DEBUG, ph, "Loading FSM counters\n");

	ps->ps_timeout = HZ * cp->pr_fsm.fsm_req_tmout;

	ps->ps_max_cfg = cp->pr_fsm.fsm_max_cfg;
	ps->ps_max_trm = cp->pr_fsm.fsm_max_trm;
	ps->ps_max_fail = cp->pr_fsm.fsm_max_fail;

	ps->ps_reason = 0;

	ps->ps_txcnt = 0;
	ps->ps_rxcnt = 0;
	ps->ps_rxbad = 0;
}

/*
 * Initialise FSM fields for a protocol
 *
 * This should be called from protocols Init routines.
 */
int
fsm_init(proto_hdr_t *ph)
{
	proto_state_t *ps = &ph->ph_state;

	psm_log(MSG_DEBUG, ph, "Initialising FSM structure\n");

	ps->ps_state = INITIAL;
	ps->ps_lastid = 0;
	ps->ps_timid = 0;
	ps->ps_badstate = 0;
	fsm_load_counters(ph);

	PH_RESETBITS(ph->ph_cod_rej);
	PH_RESETBITS(ph->ph_rej_opts);
}

/*
 * Set reason codes
 */
void
fsm_reason(proto_hdr_t *pr, short flags)
{
	proto_state_t *ps = &pr->ph_state;

	ASSERT(ps);
	ps->ps_reason |= flags;

	psm_log(MSG_DEBUG, pr, "fsm_reason: new %x, now %x\n",
		flags, ps->ps_reason);
}

/*
 * Perform an untimeout for a psm .. free off any associated
 * retransmit buffer
 */
void
fsm_untimeout(proto_hdr_t *pr)
{
	proto_state_t *ps = &pr->ph_state;

	if (ps->ps_timid) {
		untimeout(ps->ps_timid);
#ifdef DEBUG_TIMEOUT
		psm_log(MSG_DEBUG, pr, "fsm_untimeout: untime %x\n",
			ps->ps_timid);
#endif
		ps->ps_timid = 0;
	}

	if (pr->ph_rtbuf) {
		db_free(pr->ph_rtbuf);
		pr->ph_rtbuf = NULL;
	}
}

/*
 * This function is called when a Configure-Request/Nak/Ack is received.
 * It should be called before any option precessing. 
 */
void
fsm_pre_cfg(proto_hdr_t *pr)
{
	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	psm_log(MSG_DEBUG, pr, "Pre-Receive Configure processing\n");

	if (pr->ph_state.ps_state == OPENED) {
		/*
		 * Perform tld ... nice and early before
		 * Receive Configure Request Processing occurs
		 */
		(*pr->ph_psmtab->pt_act[tld - 1])(pr);
	}
}

/*
 *  PPP State Machine Interface. The bundle/link must be locked.
 */
void
fsm_state(proto_hdr_t *pr, short event, db_t *db)
{
	struct state_tbl_ent *tbl_entry;
	proto_state_t *ps = &pr->ph_state;

	psm_log(MSG_INFO_MED, pr, "FSM Event %s\n", fsm_events[event]);

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));
	ASSERT(event < PPP_EVENTS);
	ASSERT(ps->ps_state < PPP_STATES);
	ASSERT(db ? db->db_ref > 0 : 1);
	ASSERT(pr->ph_inuse == INUSE);
	
	tbl_entry = &ppp_state_tbl[event][ps->ps_state];

	psm_log(MSG_INFO_MED, pr, "     Causes State %s -> %s\n",
	       fsm_states[ps->ps_state], fsm_states[tbl_entry->next_state]);

	/* Initialize counters and timers */

	if (tbl_entry->gen_act)
		(*tbl_entry->gen_act)(pr);

	/* send out packets */

	if (tbl_entry->snd_act)
		/* Send function must ensure db is freed */
		(*tbl_entry->snd_act)(pr, db);
	else if (db)
		db_free(db);

	/* Check if we need to stop the restart timer */

	if (!fsm_timer[tbl_entry->next_state] && ps->ps_timid)
		fsm_untimeout(pr);

	if (pr->ph_psmtab->pt_flags & PT_NCP) {

		ASSERT(pr->ph_parent->ah_type == DEF_BUNDLE);

		if (ps->ps_state == OPENED &&
		    tbl_entry->next_state != OPENED) {
			psm_log(MSG_DEBUG, pr, "NCP Closing ..\n");
			pr->ph_parent->ah_bundle.ab_open_ncps--;
		}

		if (ps->ps_state != OPENED &&
		    tbl_entry->next_state == OPENED) {
			psm_log(MSG_DEBUG, pr, "NCP Opening ..\n");
			pr->ph_parent->ah_bundle.ab_open_ncps++;
		}
	}

	/* move to the next state */

	ps->ps_state = tbl_entry->next_state;

	/* execute selected routine provided by control protocol */

	switch (tbl_entry->ctrl_act) {
	case 0:
		break;
	case c_tld:	/* Crossed and this layer down */
		(*pr->ph_psmtab->pt_act[cross - 1])(pr);
		(*pr->ph_psmtab->pt_act[tld - 1])(pr);
		break;
	default:		
		(*pr->ph_psmtab->pt_act[tbl_entry->ctrl_act - 1])(pr);
		break;
	}
}

/*
 * This function is called to generate and send a configure request.
 * If the db pointer is NULL, a clean config-request is generated,
 * otherwise the db pointer will point to a received configure NAK
 * or REJ, and a configure request will be generated that is modified
 * by that.
 */
void
snd_cnf_req(proto_hdr_t *pr, db_t *db)
{
	db_t *ndb;
	proto_state_t *ps = &pr->ph_state;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	if (db || !pr->ph_rtbuf) {

		/* Free any old configure requests stored */
		if (pr->ph_rtbuf)
			db_free(pr->ph_rtbuf);

		ndb = mk_cfg_req(pr, ++ps->ps_lastid, db);
		if (!ndb) {
			if(db)
				db_free(db);
			return;
		}

		psm_log(MSG_INFO_MED, pr, "Send Configure Request\n");

		pr->ph_rtbuf = db_dup(ndb);
	} else {
		psm_log(MSG_INFO_MED, pr, "Re-Send Configure Request\n");
		db_dup(pr->ph_rtbuf);
	}

	if(db)
		db_free(db);

	ps->ps_retransmits--;
	(*pr->ph_psmtab->pt_snd)(pr, pr->ph_rtbuf);
}

void
snd_trm_req(proto_hdr_t *pr, db_t *db)
{
	db_t *ndb;
	proto_state_t *ps = &pr->ph_state;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	if(db)
		db_free(db);

	if (!pr->ph_rtbuf) {
		ndb = mk_trm(pr, ++ps->ps_lastid, TRM_REQ, NULL);
		if (!ndb)
			return;

		ps->ps_reason |= PR_LTERM;
		pr->ph_rtbuf = db_dup(ndb);
		psm_log(MSG_INFO_MED, pr, "Send Terminate Request\n");
	} else {
		db_dup(pr->ph_rtbuf);
		psm_log(MSG_INFO_MED, pr, "Re-Send Terminate Request\n");
	}

	ps->ps_retransmits--;
	(*pr->ph_psmtab->pt_snd)(pr, pr->ph_rtbuf);
}

void
snd_trm_ack(proto_hdr_t *pr, db_t *db)
{
	db_t *ndb;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	ndb = mk_trm(pr, 0, TRM_ACK, db);
	
	if(db)
		db_free(db);

	if (!ndb)
		return;

	psm_log(MSG_INFO_MED, pr, "Send Terminate Ack\n");

	(*pr->ph_psmtab->pt_snd)(pr, ndb);
}

void
snd_cnf_ack(proto_hdr_t *pr, db_t *db)
{
	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	psm_log(MSG_INFO_MED, pr, "Send Configure Ack\n");

	(*pr->ph_psmtab->pt_snd)(pr, db);
}

/*
 * Combine config-request and config ack
 */
void
snd_cnf_rqack(proto_hdr_t *pr, db_t *db)
{
	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	snd_cnf_req(pr, NULL);
	snd_cnf_ack(pr, db);
}

void
snd_cnf_nak(proto_hdr_t *pr, db_t *db)
{
	proto_state_t *ps = &pr->ph_state;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	if (ps->ps_max_fail > 0) {
		ps->ps_max_fail--;
		psm_log(MSG_INFO_MED, pr, "Send Configure Nak/Rej\n");
	} else {
		struct lcp_hdr_s *lcp = (struct lcp_hdr_s *)(db->db_rptr + 2);
		psm_log(MSG_INFO_MED, pr,
			 "Max failure reached - send Configure Rej\n");
		ps->ps_reason |= PR_MAXFAIL;
		ASSERT(lcp->lcp_code == CFG_REJ);
	}
	(*pr->ph_psmtab->pt_snd)(pr, db);
}

/*
 * Combine config-request and config nak
 */
void
snd_cnf_rqnak(proto_hdr_t *pr, db_t *db)
{
	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	snd_cnf_req(pr, NULL);
	snd_cnf_nak(pr, db);
}

void
snd_cd_rej(proto_hdr_t *pr, db_t *db)
{
	db_t *ndb;
	proto_state_t *ps = &pr->ph_state;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));
	ASSERT(db);

	ndb = mk_cd_rej(pr, ++ps->ps_lastid, db);

	db_free(db);

	if (!ndb)
		return;

	psm_log(MSG_WARN, pr, "Send Code Reject\n");

	(*pr->ph_psmtab->pt_snd)(pr, ndb);
}

void
snd_echo_rply(proto_hdr_t *pr, db_t *db)
{
	db_t *ndb;
	proto_state_t *ps = &pr->ph_state;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	if (PH_TSTBIT(ECO_RPL, pr->ph_cod_rej)) {
		psm_log(MSG_WARN, pr,
			 "Echo request - but can't reply - Code Rejected\n");
		db_free(db);
		return;
	}

	ndb = (db_t *)mk_echo_rply(pr, db);

	db_free(db);

	if (!ndb)
		return;
		
	psm_log(MSG_INFO_MED, pr, "Send Echo Reply\n");

	(*pr->ph_psmtab->pt_snd)(pr, ndb);
}

/*
 * This routine is called when a timeout when waiting for a response to
 * a Configure-Request of Terminate-Request occurs.
 */
int
req_timeout(proto_hdr_t *pr, act_hdr_t *ah)
{
	proto_state_t *ps = &pr->ph_state;

	if (ah)
		MUTEX_LOCK(&ah->ah_mutex);
	MUTEX_LOCK(&pr->ph_parent->ah_mutex);

	/*
	 * Check that we still want this event .. an untimeout could
	 * have occured while we were waiting for the ah_mutex
	 */
	if (ps->ps_timid) {
		psm_log(MSG_INFO_MED, pr,
			 "Restart Timer Expired (Count %d)\n", 
			 ps->ps_retransmits);
#ifdef DEBUG_TIMEOUT
		psm_log(MSG_DEBUG, pr, 
			 "req_timeout: id = %x, ph_rtbuf = %x\n",
			 ps->ps_timid, pr->ph_rtbuf);
#endif
		ps->ps_timid = NULL;
		if (ps->ps_retransmits > 0) {

			fsm_state(pr, TO_P, 0);

			ps->ps_timid = timeout(ps->ps_timeout,
						  req_timeout,
						  (caddr_t)pr, (caddr_t)ah);
		} else {
			if (ps->ps_state != CLOSING &&
			    ps->ps_state != STOPPING)
				ps->ps_reason |= PR_MAXCFG;
			fsm_state(pr, TO_M, 0);
		}
	}
#ifdef DEBUG_TIMEOUT
	else psm_log(MSG_DEBUG, pr, "req_timeout: no longer required !\n");
#endif

	MUTEX_UNLOCK(&pr->ph_parent->ah_mutex);
	if (ah)
		MUTEX_UNLOCK(&ah->ah_mutex);
}

void
init_rst_cnf_cnt(proto_hdr_t *pr, db_t *db)
{
	proto_state_t *ps = &pr->ph_state;
	act_hdr_t *ah;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));
#ifdef DEBUG_TIMEOUT
	psm_log(MSG_DEBUG, pr, "init_rst_cnf_cnt: Protocol %s\n",
		 pr->ph_psmtab->pt_desc);
#endif	

	fsm_untimeout(pr);

	ah = pr->ph_parent->ah_type == DEF_LINK ? pr->ph_parent->ah_link.al_bundle : NULL;
	ps->ps_timid = timeout(ps->ps_timeout, req_timeout,
			       (caddr_t)pr, (caddr_t)ah);

	ps->ps_retransmits = ps->ps_max_cfg;

#ifdef DEBUG_TIMEOUT
	psm_log(MSG_DEBUG, pr, 
		 "init_rst_cnf_cnt: Rst ctr set to %d, tmout in %d, id %x\n",
		 ps->ps_retransmits, ps->ps_timeout, ps->ps_timid);
#endif	
}

void
init_rst_trm_cnt(proto_hdr_t *pr, db_t *db)
{
	proto_state_t *ps = &pr->ph_state;
	act_hdr_t *ah;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

#ifdef DEBUG_TIMEOUT
	psm_log(MSG_DEBUG, pr, "init_rst_trm_cnt: Protocol %s\n",
		 pr->ph_psmtab->pt_desc);
#endif	
	fsm_untimeout(pr);

	ah = pr->ph_parent->ah_type == DEF_LINK ?
		pr->ph_parent->ah_link.al_bundle : NULL;

	ps->ps_timid = timeout(ps->ps_timeout, req_timeout, (caddr_t)pr, (caddr_t)ah);

	ps->ps_retransmits = ps->ps_max_trm;

#ifdef DEBUG_TIMEOUT
	psm_log(MSG_DEBUG, pr, 
		 "init_rst_trm_cnt: Rst ctr set to %d, tmout in %d, id %x\n",
		 ps->ps_retransmits, ps->ps_timeout, ps->ps_timid);
#endif	
}

void
zero_restart_cnt(proto_hdr_t *pr, db_t *db)
{
	proto_state_t *ps = &pr->ph_state;
	act_hdr_t *ah;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));
#ifdef DEBUG_TIMEOUT
	psm_log(MSG_DEBUG, pr, "zero_restart_cnt: Protocol %s\n",
		 pr->ph_psmtab->pt_desc);
#endif	

	fsm_untimeout(pr);

	ps->ps_retransmits = 0;
	ah = pr->ph_parent->ah_type == DEF_LINK ? pr->ph_parent->ah_link.al_bundle : NULL;
	ps->ps_timid = timeout(ps->ps_timeout, req_timeout,
				  (caddr_t)pr, (caddr_t)ah);
#ifdef DEBUG_TIMEOUT
	psm_log(MSG_DEBUG, pr, 
		 "zero_restart_cnt: Rst ctr set to %d, tmout in %d, id %x\n",
		 ps->ps_retransmits, ps->ps_timeout, ps->ps_timid);
#endif	
}

void
illegal_event(proto_hdr_t *pr, db_t *db)
{
	proto_state_t *ps = &pr->ph_state;

	ASSERT(MUTEX_LOCKED(&pr->ph_parent->ah_mutex));

	psm_log(MSG_ERROR, pr, "Illegal Event Occured\n");
	ps->ps_badstate++;
}


