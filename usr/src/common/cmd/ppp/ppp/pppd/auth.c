#ident	"@(#)auth.c	1.6"

#include <stdio.h>
#include <assert.h>
#include <dial.h>
#include <errno.h>
#include <sys/conf.h>
#include <sys/byteorder.h>
#include <string.h>
#include <stdlib.h>

#include "ppp_cfg.h"
#include "ppp_type.h"
#include "fsm.h"
#include "psm.h"
#include "auth.h"
#include "ppp_proto.h"

void auth_open(act_hdr_t *ah);

/*
 * auth_init
 *
 * This function is called when we enter the Authentication Phase.
 * It initiate the negotiated authentication protocol negotiations.
 */
void
auth_init(act_hdr_t *ah)
{
	struct proto_hdr_s *ph;
	struct cfg_link *cl = (struct cfg_link *)ah->ah_cfg;

	ASSERT(ah->ah_type == DEF_LINK);
	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));

	ppplog(MSG_INFO_LOW, ah,
	       "Authentication Initialised (timeout %d seconds)\n",
	       ah->ah_link.al_auth_tmout);

	if (ah->ah_link.al_peer_auth_name) {
		free(ah->ah_link.al_peer_auth_name);
		ah->ah_link.al_peer_auth_name = NULL;
	}

	ah->ah_link.al_flags &= ~(ALF_LOCAL_AUTH | ALF_PEER_AUTH);

	ph = ah->ah_protos;

	while (ph) {
		if (ph->ph_psmtab->pt_flags & PT_AUTH)
			/* Call the start routine */
			(*ph->ph_psmtab->pt_start)(ph);
		ph = ph->ph_next;
	}

	auth_open(ah);
}

/*
 * auth_open
 *
 * Check if authentication has completed
 */
void
auth_open(act_hdr_t *ah)
{
	struct proto_hdr_s *ph;

	ASSERT(ah->ah_type == DEF_LINK);
	ASSERT(MUTEX_LOCKED(&ah->ah_mutex));

	ppplog(MSG_DEBUG, ah, "auth_open: local_opts %x, peer_opts %x\n",
	       ah->ah_link.al_local_opts, ah->ah_link.al_peer_opts);

	if (ah->ah_link.al_local_opts & (ALO_PAP | ALO_CHAP))
		if (!(ah->ah_link.al_flags & ALF_LOCAL_AUTH))
			return;

	if (ah->ah_link.al_peer_opts & (ALO_PAP | ALO_CHAP))
		if (!(ah->ah_link.al_flags & ALF_PEER_AUTH))
			return;

	if (ah->ah_link.al_peer_auth_name)
		ppplog(MSG_INFO_MED, ah,
		       "Authentication -  Peer name is '%*.*s'\n",
		       ah->ah_link.al_peer_auth_namelen,
		       ah->ah_link.al_peer_auth_namelen,
		       ah->ah_link.al_peer_auth_name);

	ppplog(MSG_INFO_LOW, ah, "Authentication Complete\n");

	ph = ah->ah_protos;

	while (ph) {
		if (ph->ph_psmtab->pt_flags & PT_AUTH)
			/* Call the terminate routines */
			(*ph->ph_psmtab->pt_term)(ph);
		ph = ph->ph_next;
	}

	ppp_phase(ah, PHASE_NETWORK);
}

/*
 * Get the local hosts name, if there is an authname override
 * return that value, otherwise return the local host name.
 */
uchar_t *
auth_localname(proto_hdr_t *ph)
{
	act_hdr_t *ah = ph->ph_parent;
	char *np = NULL, *name;
	struct cfg_global *gl = NULL;
	extern char host[];
	extern struct cfg_global *global[];

	ASSERT(ah->ah_type == DEF_LINK);

	if (ah->ah_link.al_flags & ALF_AUTO ||
	    !ah->ah_link.al_bundle) {

		struct cfg_global *gl;

		/*
		 * We have an incoming call, auto detected or not
		 * yet matching a bundle ... use global bundle
		 */
		psm_log(MSG_DEBUG, ph, "auth_localname: incoming\n");
		ucfg_lock();
		if (gl = global[DEF_BUNDLE]) {
			gl->gi_ch.ch_refcnt++;
			np = ucfg_str(&(gl->gi_ch), gl->gi_authname);
		} else {
			psm_log(MSG_DEBUG, ph,
				"No auth defined for incoming\n");
		}
		ucfg_release();
	} else {
		/*
		 * We have an outgoing call .. or login incoming, use
		 * bundles 
		 */
		struct cfg_bundle *cb;

		psm_log(MSG_DEBUG, ph,
			"auth_localname: outgoing/login incoming\n");

		cb = (struct cfg_bundle *) ah->ah_link.al_bundle->ah_cfg;
		np = ucfg_str(&(cb->bn_ch), cb->bn_authname);
	} 

	if (np && *np) {
		name = strdup(np);
	} else
		name = strdup(host);

	/* Dec any refcnt */

	if (gl)
		gl->gi_ch.ch_refcnt--;

	psm_log(MSG_DEBUG, ph, "auth_localname: Using name %s\n",
		 name ? name : "Null string");
	return (uchar_t *)name;
}

/*
 * Get the peer name, if there is a peerauthname override
 * return that value otherwise return the supplied name.
 *
 * Returns a malloc()ed piece of memory containing the name.
 */
