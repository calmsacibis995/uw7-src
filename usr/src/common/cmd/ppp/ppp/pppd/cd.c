#ident	"@(#)cd.c	1.10"

#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <termios.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <synch.h>
#include <thread.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ppp.h>
#include <sys/ppp_psm.h>
#include <sys/ppp_pcid.h>
#include <sys/conf.h>

#include "pathnames.h"
#include "fsm.h"
#include "psm.h"
#include "ppp_type.h"
#include "ppp_proto.h"
#include "ppp_cfg.h"
#include "act.h"
#include "lcp_hdr.h"
#include "ulr.h"

/*
 * This is the exact size of control messages received 
 * from the PCID driver
 */
#define MCTL_SIZE (sizeof(struct ppp_inf_ctl_s))

/*
 * This is the maximum size of received data messages
 * from the PCID driver. Note: Our MRU or MMRU must
 * not exceed this value on any link or bundle.
 */
#define MDATA_SIZE 4096

extern int pcid_fd;
struct psm_tab_s *psm_findpt(ushort_t protocol);

/*
 * This function is called to handle incoming PPP messages
 * that have been received from the PCID driver.
 */
void
cd_k2d_proto(db_t *db)
{
	int i, ret;
	ushort_t proto;
	act_hdr_t *al = NULL, *ab = NULL;
	psm_tab_t *pt;
	struct proto_hdr_s *pr;

	if (db->db_rptr == db->db_wptr) {
		ppplog(MSG_ERROR, 0, "Zero length packet received on bundle %d, link %d\n",
		       db->db_bindex, db->db_lindex);
		db_free(db);
		return;
	}

	/* Look at the protocol field, so we know who processes this message */

	proto = *(db->db_rptr++);

	/* Check for a compressed protocol field */

	if (!(proto & 0x01)) {
		if (db->db_rptr == db->db_wptr) {
			ppplog(MSG_WARN, 0, "Packet too short on bundle %d, link %d\n",
			       db->db_bindex, db->db_lindex);
			db_free(db);
			return;
		}
		proto <<= 8;
		proto |= *(db->db_rptr++);
	}

busy:
	/*
	 * Find the transport (link or bundle) that this message is from
	 */
	ret = act_findtrans(db->db_lindex, db->db_bindex, &al, &ab);
	if (ret) {
		/*
		 * This condition can occur if data arrives for a link/bundle
		 * pair while we are destroying it.
		 */
		ppplogdb(MSG_INFO_LOW, 0, "Received Unexpected", db);
		ppplog(MSG_INFO_LOW, 0,
		       "Discarded data, didn't find active link bundle pair (lindex %d, bindex %d)\n", ret, db->db_lindex, db->db_bindex);
		db_free(db);
		return;
	}

	/* We must be the only process accessing the link ! */

	if (al && ATOMIC_INT_READ(al->ah_refcnt) > 0) {
		act_release(al);
		if (ab)
			act_release(ab);
		nap(200);
		goto busy;
	}

	/*
	 * We have this message arriving on a link, ensure it is opened
	 */
	if (al && !(al->ah_link.al_flags & ALF_INUSE)) {
		ppplog(MSG_WARN, al,
		       "Received data on closed link - discard\n");
		db_free(db);
		act_release(al);
		if (ab)
			act_release(ab);
		return;
	}

	/*
	 * Check that this link/bundle is configured for this protocol
	 * and call the receive function
	 */
	pr = act_findproto((al ? al : ab), proto, 0);
	if (!pr) 
		goto reject;

	/*
	 * The owner of the message (link or bundle) is pointeed
	 * to by the proto header .. drop the lock of the non owner
	 */
	ppplogdb(MSG_INFO_HIGH, pr->ph_parent, "Received", db);

	(*pr->ph_psmtab->pt_rcv)(pr, db, proto);
	
	if (al)
		act_release(al);
	if (ab)
		act_release(ab);
	return;

reject:
	/* Received an unexpected protocol ... see if we need to reject it */

	ppplogdb(MSG_INFO_HIGH, (al ? al : ab), "Received", db);


	ppplog(MSG_DEBUG, (al ? al : ab),
	       "cd_k2d_proto: Reject: al = %x, ab = %x, proto = %x\n",
	       al, ab, proto);

	/*
	 * Check if the the protocol is one that is supported ..
	 * If we receive IP datagrams, for example, before IPCP is opened
	 * then they will be passed up to us ... we should silently 
	 * discard them
	 */
	pr = act_findproto(al ? al : ab, proto, 1);
	if (pr) {
		/* If the PSM can handle datagrams .. pass them on */
		if (pr->ph_psmtab->pt_flags & PT_WANTD)
			(*pr->ph_psmtab->pt_rcv)(pr, db, proto);
		else {
			ppplog(MSG_INFO_MED, (al ? al : ab),
			       "Silent discard of protocol 0x%4.4x\n", proto);
			db_free(db);
		}
	} else
		/*
		 * Protocol not supported - Send a protocol reject
		 * if LCP is OPENED
		 */
		psm_reject(al ? al : ab, db, proto);
	
	if (al)
		act_release(al);
	if (ab)
		act_release(ab);
	return;
}

