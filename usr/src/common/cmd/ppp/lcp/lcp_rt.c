#ident	"@(#)lcp_rt.c	1.11"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <synch.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ppp.h>
#include <sys/ppp_pcid.h>
#include <sys/ppp_ml.h>
#include <sys/mod.h>
#include <fcntl.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/stropts.h>
#define DLGADDR (('D' << 8) | 5)

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "psm.h"
#include "act.h"
#include "lcp_hdr.h"
#include "ppp_proto.h"
#include "lcp.h"
#include "lcp_cfg.h"
#include "auth.h"

STATIC void lcp_up(proto_hdr_t *ph);
STATIC void lcp_down(proto_hdr_t *ph);
STATIC void lcp_start(proto_hdr_t *ph);
STATIC void lcp_finish(proto_hdr_t *ph);
STATIC void lcp_restart(proto_hdr_t *ph);
STATIC void lcp_crossed(proto_hdr_t *ph);

#define min(a,b) ((a) > (b) ? (b) : (a))

#define PPPML_MOD "pppml"	/* The multilink module */

#define NETINFO "/usr/sbin/netinfo -l dev" /* Get net devices */
#define DEVLEN 255	/* Max device name length */
#define DEVPATH "/dev/" /* Where devices live */
#define PATHLEN 255 + 5	/* Max pathname length for a device */

extern struct cfg_global *global[];

/*
 * Lcp private data
 */
struct lcp_s {
	ushort_t	lcp_echo_timeout;
	ushort_t	lcp_echo_maxfail;
	ushort_t	lcp_echo_sample;
	ushort_t	lcp_echo_zeros;
	uint_t		lcp_echo_bits;
	ushort_t	lcp_authopts;	/* Configured auth algs 060f*/
	ushort_t	lcp_flags;
	void 		*lcp_echo_timid;
};

#define LF_ECHO_OK	0x0001	/* Received an LCP Echo reply */

#define ECHO_TEST(lcp, id) \
	((lcp)->lcp_echo_bits & (1 << ((id) % (lcp)->lcp_echo_sample)))

#define ECHO_SET(lcp, id) \
	((lcp)->lcp_echo_bits |= (1 << ((id) % (lcp)->lcp_echo_sample)))

#define ECHO_UNSET(lcp, id) \
	((lcp)->lcp_echo_bits &= ~(1 << ((id) % (lcp)->lcp_echo_sample)))

/*
 * Multi Link options MRRU, SSN, ED
 *
 * These options must have consistent values across all links within
 * a single bundle
 *
 * When an incoming link wishes to join a bundle, we must check that these 
 * values match those already negotiated for the bundle. If the links
 * values don't match then we need to re-negotiate LCP.
 *
 * On outgoing links, if we are a primary link (first) then try for ML, with
 * configured values. If we are a secondary, then we should attempt the values
 * alreay obtained for the bundle. ... but we may not get these .. the peer may
 * wait until we have authenticated before allowing ML. ... so we allow the
 * link to have different values ... but it must not reach NETWORK PHASE
 * until the paramters for the link match those for the bundle.
 */


/*
 * Generate a new magic number given the current one
 */
STATIC ulong_t
gen_magic(ulong_t current)
{
	return ((current + time(NULL)) * 125621 + 3);
}

/*
 * Send an LCP Echo Request.	
 */
STATIC int
lcp_echo_snd(proto_hdr_t *pr)
{
	db_t *db;
	struct lcp_echo_s *echo;
	proto_state_t *ps = &pr->ph_state;
	struct lcp_s *lcp = (struct lcp_s *)pr->ph_priv;

	db = db_alloc(PSM_MTU(pr->ph_parent));
	if (!db) {
		psm_log(MSG_WARN, pr, "No memory - no Echo Request sent\n");
		return;
	}

	psm_add_pf(pr, db);

	psm_log(MSG_INFO_MED, pr, "Sending Echo Request\n");
	
	echo = (struct lcp_echo_s *)db->db_wptr;
	echo->lcp_hdr.lcp_code = ECO_REQ;
	echo->lcp_hdr.lcp_id = ++ps->ps_lastid;
	echo->lcp_hdr.lcp_len = htons(sizeof(struct lcp_echo_s));
	echo->lcp_magic = htonl(pr->ph_parent->ah_link.al_local_magic);
	db->db_wptr += sizeof(struct lcp_echo_s);

	/*
	 * If the bit we are using was set, unset it and
	 * increment the zeros count.
	 */
	if (ECHO_TEST(lcp, ps->ps_lastid)) {
		lcp->lcp_echo_zeros++;
		ECHO_UNSET(lcp, ps->ps_lastid);
	}

	lcp_snd(pr, db);
}

/*
 * Send an echo request, set a timer running
 */
STATIC int
lcp_echo_timer(proto_hdr_t *pr)
{
	proto_state_t *ps = &pr->ph_state;
	struct lcp_s *lcp = (struct lcp_s *)pr->ph_priv;
	act_hdr_t *ab, *al;

	al = pr->ph_parent;
	MUTEX_LOCK(&al->ah_mutex);

	ab = al->ah_link.al_bundle;
	if (ab) {
		/* If we have a bundle, ensure it's locked */
		ATOMIC_INT_INCR(al->ah_refcnt);
		MUTEX_UNLOCK(&al->ah_mutex);
		MUTEX_LOCK(&ab->ah_mutex);
		MUTEX_LOCK(&al->ah_mutex);
		ATOMIC_INT_DECR(al->ah_refcnt);
	}

	/*
	 * Check that we still want this event .. an untimeout could
	 * have occured while we were waiting for the ah_mutex
	 */
	if (lcp->lcp_echo_timid) {
		psm_log(MSG_DEBUG, pr,
		"lcp_echo_timer: Echo Timer Expired bits = 0x%x zeros = %d\n",
			lcp->lcp_echo_bits, lcp->lcp_echo_zeros);

		if (lcp->lcp_echo_zeros > lcp->lcp_echo_maxfail) {

			if (lcp->lcp_flags & LF_ECHO_OK) {
				psm_log(MSG_WARN, pr,
					"Link quality poor - Link Dropped\n");
				psm_close_link(pr->ph_parent, ALR_LOWQUAL);
			} else {
				psm_log(MSG_WARN, pr,
					"Echo Requests not acknowledged. Periodic echos stopped.\n");
				lcp->lcp_echo_timid = 0;
				lcp_echo_down(pr);
			}

		} else {
			lcp->lcp_echo_timid = timeout(lcp->lcp_echo_timeout,
						      lcp_echo_timer,
						      (caddr_t)pr, NULL);
			lcp_echo_snd(pr);
		}
	} 

	MUTEX_UNLOCK(&al->ah_mutex);
	if (ab)
		MUTEX_UNLOCK(&ab->ah_mutex);
}

/*
 * Start LCP Echo Requests. (Used for LQM)
 */
STATIC int
lcp_echo_up(proto_hdr_t *pr)
{
	proto_state_t *ps = &pr->ph_state;
	struct lcp_s *lcp = (struct lcp_s *)pr->ph_priv;
	act_hdr_t *ab;

	ASSERT(pr->ph_parent->ah_type == DEF_LINK);

	/*
	 * If the period is zero, this indicates LCP Echo requests
	 * are not required.
	 */
	if (lcp->lcp_echo_timeout == 0)
		return;

	lcp->lcp_echo_bits = 0xffffffff;
	lcp->lcp_echo_zeros = 0;

	/* Send the first */
	lcp_echo_snd(pr);

	/* Start the clock */
	lcp->lcp_echo_timid = timeout(lcp->lcp_echo_timeout, lcp_echo_timer,
				      (caddr_t)pr, NULL);

	psm_log(MSG_INFO_MED, pr,
		 "Echo requests Started (period %d seconds).\n",
		 lcp->lcp_echo_timeout / HZ);
}

/*
 * Stop LCP Echo Requests.
 */
STATIC int
lcp_echo_down(proto_hdr_t *pr)
{
	struct lcp_s *lcp = (struct lcp_s *)pr->ph_priv;

	if (lcp->lcp_echo_timeout == 0)
		return;

	psm_log(MSG_INFO_MED, pr, "Echo requests Stopped.\n");
	lcp->lcp_echo_timeout = 0;
	if (lcp->lcp_echo_timid)
		untimeout(lcp->lcp_echo_timid);
	lcp->lcp_echo_timid = 0;
}

/*
 * Given a device pathname return its MAC address
 */
STATIC int
get_a_macaddr(char *device, char *macaddr)
{
        int fd, ret, flags = 0;
        struct strioctl strioc;
        uchar_t llc_mc[8];
	register dl_phys_addr_req_t *brp;
	register dl_phys_addr_ack_t *bap;
	dl_error_ack_t *bep;
	unsigned char buf[80];		/* place for received table */
	unsigned char *cp;
	static union DL_primitives primbuf;
	static struct strbuf primptr = { 0, 0, (char *)&primbuf };

        if ((fd = open(device, O_RDWR)) == -1) {
		ppplog(MSG_DEBUG, 0, "get_a_macaddr: Failed to open %s\n",
		       device);
                return 0;
        }

	brp = (dl_phys_addr_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_phys_addr_req_t));
	brp->dl_primitive = DL_PHYS_ADDR_REQ;
	brp->dl_addr_type = DL_CURR_PHYS_ADDR;
	primptr.len = sizeof(dl_phys_addr_req_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0) {
		ppplog(MSG_DEBUG, 0,
		       "get_a_macaddr: putmsg failed %d\n", errno);
		close(fd);
		return(0);
	}

	primptr.maxlen = primptr.len = sizeof(primbuf) + 6;
	primptr.buf = (char *)buf;
	ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags);

	primptr.buf = (char *)&primbuf;

	if (ret < 0) {
		ppplog(MSG_DEBUG, 0,
		       "get_a_macaddr: getmsg failed %d\n", errno);
		close(fd);
		return(0);
	}

	bap = (dl_phys_addr_ack_t *)buf;
	if (bap->dl_primitive == DL_PHYS_ADDR_ACK) {
		cp = buf + bap->dl_addr_offset;
		goto got_one;
	}


        strioc.ic_cmd = DLIOCGENADDR;
        strioc.ic_timout = 15;
        strioc.ic_len = LLC_ADDR_LEN;
        strioc.ic_dp = (char *)llc_mc;
        if (ioctl(fd, I_STR, &strioc) < 0) {
                /*
                 * dlpi token driver does not recognize DLIOCGENADDR,
                 * so try DLGADDR
                 */
                strioc.ic_cmd = DLGADDR;
                strioc.ic_timout = 15;
                strioc.ic_len = LLC_ADDR_LEN;
                strioc.ic_dp = (char *)llc_mc;
                if (ioctl(fd, I_STR, &strioc) < 0) {
			ppplog(MSG_DEBUG, 0,
			       "get_a_macaddr: ioctl failed for %s\n", device);
			close(fd);
                        return 0;
                }
        }
	cp = llc_mc;

 got_one:
	close(fd);

	if (cp[0] == 0 && cp[1] == 0 && cp[2] == 0 && 
	    cp[3] == 0 && cp[4] == 0 && cp[5] == 0) {
		ppplog(MSG_DEBUG, 0, "get_a_macaddr: Address is zero\n");
		return 0;
	}

	ppplog(MSG_DEBUG, 0, "get_a_macaddr: Address vaild & returned\n");
	memcpy(macaddr, cp, 6);
        return 1;
}

