#ident	"@(#)pap_parse.c	1.8"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "ppptalk.h"
#include "pap.h"

STATIC struct psm_opttab opts[] = {
	"protocol",	STRING, MANDATORY, NULL, NULL,
	NULL, 0, 0, NULL, NULL,
};

STATIC void
defaults(struct cfg_proto *p)
{
	p->pr_ch.ch_len = p->pad - (char *)p;
	p->pr_ch.ch_stroff = p->pad - (char *)p;

}

char *pap_states[] = {
	"Initial",
	"Success",
	"Failed",
	"Bad Format",
	"Timeout",
	"No resources",
	"No secret found",
};

STATIC int
pap_status(struct pap_s *pap, int depth, int v, int (*pf)(...))
{
	char *ds;
	if (depth > 0)
		ds = "\t";
	else
		ds = "";

	switch (v) {
	case PSM_V_SILENT:
		break;

	case PSM_V_SHORT:
		(*pf)("\t%sPAP\tStatus: Local %s : Peer %s\n", ds,
		      pap_states[pap->pap_local_status],
		      pap_states[pap->pap_peer_status]);
		break;

	case PSM_V_LONG:
	(*pf)("\t%sPAP Status    Local : %-13.13s  Peer : %-13.13s\n",
		      ds,
		      pap_states[pap->pap_local_status],
		      pap_states[pap->pap_peer_status]);
	(*pf)("\t%s    Failures  Local : %-13u  Peer : %-13u\n",
		      ds, pap->pap_local_fail, pap->pap_peer_fail);
	(*pf)("\t%s    Last Peer name  : %s\n",
		      ds, pap->pap_peer_name);
	(*pf)("\n");
		break;

	}
	return -1;
}

struct psm_entry_s psm_entry = {
	opts,
	defaults,
	NULL,
	pap_status,
	NULL,
	PSMF_AUTH | PSMF_LINK,
};
