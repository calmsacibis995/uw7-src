#ident	"@(#)psm.c	1.3"

#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/ppp.h>
#include <sys/ppp_psm.h>
#include <sys/byteorder.h>

#include "ppp_type.h"
#include "fsm.h"
#include "psm.h"
#include "act.h"
#include "ppp_cfg.h"
#include "ppp_proto.h"
#include "lcp_hdr.h"
#include "ppp_util.h"
#include "pathnames.h"

#define LCP_SIZE DEFAULT_MRU

extern struct psm_code_tab_s lcp_basic_codes[];

/*
 * Check if the specified Protocol supports a code ... if so return
 * a pointer to the code table entry
 */
struct psm_code_tab_s *
psm_code(psm_tab_t *pt, ushort_t code)
{
	struct psm_code_tab_s *pc;

	pc = pt->pt_code;
	if (!pc)
		return NULL;

	while (pc->pc_code != code && pc->pc_code != 0xffff)
		pc++;

	if (pc->pc_code == 0xffff)
		return NULL;

	return pc;
}

/*
 * Called by Control Protocols that wish to use the LCP FSM machanism.
 * This function deals with the reception of LCP codes and call the fsm
 * to perform any state transisions.
 * 
 * Don't free the message (db)
 *
 * Called when a Contol Protocol message is received
 *	d  - the transport definition
 *	db - the data block containing the received message
 *	pt - the protocol table
 *
 */
int
psm_rcv(proto_hdr_t *ph, db_t *db)
{
	ushort_t event;
	ushort_t *proto;
	db_t *ndb = NULL;
	ushort_t bytes;
	struct psm_code_tab_s *pc;
	struct lcp_hdr_s *lcp = (struct lcp_hdr_s *)db->db_rptr;
	ushort_t len = ntohs(lcp->lcp_len);
	ushort_t code = lcp->lcp_code;
	psm_tab_t *pt = ph->ph_psmtab;
	proto_state_t *ps = &ph->ph_state;

	ASSERT(ph);
	ASSERT(ph->ph_psmtab);
	ASSERT(ph->ph_parent);
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	ASSERT(db->db_wptr >= db->db_rptr);
	ASSERT(ph->ph_psmtab->pt_flags & PT_FSM);
	ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/

	/* Check the length is sensible */
	bytes = db->db_wptr - db->db_rptr;

	if (bytes < sizeof(struct lcp_hdr_s) ||
	    len < sizeof(struct lcp_hdr_s)) {
		psm_log(MSG_WARN, ph, "Received Frame too short - Discarded\n");
		db_free(db);
 		ph->ph_state.ps_rxbad++;
		return;
	}
	if (bytes < len) {
		psm_log(MSG_WARN, ph,
			 "Received Frame shorter than specified length (Got %d Specified %d bytes)  - Discarded\n", bytes, len);
		db_free(db);
 		ph->ph_state.ps_rxbad++;
		return;
	}

	/*
	 * We use the lcp_len in preference to the length of the packet
	 * - there may have been padding ... which we don't want to see
	 */
	db->db_wptr = db->db_rptr + len;
	ASSERT(db->db_wptr >= db->db_rptr);


	if (code > 0 && code <= LCP_LAST_BASIC) {
		/* We know how to handle these */

		pc = &lcp_basic_codes[code];

	} else if (code > LCP_LAST_BASIC && (pc = psm_code(pt, code))) {
		/* PSM expects this one */
		ASSERT(pc->pc_rcv);
		;
	} else {
		/* Received UNKNOWN Code */
		psm_log(MSG_WARN, ph, "Received Unexpected LCP code (%d)\n", code);
		fsm_state(ph, RUC, db);
		ph->ph_state.ps_rxbad++;
		return;
	}

	/* Check that the LCP message ID matches  that expected */

	if ((pc->pc_flags & PC_CHKID) && lcp->lcp_id != ps->ps_lastid) {
		psm_log(MSG_WARN, ph,
			 "Discarding %s with mis-matched identifier\n",
			 pc->pc_name);
		db_free(db);
		ph->ph_state.ps_rxbad++;
		return;
	}

	/* Call the receive routine */

	event = (*pc->pc_rcv)(ph, db, &ndb);

	/* Now free the received message */

	db_free(db);
		
	/* Check if we need to tell the fsm about this event */

	if (event == CFG_DISCARD) {
		ph->ph_state.ps_rxbad++;
		return;
	} else if (event == CFG_DISCARD_OK) {
		/*
		 * Discard messages that aren't in error ..
		 * e.g. Discard Request
		 */
		ph->ph_state.ps_rxcnt++;
		return;
	}

	fsm_state(ph, event, ndb);
	ph->ph_state.ps_rxcnt++;
}

