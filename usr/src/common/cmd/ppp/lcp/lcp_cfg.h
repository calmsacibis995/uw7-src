#ifndef _LCP_CFG_H
#define _LCP_CFG_H

#ident	"@(#)lcp_cfg.h	1.4"

#include "fsm.h"
#include "act.h"

struct cfg_lcp {
	struct cfg_hdr 	lcp_ch;
	uint_t		lcp_name;	/* The protocol name */
	struct cfg_fsm	lcp_fsm;
	uint_t		lcp_mru;	/* Option 1 */
	uint_t		lcp_accm;	/* Option 2 */
	uint_t		lcp_acfc;
	uint_t		lcp_pfc;
	uint_t		lcp_magic;
	/*uint_t		lcp_lqm;*/
	uint_t		lcp_echo_period;
	uint_t		lcp_echo_fails;
	uint_t		lcp_echo_sample;
	uint_t		lcp_ident;
	char		lcp_var[1]; /*Strings*/
};

#define DEFAULT_MRU 1500
#define DEFAULT_ACFC 1
#define DEFAULT_PFC 1
#define DEFAULT_MAGIC 1
#define DEFAULT_ECHOPERIOD 0
#define DEFAULT_ECHOFAILS 2
#define DEFAULT_ECHOSAMPLE 5
#define DEFAULT_IDENT 1


struct lcp_status_s {
	proto_state_t st_fsm;
	act_hdr_t st_hdr;
};

#endif /*_LCP_CFG_H */