uchar_t *
auth_peername(proto_hdr_t *ph, uchar_t *name, int namelen)
{
	act_hdr_t *ah = ph->ph_parent;
	char *np = NULL;
	struct cfg_global *gl = NULL;
	extern char host[];

	ASSERT(ah->ah_type == DEF_LINK);

	if (ah->ah_link.al_flags & ALF_AUTO ||
	    !ah->ah_link.al_bundle) {
		/*
		 * We have an outgoing call, use any
		 * incoming auth config
		 */

#define DEF_INC_STR "bundle"
		psm_log(MSG_DEBUG, ph, "auth_peername: incoming\n");

		if (ucfg_findid(DEF_INC_STR, DEF_GLOBAL, &gl) == 0) {
			gl->gi_ch.ch_refcnt++;
			np = ucfg_str(&(gl->gi_ch), gl->gi_peerauthname);
			ucfg_release();
		} else {
			psm_log(MSG_DEBUG, ph,
				"No auth defined for incoming\n");
		}
	} else {
		/*
		 * We have an outgoing call, use any
		 * outgoing auth config
		 */
		struct cfg_bundle *cb;
       
		psm_log(MSG_DEBUG, ph, "auth_peername: outgoing\n");

		cb = (struct cfg_bundle *) ah->ah_link.al_bundle->ah_cfg;
		np = ucfg_str(&(cb->bn_ch), cb->bn_peerauthname);
	}

	if (np && *np) {
		/* Over-ride name supplied */
		name = (uchar_t *)strdup(np);
	} else {
		np = (char *)malloc(namelen + 1);
		if (np) {

			memcpy(np, name, namelen);
			np[namelen] = 0;
			name = (uchar_t*)np;
		} else
			psm_log(MSG_WARN, ph, "No memory. Dropped\n");
	}

	/* Dec any refcnt */

	if (gl)
		gl->gi_ch.ch_refcnt--;

	psm_log(MSG_DEBUG, ph, "auth_peername: Using name %s\n", 
		 (char *)name ? (char *)name : "Null string");
	return name;
}

/*
 * Parse the secret. \ooo is an octal representation of a character
 * Also allow \xhh as hex representation.
 * Note that the NULL character is *NOT* allowed.
 */
STATIC char *
parse_secret(char *src)
{
	char *dst, *start;
	int i;
	char buf[5];

	start = dst = (char *)malloc(strlen(src) + 1);
	if (!dst)
		return dst;

	while (*src) {
		if (*src == '\\') {
			src++;
			
			if (!*src)
				break;

			if (*src == 'x') {
				src++;

				if (!*src || !*(src+1))
					break;

				/* A 2 digit hex number */
				sprintf(buf, "0x%c%c", *src, *(src+1));
				*dst++ = (char)strtol(buf, NULL, 0);
				src += 2;

			} else if (*src >= '0' && *src <= '7') {

				buf[0] = '0';
				buf[1] = *src++;
				buf[2] = 0;

				/* A 1,2 or 3 digit octal number */
				for (i = 0; i < 2; i++) {
					if (*src >= '0' && *src <= '7') {
						buf[2 + i] = *src++;
						buf[3 + i] = 0;
					} else
						break;
				}

				*dst++ = (char)strtol(buf, NULL, 0);
			} else if (*src == '\\') {
				/* A backslash */
				*dst++ = *src++;
			}
			/* Else we ignore it */
		} else
			*dst++ = *src++;
	}
	*dst = 0;
	return start;
}

/*
 * Obtain the secrect that corresponds to the supplied name
 *
 * type 	- specifies whether we are looking up
 *		  a peer name or out local name.
 *
 * return a pointer to a null terminated secret. This is strdup()ed
 * and needs to be free()ed by the caller.
 */
uchar_t *
auth_get_secret(proto_hdr_t *ph, int type, char *name, int namelen)
{
	ushort_t protocol;
	act_hdr_t *ah = ph->ph_parent;
	char *np = NULL;
	struct cfg_global *gl = NULL;
	char *secret = NULL;
	struct cfg_auth *au;

	ASSERT(ah->ah_type == DEF_LINK);

	if (namelen == 0)
		return (uchar_t *)secret;

	/* Protocol to lookup */
	protocol = ph->ph_psmtab->pt_proto;

	/* Look up the secret */

	ucfg_open(DEF_AUTH, &au);

	while (au) {

		psm_log(MSG_DEBUG, ph, "Matching proto %4.4x, %4.4x\n",
			 au->au_protocol, protocol);

		if (au->au_protocol == protocol) {

			np = ucfg_str(&(au->au_ch), au->au_name);

			psm_log(MSG_DEBUG, ph, "Matching name len for name %s, %d, %d\n", np, strlen(np), namelen);
			
			if (strncmp(np, name, namelen) == 0) {
				switch (type) {
				case PEERSEC:
					secret = ucfg_str(&(au->au_ch),
							  au->au_peersecret);
					break;
				case LOCALSEC:
					secret = ucfg_str(&(au->au_ch),
							  au->au_localsecret);
					break;
				}

				/* Parse the secret */
				secret = parse_secret(secret);
				break;
			}
		}

		ucfg_next(&au);
	} 

	ucfg_close();

	psm_log(MSG_DEBUG, ph, "auth_get_secret: Using secret '%s'\n", 
		 secret ? secret : "Null string");
	return (uchar_t *)secret;
}
