#ident	"@(#)pdi.cmds:edt_fix.c	1.2"

/*
 * edt_fix
 *
 * edt_fix is a function that resets the xedt_ctl field to the value
 * that was passed to sdi_gethbano
 */

#include	<sys/sdi_edt.h>
#include	"edt_sort.h"

/*
 *	The algorithm for determining bootable order in sort_edt below
 *	is used in pdimkdev ( mkdev.c ), bmkdev ( boot_mkdev.c )
 *	and pdiconfig ( config.c ).
 */
int
edt_fix(EDT *xedtptr, int edtcnt)
{
	int	sorted, newvalue, value, cntl, map_count, *HBA_map;

	/*
	 *  First, get the original HBA mapping
	 */
	DTRACE;
	HBA_map = readhbamap(&map_count);

	/*
	 *  if readhbamap was unable to renumber things, tell the outside
	 *  world that it must start sorting at controller 0.
	 */
	if ( !HBA_map )
		return(0);

#ifdef DPRINTF
	for (cntl=0; cntl < map_count; cntl++)
		printf("map[%d] = %d\n",cntl,HBA_map[cntl]);
#endif

	/*
	 *  Go through the array and figure out how many existing controllers
	 *  there are in the configuration.
	 */
	DTRACE;
	sorted = 0;
	newvalue = -1;
	for ( cntl = 0; cntl < map_count; cntl++) {
		if (HBA_map[cntl] >= 0)
			sorted++;
		if (HBA_map[cntl] > newvalue)
			newvalue = HBA_map[cntl];
	}

	/*
	 *  Go through the array and assign new number to the negative
	 *  entries.  These represent the NEW devices in the EDT.
	 */
	DTRACE;
	for ( cntl = 0; cntl < map_count; cntl++)
		if (HBA_map[cntl] == SDI_NEW_MAP)
			HBA_map[cntl] = ++newvalue;

#ifdef DPRINTF
	for (cntl=0; cntl < map_count; cntl++)
		printf("map[%d] = %d\n",cntl,HBA_map[cntl]);
#endif

	/* 
	 * Walk through the EDT and replace the controller number
	 * that was assigned by sdi_gethbano with the one that was
	 * calculated above.
	 */
	DTRACE;
	for ( cntl = 0; cntl < edtcnt; cntl++, xedtptr++ )
		xedtptr->xedt_ctl = HBA_map[xedtptr->xedt_ctl];

	return(sorted);
}
