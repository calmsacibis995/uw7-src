#ident	"@(#)ccp_rt.c	1.6"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <synch.h>
#include <sys/ppp.h>
#include <sys/ppp_psm.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "psm.h"
#include "act.h"
#include "lcp_hdr.h"
#include "ppp_proto.h"
#include "cfg.h"

STATIC int ccp_load();

#define NUM_CCP_CO 255
STATIC struct psm_opt_tab_s *ccp_opt_tab;
STATIC int add_algorithm(proto_hdr_t *ph, struct psm_tab_s *pt);


/*
 * CCP Code handlers
 */
STATIC int
rcv_reset_req(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	alg_hdr_t *ag;
	int ret;

	psm_log(MSG_INFO_MED, ph, "Received Reset-Request\n");

	ag = psm_get_alg(ph, PSM_TX);

	if (!ag) {
		psm_log(MSG_WARN, ph,
		"No Primary Transmit Compression Algorithm - Discard\n");
		return CFG_DISCARD;
	}

	/* Pass on to the Primary TX algorithm PSM */
	if (ag->ag_psmtab->pt_rcv_resetreq) {
		*ndb = NULL;
		ret = (*ag->ag_psmtab->pt_rcv_resetreq)(ph, db, ndb);
	} else {
		psm_log(MSG_WARN, ph,
			"Primary Algorithm doesn't support Reset Requests\n");
		return CFG_DISCARD;
	}

	/* Send any generated message (reset ack) */

	if (ndb) {
		psm_log(MSG_INFO_MED, ph, "Sending Reset Ack\n");
		psm_snd(ph, *ndb);
	}
	return CFG_DISCARD;
}

STATIC int
rcv_reset_ack(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	alg_hdr_t *ag;

	psm_log(MSG_INFO_MED, ph, "Received Reset-Ack\n");

	/* Find the alorithms PSM ... call the reset ack function */

	ag = psm_get_alg(ph, PSM_RX);
	if (!ag) {
		psm_log(MSG_WARN, ph,
		"No Primary Transmit Compression Algorithm - Discard\n");
		return CFG_DISCARD;
	}

	/* Pass on to the Primary TX algorithm PSM */

	if (ag->ag_psmtab->pt_rcv_resetack)
		(*ag->ag_psmtab->pt_rcv_resetack)(ph, db, ndb);
	else
		psm_log(MSG_WARN, ph,
			"Primary Algorithm doesn't support Reset Acks\n");
	return CFG_DISCARD;
}


STATIC int
ccp_alloc(struct proto_hdr_s *ph)
{
	struct alg_s *as;

	ph->ph_priv = (void *)malloc(sizeof(struct alg_s));
	if (!ph->ph_priv)
		return ENOMEM;

	as = (struct alg_s *)ph->ph_priv;
	as->as_tx_algs = NULL;
	as->as_rx_algs = NULL;
	psm_set_alg(ph, PSM_TX, NULL);
	psm_set_alg(ph, PSM_RX, NULL);
	return 0;
}

STATIC void
ccp_free_list(struct proto_hdr_s *ph, alg_hdr_t *ag)
{
	alg_hdr_t *nxt;
	struct psm_tab_s *pt;

	while (ag) {
		nxt = ag->ag_next;
		pt = ag->ag_psmtab;
		if (pt)
			(*pt->pt_free)(ph, ag);
		free(ag);
		ag = nxt;
	}
}

STATIC int
ccp_free(struct proto_hdr_s *ph)
{
	struct alg_s *as = (struct alg_s *)ph->ph_priv;
	
	ccp_free_list(ph, as->as_tx_algs);
	ccp_free_list(ph, as->as_rx_algs);

	free(ph->ph_priv);
	ph->ph_priv = NULL;
}

/*
 *  This-Layer-Up (tlu)
 *
 *      This action indicates to the upper layers that the automaton is
 *      entering the Opened state.
 *
 *      Typically, this action is used by the LCP to signal the Up event
 *      to a NCP, Authentication Protocol, or Link Quality Protocol, or
 *      MAY be used by a NCP to indicate that the link is available for
 *      its network layer traffic.
 */