/*
 * Called when we receive an NCP datagram to log
 */
void
cd_k2d_log(db_t *db)
{
	act_hdr_t *ab, *ah;
	int ret;
	proto_hdr_t *pr;

	switch(db->db_cmd) {
	case CMD_LOG_NCP:
		ret = act_find_bundle(db->db_bindex, &ab);
		if (ret) {
			ppplog(MSG_DEBUG, 0,
		       "cd_k2d_log: Internal error from act_find_bundle %d\n",
			       ret);
			db_free(db);
			return;
		}

		pr = act_findproto(ab, db->db_proto, 1);
		if (!pr) {
			ppplog(MSG_WARN, ab,	
			       "Unexpected protocol for the bundle\n");
			db_free(db);
		} else
			(*pr->ph_psmtab->pt_log)(pr, db, (int)db->db_proto);
		act_release(ab);
		break;

	case CMD_LOG_WIRE:
		if (db->db_lindex > 0)
			ret = act_find_link(db->db_lindex, &ah);
		else
			ret = act_find_bundle(db->db_bindex, &ah);

		ppplog(MSG_INFO_MED, ah, "LOG_WIRE: %s\n",
		       (db->db_proto == PROTO_SENT) ? "Transmit" : "Receive");

		ppploghex(MSG_INFO_HIGH, ah, db->db_rptr,
			  db->db_wptr - db->db_rptr);
		db_free(db);
		act_release(ah);
		break;
	}
}

