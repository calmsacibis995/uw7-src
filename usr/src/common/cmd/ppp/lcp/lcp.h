#ifndef _LCP_H
#define _LCP_H

#ident	"@(#)lcp.h	1.3"

#include <sys/ppp_f.h>
/*
 * These structure needs to be packed
 */
#pragma pack(1)

struct co_mru_s {
	struct co_s h;
	u16b_t co_mru;
};

struct co_accm_s {
	struct co_s h;
	u32b_t co_accm;
};

struct co_auth_s {
	struct co_s h;
	u16b_t co_auth;
};

struct co_quality_s {
	struct co_s h;
	u16b_t co_qual;
};

struct co_magic_s {
	struct co_s h;
	u32b_t co_magic;
};

struct co_mrru_s {
	struct co_s h;
	u16b_t co_mrru;
};

struct co_ed_s {
	struct co_s h;
	u8b_t co_class;
};

struct co_ssn_s {
	struct co_s h;
};

struct co_ld_s {
	struct co_s h;
	u16b_t co_ld;
};
	
/* Identification Code */
struct lcp_ident_s {
	struct lcp_hdr_s id_hdr;
	u32b_t id_magic;
	/* Followed by the data field */
};

#pragma pack(4)
/*
 * LCP Options Types as defined in RFC 1661
 */
#define CO_MRU	1
#define CO_ACCM	2
#define CO_AUTH	3
#define CO_QUALITY	4
#define CO_MAGIC	5
#define CO_RES		6
#define CO_PFC	7
#define CO_ACFC	8
#define CO_MRRU 17
#define CO_SSN 18
#define CO_ED 19
#define CO_LD 23

/*
 * Endpoit discriminator classes
 */
#define ED_NULL 0
#define ED_LOCAL 1
#define ED_IP 2
#define ED_MAC 3
#define ED_MAGIC 4
#define ED_PSTN 5

/*
 * LCP Params
 */
#define MIN_MRRU 300
#define MAX_MRRU 16384
#endif /* _LCP_H */
