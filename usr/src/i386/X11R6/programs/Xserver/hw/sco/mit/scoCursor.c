/*
 *	@(#) scoCursor.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	??? Oct ?? ???????? PST 1990	mikep@sco.com
 *	- Created file.
 *	S001	Tue Dec 18 20:12:43 PST 1990	mikep@sco.com
 *	- Checked return value of miDCInitialize.
 *	S002	Sun May 05 14:26:33 PDT 1991	mikep@sco.com
 *	- Removed debug output.
 *	S003	Thu Jun 13 23:12:17 PDT 1991	buckm@sco.com
 *	- Add scoCursorOn routine.
 *	S004	Sat Jul 13 14:33:29 PDT 1991	mikep@sco.com
 *	- Rewrote entire file for new MI/SCO cursor interface.
 *	  Made necessary modifications for R5 mi cursor.
 *	S005	Tue Jul 16 13:01:24 PDT 1991	mikep@sco.com
 *	- Don't allow caller to pass in PointerScreenFuncs.
 *	  Force software cursor to use miSpriteOn().
 *	S006	Mon Aug 26 14:16:40 PDT 1991	mikep@sco.com
 *	- Use scoSysInfo intead of ddxScreenInfo for CursorOn.
 *	  Note this means that scoSysInit must be called first!!!
 *	  Should we check?
 *	S007	Thu Sep 12 23:04:52 PDT 1991	mikep@sco.com
 *	- Remove S005 and S006.  mipointer.c will do this for us.
 *	S008	Fri Sep 13 16:43:07 1991	staceyc@sco.com
 *	- Add scoAddPointerEvent for R5 compatability.
 *	S009	Mon Sep 30 02:48:13 PDT 1991	mikep@sco.com
 *	- Put in ugly hack to make multihead work.
 *	S010	Tue Sep 29 22:35:25 PDT 1992	mikep@sco.com
 *	- Replace S009 with R5 mieq routines.  
 *	- Write scoWarpCursor() which calls miPointerWarpCursor() and
 *	  then mucks with SetInputCheck() to force a call to 
 *	  ProcessInputEvents() if necessary.
 *	S011	Fri Oct 09 18:57:38 PDT 1992	mikep@sco.com
 *	- Set cursorType here, mainly so NFB can figure out whether to
 *	  wrap or not later.
 */

/*
 *  This file may need to go somewhere else, possibly in the gen directory
 */

#define NEED_EVENTS
#include    "sco.h"
#include    <windowstr.h>
#include    <regionstr.h>
#include    <dix.h>
#include    <dixstruct.h>
#include    <opaque.h>

#include    <servermd.h>
#include    "mipointer.h"
#include    "misprite.h"

extern scoSysInfoRec scoSysInfo;

Bool ActiveZaphod = TRUE;

extern void miPointerWarpCursor();
extern void mieqSwitchScreen();
extern void mieqEnqueue();

static Bool scoCursorOffScreen();
static void scoCrossScreen();
static void scoAddPointerEvent();
static void scoNewEventScreen();
static void scoWarpCursor();

static miPointerScreenFuncRec scoPointerScreenFuncs = {
    scoCursorOffScreen, /* Bool (*CursorOffScreen)(ppScreen, px, py) */
    scoCrossScreen, 	/* void (*CrossScreen)(pScreen, entering) */
    scoWarpCursor, 	/* void (*WarpCursor)(pScreen, x, y, generateEvent) */
    mieqEnqueue,	/* void (*EnqueueEvent)(xEvent) */ 	/* S010 */
    mieqSwitchScreen 	/* void (*NewEventScreen)(pScreen ) */	/* S010 */
};


/* For backwards compatibility, scoSWCursorInitialize() should be used now */
void scoInitCursor(pScreen)
{
    if(!scoDCInitialize(pScreen))
	FatalError("Can't initialize Software Cursor\n");
}