void
cd_k2d_adm_psm(db_t *db)
{
	act_hdr_t *ab, *al;
	int ret;
	proto_hdr_t *pr;
	struct ppp_inf_dt_s *di;

	ret = act_findtrans(db->db_lindex, db->db_bindex, &al, &ab);
	if (ret) {
		ppplog(MSG_DEBUG, 0,
 "cd_k2d_adm_psm: Internal error from act_findtrans %d, link %d, bundle %d\n",
		       ret, db->db_lindex, db->db_lindex);
		db_free(db);
		return;
	}

	ppplogdb(MSG_INFO_HIGH, ab ? ab : al, "Received", db);

	pr = act_findproto(ab ? ab : al, PSM_PROTO(db->db_proto), 1);
	if (!pr) {
		act_display(ab ? ab : al);
		ppplog(MSG_WARN, ab ? ab : al,	
	       "Unexpected protocol for the bundle/link, proto 0x%4.4x\n",
		       db->db_proto);
		goto exit;
	} 
	
	switch(db->db_cmd) {
	case CMD_MSG:
		di = (struct ppp_inf_dt_s *)db->db_rptr;

		/*
		 * Pass the contained message to the protocol
		 * handler
		 */
		if (pr->ph_psmtab->pt_k2d_msg)
			(*pr->ph_psmtab->pt_k2d_msg)(pr,
						     PSM_PROTO(db->db_proto),
						     PSM_FLAGS(db->db_proto),
						     di->di_psm_opts,
						     di->di_psm_optsz);
		break;

	case CMD_STATS:
		ASSERT(db->db_wptr - db->db_rptr <= ULR_PSM_MAX);
		/*
		 * Store the stats, we assume that the stats structure for
		 * a particular PSM is always the same size. Therefore
		 * we can re-use the memory first allocated for the stats
		 * for all subsequent stats
		 */
		switch (PSM_FLAGS(db->db_proto)) {
		case PSM_TX:
			if (!pr->ph_tx_stats)
				pr->ph_tx_stats = (void *)
					malloc(ULR_PSM_MAX);

			/* Save the stats */
			memcpy(pr->ph_tx_stats, db->db_rptr,
			       db->db_wptr - db->db_rptr);
			break;

		case PSM_RX:
			if (!pr->ph_rx_stats)
				pr->ph_rx_stats = (void *)
					malloc(ULR_PSM_MAX);

			/* Save the stats */
			memcpy(pr->ph_rx_stats, db->db_rptr,
			       db->db_wptr - db->db_rptr);
			break;
		}
		break;

	default:
		ppplog(MSG_ERROR, ab ? ab : al,
		       "Unexpected message for PSM\n");
		break;
	}

 exit:
	if (al)
		act_release(al);
	if (ab)
		act_release(ab);
	db_free(db);
}

int cd_d2k_adm_ncp(act_hdr_t *ab, int cmd, ushort_t proto, int err);

void
cd_k2d_adm_ncp(db_t *db)
{
	act_hdr_t *ab;
	int ret;
	proto_hdr_t *pr;
	ushort_t proto;

	ret = act_find_bundle(db->db_bindex, &ab);
	if (ret) {
		ppplog(MSG_DEBUG, 0,
	       "cd_k2d_adm_ncp: Internal error from act_find_bundle %d\n",
		       ret);
		db_free(db);
		return;
	}

	ppplogdb(MSG_INFO_HIGH, ab, "Received", db);

	pr = act_findproto(ab, db->db_proto, 1);
	if (!pr) {
		ppplog(MSG_WARN, ab,	
		       "Unexpected protocol for the bundle\n");
		act_release(ab);
		db_free(db);
		return;
	} 
	
	switch(db->db_cmd) {
	case CMD_OPEN:
		psm_log(MSG_DEBUG, pr, "CMD_OPEN - Ignore for now\n");
		break;

	case CMD_CLOSE:
		psm_log(MSG_DEBUG, pr, "CMD_CLOSE - Ignore for now\n");
		break;

	case CMD_IDLE:
		psm_log(MSG_DEBUG, pr, "CMD_IDLE\n");
		if (pr->ph_flags & PHF_IDLE) {
			psm_log(MSG_DEBUG, pr, "CMD_IDLE ... idle !\n");
			break;
		}
		if (!(pr->ph_flags & PHF_NCP_UP)) {
			psm_log(MSG_DEBUG, pr, "CMD_IDLE ... ncp not up !\n");
			break;
		}
		pr->ph_flags |= PHF_IDLE;
		bundle_ncp_idle(ab);
		break;

	case CMD_ACTIVE:
		psm_log(MSG_DEBUG, pr, "CMD_ACTIVE\n");
		if (!(pr->ph_flags & PHF_IDLE)) {
			psm_log(MSG_DEBUG, pr, "CMD_ACTIVE .. not idle !\n");
			break;
		}
		if (!(pr->ph_flags & PHF_NCP_UP)) {
			psm_log(MSG_DEBUG, pr, "CMD_ACTIVE .. ncp not up !\n");
			break;
		}
		pr->ph_flags &= ~PHF_IDLE;
		bundle_ncp_active(ab);
		break;

	}	
	act_release(ab);
	db_free(db);
}

