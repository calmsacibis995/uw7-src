#ident	"@(#)fsm_cfg.c	1.8"

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
#include "lcp_hdr.h"
#include "act.h"
#include "ppp_proto.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

struct psm_code_tab_s lcp_basic_codes[];

/*
 * The Control Protocol provides, for each option it wishes to
 * handle, a send and a receive function.
 *
 * Receive function
 * ----------------
 *
 * This handles received Configure Requests and generated any response
 * NAK or REJECT packet.
 *
 * int rcv_xxxx(proto_hdr_t *ph, int action, db_t *db, db_t *ndb, int state)
 *
 * Where
 *	ph	is the protocol header
 *	action	specifies what is required of the function
 *	db	contains the received packet
 *	ndb	contains a response to the received packet
 *	state	the current processing state
 *
 * action can be one of CFG_MKNAK, CFG_DEFAULT, CFG_CHECK these indicate
 *
 *	CFG_MKNAK	the function must generate a Nak (the
 *			option length was incorrect)
 *	CFG_DEFAULT	the option was not specified, set default
 *			values in any associated data structures
 *	CFG_CHECK	the option was specified and initial validation
 *			(for lengths) succeeded. The receive function
 *			should check it likes any option parameters
 *			requested. If it doesn't, it should generate
 *			a NAK/REJ.
 *
 * The state paramter indicates the current processing mode,
 *
 *	CFG_ACK	We are still expecting to ACK the request
 *	CFG_NAK We are creating a NAK to the request
 *	CFG_REJ We are creating a REJ to the request
 *
 * If a NAK is generated, the option and paramters should be placed
 * in the ndb aread and the db_wptr updated. CFG_NAK should be returned.
 *
 * If an ACK is required, no message need be created by the function,
 * CFG_ACK should be returned.
 *
 * If a Configure Reject is required, no message need be greated, just
 * return CFG_REJ.
 *
 * If the received Packet needs to be discarded, then return CFG_DISCARD.
 *
 *
 * Send function
 * -------------
 *
 * The send function is called whenever a Configure Request is required.
 * Either to send an initital Request, or a request in response to Nak/Rej.
 *
 * int snd_xxx(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
 *
 * Where
 *	ph	is the protocol header
 *	state	is the processing state
 *	co	is the Nak'ed option
 *	db	the generated packet.
 *
 * The state parameter is one of
 *
 *	CFG_ACK	this indicates that the Request is an initial request
 *	CFG_NAK	this is set when a request is required in response
 *		to a configure Nak. The co parameter will point to
 *		the option in the received configure nak. It is the
 *		value the peer would find acceptable.
 *	CFG_REJ	this indicated that the option was Rejected by the
 *		peer. The send function should set associated data
 *		structures to indicate this.
 *
 * The return value is ignored.
 */

/*
 * Reject an option - it's not supported.
 */
int
co_reject(proto_hdr_t *ph, int state, struct co_s *co, db_t *ndb)
{
	int to_copy;
	int len;

	if (state == CFG_NAK) {
		psm_log(MSG_DEBUG, ph,
			 "co_reject: Already Naking - skip reject\n");
		return state;
	}

	psm_log(MSG_DEBUG, ph, "co_reject: Rejecting option %d\n",
		co->co_type);

	len = max(co->co_len, 2);
	to_copy = min(ndb->db_lim - ndb->db_wptr, len);

#ifdef DEBUG_OPTS
	psm_log(MSG_DEBUG, ph, "len %d, to_copy %d, wptr %x, lim %x\n",
		len, to_copy, ndb->db_wptr, ndb->db_lim);
#endif
	memcpy(ndb->db_wptr, co, to_copy);
	ndb->db_wptr += to_copy;
	return CFG_REJ;
}

/*
 * Config option receive processing
 *
 * db 	- Contains the configure request
 *	  the db_rptr points to the start of the configure request options
 * ndb	- Points to a message block available for a returned ACK/NAK/REJ
 */
