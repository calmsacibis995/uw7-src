#ifndef _PAP_H
#define _PAP_H

#ident	"@(#)pap.h	1.5"

#include <sys/ppp_f.h>

#pragma pack(1)

/*
 * PAP packet format (Protocol 0xc023)
 */
struct pap_hdr_s {
	u8b_t		pap_code;
	u8b_t		pap_id;
	u16b_t		pap_len;
};

/*
 * Pap ack/nak packet format
 */
struct pap_response_s {
	struct pap_hdr_s pap_hdr;
	u8b_t pap_mlen;		/* Message length */
				/* Message follows */
};

/*
 * PAP Packet codes
 */
#define PAP_REQ 1
#define PAP_ACK 2
#define PAP_NAK 3

#pragma pack(4)

/*
 * Pap private data structure
 */
struct pap_s {
	/* Fields used for local authentication - we require auth */
	uchar_t		pap_local_auth_cnt;	/**/
	void		*pap_local_auth_tmid;
	uchar_t		pap_local_auth_id; /* ID of last auth packet sent */

	/* Fields used for peer authentication - peer requires auth */
	uchar_t		pap_peer_auth_cnt;	/**/
	void		*pap_peer_auth_tmid;
	uchar_t		pap_peer_auth_id; /* ID of last auth packet sent */

	ushort_t	pap_local_status;	/* Success/Failure for PAP */
	ushort_t	pap_peer_status;	/* Success/Failure for PAP */
	uint_t		pap_local_fail;		/* Failure count */
	uint_t		pap_peer_fail;		/* Failure count */
	/* This next field holds the name of the peer last seen */
#define PAP_PNAMELEN 20
	char		pap_peer_name[PAP_PNAMELEN + 1];

};

/*
 * Status codes for PAP
 */

/* Initial state */
#define PAP_INITIAL 0
/* Authentication succeeded */
#define PAP_SUCCESS 1
/* Authentication failed */
#define PAP_FAILED 2
/* Invalid pap packets */
#define PAP_BADFMT 3
/* Auth timeout */
#define PAP_TIMEOUT 4
/* Out of resources */
#define PAP_NORES 5
/* Secret not found in database */
#define PAP_NOSECRET 6

#endif /* _PAP_H */