void
cd_k2d_adm_link(db_t *db)
{
	act_hdr_t *al, *ab;
	struct ppp_inf_dt_s *di;
	int ret;

	ppplog(MSG_DEBUG, 0, "GOT k2d_adm_link: lindex = %d ... waiting\n",
	       db->db_lindex);

	ret = act_findtrans(db->db_lindex, -1, &al, &ab);
	if (ret) {
		ppplog(MSG_DEBUG, 0,
	       "cd_k2d_adm_link: Internal error from act_find_link %d\n",
		       ret);
		db_free(db);
		return;
	}

	ppplogdb(MSG_INFO_HIGH, al, "Received", db);

	ASSERT(al->ah_type == DEF_LINK);

	switch(db->db_cmd) {
	case CMD_CLOSE:
		/* The link has been dropped */
		ppplog(MSG_INFO_LOW, al,
		       "Physical Link has dropped. (M_HANGUP)\n");

		/* For static links, we never beleive the link goes down */
		psm_close_link(al, ALR_HANGUP);
		break;

	case CMD_STATS:
		di = (struct ppp_inf_dt_s *)db->db_rptr;

		ppplog(MSG_DEBUG, al, "Got Statistics update\n");
		memcpy(&al->ah_link.al_stats, &di->di_linkstats,
		       sizeof(struct pc_stats_s));
		break;
	default:
		ppplog(MSG_ERROR, 0,
		       "cd_k2d_adm_link: Internal Error: Cmd %d\n",
		       db->db_cmd);
	}

	act_release(al);
	if (ab)
		act_release(ab);
	db_free(db);
}

void
cd_k2d_adm_bundle(db_t *db)
{
	act_hdr_t *ab;
	struct ppp_inf_dt_s *di;
	int ret, dial_error;
	struct cfg_bundle *cb;

 open_more:
	ret = act_find_bundle(db->db_bindex, &ab);
	if (ret) {
		ppplog(MSG_DEBUG, 0,
	       "cd_k2d_adm_bundle: Internal error from act_find_bundle %d\n",
		       ret);
		db_free(db);
		return;
	}

	ASSERT(ab->ah_type == DEF_BUNDLE);

	ppplogdb(MSG_INFO_HIGH, ab, "Received", db);

	switch(db->db_cmd) {
	case CMD_OPEN:
		/* Kernel wants the bundle UP */
		ret = bundle_open(ab, &dial_error);

		switch (ret) {
		case 0:
			break;
		case EBUSY:
			act_release(ab);
			nap(500);
			goto open_more;
		default:
			if (ab->ah_bundle.ab_open_links == 0) {
				/* Failed to open a link ... */
				ppplog(MSG_WARN, ab,
	       "Could not obtain link for bundle (error %d, dial %d)\n",
				       ret, dial_error);
				cd_d2k_adm_bundle(ab, CMD_OPEN, ret);
			}
			break;
		}
		break;

	case CMD_CLOSE:
		abort();
		break;

	case CMD_ADDBW:	/* Add some bandwidth */
		/* Check we are configured for BOD */
		cb = (struct cfg_bundle *)ab->ah_cfg;
		switch (cb->bn_bod) {
		case BOD_IN:
			if (ab->ah_bundle.ab_flags & ABF_CALLED_OUT)
				ret = ENODEV;
			break;
		case BOD_OUT:
			if (ab->ah_bundle.ab_flags & ABF_CALLED_IN)
				ret = ENODEV;
			break;
		case BOD_ANY:
			break;
		case BOD_NONE:
			ret = ENODEV;	
			break;
		}

		if (ret) {
			ppplog(MSG_INFO_MED, ab, "No BOD for this bundle\n");
			cd_d2k_adm_bundle(ab, CMD_ADDBW, ret);
			break;
		}

		ret = bundle_linkadd(ab, NULL, NULL, LNK_ANY, &dial_error);
		if (ret == EBUSY) {
			act_release(ab);
			nap(500);
			goto open_more;
		}
		break;

	case CMD_REMBW:	/* Remove some bandwidth */

		/* Check we are configured for BOD */
		cb = (struct cfg_bundle *)ab->ah_cfg;
		switch (cb->bn_bod) {
		case BOD_IN:
			if (ab->ah_bundle.ab_flags & ABF_CALLED_OUT)
				ret = ENODEV;
			break;
		case BOD_OUT:
			if (ab->ah_bundle.ab_flags & ABF_CALLED_IN)
				ret = ENODEV;
			break;
		case BOD_ANY:
			break;
		case BOD_NONE:
			ret = ENODEV;	
			break;
		}
		if (ret) {
			ppplog(MSG_INFO_MED, ab, "No BOD for this bundle\n");
			cd_d2k_adm_bundle(ab, CMD_REMBW, ret);
		} else
			bundle_linkdrop(ab, NULL, ALR_LOWBW);
		break;

	case CMD_STATS:
		di = (struct ppp_inf_dt_s *)db->db_rptr;

		ppplog(MSG_INFO_MED, ab, "Got Statistics update\n");
		memcpy(&ab->ah_bundle.ab_stats, &di->di_bundlestats,
		       sizeof(struct bl_stats_s));

		/*
		 * We get stats when the bundle is Up/Open for traffic
		 * ... so if we are waiting for an auditing entry
		 * then we assume that the bundle has just opened ..
		 */
		if (ab->ah_bundle.ab_flags & ABF_AUDIT) {
			ab->ah_bundle.ab_flags &= ~ABF_AUDIT;
			bundle_audit(ab);
		}
		break;
	}

	db_free(db);
	act_release(ab);
}

