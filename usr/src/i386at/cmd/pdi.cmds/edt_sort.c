#ident	"@(#)pdi.cmds:edt_sort.c	1.12"

/*
 * edt_sort
 *
 * edt_sort is a function that sorts a passed in EDT in bootable order.
 *
 * the xedt_ctl and xedt_ordinal fields in the EDT passed to edt_sort
 * are modified to reflect the correct controller numbering and the
 * ordinal position of all devices according to the controller numbering.
 *
 * The actual sort is accomplished by setting the order field in each
 * entry in the HBA array.
 */

#include	<sys/scsi.h>
#include	"edt_sort.h"

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

static int
EDT_compare(edt1,edt2)
EDT *edt1, *edt2;
{
	/*
	 * first test ctlorder
	 */
	if (edt1->xedt_ctlorder || edt2->xedt_ctlorder) {
		if (!edt1->xedt_ctlorder && edt2->xedt_ctlorder)
			return(TRUE);
		if (edt1->xedt_ctlorder && !edt2->xedt_ctlorder)
			return(FALSE);
		if (edt1->xedt_ctlorder > edt2->xedt_ctlorder)
			return(TRUE);
		if (edt1->xedt_ctlorder < edt2->xedt_ctlorder)
			return(FALSE);
	}
	/*
	 * we fall thru here if the ctlorder's are equal or both are zero
	 */

	/*
	 * now test memaddr.  memaddr is only valid if the controller
	 * has a bootable device hanging on it.  This is indicated by
	 * the value of the ordinal field which at this point is 1 if
	 * the device is bootable and 0 otherwise.
	 */
	if ((edt1->xedt_ordinal && edt1->xedt_memaddr) ||
		(edt2->xedt_ordinal && edt2->xedt_memaddr)) {
		if (!edt1->xedt_memaddr && (edt2->xedt_ordinal && edt2->xedt_memaddr))
			return(TRUE);
		if ((edt1->xedt_ordinal && edt1->xedt_memaddr) && !edt2->xedt_memaddr)
			return(FALSE);
		if (edt1->xedt_ordinal && edt2->xedt_ordinal) {
			if (edt1->xedt_memaddr > edt2->xedt_memaddr)
				return(TRUE);
			if (edt1->xedt_memaddr < edt2->xedt_memaddr)
				return(FALSE);
		}
	}
	/*
	 * we fall thru here if the memaddr's are equal or both are zero
	 */

	if (edt1->xedt_ctl > edt2->xedt_ctl)
		return(TRUE);

	return(FALSE);
}

static void
adapter_sort(struct HBA *HBA, int scsicnt, int (*compare)(), int start_sort)
{
	int		cntl, temp, sorted;

	/*
	 * This is simply a dumb bubble-sort.  Since the HBA array
	 * is relatively small, this will suffice.  At the present time,
	 * small means 3 or 4 HBAs.   Notice that if there is not enough
	 * data to sort
	 */
	sorted = FALSE;

	while ( !sorted ) {
		sorted = TRUE;
		for ( cntl = start_sort; cntl < scsicnt-1; cntl++) {
			if ( (*compare)(HBA[HBA[cntl].order].cntl, HBA[HBA[cntl+1].order].cntl) ) {
				sorted = FALSE;
				temp = HBA[cntl].order;
				HBA[cntl].order = HBA[cntl+1].order;
				HBA[cntl+1].order = temp;
			}
		}
	}
}

/*
 *	The algorithm for determining bootable order in sort_edt below
 *	is used in pdimkdev ( mkdev.c ), bmkdev ( boot_mkdev.c )
 *	and pdiconfig ( config.c ).
 */
