#ident "@(#)Space.c	29.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/cred.h>
#include <sys/mdi.h>
#include <sys/dlpimod.h>
#include <sys/sr.h>
#include <sys/moddefs.h>
#include <sys/ksynch.h>
#include <sys/ddi.h>
#include "config.h"

extern int dlpiopen(), dlpilrput(), dlpiuwput(), dlpiclose();
extern int dlpiursrv(), dlpiuwsrv(), dlpilrsrv(), dlpilwsrv();
STATIC int dlpimodopen();

STATIC struct module_info XXXXurinfo = { 
YYYYSTREAMSMID, "XXXX", 0, INFPSZ, 4096,    1 
};

STATIC struct module_info XXXXminfo = { 
YYYYSTREAMSMID, "XXXX", 0, INFPSZ,    2,    1 
};

STATIC struct qinit XXXXurinit = {
	NULL,     dlpiursrv, dlpimodopen, dlpiclose, NULL, &XXXXurinfo, NULL
};
STATIC struct qinit XXXXuwinit = {
	dlpiuwput, dlpiuwsrv, dlpimodopen, dlpiclose, NULL, &XXXXminfo, NULL
};
STATIC struct qinit XXXXlrinit = {
	dlpilrput, dlpilrsrv, dlpimodopen, dlpiclose, NULL, &XXXXminfo, NULL
};
STATIC struct qinit XXXXlwinit = {
	NULL    , dlpilwsrv, dlpimodopen, dlpiclose, NULL, &XXXXminfo, NULL
};

struct streamtab XXXXinfo = { &XXXXurinit,&XXXXuwinit,&XXXXlrinit,&XXXXlwinit };

STATIC per_card_info_t		XXXXcardinfo;
STATIC per_sap_info_t		XXXXsapinfo[YYYYSAPMAXSAPS];
STATIC struct route_param 	XXXXroute_param = {
	YYYYSRTXRESP,	YYYYSRRXARE, 	YYYYSRRXSTEBCS, 
	YYYYSRRXSTEUCS,	YYYYSRMAXTX,	YYYYSRMINTX, 
	YYYYSRTXRECUR, 	YYYYSRAREDISAB
};
STATIC dlpi_init_t XXXXcardinit = { 
    YYYYTXMONCNSMT, YYYYTXMONENABL, YYYYTXMONCNTLI
};

int XXXXdevflag = D_MP|D_UPF;
#define DRVNAME "XXXX - DLPI module instance"
static int XXXX_load(), XXXX_unload();
MOD_DRV_WRAPPER(XXXX, XXXX_load, XXXX_unload, NULL, DRVNAME);

static int
XXXX_load()
{
	return(0);
}

static int
XXXX_unload()
{
	return(0);
}

/*
 * Front-End for Open Routing which passes a pointer to the per-card
 * information to 'dlpiopen'
 */
STATIC int
dlpimodopen(q, dev, flag, sflag, crp)
queue_t *q;
dev_t *dev;
int flag, sflag;
cred_t *crp;
{
	return (dlpiopen(q,dev,flag,sflag,&XXXXcardinfo,
                         YYYYSRMAXROUTE, 
                         YYYYSRSRMODE, 
                         &XXXXroute_param,
                         XXXXsapinfo, YYYYSAPMAXSAPS,
                         sizeof(per_card_info_t),
                         sizeof(per_sap_info_t),
                         &XXXXcardinit,
                         sizeof(dlpi_init_t),
                         YYYYMCAHASHENT,
                         YYYYDISABEIIFR,
                         crp,
                         "XXXX"
                         )
                );
}