/*
 * cd_k2d_unexpected
 *
 * This function is called when an internal error occurs
 * What should we do ?
 */
void
cd_k2d_unexpected(db_t *db)
{
	ppplog(MSG_ERROR, 0,
	       "cd_k2d_unexpected: Unexpected message received %d\n",
	       db->db_func);

	db_free(db);
}

/*
 * This table is used to map PCID message types to handler functions
 * index with (ctl_func - 1)
 */
void (*cd_k2d_func[])() = {
	cd_k2d_adm_bundle,	/* PCID_ADM_BUNDLE */
	cd_k2d_adm_ncp,		/* PCID_ADM_NCP */
	cd_k2d_adm_link,	/* PCID_ADM_LINK */
	cd_k2d_adm_psm,		/* PCID_ADM_PSM */
	cd_k2d_log,		/* PCID_LOG */
	cd_k2d_proto,		/* PCID_MSG */
	cd_k2d_unexpected,	/* PCID_ADD_L2B */
};

/*
 * cd_rcv_process
 *
 * This function performs the initial processing of incoming PCID messages,
 * it determines the type of the message, and the calls the appropriate
 * handler function. The handler function is expected to free of the
 * message.
 */
void *
cd_rcv_process(void *argp)
{
	db_t *db = (db_t *)argp;

	if (db->db_func < 1 || db->db_func > PCID_LAST)
		cd_k2d_unexpected(db);
	else		
		(*cd_k2d_func[db->db_func - 1])(db);

	thr_exit(0);
}

/*
 * cd_thread
 *
 * This function reads incoming messages from the PCID driver and then
 * spawns a new thread to process the message.
 */
