#ident	"@(#)pred1_parse.c	1.2"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <sys/pred1comp.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "ppptalk.h"
#include "cfg.h"

/*
 * This module provides all the Predictor Compress specific functions required to
 * parse (and list) the users configuration
 */

/*
 * Predictor Compression specific protocol options
 */
STATIC struct psm_opttab pred1_opts[] = {
	"algorithm",	STRING, MANDATORY, NULL, NULL,
	NULL, 0, 0, NULL, NULL,
};

STATIC void
pred1_set_defaults(struct cfg_pred1 *p)
{
	p->pred1_ch.ch_len = p->pred1_var - (char *)p; 
        p->pred1_ch.ch_stroff = p->pred1_var - (char *)p; 
	/* no config parameters to set */
}

STATIC void
pred1_list(FILE *fp, struct cfg_pred1 *p)
{
	int i;

	fprintf(fp, "	algorithm = pred1\n");
}

struct psm_entry_s psm_entry = {
	pred1_opts,
	pred1_set_defaults,
	pred1_list,
	NULL,
	NULL,
	PSMF_CCP | PSMF_BUNDLE,
};