int
co_rcv_process(proto_hdr_t *ph, struct psm_opt_tab_s op_tab[],
	       ushort_t op_max, db_t *db, db_t *ndb)
{
	struct co_s *co;
	int state, istate;
	int action, i, len;
	ushort_t option;
	uchar_t *co_start = db->db_rptr;
	uchar_t *nco_start = ndb->db_wptr;

	ASSERT(db->db_wptr >= db->db_rptr);
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	/*
	 * When action is
	 *    CFG_CHECK we are checking if we like the config request
	 *    CFG_MKNAK we have found an option with incorrect length
	 *         and must nak with the value/length that we would
	 *         find acceptable
	 */
	action = CFG_CHECK;

	/*
	 * When state is
	 *    CFG_ACK we are so far happy with the request.
	 *    CFG_NAK we found a requested option that was unaccpetable
	 *         and we nak with a prefered value, nak built in ndb
	 *    CFG_REJ we found a requested option that we don't want.
	 *         Send a reject of the option, this is built int ndb
	 */
	state = CFG_ACK;

	/*
	 * Reset Peer selected options, a bit is set for each option
	 * that the peer selects.
	 */
	PH_RESETBITS(ph->ph_peer_opts);

	while (db->db_rptr < db->db_wptr) {
		co = (struct co_s *)db->db_rptr;

		option = co->co_type;

		/* 
		 * Check the length of the message, must be at least the size
		 * of the configuration option header
		 */
		if (db->db_wptr - db->db_rptr < sizeof(struct co_s)) {
			psm_log(MSG_WARN, ph,
				"Configure Request - Bad length - Discard\n");
			return CFG_DISCARD;
		}

		/*
		 * Check that we know the config option supplied
		 */
		if (option > op_max) {
			psm_log(MSG_INFO_MED, ph,
				 "Unsupported option %d - Reject\n", option);

			/* Need config reject */
			state = co_reject(ph, state, co, ndb);
			if (co->co_len < MIN_OP_LEN)
				break;
			else
				goto next;
		}

		/*
		 * Check the option length is as specified in the prototab
		 *
		 * RFC 1661 p40 para3 says
		 * "If a negotiable Configuration Option is received
		 * in a Configure-Request, but with an invalid or
		 * unrecognized Length, a Configure-Nak SHOULD
		 * be transmitted which includes the desired Configuration
		 * Option with an appropriate Length and Data.
		  */
		if (op_tab[option].op_flags & OP_LEN_EXACT) {
			if (co->co_len != op_tab[option].op_len) {
				psm_log(MSG_WARN, ph,
				 "Option %s - Exact length %d expected\n",
					 op_tab[option].op_name,
					 op_tab[option].op_len);
				action = CFG_MKNAK;
			}
		} else if (op_tab[option].op_flags & OP_LEN_MIN) {
			if (co->co_len < op_tab[option].op_len) {
				psm_log(MSG_WARN, ph,
				 "Option %s - Min length %d expected\n",
					 op_tab[option].op_name,
					 op_tab[option].op_len);
				action = CFG_MKNAK;
			}
		} else if (co->co_len < MIN_OP_LEN) {
			psm_log(MSG_WARN, ph,
				"Option %s - Length < %d bytes\n",
				op_tab[option].op_name,
				MIN_OP_LEN);
			action = CFG_MKNAK;
		}

		/*
		 * RFC 1661 p40 para5 say if the data field extends
		 * past the end of the packet, the whole packet is
		 * silently discarded
		 */
		if(co->co_len + db->db_rptr > db->db_wptr) {
			psm_log(MSG_WARN, ph,
 "Configure Request - Bad length (Extends beyond end of frame) - Discard\n");
			return CFG_DISCARD;
		}

		/*
		 * Check that we support the config option supplied
		 */
		if (!op_tab[option].op_rcv) {
			/* Unsupported option ... need config reject */
			psm_log(MSG_INFO_MED, ph,
				"Unsupported option '%s' - reject\n",
				op_tab[option].op_name);

			state = co_reject(ph, state, co, ndb);

			if (action == CFG_MKNAK)
				break;
			else
				goto next;
		}

		/*
		 * Test if the option is allowed mutliple times in a configure
		 * request. If NOT ensure that it has not previously been
		 * selected.
		 */
		if (!(op_tab[option].op_flags & OP_MULTI)) {
			if (PH_TSTBIT(option, ph->ph_peer_opts)) {
				psm_log(MSG_WARN, ph,
		 "Duplicate option in Configure Request - option %d (%s)\n",
					 option, op_tab[option].op_name);
				return CFG_DISCARD;
			}
		}

		/* Indicate that the option has been selected */

		PH_SETBIT(option, ph->ph_peer_opts);

		istate = CFG_ACK;

		/* Check the options acceptablity */
		switch (state) {
		case CFG_ACK:
			istate = (*op_tab[option].op_rcv)(ph, action,
							  db, ndb, state);
			state =	istate;
			break;

		case CFG_REJ:
			/*
			 * Only check options that we could reject
			 */
			if (op_tab[option].op_flags & OP_NO_REJ)
				break;
			istate = (*op_tab[option].op_rcv)(ph, action,
							  db, ndb, state);
			break;

		case CFG_NAK:
			/*
			 * Only check options that we could nak
			 */
			if (op_tab[option].op_flags & OP_NO_NAK)
				break;
			istate = (*op_tab[option].op_rcv)(ph, action,
							  db, ndb, state);
			break;

		default:
			istate = -1;
			psm_log(MSG_WARN, ph,
			 "Unexpected State in Receive processing. %d\n",
				 state);
			break;
		}

		if (action == CFG_MKNAK)
			break;

		switch (istate) {
		case CFG_REJ:
			/* Check we are doing rejects */
			if (state != istate)
				break;

			/* Copy the option to the destintation */
#ifdef DEBUG_OPTS
			psm_log(MSG_DEBUG, ph,
			 "co_rcv_process: Rejecting option %s\n",
				 op_tab[option].op_name);
#endif

			memcpy(ndb->db_wptr, db->db_rptr, co->co_len);
			ndb->db_wptr += co->co_len;
			break;

		case CFG_DISCARD:
			return CFG_DISCARD;
		}


	next:
		/* Move on to the next option */
		db->db_rptr += co->co_len;
		ASSERT(db->db_wptr >= co_start);
	}

	/*
	 * If we aren't happy with the config request ... return,
	 * otherwise go on to make a CFG_ACK.
	 */
	switch (state) {
	case CFG_DISCARD:
		return state;

	case CFG_ACK:
		if (ph->ph_state.ps_max_fail < 1) {
			psm_log(MSG_INFO_MED, ph,
			"Max failure reached. Don't try for any options.\n");
			/*
			 * The config request probably has no options ...
			 * the Control Protocols UP routine must ensure
			 * that the minimum stat of acceptable options
			 * has been negotiated.
			 */
			return state;
		}
		/*
		 * Check if we need defaults, or to nak for
		 * options the peer didn't configure
		 */
		break;

	case CFG_REJ:
	case CFG_NAK:
		/*
		 * If we have reached max failure, then we convert the
		 * config request to a reject, all options the peer
		 * selected are rejected.
		 */
		if (ph->ph_state.ps_max_fail < 1) {
			psm_log(MSG_INFO_MED, ph,
			"Max failure reached. Reject all options selected.\n");

			len = db->db_wptr - co_start;
			ndb->db_wptr = nco_start;

			ASSERT(len >= 0);

			/* Copy the input options to the output */

			if (len > 0) {
				ASSERT(ndb->db_wptr + len < ndb->db_lim);

				memcpy(ndb->db_wptr, co_start, len);
				ndb->db_wptr += len;
			}
		
			state = CFG_REJ;
		}
		return state;		

	default:
		psm_log(MSG_ERROR, ph, "Internal error, co_rcv_process().\n");
		return state;
	}

	/*
	 * For all options not specified in the configure request,
	 * set the default value
	 */
	action = CFG_DEFAULT;

	for (option = 0; option <= op_max; option++) {

		if (!op_tab[option].op_rcv || 
		    PH_TSTBIT(option, ph->ph_peer_opts))
			continue;

		switch (state) {
		case CFG_ACK:
			state =	istate
				= (*op_tab[option].op_rcv)(ph, action,
							   NULL, ndb, state);
			break;

 		case CFG_REJ:
			psm_log(MSG_WARN, ph,
 "PSM Requests Reject option %s - Not supported in Mode CFG_DEFAULT\n",
				 op_tab[option].op_name);
			return CFG_DISCARD;

		case CFG_NAK:
			if (op_tab[option].op_flags & OP_NO_NAK)
				break;
			istate = (*op_tab[option].op_rcv)(ph, action,
							  NULL, ndb, state);
			break;

		default:
			istate = -1;
			psm_log(MSG_ERROR, ph,
				"Unexpected State in Receive processing. %d\n",
				state);
			break;
		}

		if (istate == CFG_DISCARD)
			return CFG_DISCARD;
	}

	if (state != CFG_ACK)
		return state;

	/*
	 * Message passed to us ndb will form the reply ACK, we need
	 * to copy all the ACK'ed options into that message
	 */
	len = db->db_wptr - co_start;
	ASSERT(len >= 0);

	/* Copy the input options to the output */
	if (len > 0) {
		ASSERT(ndb->db_wptr + len < ndb->db_lim);

		memcpy(ndb->db_wptr, co_start, len);
		ndb->db_wptr += len;
	}
		
	return state;
}