/*
 * Generic protocol message send routine
 */
int
psm_snd(proto_hdr_t *ph, db_t *db)
{
	act_hdr_t *ah = ph->ph_parent;

	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));
	ASSERT(ah->ah_inuse == INUSE); /*DEBUG*/

	if (ah->ah_phase == PHASE_DEAD) {
		psm_log(MSG_WARN, ph, "Discarding message - Phase DEAD\n");
		db_free(db);
		return;
	}

	/*#ifdef DEBUG_LCP*/
	{
		struct lcp_hdr_s *lcp;
		int len, bytes;
		lcp = (struct lcp_hdr_s *)(db->db_rptr + 2);
		len = ntohs(lcp->lcp_len);
		bytes = db->db_wptr - 2 - db->db_rptr;
		if (len != bytes) {
			psm_log(MSG_WARN, ph, "Sending packet wrong length (len %d, bytes %d)\n", len, bytes);
			abort();
		}
	}
	/*#endif*/

	switch(ah->ah_type) {
	case DEF_LINK:
		db->db_lindex = ah->ah_index;
		db->db_bindex = -1;
		break;
	case DEF_BUNDLE:
		db->db_lindex = -1;
		db->db_bindex = ah->ah_index;
		break;
	default:
		psm_log(MSG_ERROR, ph,
		       "psm_snd: Cannot send message, definition wrong type %d\n",
		       ah->ah_type);
		db_free(db);
		return;
	}

	ph->ph_state.ps_txcnt++;
	cd_d2k_proto(ah, db);
}
	
/*
 * Add the protocol field to a message
 */
void
psm_add_pf(proto_hdr_t *ph, db_t *db)
{
	ushort_t *proto = (ushort_t *)db->db_wptr;
	psm_tab_t *pt = ph->ph_psmtab;

	ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/
	/*
	 * Assume no PFC, if PFC is required later, we just
	 * ignore the first byte of the message
	 */
	*proto = htons(pt->pt_proto);
	db->db_wptr += sizeof(ushort_t);
	db->db_band = pt->pt_pri;
}

/*
 * Set a messages priority
 */
void
psm_set_pri(proto_hdr_t *ph, db_t *db)
{
	psm_tab_t *pt = ph->ph_psmtab;
	db->db_band = pt->pt_pri;
}

/*
 * Routines to dynamically load the protocol support modules
 */
STATIC struct cp_tab_s {
	char *cp_psm_name;
	void *cp_handle;
	struct psm_tab_s *cp_entry;
} cp_tab[MAX_CP];


void
psm_init()
{
	int i;
	for (i = 0; i < MAX_CP; i++)
		cp_tab[i].cp_handle = NULL;
}

/*
 * Get the psm table entry for a specified psm
 */
