#ifndef _CCP_CFG_H
#define _CCP_CFG_H

#ident	"@(#)cfg.h	1.2"

#include "fsm.h"

/*
 * CCP specific options
 */
struct cfg_ccp {
	struct cfg_hdr 	cp_ch;
	uint_t		cp_name;	/* The protocol name */
	struct cfg_fsm	cp_fsm;
	uint_t		cp_txalg;
	uint_t		cp_rxalg;
	uint_t		cp_alg;
	char cp_var[1]; /*Strings*/
};

#define CCP_OUI 0
#define CCP_PRED_1 1
#define CCP_PRED_2 2
#define CCP_PUDDLE 3
/* 4-15 are unassigned */
#define CCP_HP	16
#define CCP_STAC 17
#define CCP_MS 18
#define CCP_GANDALF 19
#define CCP_V42BIS 20
#define CCP_BSD_LWZ 21

struct ccp_status_s {
	proto_state_t st_fsm;
};

#endif /*_CCP_CFG_H */