STATIC void
ccp_up(proto_hdr_t *ph)
{
	alg_hdr_t *ag;
	int num = 0;

	psm_log(MSG_INFO_LOW, ph, "Up\n");

	/*
	 * Tell the tx and rx algorthms they are UP
	 */
	ag = psm_get_alg(ph, PSM_TX);
	if (ag && ag->ag_psmtab->pt_alup) {
		(*ag->ag_psmtab->pt_alup)(ph, ag, PSM_TX);
		num++;
	}

	ag = psm_get_alg(ph, PSM_RX);
	if (ag && ag->ag_psmtab->pt_alup) {
		(*ag->ag_psmtab->pt_alup)(ph, ag, PSM_RX);
		num++;
	}

	if (num == 0) {
		psm_log(MSG_INFO_LOW, ph,
			"No algorithms were selected - Close protocol\n");
		fsm_state(ph, CLOSE, 0);
	}
}

/*
 *   This-Layer-Down (tld)
 *
 *      This action indicates to the upper layers that the automaton is
 *      leaving the Opened state.
 *
 *      Typically, this action is used by the LCP to signal the Down event
 *      to a NCP, Authentication Protocol, or Link Quality Protocol, or
 *      MAY be used by a NCP to indicate that the link is no longer
 *      available for its network layer traffic.
 */
STATIC void
ccp_down(proto_hdr_t *ph)
{
	alg_hdr_t *ag;
	int num = 0;

	psm_log(MSG_INFO_LOW, ph, "Down\n");

	/*
	 * Tell the tx and rx algorthms they are DOWN
	 */
	ag = psm_get_alg(ph, PSM_TX);
	if (ag && ag->ag_psmtab->pt_aldown) {
		(*ag->ag_psmtab->pt_aldown)(ph, ag, PSM_TX);
		ag->ag_flags &= ~AGF_NEG;
	}

	ag = psm_get_alg(ph, PSM_RX);
	if (ag && ag->ag_psmtab->pt_aldown) {
		(*ag->ag_psmtab->pt_aldown)(ph, ag, PSM_RX);
		ag->ag_flags &= ~AGF_NEG;
	}

	psm_set_alg(ph, PSM_TX, NULL);
	psm_set_alg(ph, PSM_RX, NULL);

	if (ph->ph_state.ps_state == REQSENT ||
	    ph->ph_state.ps_state == ACKSENT) {
		psm_log(MSG_DEBUG, ph, "Re-starting .. reset counters\n");
		/* Reset counters */
		fsm_load_counters(ph);
		PH_RESETBITS(ph->ph_rej_opts);
	}
}

/*
 *  This-Layer-Started (tls)
 *
 *      This action indicates to the lower layers that the automaton is
 *      entering the Starting state, and the lower layer is needed for the
 *      link.  The lower layer SHOULD respond with an Up event when the
 *      lower layer is available.
 *
 *      This results of this action are highly implementation dependent.
 */
STATIC void
ccp_start(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_LOW, ph, "Starting\n");

	fsm_load_counters(ph);
	PH_RESETBITS(ph->ph_cod_rej);
	PH_RESETBITS(ph->ph_rej_opts);

	fsm_state(ph, UP, 0);
}

/*
 *   This-Layer-Finished (tlf)
 *
 *      This action indicates to the lower layers that the automaton is
 *      entering the Initial, Closed or Stopped states, and the lower
 *      layer is no longer needed for the link.  The lower layer SHOULD
 *      respond with a Down event when the lower layer has terminated.
 *
 *      Typically, this action MAY be used by the LCP to advance to the
 *      Link Dead phase, or MAY be used by a NCP to indicate to the LCP
 *      that the link may terminate when there are no other NCPs open.
 *
 *      This results of this action are highly implementation dependent.
 */
STATIC void
ccp_finish(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_LOW, ph, "Finished\n");

	/*	pppstate(d, fsm, DOWN, 0);*/
	/* If we are in the stopped state, this causes us to proceed to starting !! */
}

/*
 * Adminitrative UP when not in CLOSED or INITIAL
 */
STATIC void
ccp_restart(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_LOW, ph, "Restart\n");

	/* Down -> Up */
	fsm_state(ph, DOWN, 0);
	fsm_state(ph, UP, 0);
}

STATIC void
ccp_crossed(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	printf("ccp_crossed:\n");
	abort(0);
}

/*
 * Return 1 - failure
 * Return 0 - Success
 */
STATIC int
ccp_init_alg(proto_hdr_t *ph, alg_hdr_t **ah,
	     struct psm_tab_s *pt, struct cfg_alg *ca)
{
	alg_hdr_t *ag;
	int ret;

	ag = (alg_hdr_t *)malloc(sizeof(alg_hdr_t));
	if (!ag) {
		psm_log(MSG_WARN, ph, "Out of Memory !\n");
		return 1;
	}