struct psm_tab_s *
psm_getpt(char *psm_name)
{
	int i, ret;
	void *handle;
	char buf[256];
	struct cp_tab_s *tp = cp_tab;

	/* Check if already opened this psm */

	for (i = 0; i < MAX_CP && tp->cp_handle; i++, tp++) {
		if (strcmp(tp->cp_psm_name, psm_name) == 0)
			return tp->cp_entry;
	}

	if (tp->cp_handle) {
		ppplog(MSG_ERROR, 0,
		       "Too many PSM's loaded (max %d)\n",
		       MAX_CP);
		return NULL;
	}

	/* Not loaded ... do it now */

	sprintf(buf, "%s%s_rt.so", PSM_SO_PATH, psm_name);
	handle = dlopen(buf, RTLD_LAZY);
	if (!handle) {
		ppplog(MSG_ERROR, 0,
		       "Failed loading %s support, dlerror says '%s'\n", 
		       psm_name, dlerror());
		return NULL;
	}

	/* Find the entry point */

	tp->cp_entry = (struct psm_tab_s *) dlsym(handle, "psm_entry");

	if (!tp->cp_entry) {
		ppplog(MSG_ERROR, 0,
		       "Failed loading %s support,  psm_tab not defined!\n",
		       psm_name);
		return NULL;
	}

	if (tp->cp_entry->pt_version > PSM_API_VERSION) {
		ppplog(MSG_ERROR, 0,
		       "Failed loading %s support - version not supported,\n",
		       psm_name);
		return NULL;
	}

	tp->cp_handle = handle;
	tp->cp_psm_name = strdup(psm_name);

	if (tp->cp_entry->pt_load) {
		ret = (*tp->cp_entry->pt_load)();
		if (ret) {
			ppplog(MSG_ERROR, 0,
			       "Load routine for %s failed - PSM not loaded\n",
			       psm_name);
			/* Free off stuff */
			dlclose(tp->cp_handle);
			tp->cp_handle = NULL;
			free(tp->cp_psm_name);
			tp->cp_entry = NULL;
			return NULL;
		}
	}
	return tp->cp_entry;
}

/*
 * Call all PSM's unload routines
 */
void
psm_unload()
{
	struct cp_tab_s *tp = cp_tab;

	while (tp->cp_handle) {
		if (tp->cp_entry->pt_unload)
			(*tp->cp_entry->pt_unload)();
		tp++;
	}
}

/*
 * This routine is typically used by Network Layer Protocols.
 *
 * Bind to PPP to a set of protocols, returns a file descriptor
 * that should be linked under the Netork Layer Driver
 */
int
psm_bind(proto_hdr_t *ph)
{
	int pppfd;
	int i;
	struct ppp_bind_s *pb;

	ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/

	pppfd = open(DEV_PPP, O_RDWR);
	if (pppfd < 0) {
		psm_log(MSG_ERROR, ph, "Failed to open '%s' (errno %d)\n",
			 DEV_PPP, errno);
		return -1;
	}

	/* Count the protocols supported */

	for (i = 0; ph->ph_psmtab->pt_netproto[i]; i++) ;

	/* Tell PPP which protocols we are interested in receiving */

	pb = (struct ppp_bind_s *)malloc(sizeof(struct ppp_bind_s)
					 + (i * sizeof(ushort_t)));
	if (!pb) {
		psm_log(MSG_ERROR, ph, "psm_bind: Out of memory\n");
		return -1;
	}

	pb->pb_index = ph->ph_parent->ah_index;
	pb->pb_numproto = i;

	for (i = 0; ph->ph_psmtab->pt_netproto[i]; i++) {
		pb->pb_proto[i] = ph->ph_psmtab->pt_netproto[i];
		psm_log(MSG_DEBUG, ph,
			 "Binding to network protocol 0x%4.4x\n",
			 pb->pb_proto[i]);
	}

	if (sioctl(pppfd, PPP_BIND, pb,
		   sizeof(struct ppp_bind_s) + (i *sizeof(ushort_t))) < 0) {
		psm_log(MSG_ERROR, ph, "psm_bind: PPP_BIND failed (%d)\n",
			 errno);
		close(pppfd);
		free(pb);
		return -1;
	}

	free(pb);
	return pppfd;
}

/*
 * Logging routine used by protocol support modules
 */
void
psm_log(msg_level_t level, proto_hdr_t *ph, char *fmt, ...)
{
	extern FILE *log_fp;
	va_list ap;

	if (!display_msg(ph ? ph->ph_parent : 0, level))
		return;

	display_transport(log_fp, ph ? ph->ph_parent : 0);

	if (ph) {
		ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/
		ASSERT(ph->ph_parent ? ph->ph_parent->ah_inuse == INUSE : 1); /*DEBUG*/

		fprintf(log_fp, "%s ", ph->ph_psmtab->pt_desc);
	}
	va_start(ap, fmt);
	PPPLOG_CMN(log_fp, level, fmt, ap);
	va_end(ap);
}