/*
 * Check the options in a received configure reject are legal
 * (i.e. that we requested them)
 *
 * db	points to the configure reject
 * ndb	points to the received configure reject
 *
 * Returns ...
 * CFG_DISCARD	if the packet is invalid ..
 * CFG_REj	if the packet is a valid config reject
 * CFG_ACK	if the packet should be treated as an Ack
 */
int
co_rcv_corej(proto_hdr_t *ph, struct psm_opt_tab_s op_tab[],
	       ushort_t op_max, db_t *db, db_t *ndb)
{
	struct co_s *rej_co, *req_co;
	int state = CFG_ACK;
	uchar_t ti;	/* Config Opt Table Index */
	uchar_t *req = db->db_base + 2 + sizeof(struct lcp_hdr_s);
	uchar_t *rej = ndb->db_rptr + sizeof(struct lcp_hdr_s);
	int data_len;

	ASSERT(db->db_wptr >= db->db_rptr);

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	while (rej < ndb->db_wptr) {
		rej_co = (struct co_s *)rej;
		ti = rej_co->co_type;

		if (ndb->db_wptr - rej < 2 || rej_co->co_len < 2) {
			psm_log(MSG_WARN, ph,
		"Invalid Configure Reject (bad option length) - dropped\n");
			return CFG_DISCARD;
		}

		if (rej + rej_co->co_len > ndb->db_wptr) {
			psm_log(MSG_WARN, ph,
		"Invalid Configure Reject (bad option length) - dropped\n");
			return CFG_DISCARD;
		}

		if (ti > op_max) {
			psm_log(MSG_WARN, ph,
				 "Peer Rejected Unknown option %d\n", ti);
			return CFG_DISCARD;
		}

		if (!op_tab[ti].op_rcv) {
			psm_log(MSG_WARN, ph,
				 "Peer Rejected Unsupported option %d\n", ti);
			return CFG_DISCARD;
		}

		/*
		 * If we haven't got a request to compare with ...
		 * just check the next option
		 */
		if (!req)
			goto next;

		/* See if we requested this option */
		while (req < db->db_wptr) {
			req_co = (struct co_s *)req;

			if (req_co->co_type > ti) {
				req_co = NULL;
				break;
			} else if (req_co->co_type == ti)
				break;

			req += req_co->co_len;
		}

		/* Check if we have run out of Naked/Rejected Options */

		if (req >= db->db_wptr)
			req_co = NULL;

		if (!req_co) {
			psm_log(MSG_WARN, ph,
			 "Did not request option '%s' but was rejected\n", 
				 op_tab[ti].op_name);
			return CFG_DISCARD;
		}

		/* Check that the option length did not change */

		if (req_co->co_len != rej_co->co_len) {
			psm_log(MSG_WARN, ph,
			 "Peer Rejected option - changed length - drop\n");
			return CFG_DISCARD;
		}

		/* Check that the option data did not change */
		data_len = req_co->co_len - sizeof(struct co_s);
		
		if (data_len > 0) {
#ifdef DEBUG_OPTS
			psm_log(MSG_DEBUG, ph, "len %d\n",
				 data_len);
			psm_loghex(MSG_DEBUG, ph,
				    (caddr_t)(req_co + 1), data_len);
			psm_loghex(MSG_DEBUG, ph,
				    (caddr_t)(rej_co + 1), data_len);
#endif


			if (memcmp((caddr_t)(req_co + 1),
				   (caddr_t)(rej_co + 1), data_len) != 0) {
				psm_log(MSG_WARN, ph,
			 "Peer Rejected option - changed data - drop\n");
				return CFG_DISCARD;
			}
		}

	next:
		if (!(op_tab[ti].op_flags & OP_REJISACK))
			state = CFG_REJ;

		/* Indicate option rejected */

		PH_SETBIT(ti, ph->ph_rej_opts);

		rej += rej_co->co_len;
	}
	return state;
}