/*
 * Get the MAC address of a Network card
 */
STATIC int
getmacaddr(char *buf)
{
	char path[PATHLEN+1];
	char device[DEVLEN+1];
	FILE *fp;

	if ((fp = popen(NETINFO, "r")) == NULL)
		return 0;

	while(fgets(device, DEVLEN, fp) != NULL) {
		if (strlen(device) > 0) 
			device[strlen(device) - 1] = 0;
		sprintf(path, "%s%s", DEVPATH, device);
		if (get_a_macaddr(path, buf)) {
			pclose(fp);
			return 1;
		}
	}
	pclose(fp);
	return 0;
}

/*
 * Create the Endpoint discriminator for this system
 *	- We prefer using the MAC address that can be obtained form a
 *	   configured NIC
 *	- Our fall back is to use a sequence of magic numbers
 * 
 * Not sure where this should live
 */
static uchar_t lcp_ed[MAX_ED_SIZE];
static int lcp_ed_len;
static int lcp_ed_class;

ed_init()
{
	ulong_t *mp, m;
	int i;

	if (getmacaddr((char *)lcp_ed)) {
		uchar_t *p = lcp_ed;
		/* Succeeded to get MAC addr */
		ppplog(MSG_DEBUG, 0,
		       "ed_init: Mac Addr %02X.%02X.%02X.%02X.%02X.%02X\n",
		       p[0], p[1], p[2], p[3], p[4], p[5]);
		lcp_ed_len = 6;
		lcp_ed_class = ED_MAC;
		return;
	}

	/* Use 5 PPP Magic numbers */
	m = gen_magic((ulong_t)&m);

	mp = (ulong_t *)lcp_ed;

	for (i = 0; i < 5; i++) {
		*mp++ = m;
		m = gen_magic(m);
	}

	lcp_ed_len = 20;
	lcp_ed_class = ED_MAGIC;
	ppplog(MSG_DEBUG, 0, "ed_init: Used magic number for ED\n");

}

int ml_modid;

STATIC int
lcp_load()
{
	int ret;

	ppplog(MSG_INFO_LOW, 0, "LCP Loaded\n");

	ed_init();

	/* Load the ppp multilink module */

	ml_modid = modload(PPPML_MOD);
	if (ml_modid < 0) {
		ppplog(MSG_ERROR, 0,
		       "LCP Failed to load Multilink support, errno %d\n",
		       errno);
		return -1;
	}
	return 0;
}

STATIC int
lcp_unload()
{
	int ret;

	if (ml_modid >= 0) {
		do { 
			ret = moduload(ml_modid);
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
				       "LCP Failed to unload Multilink support, errno %d\n",
				       errno);
				break;
			}
		} while (errno == EBUSY);
	}

	ppplog(MSG_INFO_LOW, 0, "LCP Unloaded\n");
}

STATIC int
lcp_init(struct act_hdr_s *ah, struct proto_hdr_s *ph)
{
	struct cfg_lcp *cp = (struct cfg_lcp *)ph->ph_cfg;
	act_link_t *al = &ah->ah_link;

	if (ah->ah_type == DEF_LINK)
		ah->ah_link.al_lcp = ph;

	fsm_init(ph);
	return 0;
}

STATIC int
lcp_alloc(proto_hdr_t *ph)
{
	ph->ph_priv = (void *)malloc(sizeof(struct lcp_s));
	if (!ph->ph_priv)
	 	return ENOMEM;
	return 0;
}

STATIC int
lcp_free(proto_hdr_t *ph)
{
	free(ph->ph_priv);
	ph->ph_priv = NULL;
}

STATIC int
lcp_rcv(proto_hdr_t *ph, db_t *db)
{
	if (ph->ph_parent->ah_type == DEF_BUNDLE) {
		struct lcp_hdr_s *lcp = (struct lcp_hdr_s *)db->db_rptr;
		/* Disallow codes that change fsm state */

		if (db->db_wptr - db->db_rptr < sizeof(struct lcp_hdr_s)) {
			psm_log(MSG_WARN, ph, "Packet too short - Discard\n");
			return CFG_DISCARD;
		}

		if (lcp->lcp_code <= TRM_ACK) {
			psm_log(MSG_WARN, ph,
				"Code %d not allowed on bundle\n",
				lcp->lcp_code);
			return CFG_DISCARD;
		}
	}
	return psm_rcv(ph, db);
}

STATIC int
lcp_snd(proto_hdr_t *ph, db_t *db)
{
	return psm_snd(ph, db);
}