void *
cd_thread()
{
	int ret, flags = 0;
	db_t *db;
	struct strbuf m_data, m_ctl;

	for (;;) {
		db = db_alloc(MDATA_SIZE);
		if (!db) {
			ppplog(MSG_ERROR, 0,
			       "cd_thread: NO MEMORY.\n");
			/*
			 * There is not much we can do to recover, we just wait
			 * and then retry the allocation
			 */
			sleep(5);
			continue;
		}

		m_data.buf = (char *)db->db_wptr;
		m_data.maxlen = db->db_lim - db->db_wptr;

		m_ctl.buf = (char *)&db->db_ctl;
		m_ctl.maxlen = MCTL_SIZE;

		/* Get a message from the kernel */
				
		ret = getmsg(pcid_fd, &m_ctl, &m_data, &flags);
		if (ret)
			ppplog(MSG_FATAL, 0, "cd_thread: getmsg failed (%d)\n",
			       errno);

		db->db_wptr += m_data.len;

		if (m_ctl.len != MCTL_SIZE)
			ppplog(MSG_FATAL, 0,
			       "cd_thread: Message has CTL wrong size\n");

		/*
		 * Create a new thread to process the message, the thread
		 * created must not sleep/block.
		 */
		ret = thr_create(NULL, 0, cd_rcv_process, (void *)db,
				 THR_DETACHED | THR_BOUND, NULL);

		if (ret) {
			ppplog(MSG_ERROR, 0,
			       "cd_thread: Failed to create new thread (%d)\n",
			       errno);
			db_free(db);	
		}
	}
}

/*
 * Send a PPP Protocol message to the kernel for transmission
 */
int
cd_d2k_proto(act_hdr_t *ah, db_t *db)
{
	struct strbuf m_data, m_ctl;

	db->db_func = PCID_MSG;

	if (ah->ah_type == DEF_LINK && !(ah->ah_link.al_flags & ALF_COND)) {
		ppplog(MSG_WARN, ah,
		       "Physical Link not available, dropping frame\n");
		db_free(db);
		return EIO;
	}
	return cd_snd_msg(ah, db);
}

/*
 * cd_snd_msg
 *
 * Send a message to the PCID driver. The destination link/bundle is 
 * indicated by the act_hdr_t and the data to send by the db_t
 */
int
cd_snd_msg(act_hdr_t *ah, db_t *db)
{
	struct strbuf m_data, m_ctl;
	int ret;

	ASSERT(db->db_wptr <= db->db_lim);
	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));
	ASSERT(db->db_ref > 0);

	m_data.buf = (char *)db->db_rptr;
	m_data.len = m_data.maxlen = db->db_wptr - db->db_rptr;
	m_ctl.buf = (char *)&db->db_ctl;
	m_ctl.maxlen = m_ctl.len = MCTL_SIZE;

	ppplogdb(MSG_INFO_HIGH, ah, "Sent", db);

	ret = putpmsg(pcid_fd, &m_ctl, &m_data, db->db_band, MSG_BAND);
	if (ret)
		ppplog(MSG_ERROR, 0, "cd_snd_msg: putmsg failed (%d)\n",
		       errno);

	db_free(db);
	return(ret);
}

int
cd_add_link2bundle(act_hdr_t *al, act_hdr_t *ab, int error)
{
	db_t *db;
	struct ppp_inf_dt_s *di;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));
	db->db_bindex = ab->ah_index;
	db->db_func = PCID_ADD_L2B;
	db->db_error = error;

	di = (struct ppp_inf_dt_s *)db->db_wptr;
	db->db_wptr += sizeof(struct ppp_inf_dt_s);
	if (al)
		di->di_lindex = al->ah_index;
	else
		di->di_lindex = -1;

	return cd_snd_msg(ab, db);
}

int
cd_d2k_adm_link(act_hdr_t *al, int cmd, int ret)
{
	db_t *db;
	struct ppp_inf_dt_s *di;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));
	db->db_lindex = al->ah_index;
	db->db_bindex = -1;
	db->db_func = PCID_ADM_LINK;
	db->db_error = ret;
	db->db_cmd = cmd;

	return cd_snd_msg(al, db);
}

