/*
 *	@(#) scoEvents.c 11.1 97/10/22 
 *
 *	Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
 *
 *	The information in this file is provided for the exclusive use of the
 *	licensees of The Santa Cruz Operation, Inc.  Such users have the right
 *	to use, modify, and incorporate this code into other products for
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */
/* 
 *	S000, Wed Jul 17 00:42:37 GMT 1996, kylec
 * 		- create from existing code (xf86,usl,sco)
 *		- fix many bugs
 * 	S001, Tue Sep 24 13:11:34 PDT 1996, kylec
 *		- rewrite of key event handling - see scoProcessKbdEvent(),
 * 		  and scoScancodeToKeycode().
 *	S002, Thu Apr  3 15:47:43 PST 1997, kylec@sco.com
 *		- add i18n support
 *	S003, Wed Oct  8 11:27:36 PDT 1997, hiramc@sco.COM
 *		- add XqueCheckMouse (taken out of XqueEnable
 */

#define NEED_EVENTS

#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <stropts.h>

#include <sys/page.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/event.h>
#include <sys/vt.h>
#include <sys/kd.h>
#include <sys/times.h>
#include <sys/xque.h>

#include "sco.h"
#include "scoModKeys.h"
#include "Xproto.h"
#include "keysym.h"
#include "input.h"
#include "inputstr.h"
#include "pckeys.h"
#include "xsrv_msgcat.h"
#include "scoEvents.h"

#include "xsrv_msgcat.h"	/* S002 */

static xqEventQueue 	*XqueQaddr = NULL;
static int      	XqueSema = 0;
static int		XqueSigEnable = 1;
static int      	XqueDeltaX = 0, XqueDeltaY = 0;
static int      	XqueLastButtons = 7;

extern CARD32		lastEventTime;

static void		XqueRequest();


/*
 * ProcessInputEvents --
 *      Retrieve all waiting input events and pass them to DIX in their
 *      correct chronological order. Only reads from the system pointer
 *      and keyboard.
 */

void
ProcessInputEvents()
{
    extern scoSysInfoRec scoSysInfo;

    scoSysInfo.inputPending = FALSE;
    mieqProcessInputEvents();
    miPointerUpdate();
}


/*
 * XqueRequest --
 *      Notice an i/o request from the xqueue.
 */

/* ARGSUSED */
static void
XqueRequest (int signo)
{
    xqEvent     	*XqueEvents;
    int         	XqueHead, dx, dy, id, changes;
    KeyCode     	keycode;
    PtrCtrl     	*ctrl;
    xEvent      	xE;
    unchar      	scanCode;
    Bool        	down;
    DeviceIntPtr 	pPointer;


    pPointer = (DeviceIntPtr)LookupPointerDevice();
    XqueEvents = XqueQaddr->xq_events;
    XqueHead   = XqueQaddr->xq_head;
    
    while (XqueHead != XqueQaddr->xq_tail)
    {
        switch(XqueEvents[XqueHead].xq_type) {
        
          case XQ_MOTION:

            /* Do nothing while waiting for an answer. */
            if(scoStatusRecord & AltSysRequest)
                goto skip_event;

            xE.u.keyButtonPointer.time = NewInputEventTime();
            dx = XqueEvents[XqueHead].xq_x;
            dy = XqueEvents[XqueHead].xq_y;

            if (dx || dy) {
                ctrl = &(pPointer->ptrfeed->ctrl);

                if ((abs(dx) + abs(dy)) >= ctrl->threshold) {
                    dx = (dx * ctrl->num) / ctrl->den;
                    dy = (dy * ctrl->num)/ ctrl->den;
                }

                XqueDeltaX += dx;
                XqueDeltaY += dy;

                if (XqueDeltaX || XqueDeltaY)
                {
                    miPointerDeltaCursor(XqueDeltaX, XqueDeltaY,
                                         lastEventTime);
                    XqueDeltaX = XqueDeltaY = 0;
                }

            }
            /* FALLTHRU */


          case XQ_BUTTON:

            /* Do nothing while waiting for an answer. 		*/
            if(scoStatusRecord & AltSysRequest)
                goto skip_event;

            changes = (XqueEvents[XqueHead].xq_code & 7) ^ XqueLastButtons;

            while (changes)
            {
                id = ffs(changes);
                changes &= ~(1 << (id-1));

                xE.u.keyButtonPointer.time = NewInputEventTime();
                xE.u.u.detail = (4 - id);
                xE.u.u.type   = ((XqueEvents[XqueHead].xq_code & (1l << (id-1)))
                                 ? ButtonRelease
                                 : ButtonPress);

                mieqEnqueue(&xE);
            }

            XqueLastButtons = (XqueEvents[XqueHead].xq_code & 7);
            break;


          case XQ_KEY:

            keycode = scoScancodeToKeycode(XqueEvents[XqueHead].xq_code, &down);
            scoProcessKbdEvent(keycode, down);
            break;

          default:
            ErrorF(MSGSCO(XSCO_4,"Unknown Xque Event: 0x%02x\n"), XqueEvents[XqueHead].xq_type);
            break;
       
        }

      skip_event:            
        if ((++XqueHead) == XqueQaddr->xq_size) XqueHead = 0;
        
    }

    XqueReset();

}


