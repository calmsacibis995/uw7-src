#ident	"@(#)lcp_parse.c	1.3"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ppp.h>
#include <sys/ppp_psm.h>
#include <sys/ppp_ml.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "ppptalk.h"
#include "lcp_cfg.h"


/*
 * Supported CHAP algorithms
 */
STATIC struct str2val_s def_chap_alg[] = {
	"MD5",	(void *)CHAP_MD5,
	NULL, NULL,
};

STATIC struct psm_opttab lcp_opts[] = {
	"protocol", 	STRING, MANDATORY, NULL, NULL,

	/* FSM Options first */
	FSM_OPT_NAMES,

	/* LCP specific opts */
	"mru",		NUMERIC_R, OPTIONAL, (void *)256, (void *)16384,
	"accm",		NUMERIC_R, OPTIONAL, (void *)0, (void *)0xffffffff,
	"acfc", 	BOOLEAN, OPTIONAL, NULL, NULL,
	"pfc", 		BOOLEAN, OPTIONAL, NULL, NULL,
	"magic",	BOOLEAN, OPTIONAL, NULL, NULL,
	/*"lqm", 		BOOLEAN, OPTIONAL, NULL, NULL,*/
	"echoperiod",	NUMERIC_R, OPTIONAL, (void *)0, (void *)300,
	"echofails", 	NUMERIC_R, OPTIONAL, (void *)1, (void *)100,
	"echosample", 	NUMERIC_R, OPTIONAL, (void *)1, (void *)100,
	"identification", BOOLEAN, OPTIONAL, NULL, NULL,
	NULL, 0, 0, NULL, NULL,

};

STATIC void
lcp_set_defaults(struct cfg_lcp *p)
{
	p->lcp_ch.ch_len = p->lcp_var - (char *)p;
	p->lcp_ch.ch_stroff = p->lcp_var - (char *)p;
	p->lcp_mru = DEFAULT_MRU;
	p->lcp_accm = DEFAULT_ACCM;
	p->lcp_acfc = DEFAULT_ACFC;
	p->lcp_pfc = DEFAULT_PFC;
	p->lcp_magic = DEFAULT_MAGIC;
	/*p->lcp_lqm = 1;*/
	p->lcp_echo_period = DEFAULT_ECHOPERIOD;
	p->lcp_echo_fails = DEFAULT_ECHOFAILS;
	p->lcp_echo_sample = DEFAULT_ECHOSAMPLE;
	p->lcp_ident = DEFAULT_IDENT;
	FSM_OPT_DEFAULTS(&p->lcp_fsm);
}

STATIC void
lcp_list(FILE *fp, struct cfg_lcp *p)
{
	fprintf(fp, "	protocol = lcp\n");
	fprintf(fp, "	mru = %d\n", p->lcp_mru);
	fprintf(fp, "	accm = 0x%x\n", p->lcp_accm);
	fprintf(fp, "	acfc = %s\n", def_get_bool(p->lcp_acfc));
	fprintf(fp, "	pfc = %s\n", def_get_bool(p->lcp_pfc));
	fprintf(fp, "	magic = %s\n", def_get_bool(p->lcp_magic));
	/*fprintf(fp, "	lqm = %s\n", def_get_bool(p->lcp_lqm));*/
	fprintf(fp, "	echoperiod = %d\n", p->lcp_echo_period);
	fprintf(fp, "	echofails = %d\n", p->lcp_echo_fails);
	fprintf(fp, "	echosample = %d\n", p->lcp_echo_sample);
	fprintf(fp, "	identification = %s\n", def_get_bool(p->lcp_ident));
	FSM_OPT_DISPLAY(fp, &p->lcp_fsm);
}

char *act_link_opts[] = {
	"Chap",			/* 0001 */
	"Pap",			/* 0002 */
	"LQM",			/* 0004 */
	"ACFC",			/* 0008 */
	"PFC",			/* 0010	*/
	"Magic#",		/* 0020 */
	"MRRU",			/* 0040 */
	"SSN",			/* 0080 */
	"ED",			/* 0100 */
	"ML Negotiable",	/* 0200 */
	"LD",			/* 0400 */
};
	