int
edt_sort(EDT *xedtptr, int edtcnt, struct HBA *HBA, int start_sort, char do_sort)
{
	EDT *xedtptr2;
	int	largest, index, inner, bootable, cntl, target, can_boot, scsicnt, ntargets;

	/* 
	 * Setup the HBA array and ntargets so we can go through
	 * the EDT in a direct-access manner.  Do this for all devices
	 * in the EDT whether they need to be sorted or not.
	 */
	DTRACE;
	xedtptr2 = xedtptr;
	largest = -1;
	for ( scsicnt = target = 0; target < edtcnt; target++, xedtptr2++ ) {
		if (HBA[scsicnt].edtptr == NULL)
			HBA[scsicnt].edtptr = xedtptr2;
		if (HBA[scsicnt].cntl == NULL && xedtptr2->xedt_target == xedtptr2->xedt_ha_id)
			HBA[scsicnt].cntl = xedtptr2;   /* point at the HBA entry */
		HBA[scsicnt].ntargets++;
		/*
		 * If next device is on next controller, bump scsicnt
		 */
		if (((target+1) >= edtcnt) || ((xedtptr2+1)->xedt_ctl != xedtptr2->xedt_ctl)) {
			HBA[scsicnt].index = xedtptr2->xedt_ctl;
			if ( xedtptr2->xedt_ctl > largest )
				largest = xedtptr2->xedt_ctl;
			scsicnt++;
		}
	}

	for ( cntl = index = 0; index <= largest; index++ ) {
		for ( inner = 0; inner < scsicnt; inner++) {
			if ( HBA[inner].index == index ) {
				HBA[inner].order = cntl++;
				break;
			}
		}
	}

	if ( do_sort ) {
	/* 
	 * Walk through the EDT and determine the order of bootable
	 * and non-bootable HBA's, based simply on whether or not
	 * any targets attached to the HBA are bootable devices.
	 */
	can_boot = FALSE;
	DTRACE;
	for ( bootable = cntl = start_sort; cntl < scsicnt; cntl++) {

		xedtptr2 = HBA[HBA[cntl].order].edtptr;
		ntargets = HBA[HBA[cntl].order].ntargets;

		for (target = 0; target < ntargets; target++, xedtptr2++) {
			if (BOOTABLE(xedtptr2))
				can_boot = TRUE;
		}
		xedtptr2 = HBA[HBA[cntl].order].cntl;
		if (can_boot) {
			xedtptr2->xedt_ordinal = TRUE;
			HBA[HBA[cntl].order].index = bootable++;
			can_boot=FALSE;
		} else {
			xedtptr2->xedt_ordinal = FALSE;
			HBA[HBA[cntl].order].index = -1;
		}
	}

	DTRACE;
	for (cntl = start_sort; cntl < scsicnt; cntl++) {
		if (HBA[HBA[cntl].order].index < 0) {
			HBA[HBA[cntl].order].index = bootable++;
		}
	}

	DTRACE;
	/*
	 * At this point, the HBA array contains valid values for
	 * *edtptr and ntargets.  index gives a numbering of the
	 * HBAs which represents the original ( UW1.0 ) ordering.
	 * This is used to break ties in the sort that follows.
	 *
	 * Now, lets sort the bloody thing.
	 */
	for ( cntl = start_sort; cntl < scsicnt; cntl++ ) {
		xedtptr2 = HBA[HBA[cntl].order].edtptr;
		ntargets = HBA[HBA[cntl].order].ntargets;

		for (target = 0; target < ntargets; target++, xedtptr2++) {
			xedtptr2->xedt_ctl = (ushort_t)HBA[HBA[cntl].order].index;
		}
	}
	DTRACE;

	adapter_sort(HBA, scsicnt, EDT_compare, start_sort);
	} /* do_sort */

	for ( cntl = 0; cntl < scsicnt; cntl++ ) {
		xedtptr2 = HBA[HBA[cntl].order].edtptr;
		ntargets = HBA[HBA[cntl].order].ntargets;

		for (target = 0; target < ntargets; target++, xedtptr2++) {
			if ( cntl >= start_sort )
				xedtptr2->xedt_ctl = (ushort_t)cntl;
			xedtptr2->xedt_ordinal = ORDINAL(xedtptr2);
		}
	}

	return (scsicnt);
}
