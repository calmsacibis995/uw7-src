#ifndef _CHAP_H
#define _CHAP_H

#ident	"@(#)chap.h	1.6"

#include <sys/ppp_f.h>

#pragma pack(1)

/*
 * The CHAP packet format (Protocol 0xc223)
 */
struct chap_hdr_s {
	u8b_t		chap_code;
	u8b_t		chap_id;
	u16b_t		chap_len;
};

/*
 * CHAP Packet codes
 */
#define CHAP_CHALLENGE	1
#define CHAP_RESPONSE	2
#define CHAP_SUCCESS	3
#define CHAP_FAIL	4

#pragma pack(4)

/*
 * CHAP private data structure
 */
struct chap_s {
	ulong_t 	chap_challenge[4]; 
	/* Fields used for local authentication - we require auth */
	uchar_t		chap_local_auth_cnt;	/**/
	void		*chap_local_auth_tmid;
	uchar_t		chap_local_auth_id; /* ID of last auth packet sent */

	/* Fields used for peer authentication - peer requires auth */
	uchar_t		chap_peer_auth_cnt;	/**/
	void		*chap_peer_auth_tmid;
	uchar_t		chap_peer_auth_id; /* ID of last auth packet sent */

	ulong_t		chap_seed;		/* seed for challenges */

	ushort_t	chap_local_status;	/* Local auth status */
	ushort_t	chap_peer_status;	/* Peer auth status */
	uint_t		chap_local_fail;	/* Failure count */
	uint_t		chap_peer_fail;	/* Failure count */

	/* This next field holds the name of the peer last seen */
#define CHAP_PNAMELEN 20
	char		chap_peer_name[CHAP_PNAMELEN + 1];
};

/*
 * Possible status values
 */

#define CHAPS_INITIAL	0	/* Initial state */
#define CHAPS_SUCCESS	1	/* Success */
#define CHAPS_FAILED 	2	/* Failure */
#define CHAPS_BADFMT	3	/* Packet format incorrect / id mismatch*/
#define CHAPS_NORES	4	/* Out of resources */
#define CHAPS_TIMEOUT	5	/* Timeout */
#define CHAPS_NOSEC 	6	/* No secret defined */

#endif /* _CHAP_H */
