/*
 * 	@(#) scoMouse.c 11.1 97/10/22 
 *
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * scoMouse.c
 *	Functions for playing cat and mouse... sorry.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */

/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	??? Sept ??????????????? 1990	mikep@sco.com
 *	- Created File from ddx/sun.
 *
 *	S001	Wed Jan 23 13:36:52 PST 1991	mikep@sco.com
 *	-  Closed event queue upon server reset to be sure
 *	it worked on the next open.
 *
 *	S002	Wed Jan 23 13:45:23 PST 1991	mikep@sco.com
 *	-  Added NFB_CURSOR #ifdefs.  A more general solution is
 *      needed here.
 *
 *	S003	Wed Jan 23 19:35:48 PST 1991	mikep@sco.com
 *	- Added scoGenerateMouseEvent for nfbCursor.  This may not be
 *	necessary in the future.
 *
 *	S004	Tue Apr 02 22:24:21 PST 1991	mikep@sco.com
 *	- Changed mwStatusRecord to scoStatusRecord
 *
 *	S005	Sun Apr 07 22:38:37 PDT 1991	mikep@sco.com
 *	- Added check of scoStatusRecord to see if we are in the
 *	middle of handling AltSysRequest
 *
 *	S006	Mon Apr 08 12:41:38 PDT 1991	mikep@sco.com
 *	- Removed WaitForAnswer routine.
 *
 *	S007	Wed Apr 10 20:23:51 PDT 1991	buckm@sco.com
 *	- Don't close mouse (event) fd on DEVICE_CLOSEs.
 *	- Flush mouse (events) on DEVICE_INITs.
 *
 *	S008	Thu Apr 11 00:15:04 PDT 1991	mikep@sco.com
 *	- Put mouse acceleration back in.
 *
 *	S009	Thu Apr 18 00:03:37 PDT 1991	mikep@sco.com
 *	- Removed setting of lastEventTime in this function.  Put 
 *	  in scoIo.c to avoid duplication with scoKbd.c
 *
 *	S010	Sat Jun 22 20:05:03 PDT 1991	mikep@sco.com
 *	- Cleaned up code considerably.  Removed all devInfo garbage.
 *	Removed redundant call to processInputProc().   Removed dead
 *	code from scoMouseProcessEvent().  Should result in a slight 
 *	mouse performance improvement.  
 *	Also removed scoGenerateEvent() and scoMouseDoneEvents()
 *
 *	S011	Sat Jul 13 13:26:19 PDT 1991	mikep@sco.com
 *	- Modified mi event interface to match R5 mi cursor.
 *
 *	S012	Thu Jul 25 23:07:02 PDT 1991	mikep@sco.com
 *	- Added support for absolute pointer devices.
 *
 *	S013	Thu Aug 08 16:23:01 PDT 1991	mikep@sco.com
 *	- Added XTEST Extension
 *
 *	S014	Mon Oct 21 00:05:07 PDT 1991	mikep@sco.com
 *	- Allow scoOpenMouse() to be called twice.
 *
 *	S015	Mon Oct 21 00:58:47 PDT 1991	mikep@sco.com
 *	- Call SetInputCheck() after scoOpenMouse() where it will do some
 *	good.  It was useless in InitOutput() since qp wasn't initialized
 *	yet.
 *
 *	S016	Tue Sep 29 22:20:33 PDT 1992	mikep@sco.com
 *	- Call mieqEnqueue() now instead of passing events to DIX.  The mi
 *	  event queue nicely handles all the multihead issues as well as
 *	  motion buffer history.
 *      - Change the name of scoMouseProcessEvent to scoMouseEnqueueEvent
 *	  since that's what it does now.
 *	- Remove dead code left over from sunMouse.c
 *	- Remove the scoQueue initialization.
 *
 *	S017	Mon Nov 30 17:31:13 PST 1992	buckm@sco.com
 *	- Clean up scoOpenMouse() and add user-helpful error message.
 *
 *      S018    Tue Jul 20 10:02:11 PDT 1993    davidw@sco.com
 *      -  Added XPG4 Message Catalogue support.
 */

#define NEED_EVENTS