char *
onoff(int onoff)
{
	return onoff ? "Enabled" : "Disabled";
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
lcp_up(proto_hdr_t *ph)
{
	act_link_t *al = &ph->ph_parent->ah_link;
	struct cfg_lcp *cp = (struct cfg_lcp *)ph->ph_cfg;
	struct lcp_s *lcp = (struct lcp_s *)ph->ph_priv;
	uint_t auth;

	psm_log(MSG_INFO_LOW, ph, "Up\n");

	/* Convert ph_peer_opts to lcp flags (these are more convenient) */

	if (!PH_TSTBIT(CO_AUTH, ph->ph_peer_opts))
		al->al_peer_opts &= ~(ALO_CHAP | ALO_PAP);
	if (!PH_TSTBIT(CO_MAGIC, ph->ph_peer_opts))
		al->al_peer_opts &= ~ALO_MAGIC;
	if (!PH_TSTBIT(CO_PFC, ph->ph_peer_opts))
		al->al_peer_opts &= ~ALO_PFC;
	if (!PH_TSTBIT(CO_ACFC, ph->ph_peer_opts))
		al->al_peer_opts &= ~ALO_ACFC;
	if (!PH_TSTBIT(CO_MRRU, ph->ph_peer_opts))
		al->al_peer_opts &= ~ALO_MRRU;
	if (!PH_TSTBIT(CO_SSN, ph->ph_peer_opts))
		al->al_peer_opts &= ~ALO_SSN;
	if (!PH_TSTBIT(CO_ED, ph->ph_peer_opts))
		al->al_peer_opts &= ~ALO_ED;
	if (!PH_TSTBIT(CO_LD, ph->ph_peer_opts))
		al->al_peer_opts &= ~ALO_LD;

	/*
	 * auth contains the type of authentication that we
	 * negotiated ... that the peer will use to
	 * authenticate with us
	 */
	auth = al->al_local_opts & (ALO_CHAP | ALO_PAP);

	/*
	 * Check that we got the authentication we required
	 * (our configured auth is in lcp_authopts)
	 */
	switch (lcp->lcp_authopts & (ALO_CHAP | ALO_PAP)) {
	case (ALO_CHAP | ALO_PAP):
		if (!auth) {
			psm_log(MSG_INFO_MED, ph,
		"We require authentication. Peer wouldn't authenticate\n");
			psm_close_link(ph->ph_parent, ALR_AUTHNEG);
			return;
		}
		break;
	case ALO_CHAP:
		if (auth != ALO_CHAP) {
			psm_log(MSG_INFO_MED, ph,
		"We require CHAP. Peer wouldn't authenticate using CHAP\n");
			psm_close_link(ph->ph_parent, ALR_AUTHNEG);
			return;
		}
		break;
	case ALO_PAP:
		if (auth != ALO_PAP) {
			psm_log(MSG_INFO_MED, ph,
		"We require PAP. Peer wouldn't authenticate using PAP\n");
			psm_close_link(ph->ph_parent, ALR_AUTHNEG);
			return;
		}
		break;
	case 0:
		if (auth) {
			psm_log(MSG_INFO_MED, ph,
		"We don't want to authenticate the peer. Peer asked us to.");
			psm_close_link(ph->ph_parent, ALR_AUTHNEG);
			return;
		}
		break;
	}

	/* Display the configured options (local and remote) */

	psm_log(MSG_INFO_LOW, ph, "Local options :\n");
	psm_log(MSG_INFO_LOW, ph, "   Mru %d\n",
		 al->al_local_mru);
	psm_log(MSG_INFO_LOW, ph, "   Accm 0x%8.8x\n",
		 al->al_local_accm);
#ifdef _LQM_
	psm_log(MSG_INFO_LOW, ph, "   LQM %s\n", 
		 onoff(al->al_local_opts & ALO_LQM));
#endif
	psm_log(MSG_INFO_LOW, ph, "   ACFC %s\n",
		 onoff(al->al_local_opts & ALO_ACFC));
	psm_log(MSG_INFO_LOW, ph, "   PFC %s\n",
		 onoff(al->al_local_opts & ALO_PFC));

	if (al->al_local_opts & ALO_MAGIC)
		psm_log(MSG_INFO_LOW, ph, "   Magic Number 0x%8.8x\n",
			 al->al_local_magic);
	else
		psm_log(MSG_INFO_LOW, ph, "   Magic Number Disabled\n");

	if (al->al_local_opts & ALO_PAP)
		psm_log(MSG_INFO_LOW, ph, "   Require PAP authentication\n");

	if (al->al_local_opts & ALO_CHAP)
		psm_log(MSG_INFO_LOW, ph, "   Require CHAP authentication\n");

	if (al->al_local_opts & ALO_MRRU) {
		psm_log(MSG_INFO_LOW, ph, "   MultiLink Enabled MRRU %d\n",
			 al->al_local_mrru);

		/* Display other ML related options */

		psm_log(MSG_INFO_LOW, ph, "   SSN %s\n",
			 onoff(al->al_local_opts & ALO_SSN));

		psm_log(MSG_INFO_LOW, ph,
			 "   Endpoint Discriminator %s (class %d, len %d)\n",
			 onoff(al->al_local_opts & ALO_ED),
			 lcp_ed_class, lcp_ed_len);

		if (al->al_local_opts & ALO_ED) {
			psm_loghex(MSG_INFO_MED, ph, (char *)lcp_ed,
				   lcp_ed_len);
		}
		
	} else
		psm_log(MSG_INFO_LOW, ph, "   MultiLink Disabled\n");

	if (al->al_local_opts & ALO_LD)
		psm_log(MSG_INFO_LOW, ph, "   Link Discriminator %d\n",
			al->al_local_ld);
	
	psm_log(MSG_INFO_LOW, ph, "Peer options :\n");
	psm_log(MSG_INFO_LOW, ph, "   Mru %d\n",
		 al->al_peer_mru);
	psm_log(MSG_INFO_LOW, ph, "   Accm 0x%8.8x\n",
		 al->al_peer_accm);
#ifdef _LQM_
	psm_log(MSG_INFO_LOW, ph, "   LQM %s\n", 
		 onoff(al->al_peer_opts & ALO_LQM));
#endif
	psm_log(MSG_INFO_LOW, ph, "   ACFC %s\n",
		 onoff(al->al_peer_opts & ALO_ACFC));
	psm_log(MSG_INFO_LOW, ph, "   PFC %s\n",
		 onoff(al->al_peer_opts & ALO_PFC));

	if (al->al_peer_opts & ALO_MAGIC)
		psm_log(MSG_INFO_LOW, ph, "   Magic Number 0x%8.8x\n",
			 al->al_peer_magic);
	else
		psm_log(MSG_INFO_LOW, ph, "   Magic Number Disabled\n");

	if (al->al_peer_opts & ALO_PAP)
		psm_log(MSG_INFO_LOW, ph, "   Require PAP authentication\n");

	if (al->al_peer_opts & ALO_CHAP)
		psm_log(MSG_INFO_LOW, ph, "   Require CHAP authentication\n");

	if (al->al_peer_opts & ALO_MRRU) {
		psm_log(MSG_INFO_LOW, ph, "   MultiLink Enabled MRRU %d\n",
			 al->al_peer_mrru);

		/* Display other ML related options */

		psm_log(MSG_INFO_LOW, ph, "   SSN %s\n",
			 onoff(al->al_peer_opts & ALO_SSN));

		psm_log(MSG_INFO_LOW, ph,
			 "   Endpoint Discriminator %s (class %d, len %d)\n",
			 onoff(al->al_peer_opts & ALO_ED),
			 al->al_peer_ed_class, al->al_peer_ed_len);

		if (al->al_peer_opts & ALO_ED) {
			psm_loghex(MSG_INFO_MED, ph, (char *)&al->al_peer_ed,
				    al->al_peer_ed_len);
		}

	} else
		psm_log(MSG_INFO_LOW, ph, "   MultiLink Disabled\n");

	if (al->al_peer_opts & ALO_LD)
		psm_log(MSG_INFO_LOW, ph, "   Link Discriminator %d\n",
			al->al_peer_ld);
	
	cd_d2k_cfg_link(ph->ph_parent);

	if (cp->lcp_ident)
		snd_identification(ph);

	lcp_echo_up(ph);

	ppp_phase(ph->ph_parent, PHASE_AUTH);
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
lcp_down(proto_hdr_t *ph)
{
	if (ph->ph_parent->ah_type != DEF_LINK) {
		ph->ph_state.ps_state = CLOSED;
		return;
	}

	psm_log(MSG_INFO_LOW, ph, "Down\n");

	lcp_echo_down(ph);

	if (ph->ph_state.ps_state == REQSENT ||
	    ph->ph_state.ps_state == ACKSENT) {
		psm_log(MSG_DEBUG, ph, "Re-starting .. reset counters\n");
		/* Reset counters */
		fsm_load_counters(ph);
		PH_RESETBITS(ph->ph_rej_opts);
	}

	/* Indicate to the bundle that the lower layer has gone down */

	bundle_link_down(ph->ph_parent);
}

/*
 *  This-Layer-Started (tls)
 *
 *      This action indicates to the lower layers that the automaton is
 *      entering the Starting state, and the lower layer is needed for the
 *      link.  The lower layer SHOULD respond with an Up event when the
 *      lower layer is available.
 *
 *
 * This routine initialises the lcp related data structures with the
 * required options, this is performed based on the configuration and
 * any previous links in the owning bundle (if any).
 */
STATIC void
lcp_start(proto_hdr_t *ph)
{
	struct cfg_lcp *cp = (struct cfg_lcp *)ph->ph_cfg;
	struct cfg_global *gl;
	struct cfg_bundle *cb;
	struct lcp_s *lcp = (struct lcp_s *)ph->ph_priv;
	act_link_t *al = &ph->ph_parent->ah_link;
	act_hdr_t *ab;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	if (ph->ph_parent->ah_type != DEF_LINK) {
		ph->ph_state.ps_state = OPENED;
		return;
	}

	psm_log(MSG_INFO_LOW, ph, "Starting\n");

	/* Load to configured values */

	al->al_local_mru = cp->lcp_mru;
	al->al_local_accm = cp->lcp_accm;
	al->al_local_magic = time(NULL) * 123;

	al->al_local_opts = 0;
	al->al_reason &= ~ALR_LOOPBACK;

#ifdef _LQM_
	if (cp->lcp_lqm)
		al->al_local_opts |= ALO_LQM;
#endif
	if (cp->lcp_acfc)
		al->al_local_opts |= ALO_ACFC;
	if (cp->lcp_pfc)
		al->al_local_opts |= ALO_PFC;
	if (cp->lcp_magic)
		al->al_local_opts |= ALO_MAGIC;

	al->al_local_qual_proto = PROTO_LQR;
	al->al_peer_mru = DEFAULT_MRU;
	al->al_peer_accm = DEFAULT_ACCM;
	al->al_peer_opts = 0;
	al->al_peer_chap_alg = 0;
	al->al_peer_qual_proto = 0;
	al->al_peer_ed_len = 0;
	al->al_peer_ed_class = 0;

	lcp->lcp_flags = 0;
	lcp->lcp_echo_timeout = cp->lcp_echo_period * HZ;
	lcp->lcp_echo_maxfail = cp->lcp_echo_fails;
	lcp->lcp_echo_sample = cp->lcp_echo_sample; 

	ab = al->al_bundle;
	if (ab) {
		/*
		 * We have a bundle in the following
		 *	- All outgoing calls.
		 *	- Login style incoming calls
		 *	- ?? Caller ID incoming calls ???
		 */
		cb = (struct cfg_bundle *)ab->ah_cfg;

		psm_log(MSG_DEBUG, ph,	
			 "lcp_start: Have bundle, set flags as in bundle\n");
		/*
		 * Multilink options are defined in the bundle, however we need
		 * a link copy that may be negotiated
		 */
		if (cb->bn_maxlinks > 1) {
			psm_log(MSG_DEBUG, ph,
				 "lcp_start: maxlinks > 1, ML cfgd\n");

			bundle_set_linkattr(ab, al);

			/*
			 * If this is the first link in a bundle then
			 * allow the Multilink options (mrru, ed and ssn)
			 * to be negotiated .... otherwise they must
			 * match the values negotiated in a previous
			 * member link.
			 */
			if (ab->ah_bundle.ab_open_links == 0) {
				/* The first link in the bundle */
				al->al_local_opts |= ALO_ML_NEG;
				al->al_peer_opts |= ALO_ML_NEG;

				/* Try for a link discriminator */
				al->al_local_opts |= ALO_LD;

				/* Set the bundles local ed */
				ab->ah_bundle.ab_local_ed_class = lcp_ed_class;
				ab->ah_bundle.ab_local_ed_len = lcp_ed_len;
				memcpy(ab->ah_bundle.ab_local_ed_addr,
				       lcp_ed, lcp_ed_len);
			} else
				psm_log(MSG_DEBUG, ph,
 			        "lcp_start: Enforce peer/local mrru,ssn,ed\n");
		}
	} else {
		/*
		 * This case occurs when we have an auto detected 
		 * PPP session. We have no idea which bundle the
		 * link will join until authentication occurs and
		 * therefore, we use the global bundle configuration
		 * or failing that ... some defaults that I chose.
		 */
		ucfg_lock();
		if (gl = global[DEF_BUNDLE]) {
			psm_log(MSG_DEBUG, ph,
			 "lcp_start: No bundle, use global bundle\n");
			al->al_local_mrru = gl->gi_mrru;
			if (gl->gi_ssn)
				al->al_local_opts |= ALO_SSN;
			if (gl->gi_ed)
				al->al_local_opts |= ALO_ED;

			al->al_peer_opts |= ALO_MRRU;
			al->al_local_opts |= ALO_MRRU;
		} else {
			psm_log(MSG_DEBUG, ph,
	 "lcp_start: No bundle, no global bundle, set flags ssn, ed, mrru\n");

			/*
			 * Assume ML is available for incoming .. if not we
			 * may need to renegotiate LCP later
			 */
			al->al_local_opts |= ALO_MRRU | ALO_SSN | ALO_ED;
			al->al_peer_opts |= ALO_MRRU | ALO_SSN | ALO_ED;
			al->al_local_mrru = al->al_local_mru; /* Best guess */
		}
		ucfg_release();

		/* Try for a link discriminator */
		al->al_local_opts |= ALO_LD;
		al->al_local_opts |= ALO_ML_NEG;
		al->al_peer_opts |= ALO_ML_NEG;
	}		

	if (al->al_peer_auth_name) {
		free(al->al_peer_auth_name);
		al->al_peer_auth_name = NULL;
	}

	lcp->lcp_authopts = 0;
	al->al_local_chap_alg = 0;
	al->al_auth_tmout = 0;

	if ((al->al_flags & (ALF_INCOMING | ALF_AUTO)) == 
	    (ALF_INCOMING | ALF_AUTO) || !ab) {
		ucfg_lock();
		if (gl = global[DEF_BUNDLE]) {

			if (gl->gi_chap)
				lcp->lcp_authopts |= ALO_CHAP;
			if (gl->gi_pap)
				lcp->lcp_authopts |= ALO_PAP;
			al->al_local_chap_alg = gl->gi_chapalg;
			al->al_auth_tmout = gl->gi_authtmout;

			psm_log(MSG_DEBUG, ph,
				"lcp_start: Set auth for Auto incoming\n");
		} else {
			psm_log(MSG_DEBUG, ph,
			"lcp_start: No auth defined for Auto incoming\n");
			al->al_local_chap_alg = CHAP_MD5;
			al->al_auth_tmout = DEFAULT_AUTHTMOUT;
			lcp->lcp_authopts |= ALO_CHAP;
		}
		ucfg_release();
	} else {
		if (cb->bn_chap)
			lcp->lcp_authopts |= ALO_CHAP;
		if (cb->bn_pap)
			lcp->lcp_authopts |= ALO_PAP;
		al->al_local_chap_alg = cb->bn_chapalg;
		al->al_auth_tmout = cb->bn_authtmout;

		psm_log(MSG_DEBUG, ph,
			"lcp_start: Set auth for outgoing/login incoming\n");
	}

	fsm_load_counters(ph);
	PH_RESETBITS(ph->ph_cod_rej);
	PH_RESETBITS(ph->ph_rej_opts);

	psm_log(MSG_DEBUG, ph, "lcp_start: al_local_opts = 0x%x\n",
		 al->al_local_opts);
	psm_log(MSG_DEBUG, ph, "lcp_start: al_peer_opts = 0x%x\n",
		 al->al_peer_opts);

	/* Configure the link with initial values */
	cd_d2k_cfg_link(ph->ph_parent);

	if (ph->ph_parent->ah_link.al_flags & ALF_PHYS_UP) {
		psm_log(MSG_DEBUG, ph, "UP from lcp_start\n");
		fsm_state(ph, UP, 0);
		ppp_phase(ph->ph_parent, PHASE_ESTAB);
	} else {
		psm_log(MSG_ERROR, ph,
			 "lcp_start: Lower layer not available\n");
	}
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
lcp_finish(proto_hdr_t *ph)
{
	struct cfg_lcp *cp = (struct cfg_lcp *)ph->ph_cfg;

	if (ph->ph_parent->ah_type != DEF_LINK) {
		return;
	}

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_LOW, ph, "Finished\n");

/*TODO FIX THIS */

	/* BUG to fix after functional freeze .... when we uncomment this line
	 * an configure a link to be static, and direct in the Devices file ...
	 * but really attach a modem ... then we get a Memory fault ..
	 */

	if (cp->lcp_ident)
		snd_identification(ph);

	cd_d2k_adm_link(ph->ph_parent, CMD_CLOSE, 0);

	switch (ph->ph_parent->ah_link.al_type) {
	case LNK_STATIC:
		psm_log(MSG_INFO_MED, ph, "Static link has finished.\n");

		/*
		 * If we are in the CLOSED state
		 * 	then this means an
		 * 	administrative close has occured on the link ...
		 * 	and so we drop the transport 
		 *... else
		 *	we don't drop the link ... it has temporarily 
		 *	gone down
		 */
		if (ph->ph_state.ps_state == CLOSED)
			bundle_link_finished(ph->ph_parent);
		else
			ppp_phase(ph->ph_parent, PHASE_DEAD);
		break;

	default:
		/* Make sure the link is administratively closed */
		fsm_state(ph, CLOSE, 0);

		/* Indicate that we no longer need the transport */
		bundle_link_finished(ph->ph_parent);
		break;
	}

	psm_log(MSG_INFO_LOW, ph, "Finished finishing\n");
}

/*
 * Adminitrative UP when not in CLOSED or INITIAL
 * - usually call to re-netotiate configuration
 */
STATIC void
lcp_restart(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_LOW, ph, "Restart\n");

	/* Down -> Up */
	fsm_state(ph, DOWN, 0);
	psm_log(MSG_DEBUG, ph, "UP from lcp_restart\n");
	if (ph->ph_state.ps_state == STARTING ||
	    ph->ph_state.ps_state == INITIAL)
		fsm_state(ph, UP, 0);
	ppp_phase(ph->ph_parent, PHASE_ESTAB);
}

STATIC void
lcp_crossed(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_ERROR, ph, "Crossed.\n");
}