int
psm_loghex(int level, proto_hdr_t *ph, char *p, int cnt)
{
	extern FILE *log_fp;
	int i, j;


	if (!display_msg(ph ? ph->ph_parent : 0, level))
		return;

#define CHAR_LINE 16

	for (i = 0; i < cnt; i += CHAR_LINE) {

		display_transport(log_fp, ph ? ph->ph_parent : 0);

		for (j = 0; j < CHAR_LINE; j++) {
			if (i + j < cnt) {
				fprintf(log_fp, "%2.2x",
					((int)*(p + j) & 0xff));
			} else {
				fprintf(log_fp, "..");
			}

			if (((j + 1) % 4) == 0)
				fprintf(log_fp, " ");
		}

		fprintf(log_fp, " ");

		for (j = 0; j < CHAR_LINE; j++) {
			if (i + j < cnt && isprint(*(p + j) & 0x7f)) {
				fprintf(log_fp, "%c", (*(p + j) & 0x7f));
			} else {
				fprintf(log_fp, ".");
			}
		}

		p += CHAR_LINE;
		fprintf(log_fp, "\n");
	}
	fflush(log_fp);
}
#undef CHAR_LINE

int
psm_logstr(int level, proto_hdr_t *ph, char *p, int cnt)
{
	extern FILE *log_fp;
	int i, j;

	if (!display_msg(ph ? ph->ph_parent : 0, level))
		return;

#define CHAR_LINE 32

	for (i = 0; i < cnt; i += CHAR_LINE) {

		display_transport(log_fp, ph ? ph->ph_parent : 0);

		fprintf(log_fp, "    ");

		for (j = 0; j < CHAR_LINE; j++) {
			if (i + j < cnt && isprint(*(p + j) & 0x7f)) {
				fprintf(log_fp, "%c", (*(p + j) & 0x7f));
			} else {
				fprintf(log_fp, " ");
			}
		}

		p += CHAR_LINE;
		fprintf(log_fp, "\n");
	}
	fflush(log_fp);
}

int
psm_assert(char *x, char *f, int l)
{
	psm_log(MSG_ERROR, 0,
		"ASSERT FAILED: %s, file %s, line %d\n", x, f, l);
}

/*
 * Tell the kernel that an NCP is on it's way to being available
 * for traffic.
 *
 * This should be called as soon as an NCP has reached the OPENED state
 * and is typically called from the NCP's PSM _up() function.
 */
void
psm_ncp_up(proto_hdr_t *ph)
{
	ushort_t proto;

	ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/
	ASSERT(ph->ph_parent ? ph->ph_parent->ah_inuse == INUSE : 1); /*DEBUG*/
	ASSERT(!(ph->ph_flags & PHF_NCP_UP));
	psm_log(MSG_INFO_MED, ph, "Notify kernel protocol UP\n");
	proto = ph->ph_psmtab->pt_netproto[0];
	cd_d2k_adm_ncp(ph->ph_parent, CMD_UP, proto, 0);
}

/*
 * Tell the kernel that an NCP available for traffic.
 *
 * This should be called as soon as an NCP is ready to pass traffic
 * and is typically called from the NCP's PSM _up() function.
 */
void
psm_ncp_open(proto_hdr_t *ph)
{
	ushort_t proto;

	ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/
	ASSERT(ph->ph_parent ? ph->ph_parent->ah_inuse == INUSE : 1); /*DEBUG*/
	ASSERT(!(ph->ph_flags & PHF_NCP_UP));
	psm_log(MSG_INFO_MED, ph, "Notify kernel protocol OPENED\n");
	proto = ph->ph_psmtab->pt_netproto[0];
	ph->ph_flags &= ~PHF_IDLE;
	ph->ph_flags |= PHF_NCP_UP;
	cd_d2k_adm_ncp(ph->ph_parent, CMD_OPEN, proto, 0);
	cd_d2k_cfg_ncp(ph->ph_parent, proto);
}