#if defined(usl)
#include <sys/time.h>
#include <uwareIo.h>
#endif
#include "sco.h"
#include "mipointer.h"
#include "misprite.h"
#include "xsrv_msgcat.h"        /* S018 */

#if !defined(usl)
/* Our event queue */
extern QUEUE *qp;
#endif

#ifdef XTESTEXT1                /* S013 */
/*
 * defined in xtest1dd.c
 */
extern int      on_steal_input;
extern Bool     XTestStealKeyData();
#endif /* XTESTEXT1 */

static void 	  	scoMouseCtrl();
static devEvent 	*scoMouseGetEvents();
static void 	  	scoMouseEnqueueEvent();			/* S016 */

#if !defined(usl)
static PtrPrivRec 	sysMousePriv = {
    scoMouseEnqueueEvent,	/* Function to process an event (to mi) S016 */
    -1,				/* Descriptor to device */
};
#endif

/*-
 *-----------------------------------------------------------------------
 * scoMouseProc --
 *	Handle the initialization, etc. of a mouse
 *
 * Results:
 *	none.
 *
 * Side Effects:
 *
 *
 *-----------------------------------------------------------------------
 */
int
scoMouseProc (pMouse, what)
    DevicePtr	  pMouse;   	/* Mouse to play with */
    int	    	  what;	    	/* What to do with it */
{
    BYTE    	  map[4];
#if defined(usl)
    extern Bool XqueEnable(void);
#endif /*	usl	*/

    switch (what) {
      case DEVICE_INIT:
        if (pMouse != LookupPointerDevice()) {
            ErrorF ("Cannot open non-system mouse");	
            return (!Success);
        }

#if !defined(usl)
        if (sysMousePriv.fd < 0) 
        {
            sysMousePriv.fd = scoOpenMouse();
        }

        ev_flush();             /* S007 */

        /* Can't use qp until the event driver is working 
         * But it must be done every reset since dix/main.c 
         * calls this as well!
         */
        SetInputCheck((long *)&qp->head, (long *)&qp->tail); /* S015 */
        pMouse->devicePrivate = (pointer) &sysMousePriv;
#endif /*	! usl	*/

        pMouse->on = FALSE;

        map[1] = 1;
        map[2] = 2;
        map[3] = 3;
        InitPointerDeviceStruct (pMouse, map, 3,
                                 miPointerGetMotionEvents,
                                 scoMouseCtrl, 
                                 miPointerGetMotionBufferSize()); /* S011 */
        break;


      case DEVICE_ON:
        pMouse->on = TRUE;
#if !defined(usl)
        AddEnabledDevice (sysMousePriv.fd);
#else
        return (XqueEnable());
#endif
        break;

      case DEVICE_CLOSE:
        /* S007 - no more closes here */
        /* no break */

      case DEVICE_OFF:
        pMouse->on = FALSE;
#if !defined(usl)
        RemoveEnabledDevice (sysMousePriv.fd);
#else
        return (XqueDisable());
#endif /* usl */
        break;
    }
    return (Success);
}
	    
/*-
 *-----------------------------------------------------------------------
 * scoMouseCtrl --
 *	Alter the control parameters for the mouse. Since acceleration
 *	etc. is done from the PtrCtrl record in the mouse's device record,
 *	there's nothing to do here.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
scoMouseCtrl (pMouse)
    DevicePtr	  pMouse;
{
}

/*-
 *-----------------------------------------------------------------------
 * MouseAccelerate --
 *	Given a delta and a mouse, return the acceleration of the delta.
 *
 * Results:
 *	The corrected delta
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static short
MouseAccelerate (pMouse, delta)
    DevicePtr	  pMouse;
    int	    	  delta;
{
    register int  sgn = sign(delta);
    register PtrCtrl *pCtrl;

    delta = abs(delta);
    pCtrl = &((DeviceIntPtr) pMouse)->ptrfeed->ctrl;

    if (delta > pCtrl->threshold) {
	return ((short) (sgn * (pCtrl->threshold +
				((delta - pCtrl->threshold) * pCtrl->num) /
				pCtrl->den)));
    } else {
	return ((short) (sgn * delta));
    }
}

/*-
 *-----------------------------------------------------------------------
 * scoMouseEnqueueEvent --
 *	Given a devEvent for a mouse, pass it off to the mi event queue
 *	properly converted...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be redrawn...? devPrivate/x/y will be altered.
 *
 *-----------------------------------------------------------------------
 */
