#ident	"@(#)phase.c	1.10"

#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <synch.h>
#include <thread.h>
#include <errno.h>
#include <sys/ppp.h>
#include <sys/ppp_pcid.h>
#include <sys/ppp_ml.h>
#include <sys/ppp_psm.h>

#include "pathnames.h"
#include "psm.h"
#include "ppp_type.h"
#include "ppp_proto.h"
#include "ppp_cfg.h"
#include "act.h"
#include "hist.h"

STATIC int illegal(struct act_hdr_s *ah);
STATIC int phase_lcp_start(struct act_hdr_s *ah);
STATIC int phase_auth_start(struct act_hdr_s *ah);
STATIC int phase_start_bundle(struct act_hdr_s *ah);
STATIC int phase_bundle_up(struct act_hdr_s *ah);
STATIC int phase_start_link(struct act_hdr_s *ah);
STATIC int phase_term_start(struct act_hdr_s *ah);
STATIC int phase_dead(struct act_hdr_s *ah);

char *phase_name[] = {
	"DEAD",
	"ESTABLISH",
	"AUTHENTICATE",
	"NETWORK",
	"TERMINATE",
};

struct phase_entry_s {
	int	(*pe_act)();	/* Action to perform */
};

struct phase_entry_s link_phase[NUM_PHASE] = {
	phase_dead,		/* Phase Dead */       
	NULL,			/* Phase Estab */
	phase_auth_start,	/* Phase Auth */
	phase_start_link,	/* Phase Network */
	phase_term_start,	/* Phase Term */
};
	
struct phase_entry_s bundle_phase[NUM_PHASE] = {
	phase_dead,		/* Phase Dead */
	illegal,		/* Phase Estab */
	illegal,		/* Phase Auth */
	phase_start_bundle,	/* Phase Network */
	phase_term_start,	/* Phase Term */
};

/*
 * This module implements a phase state machine for links and bundles
 * One of the main functions is to detect when we leave/enter the network
 * phase .... it then informs the kernel of the state changes.
 */
ppp_phase(struct act_hdr_s *ah, int phase)
{
	struct phase_entry_s *pe;

	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));
	ASSERT(phase >= PHASE_DEAD && phase < NUM_PHASE);

	if (ah->ah_phase == phase)
		return;

	ppplog(MSG_INFO_LOW, ah, "Phase was %s, Phase Now %s\n",
	       phase_name[ah->ah_phase], phase_name[phase]);
	
	switch(ah->ah_type) {
	case DEF_LINK:
		pe = &link_phase[phase];
		break;
	case DEF_BUNDLE:
		pe = &bundle_phase[phase];
		break;
	default:
		ppplog(MSG_DEBUG, ah, "aborting ... not link not bundle\n");
		abort();
	}

	if (ah->ah_phase == PHASE_NETWORK && ah->ah_type == DEF_BUNDLE) {
		ppplog(MSG_INFO_MED, ah,
		       "Bundle Leaving NETWORK phase, notify kernel\n");
		cd_d2k_adm_bundle(ah, CMD_CLOSE, 0);
	}

	ah->ah_phase = phase;
	if (pe->pe_act)
		(*pe->pe_act)(ah);
}

STATIC int
illegal(struct act_hdr_s *ah)
{
	act_display(ah);
	ppplog(MSG_ERROR, ah, "Illegal phase\n");
	ASSERT(0);
}

STATIC int
phase_auth_start(struct act_hdr_s *ah)
{
	struct proto_hdr_s *ph;

	ASSERT(ah->ah_type == DEF_LINK);
	auth_init(ah);
}