	/* Initialise the desciption */

	ag->ag_inuse = INUSE; /*DEBUG*/
	ag->ag_cfg = ca;

	ag->ag_id = pt->pt_algnum;
	ag->ag_psmtab = pt;
	ag->ag_flags = 0;

	/* Put it in the chain */
	ag->ag_next = *ah;
	*ah = ag;

	/* Configure it */
	if (pt->pt_alloc) {
		ret = (*pt->pt_alloc)(ph, ag);
		if (ret)
			goto err_out;

	}

	if (pt->pt_init) {
		ret = (*pt->pt_init)(ph, ag);
		if (ret)
			goto err_out;
	}

	ca->al_ch.ch_refcnt++;
	return 0;

 err_out:
	*ah = ag->ag_next;
	free(ag);
	return 1;
}

/*
 * Return number of algorithms successfully added
 */
STATIC int
ccp_init_list(struct act_hdr_s *ah, struct proto_hdr_s *ph, char *p, int flags)
{
	int ret, donesome = 0;
	char  *alg, buf[MAXID];
	struct psm_tab_s *pt;
	struct cfg_alg *ca;
	struct alg_s *as = (struct alg_s *)ph->ph_priv;

	while (*p) {
		
		p = ucfg_get_element(p, buf);

		psm_log(MSG_DEBUG, ph, 
			"ccp_init: add algotithm '%s', flags 0x%x\n",
			buf, flags);

		/* Get the algorithm */

		ret = ucfg_findid(buf, DEF_ALG, (cfg_hdr_t *)&ca);
		if (ret) {
			psm_log(MSG_ERROR, ph,
				"Algorithm not found '%s'\n", buf);
			goto err_out;
		}

		alg = ucfg_str(&ca->al_ch, ca->al_name);

	       /* Ensure algorithm is loaded */

		pt = psm_getpt(alg);
		if (!pt) {
			psm_log(MSG_ERROR, ph,
				"Failed to get support for Algorithm  '%s'\n",
				buf);
			ucfg_release();
			goto err_out;
		}

		ca->al_ch.ch_refcnt++;
		ucfg_release();

		/* Ensure it's a compression algorithm */
		if (!(pt->pt_flags & PT_COMP)) {
			psm_log(MSG_ERROR, ph,
			"Algorithm %s is not declared as compression.\n",
				alg);
				goto err_out;
		}
		
		/* Update the ccp options table for this algorithm */
		ccp_add_algorithm(ph, pt);

		if (flags & PSM_TX)
			if (ccp_init_alg(ph, &as->as_tx_algs, pt, ca))
				goto err_out;

		if (flags & PSM_RX)
			if (ccp_init_alg(ph, &as->as_rx_algs, pt, ca))
				goto err_out;

		ca->al_ch.ch_refcnt--;
		donesome++;
	}
	return donesome;

 err_out:
	psm_log(MSG_DEBUG, ph, "OOPS ... ERROR\n");
	/* Free stuff off */
	return 0;
}

/*
 * If we have transmit or receive algorithms configured,
 * establish a list of tx and rx algorithm data structures.
 */
STATIC int
ccp_init(struct act_hdr_s *ah, struct proto_hdr_s *ph)
{
	struct cfg_ccp *cp = (struct cfg_ccp *)ph->ph_cfg;
	char *p;
	int donesome = 0;

	fsm_init(ph);

	/* Check all the specified protocols are loaded */

	p = ucfg_str(&cp->cp_ch, cp->cp_alg);
	if (!ccp_init_list(ah, ph, p, PSM_TX | PSM_RX)) {

		p = ucfg_str(&cp->cp_ch, cp->cp_txalg);
		if (ccp_init_list(ah, ph, p, PSM_TX))
			donesome = 1;

		p = ucfg_str(&cp->cp_ch, cp->cp_rxalg);
		if (ccp_init_list(ah, ph, p, PSM_RX))
			donesome = 1;

		if (!donesome) {
			psm_log(MSG_WARN, ph, "No algorithms specified\n");
			return EINVAL;
		}
	}
	return 0;
}

STATIC int
ccp_rcv(proto_hdr_t *ph, db_t *db)
{
	if (ph->ph_parent->ah_phase != PHASE_NETWORK) {
		psm_log(MSG_WARN, ph,
			 "Got packet before NETWORK phase reached\n");
		return -1;
	}
	return psm_rcv(ph, db);
}

