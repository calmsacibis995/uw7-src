/*
 *
 *	@(#) scoIo.c 11.5 97/10/24 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * scoIo.c
 *	Functions to handle input from the keyboard and mouse.
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
 *	S001	Tue Nov 20 18:37:56 PST 1990	mikep@sco.com
 *	- Changed EV_TIME() to GetTimeInMillies() cause X Server time
 *	is not the same as SCO Event time.
 *
 *	S002	Tue Dec 04 19:33:48 PST 1990	buckm@sco.com
 *	- Added calls to keyboard cleanup routine.
 *
 *	S003	Thu Dec 06 01:44:19 PST 1990	buckm@sco.com
 *	- Look for "-mode" arg instead of "-mono".
 *
 *	S004	Wed Jan 23 17:17:01 PST 1991	mikep@sco.com
 *	- Added NFB ifdefs.
 *
 *	S005	Thu Jan 31 14:44:03 PST 1991	mikep@sco.com
 *	- Added special case for e0 and e1 scancode translation.
 *
 *	S006	Thu Jan 31 19:33:34 PST 1991	buckm@sco.com
 *	- Switch to using GrafInfo instead of VidConf.
 *
 *	S007	Wed Feb 27 21:26:53 PST 1991	buckm@sco.com
 *	- Add '-crt' command-line arg.
 *	- Change '-mode' command-line arg to '-d' to match Xsight.
 *
 *	S008	Sat Mar 16 16:38:51 PST 1991	mikep@sco.com
 *	- Pass ScreenInfo struct to acquire and release screen.  This
 *	may be overkill.  It needs to be redesigned.
 *
 *	S009	Tue Apr 02 21:10:50 PST 1991	mikep@sco.com
 *	- Redesigned screen switching.  Acquire and Release screen
 *	now know who to save.
 *	- Broke down and made scoStatusRecord a global.
 *	- Removed mw.h
 *
 *	S010	Mon Apr 08 12:39:45 PDT 1991	mikep@sco.com
 *	- Removed WaitForAnswer garbage.
 *
 *	S011	Tue Apr 09 23:57:14 PDT 1991	buckm@sco.com
 *	- Remove NFB ifdef in ddxProcessArgument().
 *
 *	S012	Wed Apr 10 14:47:38 PDT 1991	staceyc@sco.com
 *	- command line arguments backward compatable
 *
 *	S013	Thu Apr 18 00:06:24 PDT 1991	mikep@sco.com
 *	- Added setting of lastEventTime to keep the screenSaver
 *	  honest.
 *
 *	S014	Thu Apr 18 00:06:24 PDT 1991	mikep@sco.com
 *	- Call scoLeaveVtProcMode() in ddxGiveUp().  This makes life
 *	  easier in the debuggers.
 *
 *	S015	Fri May 03 04:02:21 PDT 1991	mikep@sco.com
 *	- Undo S001.  They are supposed to fix the event driver
 *	for ODT 1.2.
 *	- Remove all the screen switch code.   It's now in WaitFor.c.
 *	- Implemented -nice option
 *
 *	S016	Mon May 06 16:38:46 PDT 1991	hiramc@sco.COM
 *	- Took out the -d option processing here, sent it down to
 *	- ddx/sgi/mw/mwInit.c
 *
 *	S017	Tue May 07 06:23:41 PDT 1991	buckm@sco.COM
 *	- Ifdef S016 on NFB.
 *
 *	S018	Wed May 08 18:52:27 PDT 1991	buckm@sco.COM
 *	- Add evsync flag to force event times to synchronize
 *	with system times.  This means using GetTimeInMillis
 *	instead of the event driver timestamp.
 *
 *	S019, 31-MAY-91, hiramc@sco.COM
 *		This stuff with S016 and S017, we must return a 2
 *		on the -d option, but I can't see why it is a conditional
 *		compile.  I took out the ifndef NFB so that this stuff
 *		would be compiled.  S017 actually made it disappear.
 *
 *	S020	Tue Jun 11 22:22:37 PDT 1991	mikep@sco.com
 *	- Remove ddxUseMsg ddxProcessArgs.  These are now in nfb/common.
 *
 *	S021	Wed Jun 12 02:57:46 PDT 1991	buckm@sco.com
 *	- Rename above to scoProcessArgument and scoUseMsg,
 *	  called by ProcessCommandLine and UseMsg.
 *	- Move "-s" arg processing from ProcessCommandLine to here.
 *
 *      S022    Fri Jun 14 02:35:33 PDT 1991	buckm@sco.com
 *      - Move scoVidModeStr here from scoInit.c.
 *
 *	S023	Sat Jun 22 20:43:31 PDT 1991	mikep@sco.com
 *	- Removed devInfo garbage.
 *
 *	S024	Sat Jul 13 13:31:24 PDT 1991	mikep@sco.com
 *	- Added R5 Mi Cursor call.
 *
 *	S025	Thu Jul 25 23:03:02 PDT 1991	mikep@sco.com
 *	- Added support for absolute pointer device.  For now they are
 *	just another mouse.  The reported Y coordinate may be incorrect.
 *	Where is (0,0) on a tablet?
 *
 *      S026    Tue Oct 01 02:27:42 PDT 1991	buckm@sco.com
 *      - Add call to scoSetText() in AbortDDX.
 *	- Fix ddxGiveUp() comment.
 *
 *      S027    Thu Oct 03 23:03:53 PDT 1991	pavelr@sco.com
 *	- add -noanalog option
 *
 *      S028    Wed Oct 16 22:50:20 PDT 1991	buckm@sco.com
 *	- add -static option and useStaticVisualClass variable.
 *
 *	S029	Mon Oct 21 00:06:05 PDT 1991	mikep@sco.com
 *	- initialize scoQueue.
 *
 *	S030	Wed Nov 13 12:57:59 PST 1991	mikep@sco.com
 *	- cleaned up usage message
 *
 *	S031	Wed Sep 02 22:43:35 PDT 1992	mikep@sco.com
 *	- allow MPX locking to be turned off.
 *
 *	S032	Tue Sep 29 22:04:04 PDT 1992	mikep@sco.com
 *	- Greatly simplify the event handling code by doing away with the
 *	  scoQueue garbage.  Now just process each mouse event as we pull
 *	  it off the event queue.  
 *	- Make the event library calls into macros for speed.
 *	- Add code to use the mi event queue for multihead and motion buffer
 *	  history.
 *	- There's no need to check ScreenBlanking in ProcessInput()
 *
 *	S033	Thu Oct 01 22:06:39 PDT 1992	mikep@sco.com
 *	- mieq expects ProcessInput() to be called whenever something
 *	  is stuck in it's event queue.  This will always happen, except
 *	  in the case of a WarpPointer request.  Hence scoWarpPointer 
 *	  will change the InputCheck pointers.   Change them back when 
 *	  necessary.
 *	- If there's more event data to be processed, do it before we leave.
 *
 *	S034	Fri Oct 09 12:55:12 PDT 1992	mikep@sco.com
 *	- Let's start using -s for screen saver, to be compatible with
 *	  the O'Reilly books.  -save is compatible with Xsight.
 *
 *	S035	Sun Oct 11 15:42:43 PDT 1992	mikep@sco.com
 *	- Add enforceProtocol (-ppp) option.  This should tell the
 *	  pt-to-pt line drawers to do it right.
 *
 *	S036	Mon Oct 26 13:54:16 PST 1992	mikep@sco.com
 *	- Rearrange the check's of buttons verses movement events
 *	  to insure that the button press is processed first if
 *	  received in the same event as the movement.  Could this fix
 *	  the infamous title bar move bug?
 *	- Remove check of deve_type from ProcessEvents.  Either it's
 *	  a keyboard or mouse event for now.
 *	S037	Thu Nov 5 16:00:00 PST 1992	chrissc@sco.com
 *	- Had AbortDDX call ddxGiveUp to remove duplicate code.
 *	S038	Thu Nov 5 16:00:00 PST 1992	chrissc@sco.com
 *	- added ifdef'd PESUDOCONN rmptty() (in os/connection.c) to
 *	the routine ddxGiveUp so that the /dev/X/server.* device is
 *	removed on most forms of exiting.
 *	S039	Thu May 06 15:18:49 PDT 1993	brianm@sco.com
 *	- moved functions SaveTtyMode, RestoreTtyMode and SetTtyRawMode
 *	  to sys/intel/scoIo.c from os/utils.c.  Also removed major portions
 *	  of termio setting as it was already done by the event driver
 *	  API (except for turning off ISIG).
 *
 *	S040	Thu Jun 24 16:36:05 PDT 1993	buckm@sco.com
 *	- Add scoRunSwitched and -norunsw arg.
 *	- Put args back in alpha order.
 *	- Get rid of some unused stuff.
 *
 *      S041    Tue Jul 20 09:49:02 PDT 1993    davidw@sco.com
 *      - Added XPG4 Message Catalogue support.
 *      S042    Tue Sep 21 13:02:32 PDT 1993    edb@sco.com
 *	- Use scancode API
 *	S043	Mon Oct 10 11:10:29 1994	kylec@sco.com
 *	- scancode API has some limitations/problems.  Fix
 *	  this by using our own translation table that
 *	  maps scancodes to mapcodes.  Translation table is
 *	  configurable via xsconfig.  BUG SCO-59-4153.
 *	- scancode api controls state of LEDs even when you
 *	  only interested in receiving mapcodes from the
 *	  scancode api.  At some point the api may be enhanced
 *	  so that control of LEDs by the api can be disabled.
 *	  In the meantime we remap the keyboard so it appears
 *	  that it has *no* locking keys.  The X server can then
 *	  have complete control of the LEDs.  
 *	  BUG SCO-59-4156.
 *	S044	Wed Jan 11 14:54:07 PST 1995	hiramc@sco.COM
 *	- fix -save argument processing - was not correct
 *	- also make -nice, -crt and -d options safe
 *	- -save problem is bug report SCO-59-3345
 *	S045	Fri Jan 26 14:37:20 PST 1996	hiramc@sco.COM
 *	- begin modifications to allow compile on Unixware
 *	- all modifications are marked by define(usl)
 *      S045    Fri Aug  9 23:49:40 GMT 1996    kylec@sco.COM
 *	- add -reset arg to put console back into text mode.
 *	S046	Tue Sep 24 13:31:21 PDT 1996	kylec@sco.com
 *	- New routine scoScancodeToKeycode().  Uses translation table
 *	  to correctly map scancode sequences to keycodes.  Use this
 *	  instead of scancode api from OSR5.
 * 	- New routine scoProcessKbdEvent()
 *	S046	Thu Oct  3 18:11:03 PDT 1996	kylec@sco.com
 *	- force keyboard into XT mode
 *	- misc changes to *TtyMode() functions
 *	S047	Fri Oct 11 12:46:26 PDT 1996	kylec@sco.com
 *	- use pPriv->offset in scoScancodeToKeycode() instead
 *	  of a hard coded value.
 *	S048	Tue Jan 21 15:16:48 PST 1997	kylec@sco.com
 *	- ensure ddxGiveUp() works correctly when screen
 *	  is not active.
 *	S049	Thu Apr  3 16:10:56 PST 1997	kylec@sco.com
 *	- add i18n support
 *	S050	Fri Jun 20 10:43:03 PDT 1997	hiramc@sco.COM
 *	- add -stdvga argument
 *	S051	Thu Oct 23 11:36:03 PDT 1997	hiramc@sco.COM
 *	- no evsync or mpxlock in Gemini
 */