void
psm_ncp_down(proto_hdr_t *ph)
{
	ushort_t proto;

	ASSERT(ph->ph_inuse == INUSE);/*DEBUG*/
	ASSERT(ph->ph_parent ? ph->ph_parent->ah_inuse == INUSE : 1); /*DEBUG*/

	if (!(ph->ph_flags & PHF_NCP_UP))
		return;

	psm_log(MSG_INFO_MED, ph, "Notify kernel protocol CLOSED\n");

	proto = ph->ph_psmtab->pt_netproto[0];
	cd_d2k_adm_ncp(ph->ph_parent, CMD_CLOSE, proto, 0);

	ph->ph_flags &= ~PHF_NCP_UP;

	/* If we were idle ... don't count us as idle any more */
	if (ph->ph_flags & PHF_IDLE) {
		ph->ph_flags &= ~PHF_IDLE;
		bundle_ncp_active(ph->ph_parent);
	}
}

void
psm_close_link(act_hdr_t *al, int reason)
{
	ASSERT(al->ah_type == DEF_LINK);
	ASSERT(al->ah_inuse == INUSE);

	ppplog(MSG_DEBUG, al, "psm_close_link: Entered - Reason %d\n", reason);

	al->ah_link.al_reason |= reason;

	if (al->ah_link.al_type == LNK_STATIC) {

		ppp_phase(al, PHASE_ESTAB);
		fsm_state(al->ah_link.al_lcp, OPEN, 0);

	} else if (al->ah_link.al_flags & ALF_INUSE) {

		fsm_state(al->ah_link.al_lcp, CLOSE, 0);
		/* Allow some time for packets to transfer */
		nap(100);
		bundle_link_finished(al);

	}
	ppplog(MSG_DEBUG, al, "psm_close_link: Closed.\n");
}

void
psm_reject(act_hdr_t *ah, db_t *db, ushort_t proto)
{
	proto_hdr_t *ph;
	db_t *ndb;

	if (ah->ah_phase != PHASE_AUTH && ah->ah_phase != PHASE_NETWORK) {
		ppplog(MSG_INFO_MED, ah,
       "LCP Silently Discarding packet with unknown protocol (0x%4.4x)\n",
		       proto);
		return;
	}

	if (ah->ah_type == DEF_BUNDLE) {
		ph = act_findproto(ah, PROTO_LCP, 0);

		if (!ph) {
			ppplog(MSG_ERROR, ah, "No LCP on bundle .. \n");
			db_free(db);
			return;
		}
	} else
		ph = ah->ah_link.al_lcp;

	ndb = (db_t *)mk_proto_rej(ph, db, proto);

	db_free(db);

	if (!ndb)
		return;

	psm_log(MSG_INFO_MED, ph, "Send Protocol Reject for 0x%4.4x\n", proto);
	psm_snd(ph, ndb);
}

/********************************************
 *	Algorithm support routines
 ********************************************/
/*
 * Get an algorithms header given its number
 */
alg_hdr_t *
psm_find_alg(proto_hdr_t *ph, ushort_t alg, int flags)
{
	struct alg_s *as = (struct alg_s *)ph->ph_priv;
	alg_hdr_t *al;

	if (flags == PSM_TX)
		al = as->as_tx_algs;
	else
		al = as->as_rx_algs;

	while (al) {
		if (al->ag_id == alg)
			break;
		al = al->ag_next;
	}
	return al;
}

/*
 * Get the specified primary algorithm
 */
alg_hdr_t *
psm_get_alg(proto_hdr_t *ph, int flags)
{
	struct alg_s *as = (struct alg_s *)ph->ph_priv;

	if (flags == PSM_TX)
		return as->as_tx;
	else
		return as->as_rx;
}

/*
 * Set the primary algorithm to that specified
 */
void
psm_set_alg(proto_hdr_t *ph, int flags, alg_hdr_t *ag)
{
	struct alg_s *as = (struct alg_s *)ph->ph_priv;

	if (flags == PSM_TX)
		as->as_tx = ag;
	else
		as->as_rx = ag;
}