STATIC int
ccp_snd(proto_hdr_t *ph, db_t *db)
{
	return psm_snd(ph, db);
}

STATIC int
ccp_k2d_msg(proto_hdr_t *ph, ushort_t proto, ushort_t flags,
	    void *arg, ushort_t argsz)
{
	alg_hdr_t *ag;

	psm_log(MSG_DEBUG, ph,
		"Got a kernel message, proto 0x%4.4x, flags %d\n",
		proto, flags);

	/*
	 * If the protocol is the PROTO_CCP or PROTO_ILCCP
	 * then the message is for the primary algorithm
	 * else ... it's for a secondary
	 */
	switch (proto) { 
	case PROTO_CCPDG:
	case PROTO_ILCCPDG:
		ag = psm_get_alg(ph, flags);
		break;
	default:
		ag = NULL;
		break;
	}

	if (!ag) {
		psm_log(MSG_WARN, ph,
		"Received kenel message for unknown protocol 0x%4.4x\n", 
			proto);
		return;
	}

	if (ag->ag_psmtab->pt_alk2d_msg)
		(*ag->ag_psmtab->pt_alk2d_msg)(ph, proto, flags, arg, argsz);
	else
		psm_log(MSG_WARN, ph,
			"No kernel support for protocol 0x%4.4x\n", proto);

}

STATIC int
ccp_status(proto_hdr_t *ph, struct ccp_status_s *st)
{
	st->st_fsm = ph->ph_state;
	/*st->st_ip = *(struct ipcp_s *)ph->ph_priv;*/
}

STATIC struct psm_code_tab_s ccp_codes[] = {

	14,
	PC_ACCEPT | PC_NOREJ,
	rcv_reset_req,
	"Reset-Request",

	15,
	PC_ACCEPT | PC_NOREJ | PC_CHKID,
	rcv_reset_ack,
	"Reset-Ack",

	0xffff, 0, NULL, NULL,
};

STATIC ushort_t ccp_protos[] = {
	PROTO_CCPDG,
	PROTO_ILCCPDG,
	0,
};

struct psm_tab_s psm_entry = {
	PSM_API_VERSION,
	"CCP",
	PROTO_CCP,
	PT_FSM | PT_BUNDLE,
	PT_PRI_MED,

	ccp_protos, 	/* Proto list */

	ccp_load,	/* load */
	NULL,		/* Unload */
	ccp_alloc,
	ccp_free,
	ccp_init,

	ccp_rcv,
	ccp_snd,
	NULL,		/* log */
	ccp_status,	/* status */

	/* FSM Specific fields */

	ccp_up,
	ccp_down,
	ccp_start,
	ccp_finish,
	ccp_restart,
	ccp_crossed,

	/* FSM Option support */
	NULL,
	0,

	/* Extended Codes */

	ccp_codes,	/* Codes for ccp */

	(int (*)())ccp_k2d_msg,
};


STATIC int
ccp_load()
{
	int i, num_protos;

	/* Allocate space for the options table */

	ccp_opt_tab = (struct psm_opt_tab_s *)
		malloc(sizeof(struct psm_opt_tab_s) * (NUM_CCP_CO + 1));

	/* Initialise the table */
	for (i = 0; i <= NUM_CCP_CO; i++) {
		ccp_opt_tab[i].op_rcv = NULL;
		ccp_opt_tab[i].op_snd = NULL;
		ccp_opt_tab[i].op_flags = 0;
		ccp_opt_tab[i].op_len = 0;
		ccp_opt_tab[i].op_name = "Not Supported";
	}

	psm_entry.pt_option = ccp_opt_tab;
	psm_entry.pt_numopts = NUM_CCP_CO;

	psm_log(MSG_INFO, 0, "CCP Loaded\n");
	return 0;
}

/*
 * Check that support for the specified algorithm has been
 * added to the CCP options table .. if not add it
 */
STATIC int
ccp_add_algorithm(proto_hdr_t *ph, struct psm_tab_s *pt)
{
	int alg_num = pt->pt_algnum;
	int num_new, num_old, i;
	ushort_t *new_proto;

	psm_log(MSG_DEBUG, ph, "Adding support for '%s'\n", pt->pt_desc);

	if (ccp_opt_tab[alg_num].op_rcv)
		/* Already got this one */
		return;

	/* Update to options table */
	ccp_opt_tab[alg_num] = *pt->pt_algoption;
}