/*
 *   LCP Configuration options functions - RECEIVE side
 */
STATIC int
rcv_mru(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct co_mru_s *mru, *nmru;
	u16b_t v;
	act_link_t *al = &ph->ph_parent->ah_link;

	switch (action) {
	case CFG_CHECK:
		mru = (struct co_mru_s *)db->db_rptr;
		v = ntohs(mru->co_mru);
		psm_log(MSG_INFO_MED, ph, "Peer requests MRU %d (Ack)\n", v);

		/* Check acceptability - we always like peers mru */
		if (v < MIN_MRU)
			goto nak_mru;

		al->al_peer_mru = v;
		return CFG_ACK;

	case CFG_MKNAK:
		goto nak_mru;

	case CFG_DEFAULT:
		psm_log(MSG_INFO_MED, ph, "Setting default MRU\n");
		al->al_peer_mru = DEFAULT_MRU;
		return CFG_ACK;
	}
	ASSERT(0);

nak_mru:
	nmru = (struct co_mru_s *)ndb->db_wptr;
	psm_log(MSG_INFO_MED, ph, "Naking MRU with %d\n", DEFAULT_MRU);
	nmru->h.co_type = CO_MRU;
	nmru->h.co_len = sizeof(struct co_mru_s);
	nmru->co_mru = htons(DEFAULT_MRU);
	ndb->db_wptr += sizeof(struct co_mru_s);
	return CFG_NAK;
}

STATIC int
rcv_accm(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct co_accm_s *accm, *naccm;
	u32b_t v;
	act_link_t *al = &ph->ph_parent->ah_link;

	switch (action) {
	case CFG_CHECK:
		accm = (struct co_accm_s *)db->db_rptr;
		v = ntohl(accm->co_accm);

		if ((v & al->al_local_accm) != al->al_local_accm) {

			al->al_peer_accm = v;		
		
			psm_log(MSG_INFO_MED, ph,
				 "Peer requests ACCM 0x%8.8X (Nak with 0x%8.8X)\n",
				 v, v | al->al_local_accm);

			v |= al->al_local_accm;
			goto nak_accm;
		}
		psm_log(MSG_INFO_MED, ph,
			"Peer requests ACCM 0x%8.8X (Ack)\n", v);
		al->al_peer_accm = v;		
		return CFG_ACK;

	case CFG_MKNAK:
		psm_log(MSG_INFO_MED, ph,
			 "Naking ACCM with 0x%8.8X\n", DEFAULT_ACCM);
		v = DEFAULT_ACCM;
		goto nak_accm;

	case CFG_DEFAULT:
		psm_log(MSG_INFO_MED, ph, "Default ACCM\n");
		al->al_peer_accm = DEFAULT_ACCM;		
		return CFG_ACK;
	}
	ASSERT(0);

nak_accm:
	naccm = (struct co_accm_s *)ndb->db_wptr;
	ASSERT(naccm);

	naccm->h.co_type = CO_ACCM;
	naccm->h.co_len = sizeof(struct co_accm_s);
	naccm->co_accm = htonl(v);
	ndb->db_wptr += sizeof(struct co_accm_s);
	return CFG_NAK;

}

STATIC int
rcv_auth(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct co_auth_s *auth, *nauth;
	u16b_t proto;
	u8b_t alg;
	act_link_t *al = &ph->ph_parent->ah_link;

	switch(action) {
	case CFG_DEFAULT:
		al->al_peer_opts &= ~(ALO_PAP | ALO_CHAP);
		psm_log(MSG_INFO_MED, ph, "Default AUTH (none)\n");
		return CFG_ACK;
	case CFG_MKNAK:
		al->al_peer_opts &= ~(ALO_PAP | ALO_CHAP);

		auth = (struct co_auth_s *)db->db_rptr;
		proto = ntohs(auth->co_auth);

		if (proto == PROTO_PAP) {
			psm_log(MSG_INFO_MED, ph,
				 "Naking AUTH with PAP\n");
			goto nak_with_pap;
		} else {
			psm_log(MSG_INFO_MED, ph,
				 "Naking AUTH with CHAP MD5\n");
			goto nak_with_md5;
		}
	default:
		break;
	}

	auth = (struct co_auth_s *)db->db_rptr;
	proto = ntohs(auth->co_auth);

	switch(proto) {
	case PROTO_PAP:	/* That's fine */
		if (al->al_peer_opts & ALO_CHAP) {
			/* Already selected CHAP !! */
			psm_log(MSG_WARN, ph,
		"Peer selected both PAP and CHAP - Illegal - Discard Request\n");
			return CFG_DISCARD;
		}

		if (auth->h.co_len != 4) {
			psm_log(MSG_WARN, ph,
				"Peer's PAP option has wrong length\n");
			/* Config NAK with CHAP MD5 */
			goto nak_with_pap;
		}

		psm_log(MSG_INFO_MED, ph, "Peer requests PAP (Ack)\n");
		al->al_peer_opts |= ALO_PAP;
		break;
		
	case PROTO_CHAP: /* Check the algorithm */
		if (al->al_peer_opts & ALO_PAP) {
			/* Already selected CHAP !! */
			psm_log(MSG_WARN, ph,
		"Peer selected both PAP and CHAP - Illegal - Discard Request\n");
			return CFG_DISCARD;
		}
		
		/* Do we have an algorithm */
		
		if (auth->h.co_len != 5) {
			psm_log(MSG_WARN, ph,
				"Peer's CHAP option has wrong length\n");
			/* Config NAK with CHAP MD5 */
			goto nak_with_md5;
		}

		alg = *(unsigned char *)(auth + 1);

		switch (alg) {
		case CHAP_MD5:
			psm_log(MSG_INFO_MED, ph,
				 "Peer requests CHAP MD5 (Ack)\n");
			al->al_peer_chap_alg = alg;
			al->al_peer_opts |= ALO_CHAP;
			al->al_peer_chap_alg = alg;
			break;

		case CHAP_MS:
			psm_log(MSG_INFO_MED, ph,
				"Peer requests Microsoft CHAP  (Nak with MD5)\n");
			goto nak_with_md5;

		default:
			/* NAK with an acceptable algorithm */	
			psm_log(MSG_INFO_MED, ph,
				"Peer requests CHAP not MD5 (Nak with MD5)\n");
			goto nak_with_md5;
		}
		break;

	case PROTO_SPAP:
		psm_log(MSG_INFO_MED, ph,
		"Peer requests Shiva PAP - Not supported (Nak with MD5)\n");
		goto nak_with_md5;
	case PROTO_OSPAP:
		psm_log(MSG_INFO_MED, ph,
	"Peer requests Original Shiva PAP - Not supported (Nak with MD5)\n");
		goto nak_with_md5;
		
	default:	/* Don't know this protocol NAK*/
		psm_log(MSG_WARN, ph,
			 "Peer requests Authentication - protocol unknown 0x%x (Nak with CHAP MD5)\n", proto);
		goto nak_with_md5;
	}

	return CFG_ACK;

nak_with_md5:
	nauth = (struct co_auth_s *)ndb->db_wptr;
	nauth->h.co_type = CO_AUTH;
	nauth->h.co_len = 5;
	nauth->co_auth = htons(PROTO_CHAP);
	*(ndb->db_wptr + 4) = CHAP_MD5;
	ndb->db_wptr += 5;
	return CFG_NAK;

nak_with_pap:
	nauth = (struct co_auth_s *)ndb->db_wptr;
	nauth->h.co_type = CO_AUTH;
	nauth->h.co_len = 4;
	nauth->co_auth = htons(PROTO_PAP);
	ndb->db_wptr += 4;
	return CFG_NAK;
}