/*
 * Check the options in a received configure Nak are legal
 * (i.e. option lengths are legal)
 *
 * ndb	points to the received configure Nak
 */
int
co_rcv_conak(proto_hdr_t *ph, struct psm_opt_tab_s op_tab[],
	       ushort_t op_max, db_t *ndb)
{
	struct co_s *nak_co;
	uchar_t ti;	/* Config Opt Table Index */
	uchar_t *nak = ndb->db_rptr + sizeof(struct lcp_hdr_s);

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	while (nak < ndb->db_wptr) {
		nak_co = (struct co_s *)nak;
		ti = nak_co->co_type;

		if (ndb->db_wptr - nak < 2 || nak_co->co_len < 2) {
			psm_log(MSG_WARN, ph,
		"Invalid Configure Nak (bad option length) - dropped\n");
			return CFG_DISCARD;
		}

		if (nak + nak_co->co_len > ndb->db_wptr) {
			psm_log(MSG_WARN, ph,
		"Invalid Configure Nak (bad option length) - dropped\n");
			return CFG_DISCARD;
		}

		if (ti > op_max) {
			psm_log(MSG_WARN, ph,
				 "Peer Nak'ed Unknown option %d\n", ti);
		} else if (!op_tab[ti].op_rcv) {
			psm_log(MSG_WARN, ph,
				 "Peer Nak'ed Unsupported option %d\n", ti);
		} else if (op_tab[ti].op_flags & OP_LEN_EXACT) {
			if (nak_co->co_len != op_tab[ti].op_len) {
				psm_log(MSG_WARN, ph,
				"Option %s - Exact length %d expected\n",
					 op_tab[ti].op_name,
					 op_tab[ti].op_len);
				return CFG_DISCARD;
			}
		} else if (op_tab[ti].op_flags & OP_LEN_MIN) {
			if (nak_co->co_len < op_tab[ti].op_len) {
				psm_log(MSG_WARN, ph,
				 "Option %s - Min length %d expected\n",
					 op_tab[ti].op_name,
					 op_tab[ti].op_len);
				return CFG_DISCARD;
			}
		} else if (nak_co->co_len < MIN_OP_LEN) {
			psm_log(MSG_WARN, ph,
				"Option %s - Length < %d bytes\n",
				op_tab[ti].op_name,
				MIN_OP_LEN);
			return CFG_DISCARD;
		}

		nak += nak_co->co_len;
	}
	return CFG_ACK;
}

/*
 * This function will be called to create a config request, if db is non-NULL
 * then this implies that we require a request modified by a received confg
 * ack or reject.
 */
