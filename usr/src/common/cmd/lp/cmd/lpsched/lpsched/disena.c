/*		copyright	"%c%" 	*/


#ident	"@(#)disena.c	1.2"
#ident	"$Header$"

#include "lpsched.h"

extern long		time();

/**
 ** enable() - ENABLE PRINTER
 **/

#ifdef	__STDC__
int
enable (PSTATUS *pps, MESG *md)
#else
int
enable (pps, md)

PSTATUS *	pps;
MESG *		md;
#endif
{
	DEFINE_FNNAME (enable)

	int	newRange;

	ENTRYP
	/*
	 * ``Enabling a printer'' includes clearing a fault and
	 * clearing the do-it-later flag to allow the printer
	 * to start up again.
	 */
	if (!(pps->status & (PS_FAULTED|PS_DISABLED|PS_LATER)))
	{
		errno = EINVAL;
		return	-1;
	}
	/*
	**  ES note:
	**  If 'md' then a user has effected the call of this routine.
	**  It is possible that new MAC level range on the device
	**  may allow the user to be able to enable the printer.
	**  But, we do not check the device before the user because
	**  a new level range will cause a lot of work to be done
	**  even if the user is not able to enable the device.
	**  This could be a very severe covert channel and so we
	**  do the user check first.
	*/
	if (md)
	{
		if (! ValidatePrinterUser (pps->printer, md) &&
	    	    ! ValidateAdminUser (md))
		{
			/*
			**  ES note:
			**  For 'enable/disable' only an admin user
			**  is OK regardless of whether the admin can
			**  print on that printer or not.
			**  To the user the printer does not exist so
			**  we use ENOENT versus EACCESS.
			*/
			errno = ENOENT;
			return	-1;
		}
	}
	/*
	**  Check to see if the device can be accessed and opened.
	**  It also get any new MAC-level range that may be in effect.
	**
	**  If the range on the print device has changed then
	**  CheckPrinter() returns 2 for success, 1 otherwise,
	**  and 0 for failure (i.e. device is off line).
	**
	**  If there is a new range associated with the device then
	**  all requests destined for that printer must be re-evaluated.
	**  Requests that can no longer print on that device are canceled
	**  and the user notified.
	*/
	if (! (newRange = CheckPrinter (pps)))
	{
		errno = EBUSY;
		return	-1;
	}
	newRange = newRange == 2 ? 1 : 0;

	pps->status &= ~(PS_FAULTED|PS_DISABLED|PS_LATER);
	(void) time (&pps->dis_date);

	dump_pstatus ();

	if (pps->alert->active)
		cancel_alert (A_PRINTER, pps);

	/*
	**  Cancel all requests that were destined for this printer
	**  but will no longer print because of the change of
	**  MAC-level range.
	**
	**  Attract the FIRST request that is waiting to
	**  print to this printer. In this regard we're acting
	**  like the printer just finished printing a request
	**  and is looking for another.
	*/
	if (newRange)
	{
/*
**		RSTATUS *	rp;
**		RSTATUS *	crp;
**
**		rp = Request_List;
**		while (rp)
**		{
**			if (! ValidatePrinter (pps->printer, rp->secure))
**			{
**				crp = rp;
**				rp = rp->next;
**				cancel (crp, 1);
**				continue;
**			}
**			rp = rp->next;
**		}
*/
		queue_repel (pps, 0, 0);
	}
	queue_attract (pps, qchk_waiting, 1);
	return	0;
}

/**
 ** disable() - DISABLE PRINTER
 **/

int
#ifdef	__STDC__
disable (PSTATUS *pps, char *reason, int when)
#else
disable (pps, reason, when)

register
PSTATUS *	pps;
char *		reason;
int		when;
#endif
{
	DEFINE_FNNAME (disable)

	if (pps->status & PS_DISABLED)
		return	-1;

	else {
		pps->status |= PS_DISABLED;
		(void) time (&pps->dis_date);
		load_str (&pps->dis_reason, reason);

		dump_pstatus ();

		if (pps->status & PS_BUSY)
			switch (when) {

			case DISABLE_STOP:
				/*
				 * Stop current job, requeue.
				 */
				if (pps->request)
				    pps->request->request->outcome |=
					RS_STOPPED;
				terminate (pps->exec);
				break;

			case DISABLE_FINISH:
				/*
				 * Let current job finish.
				 */
				break;

			case DISABLE_CANCEL:
				/*
				 * Cancel current job outright.
				 */
				if (pps->request)
				    (void) cancel (pps->request, 1);
				break;

			}

		/*
		 * Need we check to see if requests assigned to
		 * this printer should be assigned elsewhere?
		 * No, if the "validate()" routine is properly
		 * assigning requests. If another printer is available
		 * for printing requests (that would otherwise be)
		 * assigned to this printer, at least one of those
		 * requests will be assigned to that other printer,
		 * and should be currently printing. Once it is done
		 * printing, the queue will be examined for the next
		 * request, and the one(s) assigned this printer will
		 * be picked up.
		 */
/*		(void)queue_repel (pps, 0, (qchk_fnc_type)0);	*/

		return (0);
	}
}