/*
 * This function tells the ppp kernel drivers about the negotiated options
 */
#define max(a, b) ((a) > (b) ? (a) : (b))
int
cd_d2k_cfg_link(act_hdr_t *ah)
{
	db_t *db;
	struct ppp_inf_dt_s *di;
	extern pppd_debug;

	act_link_t *al = &ah->ah_link;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));
	db->db_lindex = ah->ah_index;
	db->db_func = PCID_ADM_LINK;
	db->db_cmd = CMD_CFG;

	di = (struct ppp_inf_dt_s *)db->db_wptr;
	db->db_wptr += sizeof(struct ppp_inf_dt_s);

	di->di_localaccm = al->al_local_accm;
	di->di_remoteaccm = al->al_peer_accm;

	/*
	 * RFC 1661 says that we must be prepared to accept frames 
	 * that are 1500 bytes, even if we negotiated a smaller mru
	 */
	di->di_localmru = max(al->al_local_mru, DEFAULT_MRU);
	di->di_remotemru = al->al_peer_mru;
	di->di_opts = al->al_peer_opts;
	if (pppd_debug & DEBUG_ANVL)
		di->di_opts |= ALO_STRICT;
	di->di_bandwidth = al->al_bandwidth;
	di->di_pc_debug = (ah->ah_debug >= DBG_WIRE);
		
	cd_snd_msg(ah, db);
}

int
cd_d2k_adm_ncp(act_hdr_t *ab, int cmd, ushort_t proto, int err)
{
	db_t *db;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));
	db->db_lindex = -1;
	db->db_bindex = ab->ah_index;
	db->db_func = PCID_ADM_NCP;
	db->db_cmd = cmd;
	db->db_error = err;
	db->db_proto = proto;

	return cd_snd_msg(ab, db); 
}

int
cd_d2k_cfg_ncp(act_hdr_t *ab, ushort_t proto)
{
	db_t *db;
	struct ppp_inf_dt_s *di;
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));
	db->db_lindex = -1;
	db->db_bindex = ab->ah_index;
	db->db_func = PCID_ADM_NCP;
	db->db_cmd = CMD_CFG;
	db->db_error = 0;
	db->db_proto = proto;

	di = (struct ppp_inf_dt_s *)db->db_wptr;
	db->db_wptr += sizeof(struct ppp_inf_dt_s);
	di->di_maxidle = cb->bn_maxidle;

	return cd_snd_msg(ab, db);
}

/*
 * Used for open/close administration
 */
cd_d2k_adm_bundle(act_hdr_t *ab, int cmd, int ret)
{
	db_t *db;
	struct ppp_inf_dt_s *di;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));
	db->db_lindex = -1;
	db->db_bindex = ab->ah_index;
	db->db_func = PCID_ADM_BUNDLE;
	db->db_cmd = cmd;
	db->db_error = ret;

	db->db_wptr += sizeof(struct ppp_inf_dt_s);

	/* We expect an audit entry after open/close */

	switch (cmd) {
	case CMD_OPEN:
	case CMD_CLOSE:
		ab->ah_bundle.ab_flags |= ABF_AUDIT;
		break;
	}

	return cd_snd_msg(ab, db);
}

/*
 * Used to configure bundle parameters
 */
cd_d2k_cfg_bundle(act_hdr_t *ab)
{
	db_t *db;
	struct ppp_inf_dt_s *di;
	struct cfg_bundle *cb = (struct cfg_bundle *)ab->ah_cfg;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));
	db->db_lindex = -1;
	db->db_bindex = ab->ah_index;
	db->db_func = PCID_ADM_BUNDLE;
	db->db_cmd = CMD_CFG;
	db->db_error = 0;

	di = (struct ppp_inf_dt_s *)db->db_wptr;
	db->db_wptr += sizeof(struct ppp_inf_dt_s);
	di->di_mtu = ab->ah_bundle.ab_mtu;
	di->di_addload = cb->bn_addload;
	di->di_dropload = cb->bn_dropload;
	di->di_addsample = cb->bn_addsample;
	di->di_dropsample = cb->bn_dropsample;
	di->di_thrashtime = cb->bn_thrashtime;
	di->di_bl_debug = (ab->ah_debug >= DBG_WIRE);
	return cd_snd_msg(ab, db);
}