co_snd_process(proto_hdr_t *ph, int state, struct psm_opt_tab_s op_tab[],
	       ushort_t op_max, db_t *db, db_t *ndb)
{
	int i;
	struct co_s *co = NULL;

#ifdef DEBUG_OPTS
	psm_log(MSG_DEBUG, ph, "co_snd_process:\n");
#endif


	/*
	 * When 'co' is null, then the option support routine will be passed
	 * the CFG_ACK state, meaning that we wan't the configured value.
	 * When 'co' is set, then the support routine is called with a
	 * pointer to the nak/rej option and whether it was a NAK or REJ
	 * that we are replying to.
	 *
	 * The options sanity will already have been checked by the time
	 * we get here (by co_rcv_corej or co_rcv_conak).
	 */

	for (i = 0; i <= op_max; i++) {

		/*
		 * If we have a NAK or REJ for this option, then
		 * point co at the option.
		 */
		while (db && db->db_rptr < db->db_wptr) {
			co = (struct co_s *)db->db_rptr;

			if (co->co_type > i) {
				co = NULL;
				break;
			} else if (co->co_type == i)
				break;

			ASSERT(db->db_rptr + co->co_len <= db->db_wptr);
			db->db_rptr += co->co_len;
		}

		/* Check if we have run out of Naked/Rejected Options */
		if (db->db_rptr >= db->db_wptr)
			co = NULL;

		/* Configure the option */
		if (op_tab[i].op_snd) {

			/*
			 * If we are cfg_ack or cfg_nak then
			 * ensure the peer hasn't rejected this option
			 */
			if (state != CFG_REJ &&
			    PH_TSTBIT(i, ph->ph_rej_opts)) {
#ifdef DEBUG_OPTS
				psm_log(MSG_DEBUG, ph,
			"co_snd_process: opt %s previously rejected skip\n",
					op_tab[i].op_name);
#endif
				continue;
			}
#ifdef DEBUG_OPTS
			psm_log(MSG_DEBUG, ph,
		"co_snd_process: Option = %s (%d) co = %x (opt %d, len %d)\n",
				op_tab[i].op_name, i,
				co, co->co_type, co->co_len);

			if (db) {
				if (co) {
					psm_log(MSG_DEBUG, ph,
			"co_snd_process: Have nak/rej for this - state %d\n",
						state);
				} else {
					psm_log(MSG_DEBUG, ph,
			"co_snd_process: This option wasn't nak/rej\n");
				}
			}
#endif

			(*op_tab[i].op_snd)(ph, co ? state : CFG_ACK, co, ndb);
		}
	}

	/* If we have any config options left, then OOps */
}

#define LCP_SIZE DEFAULT_MRU

/*
 * Create a configure request using the db (cfg_nak cfg_rej) as a guide
 */
db_t *
mk_cfg_req(proto_hdr_t *ph, uchar_t id, db_t *db)
{
	struct lcp_hdr_s *lcp = (struct lcp_hdr_s *)db->db_rptr;
	struct lcp_hdr_s *nlcp;
	int i;
	int state = CFG_ACK;
	db_t *ndb;
	struct psm_tab_s *pt = ph->ph_psmtab;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	ndb = db_alloc(PSM_MTU(ph->ph_parent));
	if (!ndb) {
		psm_log(MSG_WARN, ph, "mk_cnf_req: NO MEMORY\n");
		return NULL;
	}

	psm_add_pf(ph, ndb);

	nlcp = (struct lcp_hdr_s *)ndb->db_wptr;
	nlcp->lcp_id = id;
	nlcp->lcp_code = CFG_REQ;

	if (db) {
		state = lcp->lcp_code;

		switch(state) {
		case CFG_NAK:
			psm_log(MSG_INFO_MED, ph,
			       "Constructing Configure Request (after Nak)\n");
			break;

		case CFG_REJ:
			psm_log(MSG_INFO_MED, ph,
		       "Constructing Configure Request (after Reject)\n");
			break;
		default:
			/* Internal error */
			psm_log(MSG_FATAL, ph,
				"mk_cfg_req: Unexpected lcp_code %d\n",
				lcp->lcp_code);
			/*NOTREACHED*/
		}

		db->db_rptr += sizeof(struct lcp_hdr_s);
	} else
		psm_log(MSG_INFO_MED, ph, "Constructing Configure Request\n");

	ndb->db_wptr += sizeof(struct lcp_hdr_s);

	/* Insert the options */

	co_snd_process(ph, state, pt->pt_option, pt->pt_numopts, db, ndb);

	/* Calculate the length of the reply */

	nlcp->lcp_len = htons(ndb->db_wptr - (uchar_t *)nlcp);

	return ndb;
}

/*
 * Make a code reject
 */
db_t *
mk_cd_rej(struct proto_hdr_s *ph, uchar_t id, db_t *db)
{
	db_t *ndb; 
	struct lcp_hdr_s *lcp;
	ushort_t to_copy;
	psm_tab_t *pt = ph->ph_psmtab;

	ndb = db_alloc(PSM_MTU(ph->ph_parent));
	if (!ndb) {
		psm_log(MSG_WARN, ph, "mk_cd_rej: NO MEMORY\n");
		return NULL;
	}

	psm_add_pf(ph, ndb);

	lcp = (struct lcp_hdr_s *)ndb->db_wptr;
	lcp->lcp_code = COD_REJ;
	lcp->lcp_id = id;
	lcp->lcp_len = ntohs(sizeof(struct lcp_hdr_s));
	ndb->db_wptr += sizeof(struct lcp_hdr_s);

	to_copy = min(ndb->db_lim - ndb->db_wptr, db->db_wptr - db->db_rptr);

	memcpy(ndb->db_wptr, db->db_rptr, to_copy);
	ndb->db_wptr += to_copy;
	lcp->lcp_len = htons(to_copy + ntohs(lcp->lcp_len));

	return ndb;
}

/*
 * Make a terminate request/ack
 */