STATIC int
phase_start_bundle(act_hdr_t *ah)
{
	proto_hdr_t *ph;

	ASSERT(ah->ah_type == DEF_BUNDLE);

	ah->ah_bundle.ab_reason = 0;

	ppplog(MSG_INFO_MED, ah, "Entering NETWORK phase, notify kernel\n");
	cd_d2k_adm_bundle(ah, CMD_OPEN, 0);

	nap(100);

	ppplog(MSG_DEBUG, ah, "phase_start_bundle: Start NCP's etc\n");

	ph = ah->ah_protos;
	while(ph) {
		if (ph->ph_psmtab->pt_flags & PT_FSM) {
			ppplog(MSG_INFO_LOW, ah, "Start Protocol %s\n",
			       ph->ph_psmtab->pt_desc);

			fsm_init(ph);
			fsm_state(ph, OPEN, 0);
		}
		ph = ph->ph_next;
	}
	ppplog(MSG_DEBUG, ah, "phase_start_bundle:  Start complete\n");
}

act_hdr_t * bundle_incoming_join(act_hdr_t *ah);

/*
 * This routine is called when a link has completed both the
 * Establish and Authentication phases ... 
 */
STATIC int
phase_start_link(act_hdr_t *al)
{
	struct act_hdr_s *ab;
	proto_hdr_t *ph;
	int ret;
	int just_joined = 0;
	struct cfg_bundle *cb;

	ASSERT(al->ah_type == DEF_LINK);

	/* 
	 * If the first link in the bundle then ncp's must be started
	 * on the bundle - this is done by calling ppp_phase with
	 * the links parent as an argument and the up event.
	 */
	ppplog(MSG_DEBUG, al, "phase_start_link: Link ready for Bundle\n");
	ppplog(MSG_DEBUG, al, "al_local_opts = 0x%x, al_peer_opts = 0x%x\n",
	       al->ah_link.al_local_opts, al->ah_link.al_peer_opts);

	/* Get the parent ... the bundle */
	ab = al->ah_link.al_bundle;

	ASSERT(!ab || MUTEX_LOCKED(&ab->ah_mutex));

	if (al->ah_link.al_flags & ALF_INCOMING) {
		if (!ab) {
			/* 
			 * If no active bundle for a parent ... then this
			 * is an incoming call and we must find out which
			 * bundle this will join. By now we have all info
			 * about the incoming call, we need to determine
			 * which bundle this link should join
			 */
			ab = bundle_incoming_join(al);
			if (!ab) {
				/* No bundle to join ... close the link */
				ppplog(MSG_WARN, al,
				       "No suitable bundle to join.\n");
				hist_add(HOP_ADD, al, EBADSLT);
				psm_close_link(al, ALR_BADIN);
				al->ah_phase = PHASE_DEAD;
				return;
			}

			just_joined = 1;
			/* We now hold both the bundle and link locks */
		} else if (!act_try_incoming(ab, al)) {
			/*
			 * We had a bundle (not autodetected incoming)
			 * but the authid, cid, etc didn't match the
			 * bundle definition.
			 */
			ppplog(MSG_WARN, ab,
			       "Link %s not suitable for bundle.\n",
			       al->ah_cfg->ch_id);
			hist_add(HOP_ADD, al, EBADSLT);
			psm_close_link(al, ALR_BADIN);
			al->ah_phase = PHASE_DEAD;
			return;
		}

		if (ab->ah_bundle.ab_open_links == 0)
			ab->ah_bundle.ab_reason = 0;

		hist_add(HOP_ADD, al, 0);
	}

	ab->ah_bundle.ab_open_links++;
	al->ah_link.al_flags |= ALF_OPEN;

	ppplog(MSG_DEBUG, ab, "ab_numlinks = %d, ab_open_links = %d\n",
	       ab->ah_bundle.ab_numlinks, ab->ah_bundle.ab_open_links);

	if (ab->ah_bundle.ab_open_links == 1) {

		/*
		 * The first link in a bundle.
		 */
		ppplog(MSG_DEBUG, al, "First link in bundle\n");

		/*
		 * Check both ends have negotiated the mrru option ..
		 * or that neither of them have.
		 */
		if ((al->ah_link.al_local_opts & ALO_MRRU) !=
		    (al->ah_link.al_peer_opts & ALO_MRRU)) {
			ppplog(MSG_DEBUG, ab,
       "Asymmetric multilink configuration. Renogotiate. (No Multilink)\n");
			/*
			 * We have attempted to negotiate the use
			 * of Multilink, but were unsuccessful.
			 * Re-neogitate LCP without multilink.
			 */
			al->ah_link.al_local_opts &= ~(ALO_ML_NEG | ALO_MRRU);
			al->ah_link.al_peer_opts &= ~(ALO_ML_NEG | ALO_MRRU);

			ppplog(MSG_DEBUG, al,
			       "al_local_opts = 0x%x, al_peer_opts = 0x%x\n",
			       al->ah_link.al_local_opts,
			       al->ah_link.al_peer_opts);

			fsm_state(al->ah_link.al_lcp, OPEN, 0);
			al->ah_phase = PHASE_ESTAB;
			al->ah_link.al_reason |= ALR_RENEG;
			goto exit;
		}

		if (al->ah_link.al_local_opts & ALO_MRRU) {
			struct di_ml_s ml;

			ppplog(MSG_DEBUG, al, "Multilink negotiated.\n");

			/*
			 * Check that we aren't looped-back.
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
				psm_close_link(al, ALR_LOOPBACK);
				goto exit;
			}

			/*
			 * Remember the peers MRRU, ssn and ed.
			 * We MUST observe these values 
			 * when subsequent links join the bundle.
			 */
			ab->ah_bundle.ab_peer_mrru = al->ah_link.al_peer_mrru;
			ab->ah_bundle.ab_peer_ssn =
				(al->ah_link.al_peer_opts & ALO_SSN);
			ab->ah_bundle.ab_peer_ed =
				(al->ah_link.al_peer_opts & ALO_ED);

			/*
			 * If the first link has a link discriminator ..
			 * ensure all others get one.
			 */
			if ((al->ah_link.al_local_opts & ALO_LD) &&
			    (al->ah_link.al_peer_opts & ALO_LD))
				ab->ah_bundle.ab_flags |= ABF_LDS;

			/*
			 * Bind to ML
			 */
			cb = (struct cfg_bundle *)ab->ah_cfg;
			
			ppplog(MSG_DEBUG, ab, "Bind Multilink to bundle\n");

			ml.ml_max_pend = cb->bn_maxfrags;
			ml.ml_opts = al->ah_link.al_local_opts & ALO_SSN;
			ml.ml_idle = cb->bn_mlidle;

			cd_d2k_bind_psm(ab, PROTO_ML, PROTO_ML,
					PSM_RX, &ml, sizeof(ml));

			ml.ml_min_frag = cb->bn_minfrag;
			ml.ml_opts = al->ah_link.al_peer_opts & ALO_SSN;
			if (cb->bn_nulls)
				ml.ml_idle = cb->bn_mlidle;
			else
				ml.ml_idle = 0;

			cd_d2k_bind_psm(ab, PROTO_ML, PROTO_ML,
					PSM_TX, &ml, sizeof(ml));
		} 
	
		ppplog(MSG_DEBUG, al, "Configure bundle\n");

		ab->ah_bundle.ab_mtu = al->ah_link.al_peer_mru;

		cd_d2k_cfg_bundle(ab);

	} else {
		/*
		 * Check that a secondary links Identity
		 * matches that of the bundle.
		 */
		if (!bundle_check_linkid(ab, al)) {
			ppplog(MSG_WARN, ab,
		        "Link cannot join this bundle, ID doesn't match\n");
			psm_close_link(al, ALR_MISMATCH);
			goto exit;
		}

		/*
		 * Then, if the SSN and MRRU don't match .. attempt
		 * re-negotiation
		 */
		if (!bundle_check_linkattr(ab, al)) {
			ppplog(MSG_DEBUG, ab, "Link needs re-negotiation\n");

			/* Copy the bundles mrru, ssn, ld, ed */
			
			bundle_set_linkattr(ab, &al->ah_link);

			al->ah_link.al_local_opts &= ~ALO_ML_NEG;
			al->ah_link.al_peer_opts &= ~ALO_ML_NEG;

			fsm_state(al->ah_link.al_lcp, OPEN, 0);
			al->ah_phase = PHASE_ESTAB;
			goto exit;
		}

		/*
		 * When we have a second link, we start using the
		 * peers MRRU as our MTU.
		 */
		if (ab->ah_bundle.ab_open_links == 2) {
			ab->ah_bundle.ab_mtu = al->ah_link.al_peer_mrru;
			cd_d2k_cfg_bundle(ab);
		}
	}

	ppplog(MSG_DEBUG,ab, "Bundles Mtu = %d\n", ab->ah_bundle.ab_mtu);

	ASSERT(MUTEX_LOCKED(&ab->ah_mutex));

	ppplog(MSG_INFO_MED, al, "Entering NETWORK phase, notify kernel\n");

	cd_d2k_adm_link(al, CMD_OPEN, 0);

	/* Start any protocols on the link (LQM, CCP etc) */

	ph = al->ah_protos;
	while(ph) {
		if ((ph->ph_psmtab->pt_flags & (PT_LCP|PT_FSM)) == PT_FSM) {
			ppplog(MSG_INFO_LOW, al, "Start Protocol %s\n",
			       ph->ph_psmtab->pt_desc);

			/* Init the config & fsm for the protocol */
			fsm_init(ph);
			fsm_state(ph, OPEN, 0);
		}
		ph = ph->ph_next;
	}

	/*
	 * Tell the bundle to proceed to establish phase
	 *  (if its not there already)
	 */
	ppp_phase(ab, PHASE_NETWORK);
	ppplog(MSG_DEBUG, al, "phase_start_link:  Link  ... start complete\n");

 exit:
	ppplog(MSG_DEBUG, al,
	       "al_local_opts = 0x%x, al_peer_opts = 0x%x\n",
	       al->ah_link.al_local_opts, al->ah_link.al_peer_opts);

	if (just_joined) {
		ATOMIC_INT_INCR(al->ah_refcnt);
		MUTEX_UNLOCK(&al->ah_mutex);
		act_release(ab);
		MUTEX_LOCK(&al->ah_mutex);
		ATOMIC_INT_DECR(al->ah_refcnt);
	}
}