#define NEED_EVENTS

#include    <stdio.h>
#include    "sco.h"
#include    "opaque.h"
#include    "xsrv_msgcat.h"					/* S041 */

#if defined(usl)
#include    <sys/kd.h>
#include    "xsconfig/config.h"
#else
#include    <scancode.h>
#include    <sys/vtkd.h>					/* S043 */
#include    <sys/keyboard.h>					/* S043 */
#endif

#include    <input.h>
#include    <inputstr.h>

#include    "xsrv_msgcat.h"					/* S049 */                      

static keymap_t keymap;						/* S043 */
static int kb_type;


CARD32	    	lastEventTime = 0;
extern long defaultScreenSaverTime;

int	scoStatusRecord;	/* Interesting information */	/* S008 */
static int evsync = FALSE;	/* S018 Force event time synchronization */
char *scoVidModeStr = NULL;					/* S022 */

Bool useStaticVisualClass = FALSE;				/* S028 */
Bool mpxLock = TRUE;		/* Lock process if on MPX S031 */
Bool enforceProtocol = FALSE;					/* S035 */
Bool scoRunSwitched = TRUE;					/* S040 */
Bool scoResetServer = FALSE;					/* S040 */

void scoProcessKbdEvent();
void ProcessEvent();						/* S032 vvv */

