#ident	"@(#)kern-i386at:io/cpyrt/cpyrt.cf/Space.c	1.1"
#ident	"$Header$"

/*
 * OEM COPYRIGHTS "DRIVER"
 */

/*
 * The following array is to be used by any drivers which need to put
 * OEM copyright notices out on boot up.  It is initialized to all 0's
 * (NULL).  A driver which needs to have a copyright string written to
 * the console during bootup should look (in the init routine) for the
 * first empty slot and put in a pointer at a static string which 'main'
 * will write out (using printf "%s\n\n") after the AT&T copyright.  If
 * there is more than one driver from a given OEM, the init code should
 * check the filled slots and compare the copyright strings so a given
 * message only appears once.  If somebody overflows the array, chaos
 * will probably reign.  Caveat coder.
 */

#include <config.h>


int max_copyrights = NCPYRIGHT;
char *oem_copyrights[NCPYRIGHT];

/*
 *	This is an example of how to use the cpyrt driver to insert
 *	your copyright notice:
 *
 *	int index;
 *	char *msgptr = "YOUR MESSAGE HERE";
 *
 *	for (index = 0; index < max_copyrights; index++) {
 *		if (!oem_copyrights[index]) {	 [ if open slot put in msg ]
 *			oem_copyrights[index] = msgptr;
 *			break;
 *		}
 *		if (strcmp(msgptr, oem_copyrights[index]) == 0)
 *			break;		[ msg already there ]
 *	}
 */
