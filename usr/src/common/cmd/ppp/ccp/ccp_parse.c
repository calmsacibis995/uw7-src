#ident	"@(#)ccp_parse.c	1.3"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>

#include "ppp_type.h"
#include "ppp_cfg.h"

#include "ppptalk.h"
#include "cfg.h"

/*
 * This module provides all the CCP specific functions required to
 * parse (and list) the users configuration
 */
/*
 * CCP specific protocol options
 */
STATIC struct psm_opttab ccp_opts[] = {
	"protocol", STRING, MANDATORY, NULL, NULL,

	/* FSM Options first */
	FSM_OPT_NAMES,

	"txalgorithms", STRING, OPTIONAL, NULL, NULL,
	"rxalgorithms", STRING, OPTIONAL, NULL, NULL,
	"algorithms", STRING, OPTIONAL, NULL, NULL,
	NULL, 0, 0, NULL, NULL,
};

STATIC void
ccp_set_defaults(struct cfg_ccp *p)
{
	p->cp_ch.ch_len = p->cp_var - (char *)p;
	p->cp_ch.ch_stroff = p->cp_var - (char *)p;

	/*def_insert_str(&(p->cp_ch), &(p->cp_name), "");*/
	def_insert_str(&(p->cp_ch), &(p->cp_txalg), "");
	def_insert_str(&(p->cp_ch), &(p->cp_rxalg), "");
	def_insert_str(&(p->cp_ch), &(p->cp_alg), "");
	FSM_OPT_DEFAULTS(&p->cp_fsm);
}

STATIC void
ccp_list(FILE *fp, struct cfg_ccp *p)
{
	fprintf(fp, "	protocol = ccp\n");
	fprintf(fp, "	txalgorithms = %s\n",
	       def_get_str(&p->cp_ch, p->cp_txalg));
	fprintf(fp, "	rxalgorithms = %s\n",
	       def_get_str(&p->cp_ch, p->cp_rxalg));
	fprintf(fp, "	algorithms = %s\n",
	       def_get_str(&p->cp_ch, p->cp_alg));
	FSM_OPT_DISPLAY(fp, &p->cp_fsm);
}


STATIC int
ccp_status(struct ccp_status_s *st, int depth, int v, int (*pf)(...))
{
	char *ds;
	extern char *fsm_states[];

	if (depth > 0)
		ds = "\t";
	else
		ds = "";

	switch(v) {

	case PSM_V_SILENT:
		break;

	case PSM_V_SHORT:
		(*pf)("\t%sCCP\tState:%s\n", ds,
		      fsm_state(&st->st_fsm));
		break;

	case PSM_V_LONG:	
		(*pf)("\t%sCCP State     : %s\n", ds,
		       fsm_state(&st->st_fsm));
		(*pf)("\t%sReason        : %s\n", ds,
		      fsm_reason(&st->st_fsm));
		(*pf)("\t%sPacket counts : %s\n", ds,
		      fsm_pktcnts(&st->st_fsm));
		(*pf)("\n");
		break;
	}

	return st->st_fsm.ps_state;
}

struct psm_entry_s psm_entry = {
	ccp_opts,
	ccp_set_defaults,
	ccp_list,
	ccp_status,
	NULL,
	PSMF_FSM | PSMF_CCP | PSMF_BUNDLE,
};