#if !defined(usl)
extern QUEUE 	*qp;			/* In event library */

#define ev_newdata() (qp->head != qp->tail)			/* S033 */
#define ev_pop() qp->tail = (qp->tail + 1) % QSIZE
#define ev_read() (qp->head == qp->tail) ?  (EVENT *)NULL : (EVENT *)&qp->queue[qp->tail]
								/* S032 ^^^ */
#endif	/*	! usl	*/

/*-
 *-----------------------------------------------------------------------
 * TimeSinceLastInputEvent --
 *	Function used for screensaver purposes by the os module.
 *
 * Results:
 *	The time in milliseconds since there last was any
 *	input.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
TimeSinceLastInputEvent()
{
    if (lastEventTime == 0)
	lastEventTime = GetTimeInMillis();
    return GetTimeInMillis() - lastEventTime;
}

/*-
 *-----------------------------------------------------------------------
 * SetTimeSinceLastInputEvent --
 *	Set the lastEventTime to now.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	lastEventTime is altered.
 *
 *-----------------------------------------------------------------------
 */
void
SetTimeSinceLastInputEvent()
{
    lastEventTime = GetTimeInMillis();
}


CARD32
NewInputEventTime()
{
    struct timeval now;
    extern CARD32 lastEventTime;

    lastEventTime = GetTimeInMillis();
    return (lastEventTime);
}


