#ifndef _LCP_HDR_H
#define _LCP_HDR_H

#ident	"@(#)lcp_hdr.h	1.2"

#include <sys/ppp_f.h>

/*
 * This structure needs to be packed
 */
#pragma pack(1)
/*
 * LCP Option Header
 */
struct co_s {
	u8b_t co_type;
	u8b_t co_len;
};

/*
 * LCP packet header
 */
struct lcp_hdr_s {
	u8b_t lcp_code;
	u8b_t lcp_id;
	u16b_t lcp_len;
};

/*
 * LCP Protocol reject
 */
struct lcp_prtrej_s {
	struct lcp_hdr_s lcp_hdr;
	u16b_t lcp_proto;
};

/*
 * LCP Echo request
 */
struct lcp_echo_s {
	struct lcp_hdr_s lcp_hdr;
	u32b_t lcp_magic;
};

#pragma pack(4)

#define LCP_ECHO_RPLY 0
#define LCP_CODE_REJ 1
#define LCP_PROTO_REJ 2


#endif /* _LCP_HDR_H */
