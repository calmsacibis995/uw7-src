/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#ident	"@(#)gsdwrap.c	1.2"
#ident	"$Header$"

/*
 * gsdwrap.c, Graphics System Driver Module wrappers.
 */

/*
 *	L000	7/3/97		rodneyh@sco.com
 *	- Remove Novell copyright, add SCO copyright.
 *
 */

#include <io/ws/ws.h>
#include <io/gsd/gsd.h>
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/mod/moddefs.h>

#define	DRVNAME	"gsd - Graphics System Driver Module"

int	gsd_load(void);
int	gsd_unload(void);

MOD_MISC_WRAPPER(gsd, gsd_load, gsd_unload, DRVNAME);

/*
 * gsd_load(void)
 * Load and register the driver on demand.
 *
 * Calling/Exit status:
 *
 */
int
gsd_load(void)
{
	gsdinit();

	return(0);
}

/*
 * gsd_unload(void)
 *	Unload and unregister the driver.
 *
 * Calling/Exit status:
 *
 */
int
gsd_unload(void)
{
	if (channel_ref_count == 0) {
		gs_init_flg = 0;
		return (0);
	}

	return (EBUSY);
}