CARD32
GetTimeInMillis()
{
    struct timeval now;

    gettimeofday(&now,(struct timezone *)0);
    return(((now).tv_usec/1000)+((now).tv_sec*1000));
}


#if !defined(usl)
/*
   Macros that manipulate event queue index.
*/
#define	changed(old,new,bitmsk)	((old ^ new) & bitmsk)
#define	toggle(xx,bitmsk)	(xx ^ bitmsk)

/*****************
 * ReadInputEvents:
 *    Get input events from event driver to and process them.
 *
 *    There's really no reason why we need to copy the event queue data
 *    into a devEvent and then finally convert it into an xEvent in the
 *    {kbd,mouse}Enqueue routines.   This is just left over from the way
 *    Locus originally did it.  The savings in speed would be small and
 *    changes would be large.  Numerous calls to ProcessEvent in other
 *    modules would have to be rewritten and the logic in this routine,
 *    scoMouseEnqueueEvent, and scoKbdEnqueueEvent would have to be entirely
 *    rethought.  Maybe next time.
 *****************/
ReadInputEvents()
    {
    static int	PreButtonState;
    devEvent dE;
    register EVENT	*evp;
    register  devEvent * pE = &dE;				/* S032 */
        scancode_t scanc;
        mapcode_t  mapc;
        static int prev_time = 0;

    while((evp = ev_read()) != (EVENT *)NULL ) 
	{
	while(EV_TAG(*evp) & (T_STRING | T_BUTTON | T_REL_LOCATOR 
						| T_ABS_LOCATOR)) /* S025 */
	    {
	    /* S018 - allow forcing of event times to match system times */
	    if (evsync)
		lastEventTime = pE->deve_time  = GetTimeInMillis();
	    else
		lastEventTime = pE->deve_time  = EV_TIME(*evp);	/* S015 */

	    if ( EV_TAG(*evp) & T_BUTTON ) 
		{
		pE->deve_type = DEVE_BUTTON;
		pE->deve_device = DEVE_MOUSE;

		if(changed(PreButtonState, EV_BUTTONS(*evp), RT_BUTTON)) 
		    {
		    pE->deve_key = 2;
		    pE->deve_direction = (PreButtonState & RT_BUTTON)?
					    DEVE_KBTUP : DEVE_KBTDOWN;
		    PreButtonState = toggle(PreButtonState, RT_BUTTON);
		    }
		else if(changed(PreButtonState, EV_BUTTONS(*evp), MD_BUTTON)) 
		    {
		    pE->deve_key = 1;
		    pE->deve_direction = (PreButtonState & MD_BUTTON)?
					    DEVE_KBTUP : DEVE_KBTDOWN;
		    PreButtonState = toggle(PreButtonState, MD_BUTTON);
		    }
		else 
		    {
		    pE->deve_key = 0;
		    pE->deve_direction = (PreButtonState & LT_BUTTON)?
					    DEVE_KBTUP : DEVE_KBTDOWN;
		    PreButtonState = toggle(PreButtonState, LT_BUTTON);
		    }

		if(PreButtonState == EV_BUTTONS(*evp)) 
			EV_TAG(*evp) &= ~T_BUTTON;
		}
	    else if ( EV_TAG(*evp) & T_REL_LOCATOR ) 
		{
		pE->deve_x = EV_DX(*evp);
		pE->deve_y = - EV_DY(*evp);

		pE->deve_type = DEVE_MDELTA;			/* S025 */
		pE->deve_device = DEVE_MOUSE;
		EV_TAG(*evp) &= ~T_REL_LOCATOR;
	        } 
	    else if ( EV_TAG(*evp) & T_ABS_LOCATOR ) 		/* S025 vvv */
		{
		pE->deve_x = EV_X(*evp);
		pE->deve_y = EV_Y(*evp);  /* Should this be adjusted? */

		pE->deve_type = DEVE_MABSOLUTE;
		pE->deve_device = DEVE_MOUSE;
		EV_TAG(*evp) &= ~T_ABS_LOCATOR;
		}						/* S025 ^^^ */
	    else if ( EV_TAG(*evp) & T_STRING ) 
                {  						/* S043 !!! */
                mapcode_t Mapcode();
                pE->deve_device = DEVE_DKB;
                EV_TAG(*evp) &= ~T_STRING;

                scanc = EV_BUF(*evp)[0];
                mapc = Mapcode(pE, scanc);
                if (mapc)
                {
                        pE->deve_direction =
                                (mapc & 0x8000) ? DEVE_KBTUP : DEVE_KBTDOWN;
                        pE->deve_key = mapc & 0xFF;
                }
                else
                        break;
                } 						/* S043 ^^^ */

	    ProcessEvent(pE);					/* S032 */
	    }
	ev_pop();
	}
    }