#ifdef _LQM_
STATIC int
rcv_quality(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct co_quality_s *q, *nq;
	u16b_t qual;
	act_link_t *al = &ph->ph_parent->ah_link;

	al->al_peer_opts &= ~ALO_LQM;

	switch(action) {
	case CFG_DEFAULT:
		psm_log(MSG_INFO_MED, ph, "Default QUALITY (none)\n");
		return CFG_ACK;
	case CFG_MKNAK:
		psm_log(MSG_INFO_MED, ph, "Naking QUALITY with LQR\n");
		goto nak_with_lqr;
	default:
		break;
	}

	q = (struct co_quality_s *)db->db_rptr;
	qual = ntohs(q->co_qual);

	if (qual != PROTO_LQR) {
		psm_log(MSG_INFO_MED, ph, "Peer requests LQM with unknown protocol 0x%4.4X (Nak with LQR)\n", qual);
		goto nak_with_lqr;
	} else {
		psm_log(MSG_INFO_MED, ph, "Peer resuests LQM with LQR (Ack)\n");
	}
	
	al->al_peer_qual_proto = qual;
	al->al_peer_opts |= ALO_LQM;
	return CFG_ACK;

nak_with_lqr:
	nq = (struct co_quality_s *)ndb->db_wptr;
	nq->h.co_type = CO_QUALITY;
	nq->h.co_len = 4;
	nq->co_qual = htons(PROTO_LQR);
	ndb->db_wptr += sizeof(struct co_quality_s);
	return CFG_NAK;
}
#endif

STATIC int
rcv_magic(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct co_magic_s *m;
	u32b_t magic;
	struct co_magic_s *nm;
	act_link_t *al = &ph->ph_parent->ah_link;

	al->al_peer_opts &= ~ALO_MAGIC;

	switch(action) {
	case CFG_DEFAULT:
		psm_log(MSG_INFO_MED, ph, "Default MAGIC (none)\n");
		al->al_peer_magic = 0;
		return CFG_ACK;
	case CFG_MKNAK:
		al->al_peer_magic = gen_magic((ulong_t)&m);
		psm_log(MSG_INFO_MED, ph, "Naking MAGIC with 0x%x\n",
			 al->al_peer_magic);
		goto nak_magic;
	default:
		break;
	}

	al->al_peer_opts |= ALO_MAGIC;
	
	m = (struct co_magic_s *)db->db_rptr;
	magic = ntohl(m->co_magic);

	if (magic == 0) {
		/*
		 * RFC 1661 (p47 sect6.4) - A magic number of zero is illegal
		 * and MUST be Nak'd, if it is not Rejected outright
		 */
		psm_log(MSG_WARN, ph,
			 "Peer requested invalid Magic number - Zero (Nak)\n");
		al->al_peer_magic = gen_magic((ulong_t)&m);
		goto nak_magic;
	} else if (magic == al->al_local_magic) {
		/*
		 * We are looped back - we MUST NAK with a
		 * different magic number value.
		 */
		al->al_peer_magic = gen_magic((ulong_t)&m);
		psm_log(MSG_WARN, ph,
		"Possible loopback, Magic number %8.8X (Nak with 0x%8.8X)\n",
			magic, al->al_peer_magic);
		al->al_reason |= ALR_LOOPBACK;
		goto nak_magic;
	} else {
		/*
		 * Magic nuumbers are different, ACK.
		 */
		al->al_peer_magic = magic;
		psm_log(MSG_INFO_MED, ph,
			"Peer Magic number 0x%8.8X (Ack)\n", magic);
	}
	return CFG_ACK;

nak_magic:
	nm = (struct co_magic_s *)ndb->db_wptr;
	nm->h.co_type = CO_MAGIC;
	nm->h.co_len = 6;
	nm->co_magic = htonl(al->al_peer_magic);
	ndb->db_wptr += sizeof(struct co_magic_s);
	return CFG_NAK;

}

STATIC int
rcv_pfc(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	act_link_t *al = &ph->ph_parent->ah_link;
	struct co_s *pfc;

	switch (action) {
	case CFG_CHECK:
		psm_log(MSG_INFO_MED, ph, "Peer requests PFC (Ack)\n");

		/* Okay .. just store this */
		al->al_peer_opts |= ALO_PFC;
		return CFG_ACK;
	case CFG_MKNAK:
		al->al_peer_opts &= ~ALO_PFC;
		pfc = (struct co_s *)ndb->db_wptr;
		ndb->db_wptr += sizeof(struct co_s);
		pfc->co_type = CO_PFC;
		pfc->co_len = sizeof(struct co_s);
		return CFG_NAK;
	case CFG_DEFAULT:
		al->al_peer_opts &= ~ALO_PFC;
		psm_log(MSG_INFO_MED, ph, "Default PFC\n");
		return CFG_ACK;
	}
	ASSERT(0);
}

STATIC int
rcv_acfc(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	act_link_t *al = &ph->ph_parent->ah_link;
	struct co_s *acfc;

	switch(action) {
	case CFG_CHECK:
		psm_log(MSG_INFO_MED, ph, "Peer requests ACFC (Ack)\n");

		/* Okay .. just store this */
		al->al_peer_opts |= ALO_ACFC;		
		return CFG_ACK;
	case CFG_MKNAK:
		al->al_peer_opts &= ~ALO_ACFC;
		acfc = (struct co_s *)ndb->db_wptr;
		ndb->db_wptr += sizeof(struct co_s);
		acfc->co_type = CO_ACFC;
		acfc->co_len = sizeof(struct co_s);
		return CFG_NAK;
	case CFG_DEFAULT:
		al->al_peer_opts &= ~ALO_ACFC;
		psm_log(MSG_INFO_MED, ph, "Default ACFC\n");
		return CFG_ACK;

	}
	ASSERT(0);
}

STATIC int
rcv_mrru(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	act_link_t *al = &ph->ph_parent->ah_link;
	act_hdr_t *ab = al->al_bundle;
	struct co_mrru_s *mrru;
	struct co_mrru_s *nmrru;
	u16b_t v;

	switch (action) {
	case CFG_CHECK:
		mrru = (struct co_mrru_s *)db->db_rptr;
		v = ntohs(mrru->co_mrru);

		/*
		 * Check we are configured for ML.
		 */
		if (!(al->al_local_opts & ALO_MRRU)) {
			psm_log(MSG_INFO_MED, ph,
		 "Peer requests MultiLink - Not configured - Reject\n");
			return CFG_REJ;
		}

		/*
		 * If ML options are negotiable .. believe the peer
		 * else check that the value the peer supplied
		 * is the same as that previously negotiated
		 */
		if (al->al_peer_opts & ALO_ML_NEG) {
			al->al_peer_mrru = v;
		} else if (v != ab->ah_bundle.ab_peer_mrru) {
			psm_log(MSG_INFO_MED, ph,
	 "Peer requests MRRU %d, differs from other member links (Nak)\n", v);
			v = ab->ah_bundle.ab_peer_mrru;
			goto nak;
		}

		if (v < MIN_MRRU || v > MAX_MRRU) {
			psm_log(MSG_INFO_MED, ph,
				 "Peer requests MRRU %d out of range (Nak)\n",
				 MIN_MRRU);
			v = DEFAULT_MRRU;
			goto nak;
		}
		
		al->al_peer_opts |= ALO_MRRU;
		psm_log(MSG_INFO_MED, ph, "Peer requests MRRU %d (Ack)\n", v);
		return CFG_ACK;

	case CFG_DEFAULT:
		/*
		 * Has it been rejected ? If not ... try and get 
		 * the peer to send us an MRRU
		 */
		if (al->al_peer_opts & ALO_MRRU) {
			/*
			 * Unsetting the ALO_MRRU bit means that we
			 * will not come through here a second time.
			 * i.e. they (see below) only get one chance
			 * to respond with an MRRU ...
			 */
			al->al_peer_opts &= ~ALO_MRRU;
			if (al->al_peer_opts & ALO_SSN) {
				/*
				 * Could be an RFC1717 implmentation of
				 * Multilink - where they have a default
				 * MRRU .. make them state an MRRU.
				 * (Our default is different from
				 * RFC1717's default .. so they must
				 * respond if they want multilink 
				 * configured).
				 */
				v = DEFAULT_MRRU;
				psm_log(MSG_INFO_LOW, ph,
	"Peer requested SSN but not MRRU, could be RFC1717 .. Nak MRRU\n");
				goto nak;
			}
			if ((al->al_peer_opts & ALO_MRRU) &&
			    (al->al_local_opts & ALO_MRRU)) {
				/*
				 * Could be an RF1717 implmentation of
				 * Multilink, they are using a default
				 * MRRU. Get an explicit value.
				 */
				v = DEFAULT_MRRU;
				psm_log(MSG_INFO_LOW, ph,
				"Peer didn't request MRRU, nak MRRU\n");
				goto nak;
			}
		}
		al->al_peer_mrru = DEFAULT_MRRU;
		psm_log(MSG_INFO_MED, ph, "Default MRRU (none)\n");
		return CFG_ACK;

	case CFG_MKNAK:
		goto nak;
	}
	ASSERT(0);
 nak:
	nmrru = (struct co_mrru_s *)ndb->db_wptr;
	nmrru->h.co_type = CO_MRRU;
	nmrru->h.co_len = sizeof(struct co_mrru_s);
	nmrru->co_mrru = htons(v);
	ndb->db_wptr += sizeof(struct co_mrru_s);
	return CFG_NAK;
}

STATIC int
rcv_ssn(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	act_link_t *al = &ph->ph_parent->ah_link;
	act_hdr_t *ab = al->al_bundle;
	struct co_s *ssn = (struct co_s *)db->db_wptr;

	switch (action) {
	case CFG_CHECK:
		if (al->al_peer_opts & ALO_ML_NEG) {
			/*
			 * ML Options are negotiable ... 
			 * believe what we are told
			 */
			al->al_peer_opts |= ALO_SSN;
		} else {
			/*
			 * Check that we negotiated this previously
			 */
			if (!(ab->ah_bundle.ab_peer_ssn)) {
				psm_log(MSG_INFO_MED, ph,
		 "Peer requests SSN, differs form other member links (Rej)\n");
				return CFG_REJ;
			}	
			al->al_peer_opts |= ALO_SSN;
		}
		psm_log(MSG_INFO_MED, ph, "Peer requests SSN (Ack)\n");
		return CFG_ACK;

	case CFG_MKNAK:
		al->al_peer_opts &= ~ALO_SSN;
		ssn = (struct co_s *)ndb->db_wptr;
		ndb->db_wptr += sizeof(struct co_s);
		ssn->co_type = CO_SSN;
		ssn->co_len = sizeof(struct co_s);
		return CFG_NAK;

	case CFG_DEFAULT:
		if (al->al_peer_opts & ALO_ML_NEG) {
			al->al_peer_opts &= ~ALO_SSN;
			psm_log(MSG_INFO_MED, ph, "Default SSN (none)\n");
		} else {
			if (ab->ah_bundle.ab_peer_ssn) {
				psm_log(MSG_INFO_MED, ph,
	 "Peer didn't request SSN, differs form other member links (Nak)\n");
				ssn->co_type = CO_SSN;
				ssn->co_len = sizeof(struct co_s);
				db->db_wptr += sizeof(struct co_s);
				return CFG_NAK;
			}	
			al->al_peer_opts &= ~ALO_SSN;
		}
		return CFG_ACK;
	}
}

