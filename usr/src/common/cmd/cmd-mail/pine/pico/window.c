#if	!defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*
 * Program:	Window management routines
 *
 *
 * Michael Seibel
 * Networks and Distributed Computing
 * Computing and Communications
 * University of Washington
 * Administration Builiding, AG-44
 * Seattle, Washington, 98195, USA
 * Internet: mikes@cac.washington.edu
 *
 * Please address all bugs and comments to "pine-bugs@cac.washington.edu"
 *
 *
 * Pine and Pico are registered trademarks of the University of Washington.
 * No commercial use of these trademarks may be made without prior written
 * permission of the University of Washington.
 * 
 * Pine, Pico, and Pilot software and its included text are Copyright
 * 1989-1996 by the University of Washington.
 * 
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this distribution.
 *
 */
/*
 * Window management. Some of the functions are internal, and some are
 * attached to keys that the user actually types.
 */

#include        <stdio.h>
#include	"osdep.h"
#include        "pico.h"
#include        "estruct.h"
#include	"edef.h"


/*
 * Refresh the screen. With no argument, it just does the refresh. With an
 * argument it recenters "." in the current window. Bound to "C-L".
 */
refresh(f, n)
  int f, n;
{
    /*
     * since pine mode isn't using the traditional mode line, sgarbf isn't
     * enough.
     */
    if(Pmaster)
        curwp->w_flag |= WFMODE;

    if (f == FALSE)
        sgarbf = TRUE;
    else{
        curwp->w_force = 0;             /* Center dot. */
        curwp->w_flag |= WFFORCE;
    }

    return (TRUE);
}