/* S043 !!!! */
/*
 * Mapcode()
 *
 * Return mapcode associated with the current scancode input.
 * first attempt to map the scancode from the xsconfig
 * translation table scoScanTranslate.  If this fails then
 * default to the scancode api mapping.
 */
mapcode_t
Mapcode(devEvent * pE, scancode_t scanc)
{
        extern ScanTranslation *scoScanTranslate;
        extern int scoNumScanTranslate;
        mapcode_t  mapc = 0;
        static scanModifier = 0;
	static int at_release = 0;
        register int n;
        ScanTranslation *xlate;

        if (scoNumScanTranslate > 0)
        {
                /*
                 * X server translations take precedence over the scancode
		 * api translations.
                 */
                if (scanc >= 0xe0)
                {
			if (scanc == 0xF0)
				at_release = 1;
			else
                        	scanModifier = scanc << 8;
                        sc_kb2mapcode(scanc); /* update sc_* state */
                        return 0;
                }


                for (n=scoNumScanTranslate, xlate=scoScanTranslate; 
			n--; xlate++)
                {
                        if (xlate->oldScan == (scanModifier|(scanc & 0x7F)))
                       {
                                sc_kb2mapcode(1); /* clear sc_* state */
                                if (xlate->newScan == 0)
                                {
                                        scanModifier = 0;
					at_release = 0;
                                        return 0;
                                }
                                else if (xlate->newScan < 0)
                                {
                                        /* Keep current scanModifier state */
                                        return 0;
                                }
                                mapc = xlate->newScan |
                                        (((scanc & 0x80) || at_release) 
						? 0x8000 : 0);
                                break;
                        }
                }

                scanModifier = 0;
		at_release = 0;
        }

        if (!mapc)
                mapc = sc_kb2mapcode(scanc);  /* use scancode api */

        return mapc;
}

/* S043 ^^^^ */


/*-
 *-----------------------------------------------------------------------
 * ProcessInputEvents --
 *	Retrieve all waiting input events and pass them to DIX in their
 *	correct chronological order. Only reads from the system pointer
 *	and keyboard.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Events are passed to the DIX layer.
 *
 *-----------------------------------------------------------------------
 */