db_t *
mk_trm(struct proto_hdr_s *ph, uchar_t id, uchar_t code, db_t *db)
{
	db_t *ndb; 
	struct lcp_hdr_s *lcp, *olcp;
	psm_tab_t *pt = ph->ph_psmtab;

	ndb = db_alloc(PSM_MTU(ph->ph_parent));
	if (!ndb) {
		psm_log(MSG_WARN, ph, "mk_trm: NO MEMORY\n");
		return NULL;
	}

	psm_add_pf(ph, ndb);

	if (db) {
		olcp = (struct lcp_hdr_s *)db->db_rptr;
		id = olcp->lcp_id;
	}

	lcp = (struct lcp_hdr_s *)ndb->db_wptr;
	lcp->lcp_code = code;
	lcp->lcp_id = id;
	lcp->lcp_len = ntohs(sizeof(struct lcp_hdr_s));
	ndb->db_wptr += sizeof(struct lcp_hdr_s);
	return ndb;
}

/*
 * Make an echo reply
 */
db_t *
mk_echo_rply(proto_hdr_t *ph, db_t *db)
{
	struct lcp_echo_s *le = (struct lcp_echo_s *)db->db_rptr;
	struct lcp_echo_s *nle;
	db_t *ndb;
	ushort_t space, to_copy;

	ASSERT(db);

	ndb = db_alloc(PSM_MTU(ph->ph_parent));
	if (!ndb) {
		psm_log(MSG_WARN, ph,
			 "mk_echo_rply: No Memory - didn't reply\n");
		return NULL;
	}

	psm_add_pf(ph, ndb);

	nle = (struct lcp_echo_s *)ndb->db_wptr;
	nle->lcp_hdr.lcp_code = ECO_RPL;
	nle->lcp_hdr.lcp_id = le->lcp_hdr.lcp_id;

	if (ph->ph_parent->ah_type == DEF_LINK)
		nle->lcp_magic = htonl(ph->ph_parent->ah_link.al_local_magic);
	else
		nle->lcp_magic = 0;

	ndb->db_wptr += sizeof(struct lcp_echo_s);

	/* We don't want to copy the header ... just the data */
	db->db_rptr += sizeof(struct lcp_echo_s);

	space = ndb->db_lim - ndb->db_wptr;
	to_copy = min(space, db->db_wptr - db->db_rptr);
	memcpy(ndb->db_wptr, db->db_rptr, to_copy);
	ndb->db_wptr += to_copy;

	nle->lcp_hdr.lcp_len = htons(sizeof(struct lcp_echo_s) + to_copy);

	return ndb;
}

/*
 * Create a protocol reject packet
 */
db_t *
mk_proto_rej(proto_hdr_t *ph, db_t *db, ushort_t proto)
{
	db_t *ndb;
	struct lcp_hdr_s *lcp;
	ushort_t *rej_proto;
	ushort_t space, to_copy;

	ndb = db_alloc(PSM_MTU(ph->ph_parent));
	if (!ndb) {
		ppplog(MSG_WARN, ph, "mk_proto_rej: Out of MEMORY\n");
		return NULL;
	}

	psm_add_pf(ph, ndb);

	lcp = (struct lcp_hdr_s *)ndb->db_wptr;
	lcp->lcp_code = PRT_REJ;
	lcp->lcp_id = ++ph->ph_state.ps_lastid;
	ndb->db_wptr += sizeof(struct lcp_hdr_s);

	rej_proto = (ushort_t *)ndb->db_wptr;
	*rej_proto = htons(proto);
	ndb->db_wptr += sizeof(ushort_t);

	space = ndb->db_lim - ndb->db_wptr;
	to_copy = min(space, db->db_wptr - db->db_rptr);
	memcpy(ndb->db_wptr, db->db_rptr, to_copy);
	ndb->db_wptr += to_copy;

	lcp->lcp_len = htons(sizeof(struct lcp_hdr_s) + 
			     sizeof(ushort_t) + to_copy);

	return ndb;
}

/*
 * Received CONFIG REQUEST
 *
 * This function will check all the options that have been received, if any
 * are unacceptable, then a Configure Reject/Nak will be gernerated (and
 * the pointer ndbp will point to the message).
 */