STATIC int
lcp_status(struct lcp_status_s *st, int depth, int v, int (*pf)(...))
{
	char *ds;
	extern int ppp_debug;	
	act_hdr_t *ah = &st->st_hdr;
	act_link_t *al = &ah->ah_link;

	if (ah->ah_type != DEF_LINK)
		return -1;

	if (depth > 0)
		ds = "\t";
	else
		ds = "";

	switch (v) {

	case PSM_V_SILENT:
		break;

	case PSM_V_SHORT:
		(*pf)("\t%sLCP\tState:%s\n", ds,
		      fsm_state(&st->st_fsm));
		break;

	case PSM_V_LONG:
		(*pf)("\t%sLCP State     : %s\n", ds,
		      fsm_state(&st->st_fsm));
		(*pf)("\t%sReason        : %s\n", ds,
		      fsm_reason(&st->st_fsm));
		(*pf)("\t%sPacket counts : %s\n", ds,
		      fsm_pktcnts(&st->st_fsm));
		(*pf)("\t%sLocal Options : %s\n", ds, 
		      psm_display_flags(11, act_link_opts, al->al_local_opts));
		(*pf)("\t%sPeer Options  : %s\n", ds, 
		      psm_display_flags(11, act_link_opts, al->al_peer_opts));

		if (ppp_debug) {
			(*pf)("\t%sLocal MRU     : %d\n", ds, 
			      al->al_local_mru);
			(*pf)("\t%sLocal magic # : 0x%x\n",
			      ds, al->al_local_magic);
			(*pf)("\t%sPeer MRU      : %d\n", ds, 
			      al->al_peer_mru);
			(*pf)("\t%sPeer magic #  : 0x%x\n",
			      ds, al->al_peer_magic);
			if (al->al_local_opts & ALO_LD)
				(*pf)("\t%sLocal LD      : %d\n", ds,
				      al->al_local_ld);
			if (al->al_peer_opts & ALO_LD)
				(*pf)("\t%sPeer LD       : %d\n", ds,
				      al->al_peer_ld);
		}
		(*pf)("\n");
		break;

	}

	return st->st_fsm.ps_state;
}

STATIC uchar_t
lcp_stats(struct ml_s *tx, struct ml_s *rx, int depth, int (*pf)(...))
{
	char *ds;

	if (depth > 0)
		ds = "    ";
	else
		ds = "";

	if (tx) {
		(*pf)("        %sLCP Multilink Output\n", ds);
		(*pf)("        %s  %lu packets sent\n", ds,
		      tx->ml_outpackets);
		(*pf)("        %s  %lu fragments sent\n", ds,
		      tx->ml_outfrags);
		(*pf)("        %s  %lu packets lost (no resources)\n", ds,
		      tx->ml_outlost);
	}

	if (rx) {
		if (tx)
			(*pf)("\n");
		(*pf)("        %sLCP Multilink Input\n", ds);
		(*pf)("        %s  %lu framents discarded\n", ds,
		      rx->ml_indiscards);
		(*pf)("        %s      %lu too short\n", ds,
		      rx->ml_tooshort);
		(*pf)("        %s      %lu duplicate fragments\n", ds,
		      rx->ml_dupfrag);
		(*pf)("        %s      %lu assumed lost received\n", ds,
		      rx->ml_lostfrag);
		(*pf)("        %s      %lu no memory\n", ds,
		      rx->ml_nomem);
		(*pf)("        %s      %lu invalid format\n", ds,
		      rx->ml_invalid);
		(*pf)("        %s  %lu fragments received\n", ds,
		      rx->ml_infrags);
		(*pf)("        %s  %lu fragments lost\n", ds,
		      rx->ml_inlost);
		(*pf)("        %s  %lu packets received\n", ds,
		      rx->ml_ingoodpackets);
		(*pf)("\n");
	}
}

struct psm_entry_s psm_entry = {
	lcp_opts,
	lcp_set_defaults,
	lcp_list,
	lcp_status,
	lcp_stats,
	PSMF_LINK | PSMF_FSM,
};