void
ProcessInputEvents ()
{
    /*								  S033
     * If we came here and there's no new data on the event queue
     * then scoWarpPointer() mucked with InputCheck() to force
     * mieqProcessInputEvents() and miPointerUpdate().  Fix it.
     */
    if (!ev_newdata())
	    SetInputCheck((long *)&qp->head, (long *)&qp->tail);

    do {							/* S033 */
	ReadInputEvents();
	mieqProcessInputEvents();				/* S032 */
	miPointerUpdate();					/* S024 */

	/*
	 * Under heavy load, we can get more mouse events while we were
	 * processing the old ones.  If so, process some more.  Makes for
	 * speedy mouse tracking.
	 */
    } while(ev_newdata());					/* S033 */
}
#endif	/* !usl	*/

void
scoProcessKbdEvent(keycode, down)
    KeyCode keycode;
    Bool down;
{
    DevicePtr               pKeyboard;

    pKeyboard = LookupKeyboardDevice();
    (((KbPrivPtr)pKeyboard->devicePrivate)->EnqueueEvent)(pKeyboard, keycode, down);
}

void
ProcessEvent(pE)
    register    devEvent * pE;
{
    DevicePtr               pPointer;
    DevicePtr               pKeyboard;

    if (pE->deve_device == DEVE_DKB)				/* S036 */
	    {
	    pKeyboard = LookupKeyboardDevice();
	    (((KbPrivPtr)pKeyboard->devicePrivate)->EnqueueEvent)(pKeyboard, pE);
	    }
    else if (pE->deve_device == DEVE_MOUSE)
	    {
	    pPointer = LookupPointerDevice();
	    (((PtrPrivPtr)pPointer->devicePrivate)->EnqueueEvent)(pPointer, pE);
	    }

    /* else do nothing. must be DEVE_NOP */

}

/*
 * DDX - specific abort routine.  Called by AbortServer().
 */

/* Called by main().  S026 !! */

void
ddxGiveUp()
{
#if defined(usl)
    extern scoSysInfoRec scoSysInfo;
    extern char *display;
    struct vt_mode VT;
    char   fname[PATH_MAX];
#endif

#if !defined(usl)
    if (scoStatusRecord & EventWasOpened) /* S008 */
	ev_close();             /* close event driver */
#endif /* usl */

    if (scoScreenActive() == TRUE)
      {
        scoRestoreKbd();            /* S002 */
        scoRestoreVid();
      }

#if !defined(usl)
    scoLeaveVtProcMode();       /* S014 */
#else

    (void) ioctl(scoSysInfo.consoleFd, KDQUEMODE, NULL);
    (void) ioctl(scoSysInfo.consoleFd, MODESWITCH|scoSysInfo.defaultMode, 0);
    (void) ioctl(scoSysInfo.consoleFd, KDSETMODE, KD_TEXT0);

    if (ioctl(scoSysInfo.consoleFd, VT_GETMODE, &VT) != -1)
    {
        VT.mode = VT_AUTO;
        (void) ioctl(scoSysInfo.consoleFd, VT_SETMODE, &VT);
    }

    /*
     * Garbage collection ...
     */
    sprintf(fname, "/dev/X/Nserver.%s",    display); unlink(fname);
    sprintf(fname, "/dev/X/server.%s",     display); unlink(fname);
    sprintf(fname, "/dev/X/server.%s.pid", display); unlink(fname);

#ifdef BOGUS
    close(scoSysInfo.consoleFd);       /* make the vt-manager happy */
#endif

#endif

#ifdef PSEUDOCONN               /* S032 */
    rmptty();                   /* S032 */
#endif /* S038 */

    catclose(xsrv_m_catd);      /* S041 */

}



void
AbortDDX()
{
    scoSetText();						/* S026 */
    ddxGiveUp();						/* S037 */
}


static char VGAMode[]="ibm.vga.vga.640x480-16";			/* S050	*/