STATIC int
rcv_cfg_req(proto_hdr_t *ph, db_t *db, db_t **ndbp)
{
	struct lcp_hdr_s *lcp = (struct lcp_hdr_s *)db->db_rptr;
	psm_tab_t *pt = ph->ph_psmtab;
	struct lcp_hdr_s *nlcp;
	int state;
	db_t *ndb;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	ASSERT(db->db_wptr >= db->db_rptr);

	psm_log(MSG_INFO_MED, ph, "Received Configure Request\n");

	/*
	 * Perform any pre-config req processing required
	 */
	fsm_pre_cfg(ph);

	/* RFC 1661 says recipt of an LCP Config request returns us 
	 * to the Link Establishment phase
	 */

	if (ph->ph_psmtab->pt_flags & PT_LCP) {

		switch (ph->ph_parent->ah_phase) {
		case PHASE_ESTAB:
			break;

		case PHASE_DEAD:
			/*
			 * Must be a static link ... that was not functional
			 * .. and now is
			 */
			psm_log(MSG_INFO_MED, ph,
				 "Phase Now ESTABLISH  (as a result of LCP Configure Request)\n");
			psm_log(MSG_DEBUG, ph, "Static link back to Life\n");
			ppp_phase(ph->ph_parent, PHASE_ESTAB);
			fsm_state(ph, DOWN, NULL);
			break;

		case PHASE_AUTH:
		case PHASE_NETWORK:
		case PHASE_TERM:
			psm_log(MSG_INFO_MED, ph, "Phase Now ESTABLISH  (as a result of LCP Configure Request)\n");

			ppp_phase(ph->ph_parent, PHASE_ESTAB);
			fsm_state(ph, OPEN, NULL);
			break;
		}
	} 

	ndb = db_alloc(PSM_MTU(ph->ph_parent));
	if (!ndb) {
		psm_log(MSG_WARN, ph,
			 "rcv_cfg_req: No Memory - packet dropped\n");
		return CFG_DISCARD;
	}

	ndb->db_func = PCID_MSG;
	ndb->db_lindex = db->db_lindex;
	ndb->db_bindex = db->db_bindex;

	/* Add the protocol field */

	psm_add_pf(ph, ndb);

	nlcp = (struct lcp_hdr_s *)ndb->db_wptr;

	/* Initialise the reply */
	nlcp->lcp_id = lcp->lcp_id;

	db->db_rptr += sizeof(struct lcp_hdr_s);

	ASSERT(db->db_wptr >= db->db_rptr);

	ndb->db_wptr += sizeof(struct lcp_hdr_s);

	/* What options do we have ... and do we like the values ? */

	state = co_rcv_process(ph, pt->pt_option, pt->pt_numopts, db, ndb);

	/* Calculate the length of the reply */

	nlcp->lcp_len = htons(ndb->db_wptr - (uchar_t *)nlcp);

	nlcp->lcp_code = state;

	switch(state) {
	case CFG_ACK:
		psm_log(MSG_INFO_MED, ph,
			 "Configure Request is acceptable (Ack)\n");
		*ndbp = ndb;
		return RCR_P;
	case CFG_NAK:
		psm_log(MSG_INFO_MED, ph,
			 "Configure Request is unacceptable (Nak)\n");
		*ndbp = ndb;
		return RCR_M;
	case CFG_REJ:	
		psm_log(MSG_INFO_MED, ph,
			 "Configure Request is unacceptable (Reject)\n");
		*ndbp = ndb;
		return RCR_M;
	case CFG_DISCARD:
		psm_log(MSG_WARN, ph,
			 "Configure request being Silently Discarded\n");
		db_free(ndb);
		return CFG_DISCARD;
	default:
		psm_log(MSG_FATAL, ph, "co_rcv_process returns %d\n", state);
		/*NOTREACHED*/
	}
}

STATIC int
rcv_cfg_ack(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct lcp_hdr_s *lcp = (struct lcp_hdr_s *)db->db_rptr;
	struct lcp_hdr_s *olcp;
	db_t *old = ph->ph_rtbuf;
	ushort_t len;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_INFO_MED, ph, "Received Configure Ack\n");

	if (!ph->ph_rtbuf) {
		psm_log(MSG_WARN, ph,
			 "No outstanding Config Request - Dropped Ack\n");
		return CFG_DISCARD;
	}

	/* Check this out .. we should check the ack has what we sent */

	if (db->db_wptr - db->db_rptr < sizeof(struct lcp_hdr_s) ||
	    ntohs(lcp->lcp_len) < sizeof(struct lcp_hdr_s)) {
		psm_log(MSG_WARN, ph,
			 "Configure Ack has bad length\n");
		return CFG_DISCARD;
	}

	/* Skip the protocol field */
	olcp = (struct lcp_hdr_s *)(old->db_base + 2);
	if (olcp->lcp_len != lcp->lcp_len) {
		psm_log(MSG_WARN, ph,
	 "Configure Ack has differing length (expect same as request)\n");
		return CFG_DISCARD;
	}

	len = ntohs(olcp->lcp_len) - sizeof(struct lcp_hdr_s);

	/* Skip past the lcp option headers */
	lcp++;
	olcp++;

	if (memcmp((caddr_t)lcp, (caddr_t)olcp, len) != 0) {
		psm_log(MSG_WARN, ph,
			 "Configure Ack doesn't match requested options\n");
		return CFG_DISCARD;
	}

	db_free(ph->ph_rtbuf);
	ph->ph_rtbuf = NULL;
	return RCA;
}

STATIC int
rcv_cfg_nak(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	psm_tab_t *pt = ph->ph_psmtab;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_MED, ph, "Received Configure Nak\n");

	/*
	 * Perform any pre-config nak processing required
	 */
	fsm_pre_cfg(ph);

	/* Validate the received configure nak */
	if (co_rcv_conak(ph, pt->pt_option, pt->pt_numopts, db) != CFG_ACK)
		return CFG_DISCARD;

	*ndb = db_dup(db);

	if (ph->ph_rtbuf) {
		db_free(ph->ph_rtbuf);
		ph->ph_rtbuf = NULL;
	}
	return RCN;
}

