#ident	"@(#)Space.c	1.2"
#include <config.h>
#include <sys/covert.h>
#include <sys/types.h>

int mac_cachel = MAC_CACHEL;
int mac_mru_lvls = MAC_MRU_LVLS;

int mac_installed;


/* Covert Channel stuff */

ulong_t ct_delay = CT_DELAY;	/* delay threshold in ticks */
ulong_t ct_audit = CT_AUDIT;	/* audit threshold in ticks */
ulong_t ct_cycle = CT_CYCLE;	/* cycling period in tick */
ulong_t cc_psearchmin = CC_PSEARCH;	/* minimal process search */