int
scoProcessArgument (argc, argv, i)				/* S021 */
    int	argc;
    char *argv[];
    int	i;
{

    if (strcmp(argv[i], "-d") == 0)
    {
	if( (i+1) < argc) {     /* S044 */
            scoVidModeStr = argv[i+1];
            return 2;           /*	S016 S017 S019	*/
	} else {                /* S044 */
            return 0;           /* S044 */
	} /* S044 */
    }

    else if (strcmp(argv[i], "-stdvga") == 0) {		/*	vvv S050 */
		scoVidModeStr = VGAMode;
		return 1;
	}						/*	^^^ S050 */
#if !defined(usl)					/*	S051 */
    else if (strcmp(argv[i], "-evsync") == 0) /* S018 */
    {                           /* S018 */
	evsync = TRUE;          /* S018 */
        return 1;               /* S018 */
    }
#endif							/*	S051 */

    else if (strcmp(argv[i], "-noanalog") == 0) /* S027 vvv */
    {
	return 1;
    } /* S027 ^^^ */

#if !defined(usl)					/*	S051	*/
    else if (strcmp(argv[i], "-nompxlock") == 0) /* S031 vvv */
    {
	mpxLock = FALSE;
	return 1;
    } /* S031 ^^^ */
#endif							/*	S051	*/

    else if (strcmp(argv[i], "-norunsw") == 0) /* S040 vvv */
    {
	scoRunSwitched = FALSE;
	return 1;
    } /* S040 ^^^ */

    else if (strcmp(argv[i], "-ppp") == 0) /* S035 vvv */
    {
	enforceProtocol = TRUE;
	return 1;
    } /* S035 ^^^ */

    else if ( strcmp( argv[i], "-save") == 0) /* S034 vvv */
    {
	if( (i+1) < argc) {
          defaultScreenSaverTime = ((long)atoi(argv[i+1])) * MILLI_PER_SECOND;
            return 2;
	} else {
            return 0;
	}
    } /* S034 ^^^ */

    else if (strcmp(argv[i], "-static") == 0) /* S028 vvv */
    {
	useStaticVisualClass = TRUE;
	return 1;
    } /* S028 ^^^ */

    else if (strcmp(argv[i], "-reset") == 0) /* S045 vvv */
    {
        scoResetServer = TRUE;
        return 1;                             
    } /* S045 ^^^ */                           
    
    return 0;
}

void
scoUseMsg()							/* S021, S030 */
{
    ErrorF(MSGSCO(XSCO_27, 
	"%s adaptor             grafinfo adaptor specification (vendor.model.class.mode)\n"), "-d");
#if !defined(usl)
    ErrorF(MSGSCO(XSCO_28, 
	"%s                force event time sync\n"), "-evsync");     /* S018 */
    ErrorF(MSGSCO(XSCO_29, 
	"%s             do not lock server on processor 0 (MPX only)\n"),
	"-nompxlock");/*S031*/
#endif
    ErrorF(MSGSCO(XSCO_30, 
	"%s               do not run while screen is switched\n"), "-norunsw"); /* S040 */
    ErrorF(MSGSCO(XSCO_31, 
	"%s                   request protocol perfect pixelization\n"), "-ppp"); /* S035 */
    ErrorF(MSGSCO(XSCO_32,"%s                 reset crt to text mode\n"), "-reset");
#if defined(usl)
    ErrorF("%s                use 640x480 16 color graphics mode\n", "-stdvga");
#endif
    ErrorF(MSGSCO(XSCO_33, 
	"%s #                screen-saver timeout (seconds)\n"), "-save"); /* S034 */
    ErrorF(MSGSCO(XSCO_34,"%s                default to a static visual class\n"), "-static"); /* S028 */					/* S041 ^^^ */
}

/* S039 Begin Some stuff from the Locus port */
#if defined(usl)
#undef _POSIX_SOURCE
#include <sys/termios.h>
#endif
#include <sys/termio.h>

static struct   termio  termdata;

SaveTtyMode(fd)
int fd;
{
        if (ioctl(fd,TCGETA,&termdata) < 0 )
                Error(MSGSCO(XSCO_35,"Can not get tty mode info\n"));

	/* Save original keymap data */
	if (ioctl(fd, GIO_KEYMAP, &keymap) < 0)
		Error("GIO_KEYMAP failed.\n");

        if (ioctl(fd, KBIO_GETMODE, &kb_type) < 0)
            Error("KBIO_GETMODE failed.\n");

}