STATIC int
rcv_cfg_rej(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	db_t *old = ph->ph_rtbuf;
	psm_tab_t *pt = ph->ph_psmtab;
	int state;
	int ret;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_INFO_MED, ph, "Received Configure Reject\n");

	/*
	 * Perform any pre-config rej processing required
	 */
	fsm_pre_cfg(ph);

	/*
	 * Check that the configure reject contains a proper subset of options
	 * requested
	 */
	state = co_rcv_corej(ph, pt->pt_option, pt->pt_numopts, old, db);
	switch (state) {
	case CFG_DISCARD:
		return CFG_DISCARD;
	case CFG_ACK:
		psm_log(MSG_INFO_MED, ph, "Treating Reject as Ack\n");
		ret = RCA;
		break;
	case CFG_REJ:
		*ndb = db_dup(db);
		ret = RCN;
	}

	if (ph->ph_rtbuf) {
		db_free(ph->ph_rtbuf);
		ph->ph_rtbuf = NULL;
	}
	return ret;
}

STATIC int
rcv_trm_req(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_MED, ph, "Received Terminate Request\n");

	fsm_reason(ph, PR_PTERM);

	/*
	 * If LCP is being terminated mark the link as being
	 * in the terminate phase
	 */
	if (ph->ph_psmtab->pt_flags & PT_LCP)
		ppp_phase(ph->ph_parent, PHASE_TERM);

	/* We need the term request later ... to get the message id */
	*ndb = db_dup(db);
	return RTR;
}

STATIC int
rcv_trm_ack(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct lcp_hdr_s *lcp = (struct lcp_hdr_s *)db->db_rptr;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_INFO_MED, ph, "Received Terminate Ack\n");
	if (ph->ph_state.ps_state == OPENED) {
		/*
		 * Allow unsolicited TRM ACK - means that we need
		 * re-negotiation
		 */
		psm_log(MSG_INFO_MED, ph,
			 "Unsolicited Terminate-Ack causes re-negotiation\n");
	} else {
		if (lcp->lcp_id != ph->ph_state.ps_lastid) {
			psm_log(MSG_WARN, ph,
		 "Discarding Terminate-Ack with mis-matched identifier\n");
			return CFG_DISCARD;
		}
	}

	return RTA;
}

STATIC int
rcv_cod_rej(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct lcp_hdr_s *lcp = (struct lcp_hdr_s *)db->db_rptr;
	ushort_t len;
	db_t *old = ph->ph_rtbuf;
	psm_tab_t *pt = ph->ph_psmtab;
	extern char *lcp_msg[];
	struct psm_code_tab_s *pc;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_INFO_MED, ph, "Received Code Reject\n");

	/* Check that the code reject matches the last config request !! */

	len = ntohs(lcp->lcp_len) - sizeof(struct lcp_hdr_s);

	if (len < 4) {
		psm_log(MSG_WARN, ph, "Code Reject too short - dropped\n");
		return CFG_DISCARD;
	}

	/* Move to the contained packet */
	lcp++;

	/* Check that the packet type is one legal */
	if (lcp->lcp_code > 0 && lcp->lcp_code <= LCP_LAST_BASIC)
		pc = &lcp_basic_codes[lcp->lcp_code];
	else
		pc = psm_code(pt, lcp->lcp_code);

	if (!pc) {
		psm_log(MSG_WARN, ph,
			 "Code Reject of unknown code (%d) - dropped\n",
			 lcp->lcp_code);
		return CFG_DISCARD;
	}

	psm_log(MSG_INFO_MED, ph, "Code Reject is of %s\n", pc->pc_name);

	if (pc->pc_flags & PC_NOREJ) {
		psm_log(MSG_WARN, ph, 
			 "Code Reject causes protocol to CLOSE\n");
		return RXJ_M;
	}

	/* Show that the code is rejected */
	PH_SETBIT(lcp->lcp_code, ph->ph_cod_rej);
	return RXJ_P;
}


STATIC int
rcv_ruc(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_INFO_MED, ph, "Received unknown code\n");
	psm_log(MSG_DEBUG, ph, "Received unknown code- ABORT\n");
	abort();
}

/*
 * Table mapping received messages to their handler function
 */

struct psm_code_tab_s lcp_basic_codes[] = {
	0,
	0,	/* Received unknown code */
	rcv_ruc,
	"Unknown",

	1,
	PC_ACCEPT | PC_NOREJ,
	rcv_cfg_req,
	"Configure-Request",

	2,
	PC_ACCEPT | PC_NOREJ | PC_CHKID,
	rcv_cfg_ack,
	"Configure-Ack",

	3,
	PC_ACCEPT | PC_NOREJ | PC_CHKID,
	rcv_cfg_nak,
	"Configure-Nak",

	4,
	PC_ACCEPT | PC_NOREJ | PC_CHKID,
	rcv_cfg_rej,
	"Configure-Reject",

	5,
	PC_ACCEPT | PC_NOREJ,
	rcv_trm_req,
	"Terminate-Request",

	6,
	PC_ACCEPT | PC_NOREJ,
	rcv_trm_ack,
	"Terminate-Ack",

	7,
	PC_ACCEPT | PC_NOREJ,
	rcv_cod_rej,
	"Code-Reject",
};