/*
 * XqueReset --
 *	Reset
 */
Bool
XqueReset()
{
    if (XqueSema > 0)
    {
        XqueQaddr->xq_head = XqueQaddr->xq_tail;
        XqueQaddr->xq_sigenable = XqueSigEnable;
    }

    return(Success);
}

/*
 * XqueCheckMouse --				S003
 *	See if the mouse will work
 */
Bool
XqueCheckMouse()
{
        static int mouse = -1;

        if (mouse < 0)
        {
            mouse = open("/dev/mouse", O_RDONLY|O_NDELAY);
            if (mouse < 0) {
                Error (MSGSCO(XSCO_5,"Cannot open /dev/mouse\n"));
                return (!Success);
            }
        }
	return(Success);
}

/*
 * XqueEnable --
 *      Enable the handling of the Xque
 */

Bool
XqueEnable()
{
    if (XqueSema == 0)
    {
        extern scoSysInfoRec scoSysInfo;
        static struct kd_quemode xqueMode;

	if ( XqueCheckMouse() )		/*	S003	*/
		return( !Success );

        xqueMode.qsize = 64;    /* max events */
        xqueMode.signo = SIGUSR1;

        ioctl(scoSysInfo.consoleFd, KDQUEMODE, NULL);
        if (ioctl(scoSysInfo.consoleFd, KDQUEMODE, &xqueMode) < 0)
        {
            ErrorF(MSGSCO(XSCO_6,"Cannot set KDQUEMODE\n"));
            return (!Success);
        }

        XqueQaddr = (xqEventQueue *)xqueMode.qaddr;
        XqueQaddr->xq_sigenable = 0; /* LOCK */
        (void) OsSignal(SIGUSR1, XqueRequest);
        XqueSema++;
        nap(500);               /* BUG? -  */
    }

    XqueReset();
    return(Success);
}


/*
 * XqueDisable --
 *      disable the handling of the Xque
 */

int
XqueDisable()
{
    if (XqueSema > 0)
    {
        XqueQaddr->xq_sigenable = 0; /* LOCK */
        XqueSema--;
        if (XqueSema == 0)
        {
            if (ioctl(scoSysInfo.consoleFd, KDQUEMODE, NULL) < 0)
                FatalError(MSGSCO(XSCO_6,"Cannot set KDQUEMODE\n"));
        }
        else
            XqueReset();
    }

    return(Success);
}


int
XqueLock()
{
    XqueSigEnable = 0;
    if (XqueSema > 0)
    {
        XqueQaddr->xq_sigenable = XqueSigEnable;
    }
}


int
XqueUnlock()
{
    XqueSigEnable = 1;
    if (XqueSema > 0)
    {
        XqueQaddr->xq_sigenable = XqueSigEnable;
    }
}
