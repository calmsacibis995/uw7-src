#ident	"@(#)kern-i386at:io/cpyrt/cpyrt.c	1.1"
#ident	"$Header$"

#include <util/cmn_err.h>

#include <io/ddi.h>


/*
 * OEM COPYRIGHTS "DRIVER"
 */

/*
 * The following array is to be used by any drivers which need to put
 * OEM copyright notices out on boot up.  It is initialized to all 0's
 * (NULL).  A driver which needs to have a copyright string  written to
 * the console during bootup should look (in its init routine) for the
 * first empty slot and put in a pointer at a static string which
 * cpyrtstart will write out (using printf "%s\n\n").
 *
 * If there is more than one driver from a given OEM, the init code should
 * check the filled slots and compare the copyright strings so a given
 * message only appears once.  If somebody overflows the array, chaos
 * will probably reign.  Caveat coder.
 *
 * WARNING: This mechanism will not work for drivers bound to CPUs other
 * than 0, since cpyrtstart() will be called before such a driver's init
 * routine.  Similarly, this is not useful from loadable drivers.
 */

extern int     max_copyrights;
extern char    *oem_copyrights[];

/*
 * void
 * cpyrtstart(void)
 *	Write out any OEM driver copyright notices
 *
 * Calling/Exit State:
 *	Called at driver start time.  No locking requirements.
 */
void
cpyrtstart(void)
{
	uint_t i;

	for (i = 0; i < max_copyrights; i++) {
		if (oem_copyrights[i] == NULL)
			break;
		cmn_err(CE_CONT, "%s\n\n", oem_copyrights[i]);
	}
}
