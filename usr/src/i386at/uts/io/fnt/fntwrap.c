/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#ident	"@(#)fntwrap.c	1.2"
#ident	"$Header$"

/*
 * fntwrap.c, Memory Resident Font Module
 */

/*
 *	L000	7/3/97		rodneyh@sco.com
 *	- Remove Novell copyright, add SCO copyright.
 *
 */

#include <io/fnt/fnt.h>
#include <io/gsd/gsd.h>
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/mod/moddefs.h>

#define	DRVNAME	"fnt - Memory Resident Font Module"

int	fnt_load(void);
int	fnt_unload(void);

MOD_MISC_WRAPPER(fnt, fnt_load, fnt_unload, DRVNAME);

/*
 * fnt_load(void)
 * Load and register the driver on demand.
 *
 * Calling/Exit status:
 *
 */
int
fnt_load(void)
{
	fntinit();

	return(0);
}

/*
 * fnt_unload(void)
 *	Unload and unregister the driver.
 *
 * Calling/Exit status:
 *
 */
int
fnt_unload(void)
{
	if (channel_ref_count == 0) {
		fnt_init_flg = 0;
		return (0);
	}
	return (EBUSY);
}