STATIC int
rcv_ed(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct co_ed_s *ed;
	act_link_t *al = &ph->ph_parent->ah_link;
	int len;

	switch (action) {
	case CFG_CHECK:
		ed = (struct co_ed_s *)db->db_rptr;
		al->al_peer_ed_class = ed->co_class;
		len = ed->h.co_len - sizeof(struct co_ed_s);

		switch (al->al_peer_ed_class) {
		case ED_NULL:
			if (len != 0) {
				psm_log(MSG_WARN, ph,
			"Class 0 Endpoint Discriminator too long (%d)\n",
					len);
				return CFG_REJ;			
			}
			break;
		case ED_LOCAL:
			if (len > 20) {
				psm_log(MSG_ERROR, ph,
			 "Class 1 Endpoint Discriminator too long (%d)\n",
					len);
				return CFG_REJ;
			}
			break;
		case ED_IP:
			if (len != 4) {
				psm_log(MSG_ERROR, ph,
			 "Class 2 Endpoint Discriminator too long (%d)\n",
					len);
				return CFG_REJ;
			}
			break;
		case ED_MAC:
			if (len != 6) {
				psm_log(MSG_ERROR, ph,
			 "Class 3 Endpoint Discriminator too long (%d)\n",
					len);
				return CFG_REJ;
			}
			break;
		case ED_MAGIC:
			if (len > 20) {
				psm_log(MSG_ERROR, ph,
			 "Class 4 Endpoint Discriminator too long (%d)\n",
					len);
				return CFG_REJ;
			}
			break;
		case ED_PSTN:
			if (len > 15) {
				psm_log(MSG_ERROR, ph,
			 "Class 5 Endpoint Discriminator too long (%d)\n",
					len);
				return CFG_REJ;
			}
			break;
		default:
			/* Unexpected ED Class */
			psm_log(MSG_WARN, ph,
			 "Endpoint Discriminator - Bad class (%d)- Reject\n",
				al->al_peer_ed_class);
			return CFG_REJ;			
		}

		al->al_peer_opts |= ALO_ED;
		al->al_peer_ed_len = len;

		psm_log(MSG_INFO_MED, ph,	
	 "Peer requests Endpoint Discriminator (Class %d, Len %d) (Ack)\n",
			 al->al_peer_ed_class,
			 al->al_peer_ed_len);
	
		if (len) {
			memcpy(&al->al_peer_ed, &(ed->co_class) + 1, len);
			psm_log(MSG_INFO_MED, ph, "Discriminator is ...\n");
			psm_loghex(MSG_INFO_MED, ph,
				   (char *)&al->al_peer_ed, len);
		}
		return CFG_ACK;

	case CFG_DEFAULT:
		al->al_peer_opts &= ~ALO_ED;
		psm_log(MSG_INFO_MED, ph, "Default ED (none)\n");
		return CFG_ACK;

	case CFG_MKNAK:
		psm_log(MSG_INFO_MED, ph, "ED Wrong length - Reject\n");
		return CFG_REJ;
	}
	ASSERT(0);
}

STATIC int
rcv_ld(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct co_ld_s *ld;
	act_link_t *al = &ph->ph_parent->ah_link;

	switch (action) {
	case CFG_CHECK:
		ld = (struct co_ld_s *)db->db_rptr;

		al->al_peer_opts |= ALO_LD;
		al->al_peer_ld = ntohs(ld->co_ld);

		/*
		 * The LD is checked for uniqueness when
		 * the link is added to a bundle
		 */
		psm_log(MSG_INFO_MED, ph,	
			"Peer requests Link Discriminator (%d) (Ack)\n",
			al->al_peer_ld);
		return CFG_ACK;

	case CFG_DEFAULT:
		al->al_peer_opts &= ~ALO_LD;
		psm_log(MSG_INFO_MED, ph, "Default LD (none)\n");
		return CFG_ACK;

	case CFG_MKNAK:
		al->al_peer_opts &= ~ALO_LD;
		psm_log(MSG_INFO_MED, ph, "LD Wrong length - Discard\n");
		return CFG_DISCARD;
	}
}

/*
 * Send side configuration option processing routines
 * If co is not NULL, then we are creating a config request
 *in response to a config NAK pr REJ.
 */
STATIC int
snd_mru(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_mru_s *mru = (struct co_mru_s *)db->db_wptr;
	act_link_t *al = &ph->ph_parent->ah_link;
	u16b_t v;

	mru->h.co_type = CO_MRU;
	mru->h.co_len = 4;

	switch(state) {
	case CFG_REJ:
		/* Pressume DEFAULT_MRU */
		al->al_local_mru = DEFAULT_MRU;
		psm_log(MSG_INFO_MED, ph,
			 "MRU was rejected, use the default (%d)\n",
			 DEFAULT_MRU);
		return;

	case CFG_NAK:
		v = ntohs(((struct co_mru_s *)co)->co_mru);

		psm_log(MSG_INFO_MED, ph, "MRU (%d) was Nak'ed with %d\n",
			 al->al_local_mru, v);

		if (v < MIN_MRU) {
			psm_log(MSG_INFO_MED, ph,
				 "Nak'ed value too small, using %d\n",
				 MIN_MRU);
			v = MIN_MRU;
		}
		al->al_local_mru = v;
		break;

	case CFG_ACK:
		psm_log(MSG_INFO_MED, ph, "Request MRU %d\n",
			 al->al_local_mru);
		break;
	}

	/* We don't send default vaulues for options */
	if (al->al_local_mru == DEFAULT_MRU) {
		psm_log(MSG_INFO_MED, ph,
			 "%d is the default MRU, don't request it\n",
			 al->al_local_mru);
		return;
	}

	mru->co_mru = htons(al->al_local_mru);
	db->db_wptr += sizeof(struct co_mru_s);
}

STATIC int
snd_accm(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_accm_s *accm = (struct co_accm_s *)db->db_wptr;
	u32b_t v;
	act_link_t *al = &ph->ph_parent->ah_link;

	accm->h.co_type = CO_ACCM;
	accm->h.co_len = 6;

	switch(state) {
	case CFG_REJ:
		/* Assume default */
		al->al_local_accm = DEFAULT_ACCM;
		psm_log(MSG_INFO_MED, ph,
			 "ACCM was rejected, assume the default 0x%8.8X\n",
			 DEFAULT_ACCM);
		return;

	case CFG_NAK:
		/* Retry */
		v = ntohl(((struct co_accm_s *)co)->co_accm);
		psm_log(MSG_INFO_MED, ph, 
			 "ACCM was Nak'ed with, 0x%8.8X, request 0x%8.8X\n",
			 v, v | al->al_local_accm);
		al->al_local_accm |= v;
		break;

	case CFG_ACK:
		/* Send configured value */
		psm_log(MSG_INFO_MED, ph, "Request ACCM 0x%8.8X\n", 
			 al->al_local_accm);
		break;
	}

	/* Don't send the default value */
	if (al->al_local_accm == DEFAULT_ACCM) {
		psm_log(MSG_INFO_MED, ph,
			 "0x%8.8X is the default ACCM, don't request it\n",
			 al->al_local_accm);
		return;
	}

	accm->co_accm = htonl(al->al_local_accm);
	db->db_wptr += sizeof(struct co_accm_s);
}

/*
 * Use PAP or CHAP if configured
 */
STATIC int
snd_auth(proto_hdr_t *ph, int state,  struct co_s *co, db_t *db)
{
	struct co_auth_s *auth = (struct co_auth_s *)db->db_wptr;
	struct co_auth_s *co_auth = (struct co_auth_s *)co;
	act_link_t *al = &ph->ph_parent->ah_link;
	struct cfg_lcp *cp = (struct cfg_lcp *)ph->ph_cfg;
	struct lcp_s *lcp = (struct lcp_s *)ph->ph_priv;

	auth->h.co_type = CO_AUTH;

	switch(state) {
	case CFG_REJ:
		/*
		 * This indicates that the peer doesn't implement
		 * authentication
		 */
		al->al_local_opts &= ~(ALO_CHAP | ALO_PAP);
		psm_log(MSG_INFO_MED, ph, "Peer Rejected AUTH\n");
		break;

	case CFG_NAK:
		/*
		 * If we previously asked for CHAP .. and we have PAP
		 * available .. retry with PAP ..
		 * If we previously asked for PAP ... then we are stuck
		 */
		if (al->al_local_opts & ALO_CHAP) {
			psm_log(MSG_INFO_MED, ph,
				 "Peer Nak'ed request for CHAP\n");
			if (lcp->lcp_authopts & ALO_PAP)
				goto pap;
			else {
				al->al_local_opts &= ~(ALO_CHAP | ALO_PAP);
				break;
			}
		} else if (al->al_local_opts & ALO_PAP) {
			psm_log(MSG_INFO_MED, ph,
				"Peer Nak'ed request for PAP (with 0x%4.4X)\n",
				ntohs(co_auth->co_auth));
			al->al_local_opts &= ~(ALO_CHAP | ALO_PAP);
			break;
		}
		break;

	case CFG_ACK:
		if (lcp->lcp_authopts & ALO_CHAP) {
			al->al_local_opts |= ALO_CHAP;
			al->al_local_opts &= ~ALO_PAP;
			auth->h.co_len = 5;
			auth->co_auth = htons(PROTO_CHAP);
			db->db_wptr += sizeof(struct co_auth_s);
			*db->db_wptr++ = al->al_local_chap_alg;
			psm_log(MSG_INFO_MED, ph, "Request CHAP MD5\n");
		} else if (lcp->lcp_authopts & ALO_PAP) {
		  pap:
			al->al_local_opts |= ALO_PAP;
			al->al_local_opts &= ~ALO_CHAP;
			auth->h.co_len = 4;
			auth->co_auth = htons(PROTO_PAP);
			db->db_wptr += sizeof(struct co_auth_s);
			psm_log(MSG_INFO_MED, ph, "Request PAP\n");
		} else {
			al->al_local_opts &= ~(ALO_CHAP | ALO_PAP);
			return;
		}
	}
}

