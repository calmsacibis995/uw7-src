#ident	"@(#)init.c	1.3"

#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <synch.h>
#include <thread.h>
#include <errno.h>

#include "pathnames.h"
#include "psm.h"
#include "ppp_type.h"
#include "ppp_cfg.h"
#include "act.h"
#include "ppp_proto.h"


/*
 * Perform config initialisation.
 *
 * This module contains the routines used to add the default config
 */
#define INIT_DEFAULT_PAP "pap"
#define INIT_DEFAULT_CHAP "chap"
#define INIT_DEFAULT_LCP "\tbundle_lcp"


STATIC int
default_pap()
{
	static struct cfg_proto pap;

	strcpy(pap.pr_ch.ch_id, INIT_DEFAULT_PAP);
	pap.pr_ch.ch_len = (char *)&pap.pad - (char *)&pap;
	ucfg_insert_str(&pap.pr_ch, &(pap.pr_name), INIT_DEFAULT_PAP);
	pap.pr_ch.ch_flags = CHF_RONLY;

	return ucfg_set(DEF_PROTO, &pap);
}



STATIC int
default_chap()
{
	static struct cfg_proto chap;

	strcpy(chap.pr_ch.ch_id, INIT_DEFAULT_CHAP);
	chap.pr_ch.ch_len = (char *)&chap.pad - (char *)&chap;
	ucfg_insert_str(&chap.pr_ch, &(chap.pr_name), INIT_DEFAULT_CHAP);
	chap.pr_ch.ch_flags = CHF_RONLY;

	return ucfg_set(DEF_PROTO, &chap);
}

STATIC int
default_bundle_lcp()
{
	static struct cfg_proto *lcp;

	lcp = (struct cfg_proto *)malloc(sizeof(struct cfg_proto));
	if (!lcp)
		return -1;

	memset(lcp, 0, sizeof(struct cfg_proto));
	strcpy(lcp->pr_ch.ch_id, INIT_DEFAULT_LCP);
	lcp->pr_ch.ch_len = (char *)&(lcp->pad) - (char *)lcp;
	ucfg_insert_str(&(lcp->pr_ch), &(lcp->pr_name), "lcp");
	lcp->pr_ch.ch_flags = CHF_HIDE | CHF_RONLY;
	return ucfg_set(DEF_PROTO, lcp);

}

default_init()
{
	int ret;

	/* default_lcp */
	ret = default_bundle_lcp();

	/* default_pap */
	ret = default_pap();

	/* default_chap */
	ret = default_chap();

}