void scoSWCursorInitialize(pScreen)
{
    if(!scoDCInitialize(pScreen))
	FatalError("Can't initialize Software Cursor\n");
}

/*
 * Complete mi software cursor
 */
Bool scoDCInitialize (pScreen)
    ScreenPtr pScreen;
{

    scoSysInfo.cursorType[pScreen->myNum] = SCO_SOFT_CURSOR;	/* S011 */

    if(!miDCInitialize(pScreen, &scoPointerScreenFuncs))
	return FALSE;

    return TRUE;
}

/*
 * Partial mi software cursor.  Caller must provide rendering routines.
 */
Bool
scoSpriteInitialize (pScreen, cursorFuncs)
    ScreenPtr               pScreen;
    miSpriteCursorFuncPtr   cursorFuncs;
{

    scoSysInfo.cursorType[pScreen->myNum] = SCO_HW_CURSOR;	/* S011 */

    if (!miSpriteInitialize (pScreen, cursorFuncs, &scoPointerScreenFuncs))
	return FALSE;

    return TRUE;

}

/*
 * Minimal mi cursor.  Caller must provide all drawing and movement.
 */
Bool
scoPointerInitialize(pScreen, spriteFuncs, waitForUpdate)
    ScreenPtr               pScreen;
    miPointerSpriteFuncPtr  spriteFuncs;
    Bool                    waitForUpdate;
{

    scoSysInfo.cursorType[pScreen->myNum] = SCO_HW_CURSOR;	/* S011 */

    /*
     * Caller should initialize CursorOn() in his scoSysInfo
     * or RealizeCursor() needs to handle a Null Cursor
     */

    if (!miPointerInitialize (pScreen, spriteFuncs, &scoPointerScreenFuncs, 
		waitForUpdate))
	return FALSE;

    return TRUE;
}

/*ARGSUSED*/
static Bool
scoCursorOffScreen (pScreen, x, y)
    ScreenPtr	*pScreen;
    int		*x, *y;
{
    int	    index;
    ScreenPtr tmpScreen = *pScreen;

    /*
     * Active Zaphod implementation:
     *    increment or decrement the current screen
     *    if the x is to the right or the left of
     *    the current screen.
     */
    if (ActiveZaphod &&
	screenInfo.numScreens > 1 && (*x >= (*pScreen)->width || *x < 0))
    {
	index = (*pScreen)->myNum;
	if (*x < 0)
	{
	    index = (index ? index : screenInfo.numScreens) - 1;
	    *pScreen = screenInfo.screens[index];
	    *x += (*pScreen)->width;
	}
	else
	{
	    *x -= (*pScreen)->width;
	    index = (index + 1) % screenInfo.numScreens;
	    *pScreen = screenInfo.screens[index];
	}

	return TRUE;
    }
    return FALSE;
}

static void
scoCrossScreen (pScreen, entering)
    ScreenPtr	pScreen;
    Bool	entering;
{
/* Your cursor will be set to NULL by miPointerUpdate(), 
 * which should be good enough 
 */
}

/*								S010 vvvv
 * When ever we warp the cursor we must insure that ProcessInputEvents() 
 * gets call after we have finished the current request, when we need to 
 * generate an event.  This insures that mieqProcessInputEvents() gets called 
 * and that miPointerUpdate() does it's thing for Warping and the like.  
 * In order to do this, we have to change the InputCheck, note 
 * ProcessInputEvents() will change it back to the event queue.
 */
static void
scoWarpCursor (pScreen, x, y, generateEvent)
    ScreenPtr	pScreen;
    int		x, y;
    Bool	generateEvent;
{
    static long	alwaysCheckForInput[2] = {0, 1};
    /*miPointerWarpCursor(pScreen, x, y, generateEvent);*/
    miPointerWarpCursor(pScreen, x, y );
    if (generateEvent)
	SetInputCheck(&alwaysCheckForInput[0], &alwaysCheckForInput[1]);
}								/* S010 ^^^ */