#ifdef _LQM_
STATIC int
snd_quality(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_quality_s *q = (struct co_quality_s *)db->db_wptr;
	struct cfg_lcp *cp = (struct cfg_lcp *)ph->ph_cfg;
	act_link_t *al = &ph->ph_parent->ah_link;

	q->h.co_type = CO_QUALITY;
	q->h.co_len = 4;

	switch(state) {
	case CFG_REJ:
		al->al_local_opts &= ~ALO_LQM;
		/* Peer doesn't do LQM */
		psm_log(MSG_INFO_MED, ph,
			 "Peer Rejected request for LQM with LQR\n");
		break;
	case CFG_NAK:
		al->al_local_opts &= ~ALO_LQM;
		psm_log(MSG_INFO_MED, ph, "Peer Nak'ed request for LQM with LQR\n");
		break;
	case CFG_ACK:
		if (cp->lcp_lqm) {
			psm_log(MSG_INFO_MED, ph, "Request LQM with LQR\n");
			q->co_qual = htons(PROTO_LQR);
			db->db_wptr += sizeof(struct co_quality_s);
			al->al_local_opts |= ALO_LQM;
		}
		break;
	}
}
#endif

STATIC int
snd_magic(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_magic_s *m = (struct co_magic_s *)db->db_wptr;
	u32b_t magic;
	act_link_t *al = &ph->ph_parent->ah_link;

	m->h.co_type = CO_MAGIC;
	m->h.co_len = 6;

	switch(state) {
	case CFG_REJ:
		/* Peer doesn't do magic number negotiation */
		psm_log(MSG_INFO_MED, ph,
			 "Peer Rejected Magic number negotiation\n");
		al->al_local_opts &= ~ALO_MAGIC;
		al->al_local_magic = 0;
		break;

	case CFG_NAK:
		magic = ntohl(((struct co_magic_s *)co)->co_magic);
		if (magic == al->al_local_magic) {
			/* Possible loopback */
			al->al_reason |= ALR_LOOPBACK;
			psm_log(MSG_INFO_MED, ph,
	 "Possible loopback connection. Peer Nak'ed our Magic number\n");
		}
		al->al_local_magic = gen_magic(al->al_local_magic);
		/*FALLTHRU*/

	case CFG_ACK:
		if (al->al_local_opts & ALO_MAGIC) {
			psm_log(MSG_INFO_MED, ph,
				 "Request Magic number 0x%8.8X\n",
				 al->al_local_magic);
			m->co_magic = htonl(al->al_local_magic);
			db->db_wptr += sizeof(struct co_magic_s);
		} else
			al->al_local_magic = 0;
	}
}

STATIC int
snd_pfc(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_s *pfc = (struct co_s *)db->db_wptr;
	act_link_t *al = &ph->ph_parent->ah_link;

	switch(state) {
	case CFG_REJ:
		al->al_local_opts &= ~ALO_PFC;
		psm_log(MSG_INFO_MED, ph, "Peer rejected PFC\n");
		break;
	case CFG_NAK:	/* Can't get this */
		psm_log(MSG_WARN, ph,
			 "Protocol Violation ! Received NAK'ed PFC\n");
		break;
	case CFG_ACK:
		if (al->al_local_opts & ALO_PFC) {
			psm_log(MSG_INFO_MED, ph, "Request PFC\n");
			pfc->co_type = CO_PFC;
			pfc->co_len = 2;
			db->db_wptr += sizeof(struct co_s);
		}
		break;
	}
}

STATIC int
snd_acfc(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_s *acfc = (struct co_s *)db->db_wptr;
	act_link_t *al = &ph->ph_parent->ah_link;

	switch(state) {
	case CFG_REJ:
		al->al_local_opts &= ~ALO_ACFC;
		psm_log(MSG_INFO_MED, ph, "Peer rejected ACFC\n");
		break;
	case CFG_NAK:
		psm_log(MSG_WARN, ph,
			 "Protocol Violation ! Received NAK'ed ACFC\n");
		break;
	case CFG_ACK:
		if (al->al_local_opts & ALO_ACFC) {
			psm_log(MSG_INFO_MED, ph, "Request ACFC\n");
			acfc->co_type = CO_ACFC;
			acfc->co_len = 2;
			db->db_wptr += sizeof(struct co_s);
		}
		break;
	}
}

STATIC int
snd_mrru(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_mrru_s *mrru = (struct co_mrru_s *)db->db_wptr;
	act_link_t *al = &ph->ph_parent->ah_link;
	u16b_t v;
	act_hdr_t *ah = ph->ph_parent;

	mrru->h.co_type = CO_MRRU;
	mrru->h.co_len = 4;

	switch(state) {
	case CFG_REJ:
		psm_log(MSG_INFO_MED, ph,
			 "Peer rejected MRRU  - MultiLink Disabled\n");
		/* Indicate no ML  in the link & bundle */
		al->al_local_opts &= ~ALO_MRRU;
		return;

	case CFG_NAK:
		v = ntohs(((struct co_mrru_s *)co)->co_mrru);

		if (al->al_local_opts & ALO_ML_NEG) {
			/*
			 * Multilink is negotiable .. request multilink
			 */
			if (v < MIN_MRRU || v > MAX_MRRU)
				v = DEFAULT_MRRU;

			psm_log(MSG_INFO_MED, ph,
			       "MRRU was Nak'ed (%d), try %d\n",
			       ntohs(((struct co_mrru_s *)co)->co_mrru), v);

			/* Check the value provided is acceptable */
			al->al_local_mrru = v;
		} else {
			/*
			 * Multilink is not negotiable, use configured
			 * values .. and only allow if configured.
			 */
			if (al->al_bundle && !(al->al_local_opts & ALO_MRRU)) {
				psm_log(MSG_INFO_MED, ph,
				"MRRU was Nak'ed. Not configured for Multilink - ignore\n");
				return;
			}

			/* Don't negotiate .. we already failed .. */
			v = al->al_local_mrru;
			psm_log(MSG_INFO_MED, ph,
				"MRRU was Nak'ed - dont reneg use %d\n", v);
		}
		break;
	case CFG_ACK:
		if (al->al_bundle && !(al->al_local_opts & ALO_MRRU)) {
			psm_log(MSG_INFO_MED, ph,
				 "SKIP MRRU - Not configured for MP\n");
			return;
		}
		v = al->al_local_mrru;
		psm_log(MSG_INFO_MED, ph, "Request MRRU %d\n", v);
		break;
	}
	mrru->co_mrru = htons(v);
	db->db_wptr += sizeof(struct co_mrru_s);
}

STATIC int
snd_ssn(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_s *ssn = (struct co_s *)db->db_wptr;
	act_hdr_t *ah = ph->ph_parent;
	act_link_t *al = &ph->ph_parent->ah_link;
	act_hdr_t *ab = al->al_bundle;

	switch(state) {
	case CFG_REJ:
		al->al_local_opts &= ~ALO_SSN;
		psm_log(MSG_INFO_MED, ph, "Peer rejected SSN\n");
		break;
	case CFG_NAK:	/* Can't get this */
		psm_log(MSG_WARN, ph,
			"Protocol Violation ! Received NAK'ed SSN\n");
		ah->ah_link.al_local_opts &= ~ALO_SSN;
		break;
	case CFG_ACK:
		if ((al->al_local_opts & ALO_SSN) &&
		    (al->al_local_opts & ALO_MRRU)) {
			psm_log(MSG_INFO_MED, ph, "Request SSN\n");
			ssn->co_type = CO_SSN;
			ssn->co_len = 2;
			db->db_wptr += sizeof(struct co_s);

			ah->ah_link.al_local_opts |= ALO_SSN;
		}
		break;
	}
}


STATIC int
snd_ed(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_ed_s *ed = (struct co_ed_s *)db->db_wptr;
	act_hdr_t *ah = ph->ph_parent;
	act_link_t *al = &ph->ph_parent->ah_link;
	act_hdr_t *ab = al->al_bundle;

	switch(state) {
	case CFG_REJ:
		ah->ah_link.al_local_opts &= ~ALO_ED;
		psm_log(MSG_INFO_MED, ph,
			 "Peer rejected Endpoint Discriminator\n");
		break;
	case CFG_NAK:	/* Can't get this */
		psm_log(MSG_WARN, ph,
	 "Protocol Violation ! Received NAK'ed Endpoint Discriminator\n");
		ah->ah_link.al_local_opts &= ~ALO_ED;
		break;
	case CFG_ACK:
		if ((al->al_local_opts & ALO_ED) &&
		    (al->al_local_opts & ALO_MRRU)) {
			psm_log(MSG_INFO_MED, ph,
				 "Request Endpoint Discriminator\n");
			ed->h.co_type = CO_ED;
			ed->h.co_len = 3 + lcp_ed_len;
			ed->co_class = lcp_ed_class;

			memcpy((caddr_t)(ed + 1), lcp_ed, lcp_ed_len);

			db->db_wptr += sizeof(struct co_ed_s) + lcp_ed_len;
		}
	}
}

STATIC int
snd_ld(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_ld_s *ld = (struct co_ld_s *)db->db_wptr;
	act_hdr_t *ah = ph->ph_parent;
	act_link_t *al = &ph->ph_parent->ah_link;
	act_hdr_t *ab = al->al_bundle;

	switch(state) {
	case CFG_REJ:
		ah->ah_link.al_local_opts &= ~ALO_LD;
		psm_log(MSG_INFO_MED, ph,
			 "Peer rejected Link Discriminator\n");
		break;
	case CFG_NAK:	/* Can't get this */
		psm_log(MSG_WARN, ph,
			"Received NAK'ed Link Discriminator\n");
		ah->ah_link.al_local_opts &= ~ALO_LD;
		break;
	case CFG_ACK:
		if (al->al_local_opts & ALO_LD) {
			psm_log(MSG_INFO_MED, ph,
				 "Request Link Discriminator %d\n",
				al->al_local_ld);
			ld->h.co_type = CO_LD;
			ld->h.co_len = sizeof(struct co_ld_s);
			ld->co_ld = htons(al->al_local_ld);
			db->db_wptr += sizeof(struct co_ld_s);
		}
	}
}

STATIC int
snd_identification(proto_hdr_t *ph)
{
	struct lcp_ident_s *id;
	db_t *db;
	int len;

	ASSERT(ph->ph_parent);
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	ASSERT(ph->ph_parent->ah_type == DEF_LINK);

	if (PH_TSTBIT(IDENT, ph->ph_cod_rej)) {
		/* Code rejected */
		psm_log(MSG_DEBUG, ph,
			 "LCP Identification not sent - was code rejected\n");
		return;
	}

	db = db_alloc(PSM_MTU(ph->ph_parent));
	if (!db) {
		psm_log(MSG_WARN, ph,
			 "Out of memory - Cannot send Identification code\n");
		return;
	}

	psm_log(MSG_INFO_MED, ph, "Sending LCP Identification\n");
	
	psm_add_pf(ph, db);

	id = (struct lcp_ident_s *)db->db_wptr;
	id->id_hdr.lcp_code = IDENT;
	id->id_hdr.lcp_id = ++ph->ph_state.ps_lastid;
	id->id_magic = htonl(ph->ph_parent->ah_link.al_local_magic);
	db->db_wptr += sizeof(struct lcp_ident_s);

	len = strlen(PPP_VERSION_STRING);

	memcpy((caddr_t)(id + 1), PPP_VERSION_STRING, len);

	db->db_wptr += len;

	id->id_hdr.lcp_len = htons(sizeof(struct lcp_ident_s) + len);

	lcp_snd(ph, db);
}