/*
 * Bind a protocol to a bundle
 *
 * ah		- Describes the link or bundle that we are operating on
 * id		- Registered protocol ID
 * proto	- On the wire protocol that we are binding to (want to get)
 * flags	- Send or Receive side
 * opts		- Pointer to protocol specific options
 * optsz	- Size of protocol specific options
 */
cd_d2k_bind_psm(act_hdr_t *ah, ushort_t id, ushort_t proto, int flags,
		   void *opts, int optsz)
{
	db_t *db;
	struct ppp_inf_dt_s *di;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));

	if (ah->ah_type == DEF_LINK) {
		db->db_lindex = ah->ah_index;
		db->db_bindex = -1;
	} else {
		db->db_lindex = -1;
		db->db_bindex = ah->ah_index;
	}

	db->db_func = PCID_ADM_PSM;

	db->db_proto = id | PSM_SETFLAGS(flags);
	db->db_cmd = CMD_BIND;
	db->db_error = 0;

	di = (struct ppp_inf_dt_s *)db->db_wptr;
	db->db_wptr += sizeof(struct ppp_inf_dt_s);

	di->di_psm_proto = proto;
	di->di_psm_optsz = optsz;
	memcpy(&di->di_psm_opts[0], opts, optsz);

	return cd_snd_msg(ah, db);
}

/*
 * UnBind a protocol to a bundle
 *
 * ah		- Describes the link or bundle that we are operating on
 * id		- Registered protocol ID
 * proto	- On the wire protocol that we are binding to (want to get)
 * flags	- Send or Receive side
 */
cd_d2k_unbind_psm(act_hdr_t *ah, ushort_t id, ushort_t proto, int flags)
{
	db_t *db;
	struct ppp_inf_dt_s *di;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));

	if (ah->ah_type == DEF_LINK) {
		db->db_lindex = ah->ah_index;
		db->db_bindex = -1;
	} else {
		db->db_lindex = -1;
		db->db_bindex = ah->ah_index;
	}

	db->db_func = PCID_ADM_PSM;

	db->db_proto = id | PSM_SETFLAGS(flags);
	db->db_cmd = CMD_UNBIND;
	db->db_error = 0;

	di = (struct ppp_inf_dt_s *)db->db_wptr;
	db->db_wptr += sizeof(struct ppp_inf_dt_s);

	di->di_psm_proto = proto;
	di->di_psm_optsz = 0;
	return cd_snd_msg(ah, db);
}

/*
 * Send a message to a PSM
 */
cd_d2k_psm_msg(act_hdr_t *ah, ushort_t proto, int flags,
		   void *args, int argsz)
{
	db_t *db;
	struct ppp_inf_dt_s *di;

	db = db_alloc(sizeof(struct ppp_inf_dt_s));

	if (ah->ah_type == DEF_LINK) {
		db->db_lindex = ah->ah_index;
		db->db_bindex = -1;
	} else {
		db->db_lindex = -1;
		db->db_bindex = ah->ah_index;
	}

	db->db_func = PCID_ADM_PSM;

	db->db_proto = proto | PSM_SETFLAGS(flags);
	db->db_cmd = CMD_MSG;
	db->db_error = 0;

	di = (struct ppp_inf_dt_s *)db->db_wptr;
	db->db_wptr += sizeof(struct ppp_inf_dt_s);

	di->di_psm_optsz = argsz;
	memcpy(&di->di_psm_opts[0], args, argsz);

	return cd_snd_msg(ah, db);
}
