#ident	"@(#)kern-pdi:io/target/sdi/dynstructs.c	1.16.3.1"
#ident	"$Header$"

#include <io/target/sdi/dynstructs.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>	/* must come last */

/* sdi_get(), sdi_free() supplied for compatibility */

/*
 * struct jpool *
 * sdi_get(struct head *headp, int flag)
 *	Get jpool struct.
 *
 * Calling/Exit State:
 *	None.
 */
struct jpool *
sdi_get(struct head *headp, int flag)
{
	return (struct jpool *)kmem_zalloc(headp->f_isize, flag);
}

/*
 * void
 * sdi_free(struct head *headp, struct jpool *jp)
 *	Free jpool struct.
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_free(struct head *headp, struct jpool *jp)
{
	kmem_free(jp, headp->f_isize);
	return;
}