/*
 * LCP Extended code handlers
 */
STATIC int
rcv_prt_rej(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct lcp_prtrej_s *pr = (struct lcp_prtrej_s *)db->db_rptr;
	ushort_t proto;
	act_hdr_t *ah;
	proto_hdr_t *op;	/* Offending protocol */

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	proto = ntohs(pr->lcp_proto);

	psm_log(MSG_INFO_MED, ph,
		 "Received Protocol Reject (0x%4.4x)\n", proto);

	if (ntohs(pr->lcp_hdr.lcp_len) < sizeof(struct lcp_prtrej_s)) {
		psm_log(MSG_WARN, ph,
			"Protocol Reject - Frame to short - dropped\n");
		return CFG_DISCARD;
	}

	/* Stop sending offending packet type */	

	ah = ph->ph_parent;

	switch (ah->ah_type) {
	case DEF_BUNDLE:
		psm_log(MSG_DEBUG, ph, "rcv_prt_rej: Reject on BUNDLE\n");
		break;
	case DEF_LINK:
		psm_log(MSG_DEBUG, ph, "rcv_prt_rej: Reject on LINK\n");
		break;
	default:
		ppplog(MSG_DEBUG, 0, "abort 6\n");
		abort();
	}

	/* Find the offending protocol */

	op = act_findproto(ah, proto, 1);

	if (op) {
		psm_log(MSG_INFO_MED, ph,
			"Rejected protocol is %s (0x%4.4x)\n",
			op->ph_psmtab->pt_desc, proto);

		fsm_state(op, CLOSE, NULL);
		fsm_reason(op, PR_REJ);
		if (op->ph_state.ps_state != INITIAL)
			fsm_state(op, DOWN, NULL);
/*TODO*/
/* Is rejection of this protocol acceptable ??? */
/* Do we need a function to call in the proto table ... with ... */

	} else
		psm_log(MSG_WARN, ph,
			"Rejected protocol is unknown (0x%4.4x)\n", proto);
	return RXJ_P;
}

STATIC int
rcv_eco_req(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct lcp_echo_s *lcp = (struct lcp_echo_s *)db->db_rptr;
	act_hdr_t *ah = ph->ph_parent;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_MED, ph, "Received Echo Request\n");

	if (ntohs(lcp->lcp_hdr.lcp_len) < sizeof(struct lcp_echo_s)) {
		psm_log(MSG_WARN, ph,
			"Echo Request - Frame too short - dropped\n");
		return CFG_DISCARD;
	}

	if (ah->ah_type == DEF_LINK) {
		if (ntohl(lcp->lcp_magic) != ah->ah_link.al_peer_magic) {
			psm_log(MSG_WARN, ph,
			"Echo Request - Bad Magic 0x%x (exp 0x%x) - dropped\n",
				ntohl(lcp->lcp_magic),
				ah->ah_link.al_peer_magic);
			return CFG_DISCARD;
		}

		if (ph->ph_state.ps_state != OPENED) {
			psm_log(MSG_WARN, ph, "Echo request - LCP not opened\n");
			return CFG_DISCARD;
		}
	} else {
		if (ntohl(lcp->lcp_magic) != 0) {
			psm_log(MSG_WARN, ph,
			"Echo Request - Bad Magic 0x%x (exp 0) - dropped\n",
				ntohl(lcp->lcp_magic));
			return CFG_DISCARD;
		}
	}
		
	*ndb = db_dup(db);
	return RXR;
}

STATIC int
rcv_eco_rpl(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct lcp_echo_s *echo = (struct lcp_echo_s *)db->db_rptr;
	struct lcp_s *lcp = (struct lcp_s *)ph->ph_priv;
	act_hdr_t *ah = ph->ph_parent;
	proto_state_t *ps = &ph->ph_state;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_MED, ph, "Received Echo Reply\n");

	if (ph->ph_state.ps_state != OPENED) {
		psm_log(MSG_WARN, ph, "Echo Reply - LCP not opened\n");
		return CFG_DISCARD;
	}

	if (ah->ah_type != DEF_LINK) {
		psm_log(MSG_WARN, ph,
			"Echo Reply on Bundle  - Not supported\n");
		return CFG_DISCARD;
	}

	/* Check the magic number is okay */

	if (ntohl(echo->lcp_magic) != ah->ah_link.al_peer_magic) {
		psm_log(MSG_WARN, ph,
			"Echo Reply - Bad Magic 0x%x (exp 0x%x) - dropped\n",
			ntohl(echo->lcp_magic),
				ah->ah_link.al_peer_magic);
		return CFG_DISCARD;
	}

	/* Log this reply */
	ECHO_SET(lcp, ps->ps_lastid);
	lcp->lcp_echo_zeros--;	
	lcp->lcp_flags |= LF_ECHO_OK;

	/* Don't let the fsm have this */
	return CFG_DISCARD_OK;
}

STATIC int
rcv_dsc_req(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_INFO_MED, ph, "Received Discard Request\n");
	return CFG_DISCARD_OK;
}

STATIC int
rcv_ident(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	struct lcp_ident_s *id;
	int len;

	id = (struct lcp_ident_s *)db->db_rptr;
	len = htons(id->id_hdr.lcp_len) - sizeof(struct lcp_ident_s);

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_INFO_LOW, ph, "Received Identification\n");
	psm_logstr(MSG_INFO_LOW, ph,
		   (char *)db->db_rptr + sizeof(struct lcp_ident_s) , len);

	return CFG_DISCARD_OK;
}

STATIC int
rcv_time(proto_hdr_t *ph, db_t *db, db_t **ndb)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_INFO_MED, ph, "Received Time Remaining\n");
	return CFG_DISCARD_OK;
}

STATIC int
lcp_status(proto_hdr_t *ph, struct lcp_status_s *st)
{
	st->st_fsm = ph->ph_state;
	memcpy(&st->st_hdr, ph->ph_parent, sizeof(act_hdr_t));
}

/*
 * If more LCP option numbers are added to lcp_opt_tab[] then
 * update this value
 */
#define NUM_LCP_OP 23
/*
 * LCP Configuration options table
 */
STATIC struct psm_opt_tab_s lcp_opt_tab[] = {

	/* Type 0 */
	OP_NOSUPP("Multilink Plus"),

	/* Type 1 */
	rcv_mru, snd_mru, OP_LEN_EXACT | OP_NO_REJ, 4,
	"Maximum-Receive-Unit",

	/* Type 2 */
	rcv_accm, snd_accm, OP_LEN_EXACT | OP_NO_REJ, 6,
	"Async-Control-Character-Map",
	
	/* Type 3 */
	rcv_auth, snd_auth, OP_LEN_MIN | OP_NO_REJ, 4,	
	"Authentication-Protocol",

	/* Type 4 */
#ifdef _LQM_
	rcv_quality, 
	snd_quality,
	OP_LEN_MIN | OP_NO_REJ, 4,
	"Quality-Protocol",
#else
	OP_NOSUPP("Quality Protocol"),
#endif
	/* Type 5 */
	rcv_magic, snd_magic, OP_LEN_EXACT | OP_NO_REJ | OP_REJISACK, 6,
	"Magic-Number",

	/* Type 6 */
	OP_NOSUPP("Reserved"),

	/* Type 7 */
	rcv_pfc, snd_pfc, OP_LEN_EXACT | OP_NO_NAK, 2,
	"Protocol-Field-Compression",

	/* Type 8 */
	rcv_acfc, snd_acfc, OP_LEN_EXACT | OP_NO_NAK, 2,
	"Address-and-Control-Field-Compression",

	/* Type 9 */
	OP_NOSUPP("Reserved"),

	/* Type 10 */
	NULL, NULL, 0, 0,
	"Self-Describing-Padding",

	/* Type 11 */
	OP_NOSUPP("Numbered Mode"),

	/* Type 12 */
	OP_NOSUPP("Reserved"),
	
	/* Type 13 */
	NULL, NULL, 0, 0,
	"Callback",

	/* Type 14 */
	OP_NOSUPP("Not Supported"),

	/* Type 15 */
	OP_NOSUPP("Not Supported"),

	/* Type 16 */
	OP_NOSUPP("Not Supported"),

	/* Type 17 */
	rcv_mrru, snd_mrru, OP_LEN_EXACT | OP_NO_REJ, 4,
	"Max Receive Reconstructed Unit",

	/* Type 18 */
	rcv_ssn, snd_ssn, OP_LEN_EXACT | OP_NO_REJ, 2,
	"Short Sequence Number",

	/* Type 19 */
	rcv_ed, snd_ed, OP_LEN_MIN | OP_NO_REJ | OP_NO_NAK, 3,
	"Endpoint Discriminator",

	/* Type 20 */
	OP_NOSUPP("Not Supported"),

	/* Type 21 */
	OP_NOSUPP("Not Supported"),

	/* Type 22 */
	OP_NOSUPP("Not Supported"),

	/* Type 23 */
	rcv_ld, snd_ld, OP_LEN_EXACT | OP_NO_REJ, 4,
	"Link Discriminator",

};

STATIC struct psm_code_tab_s lcp_codes[] = {

	8,
	PC_ACCEPT | PC_NOREJ,
	rcv_prt_rej,
	"Protocol-Reject",

	9,
	PC_ACCEPT | PC_NOREJ,
	rcv_eco_req,
	"Echo-Request",

	10,
	PC_ACCEPT | PC_NOREJ | PC_CHKID,
	rcv_eco_rpl,
	"Echo-Reply",

	11,
	PC_ACCEPT | PC_NOREJ,
	rcv_dsc_req,
	"Discard-Request",

	12,
	0,
	rcv_ident,
	"Identification",

	13,
	0,
	rcv_time,
	"Time-Remaining",

	0xffff,	0, NULL, NULL, 
};

STATIC ushort_t lcp_protolist[] = {
	PROTO_ML,
	0,
};

/*
 * Table of PSM entry points
 */
struct psm_tab_s psm_entry = {
	PSM_API_VERSION,
	"LCP",
	PROTO_LCP,
	PT_LCP | PT_FSM | PT_LINK | PT_BUNDLE,	/* Flags */
	PT_PRI_HI,

	lcp_protolist,

	lcp_load,
	lcp_unload,
	lcp_alloc,
	lcp_free,
	lcp_init,

	lcp_rcv,
	lcp_snd,
	NULL,	/* log */
	lcp_status,

	/* FSM specific fields .. */

	lcp_up,
	lcp_down,
	lcp_start,
	lcp_finish,
	lcp_restart,
	lcp_crossed,

	lcp_opt_tab,
	NUM_LCP_OP,

	lcp_codes,
};