static void
scoMouseEnqueueEvent (pMouse, pE)				/* S016 */
    DevicePtr	  pMouse;   	/* Mouse from which the event came */
    register    devEvent * pE;
{
    xEvent		xE;
    register dx, dy;

    /* Don't do nothing while waiting for an answer. 		S005 */
    if(scoStatusRecord & AltSysRequest)
	return;

    /*
     *  Event time stamp.
     */
    xE.u.keyButtonPointer.time = pE->deve_time;			/* S009 */

#ifdef DEBUG
ErrorF("got %d %d (%d,%d)\n",pE->deve_key, pE->deve_direction, pE->deve_x, pE->deve_y);
#endif

    switch (pE->deve_type)
	{
	case DEVE_BUTTON:
	    if (pE->deve_direction == DEVE_KBTDOWN)
		xE.u.u.type = ButtonPress;
	    else
		xE.u.u.type = ButtonRelease;
	    /* mouse buttons numbered from one */
	    xE.u.u.detail = pE->deve_key + 1;

	    miPointerPosition ( (int *)&xE.u.keyButtonPointer.rootX,
			       (int *)&xE.u.keyButtonPointer.rootY);/* S011 */

#ifdef XTESTEXT1						/* S013 */
	    /* 
	     * Don't call mieqEnqueue() if we're stealing all the events
	     */
	    if (!on_steal_input || 
		XTestStealKeyData(xE.u.u.detail, xE.u.u.type, DEVE_MOUSE, 
		    xE.u.keyButtonPointer.rootX, xE.u.keyButtonPointer.rootY))
#endif /* XTESTEXT1 */
	    mieqEnqueue(&xE);					/* S016 */
	    break;

	case DEVE_MABSOLUTE:					/* S012 vvv */
#ifdef XTESTEXT1						/* S013 */
	    if (on_steal_input)
		XTestStealMotionData(0, 0, DEVE_MOUSE, pE->deve_x, pE->deve_y);
#endif /* XTESTEXT1 */

	    miPointerAbsoluteCursor(pE->deve_x, pE->deve_y, pE->deve_time);
	    break;

	case DEVE_MDELTA:					/* S012 ^^^ */
	    dx = MouseAccelerate (pMouse, pE->deve_x);	/* S008 */
	    dy = MouseAccelerate (pMouse, pE->deve_y);	/* S008 */

#ifdef XTESTEXT1						/* S013 */
	    if (on_steal_input)
		{
		int x, y;
		miPointerPosition(&x, &y);
		XTestStealMotionData(dx, dy, DEVE_MOUSE, x, y);
		}
#endif /* XTESTEXT1 */
	    miPointerDeltaCursor (dx, dy, pE->deve_time);	/* S011 */
	    
	    break;

	default:
	    FatalError ("scoMouseEnqueueEvent: unrecognized id\n");
	    break;
	}

}

int
scoOpenMouse()
{
    int fdMouse;
#if !defined(usl)
    dmask_t dmask = D_STRING | D_REL;
    char *str = 0;
#endif /*	! usl	*/

#if defined(usl)
    if ( (fdMouse = open("/dev/mouse", O_RDONLY | O_NONBLOCK)) < 0)
        ErrorF("Cannot open /dev/mouse\n");
#else
    /* Open event driver */
    if (ev_init() < 0)
        str = "Init";
    else if ((fdMouse = ev_open(&dmask)) < 0)
        str = "Open";

    if (str) {
        ErrorF(MSGSYS(XSRV_EVFAIL, 
                      "%s event driver failed.\n"), str); /* S008 */
        FatalError(MSGSYS(XSRV_CHKMO, 
                          "Check mouse configuration.\n")); /* S008 */
    }
#endif /* usl */

    scoStatusRecord |= EventWasOpened; /* S004 */

    return fdMouse;
}