STATIC int
phase_term_start(struct act_hdr_s *ah)
{
	proto_hdr_t *ph = ah->ah_protos;
	act_hdr_t *ab, *al;

	ppplog(MSG_DEBUG, ah, "phase_term_start: start term ...\n");

	if (ah->ah_type == DEF_BUNDLE) {
		/* Nothing yet ?? */
		;
	} else	if (ah->ah_link.al_type == LNK_STATIC &&
		    ah->ah_link.al_lcp->ph_state.ps_state == CLOSED) {
		/*
		 * We get here if user has peformed detach or kill
		 */
		ppplog(MSG_DEBUG, ah, "STATIC link CLOSED\n");
		bundle_link_finished(ph->ph_parent);
	}
}

STATIC int
phase_dead(struct act_hdr_s *ah)
{
	if (ah->ah_type == DEF_BUNDLE) {
		ppplog(MSG_DEBUG, ah,
		       "phase_dead: On bundle - re-init cfg ?\n");
	} else {
		act_hdr_t *ab;

		ppplog(MSG_DEBUG, ah,
		       "phase_dead: On Link - re-init cfg ??\n");
#ifdef NOT
/* Don't like this ... it's for recovery of STATIC links */

/* MUST BE A BETTER WAY !!! */
		ab = ah->ah_link.al_bundle;
		if (ab) {
			ppplog(MSG_DEBUG, ah, "Have bundle - check if any links ..\n");
			ATOMIC_INT_INCR(ah->ah_refcnt);
			MUTEX_UNLOCK(&ah->ah_mutex);
			if (!bundle_transition(ab)) {
				ppplog(MSG_DEBUG, ah, "bundle should be marked as closed\n");
				bundle_close(ab, 0);
				cd_d2k_adm_bundle(ab, CMD_CLOSE, 0);
			}
			MUTEX_LOCK(&ah->ah_mutex);
			ATOMIC_INT_DECR(ah->ah_refcnt);
		}
			
		
#endif	
	}
}