RestoreTtyMode(fd)
    int fd;
{
    extern keymap_t keymap;

#if defined(DEBUG)
    ErrorF("RestoreTtyMode()\n");
#endif

    /* Restore original keyboard type */
    if (ioctl(fd, KBIO_SETMODE, kb_type) < 0)
        Error("KBIO_SETMODE failed.\n");

    /* Restore original keymap data */
    if (ioctl(fd, PIO_KEYMAP, &keymap) < 0)
        Error("PIO_KEYMAP failed.\n");

    /* Make sure line discipline right */
    termdata.c_line = 0;
    if (ioctl(fd,TCSETA,&termdata) < 0 )
        Error("Can not restore tty mode \n");

}

SetTtyRawMode(fd)
    int fd;
{
    extern keymap_t keymap;     /* S043 */
    keymap_t empty = {0};       /* S043 */
    struct  termio  tmpdata;
    tmpdata = termdata;
    
#if defined(DEBUG)
    ErrorF("SetTtyRawMode()\n");
#endif

    tmpdata.c_lflag &= ~(ISIG);
    if (ioctl(fd,TCSETA,&tmpdata) < 0 ) 
        Error("Can not set tty mode \n");
    
    
    /* Disable locking keys */
    /* This will prevent kbd driver controlling keyboard LEDs */
    if (ioctl(fd, GIO_KEYMAP, &empty) < 0)
        Error("GIO_KEYMAP failed.\n");
    else
    {
        int i, j;
        
        for (i=0; i<empty.n_keys; i++)
        {
            for (j=0; j<NUM_STATES; j++)
            {
                if (IS_SPECKEY(&empty, i, j) &&
                    ((empty.key[i].map[j] == K_CLK) ||
                     (empty.key[i].map[j] == K_NLK) ||
                     (empty.key[i].map[j] == K_SLK)))
                {
                    empty.key[i].map[j] = K_NOP;
                }
            }
        }
        if (ioctl(fd, PIO_KEYMAP, &empty) < 0)
            Error("PIO_KEYMAP failed.\n");
    }
        
    /* Force XT mode */
    if (ioctl(fd, KBIO_SETMODE, KBM_XT) < 0)
        Error("KBIO_SETMODE failed.\n");

}



/*
 * scoScancodeToKeycode()
 *
 * Return X KeyCode associated with the current scancode input.
 * Use scoScanTranslate table to map sequences of scancodes.
 */

KeyCode
scoScancodeToKeycode(unsigned char scancode, Bool *down)

{
    extern ScanTranslation *scoScanTranslate;
    extern int scoNumScanTranslate;
    static scanModifier = 0;
    static int at_release = 0;
    register int n;
    ScanTranslation *xlate;
    int keycode = (scancode & 0x7F);
    DevicePtr pKeyboard;
    KbPrivPtr pPriv;
    
    pKeyboard = LookupKeyboardDevice();
    pPriv = (KbPrivPtr) pKeyboard->devicePrivate;

#if defined(DEBUG) 
    ErrorF("%d(%#x) ", scancode, scancode);
#endif
            
    if ((scoNumScanTranslate > 0) &&
        ((scancode >= 0xe0) || (scanModifier > 0)))
    {
        if (scancode >= 0xe0)
        {
            if (scancode == 0xF0)
                at_release = 1;
            else
                scanModifier = (scancode << 8);
            return 0;
        }


        for (n=scoNumScanTranslate, xlate=scoScanTranslate; 
             n--; xlate++)
        {
            if (xlate->oldScan == (scanModifier|(scancode & 0x7F)))
            {
                if (xlate->newScan == 0)
                {
                    /* ignore scancode sequence */
                    scanModifier = 0;
                    at_release = 0;
                    return 0;
                }
                else if (xlate->newScan == 0xFF)
                {
                    /* keep current scanModifier state */
                    /* ignore current scancode in sequence */
                    return 0;
                }
                else if (xlate->newScan >= 0xe0)
                {
                    /* new scan modifier */
                    scanModifier = xlate->newScan << 8;
                    return 0;
                }
                else
                {
                    /* found */
                    keycode = xlate->newScan;
                    break;
                }

            }
        }

        scanModifier = 0;

    }

    if (at_release == TRUE || (scancode & 0x80))
        *down = FALSE;
    else
        *down = TRUE;
    
    at_release = FALSE;
    keycode += pPriv->offset;

#if defined(DEBUG) 
    if (*down)
        ErrorF("KeyPress(%d)\n", keycode);
    else
        ErrorF("KeyRelease(%d)\n", keycode);
#endif

    return keycode;
}

