/*
 *	@(#)scoScreen.c	11.4	11/17/97	11:57:30
 *      @(#)scoScreen.c	6.2	12/19/95	13:44:04
 *
 *	Copyright (C) 1991-1997 The Santa Cruz Operation, Inc.
 *
 *	The information in this file is provided for the exclusive use of the
 *	licensees of The Santa Cruz Operation, Inc.  Such users have the right
 *	to use, modify, and incorporate this code into other products for
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */
/*
 * scoScreen.c
 *	duh, screen routines
 */
/*
 *	S000, 20-FEB-91, hiramc, SCO-000-000
 *		Addition of above Copyright notice and this Modification
 *	S001, 16-MAR-91, mikep@sco.com, SCO-000-000
 *		Changed screen switch functions to have the screen
 *		parameters passed it.  Much more work needs to be done
 *		here.
 *	S002, 26-MAR-91, mikep@sco.com
 *		Changed nfb.h to mwDefs.h
 *	S003, 02-APR-91, mikep@sco.com
 *		Tightened up definitions of screen switch routines.
 *	S004, 02-APR-91, mikep@sco.com
 *		Changed filename to scoScreen.c.  Rewrote Screen routines.
 *	S005, 24-AUG-91, mikep@sco.com
 *		Added scoCloseScreen() and scoUnblankScreen() from 
 *		scoScrSw.c since these have nothing to do with screen 
 *		switching.
 *		Remove the BlankCursor garbage.
 *	S006, 04-SEP-91, mikep@sco.com
 *		Swap SetText() and CloseScreen() calls so that SetText()
 *		can assume all the data in the screen struct is good.
 *	S007, 01-OCT-91, buckm@sco.com
 *		Add scoSetText() routine called from AbortDDX().
 *	S008, 15-SEP-92, mikep@sco.com
 *		Add scoScreenInit() which properly wraps CloseScreen.
 *	S009, 15-SEP-92, mikep@sco.com
 *		Make scoCloseScreen() type Bool.
 *	S010, 15-SEP-92, mikep@sco.com
 *		Now scoScreenInit() really properly wraps CloseScreen.
 *	S011, 28-SEP-92, mikep@sco.com
 *		Try not to call a driver's CloseScreen routine twice.
 *		Not guarenteed to work, but should for most existing
 *		drivers.
 *	S012, 09-OCT-92, mikep@sco.com
 *		Pass cursor type to nfbFixScreenAfterInit()
 *	S013, 16-NOV-92, mikep@sco.com
 *		Let NFB call SetText & xxxCloseScreen for nfb drivers.  This
 *		solves lots of problems.
 *	S014, 17-NOV-92, mikep@sco.com
 *		Remove the cursor after posting the new window.
 *	S015, 16-DEC-92, mikep@sco.com
 *		Reverse the comparison when deciding whether to call
 *		CloseScreen or not.
 *	S016, 24-JUN-93, buckm@sco.com
 *		Modify sco{Save,Restore}Screen to tie-in with
 *		the dix/window.c screen saver.
 *      S017, Fri Dec 16 18:22:43 PST 1994, kylec
 *              SCO-59-4718 bug fix.  Save off and restore GetImage
 *              when switching screens.
 *	S018, Thu Jan 26 16:09:02 PST 1995, hiramc@sco.COM
 *		Covering window for screen switch should be border
 *		width of 1 to avoid bug SCO-59-5021
 *      S019, Wed Feb 22 14:47:47 PST 1995, kylec@sco.com
 *              Bugs: SCO-59-4718 and SCO-63-685.
 *              GetImage should work correctly on pixmaps even
 *              even while switched away from server screen.
 *	S020, Tue Jan 21 13:15:46 PST 1997, kylec@sco.com
 *		Wrap HandleExposures() and SaveScreen().  Prevents
 *		problems if server attempts to reset while switched
 *		away from the graphics screen.
 *	S021, Mon Nov 17 09:26:03 EST 1997, brianr@sco.com
 *		Excise S020 which causes screen switching bugs since
 *		Blanking is not possible without the card being active
 *		and in order to become active the card needs to unblank.
 *		S020 was most likely introduced to fix problems due to
 *		lack of "protected" SaveScreens() in dix/window.c
 */

#include <stdio.h>

#include "X.h"
#include "opaque.h"
#include "servermd.h"						/* S005 */
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "colormapst.h"
#include "input.h"
#include "windowstr.h"

#include "mi.h"

#include "scoext.h"

extern ClientPtr  serverClient; /* a dix global variable */

extern int scoGeneration;					/* S008 */
int scoScreenPrivateIndex;

                                                                /* S019 !!! */
/*
 * scoScreenGetImage()
 * 
 * GetImage routine to call while !scoScreenActive().
 */
static void
scoScreenGetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine)
     DrawablePtr pDrawable;
     int		sx, sy, w, h;
     unsigned int format;
     unsigned long planeMask;
     pointer	pdstLine;
{
  if (pDrawable->type == DRAWABLE_PIXMAP)
    {
      scoScrnPrivPtr scoPriv = SCO_PRIVATE_DATA(pDrawable->pScreen);

      scoPriv->GetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
    }
}
/* S019 ^^^ */

#ifdef NOTNOW			/* S021 */
                                                                /* S020 !!! */
/*
 * scoScreenSaveScreen()
 * 
 * SaveScreen routine to call while !scoScreenActive().
 */
static Bool
scoScreenSaveScreen (ScreenPtr pScreen,
                     int on )
{
  if (scoScreenActive())
    {
      scoScrnPrivPtr scoPriv = SCO_PRIVATE_DATA(pScreen);
      return (scoPriv->SaveScreen(pScreen, on));
    }
  else
    {
      return TRUE;
    }
}
/* S020 ^^^ */

#endif		/* S021 */



/*								(* S008 vvv *)
 * scoScreenInit(ScreenPtr pScreen)
 *
 * This is the last chance to wrap any screen functions we want
 * The scoScreenInfo serves as our screen private.
 */
void
scoScreenInit (ScreenPtr pScreen)
{
  scoScrnPrivPtr scoPriv;
  int screen = pScreen->myNum;

  /*
   * This handles old NFB drivers that don't call nfbSetOptions
   */
  if( nfbIsNFBScreen(screen) )
    {
      nfbFixScreenAfterInit(pScreen,scoSysInfo.cursorType[screen]);/* S012 */
    } else
      {
        /*	fill in new ScreenRec elements in R6	vvv	*/
        pScreen->MarkWindow = miMarkWindow;
        pScreen->MarkOverlappedWindows = miMarkOverlappedWindows;
        pScreen->ChangeSaveUnder = miChangeSaveUnder;
        pScreen->PostChangeSaveUnder = miPostChangeSaveUnder;
        pScreen->MoveWindow = miMoveWindow;
        pScreen->ResizeWindow = miSlideAndSizeWindow;
        pScreen->GetLayerWindow = miGetLayerWindow;
        pScreen->HandleExposures = miHandleValidateExposures;
        pScreen->ReparentWindow = (void (*)())0;
        pScreen->ChangeBorderWidth = miChangeBorderWidth;
#ifdef SHAPE
        pScreen->SetShape = miSetShape;
#endif
        pScreen->MarkUnrealizedWindow = miMarkUnrealizedWindow;
        /*	new ScreenRec elements in R6	^^^	*/
      }

  if (scoGeneration != serverGeneration)
    {
      scoScreenPrivateIndex = AllocateScreenPrivateIndex();
      scoGeneration = serverGeneration;
    }

  scoPriv = (scoScrnPrivPtr)xalloc(sizeof(scoScrnPriv));
  SCO_PRIVATE_DATA(pScreen) = scoPriv;                          /* S010 */

  scoPriv->GetImage = pScreen->GetImage;                        /* S017 */
#ifdef NOTNOW						/* S021 */
  scoPriv->SaveScreen = pScreen->SaveScreen; 			/* S020 */
  scoPriv->HandleExposures = pScreen->HandleExposures;          /* S020 */
#endif							/* S021 */
  scoPriv->CloseScreen = pScreen->CloseScreen;

  /* Wrappers */
#ifdef NOTNOW						/* S021 */
  pScreen->SaveScreen = scoScreenSaveScreen;
#endif							/* S021 */
  pScreen->CloseScreen = scoCloseScreen;

}								/* S008 ^^^ */


/* S016 vvv */
/*
 * Save and Restore Screen, used for screen switching.
 * Cover the screen with a huge window to prevent drawing.
 * Tie-in with the screen saver hooks in dix/window.c
 * so that the window will always remain on top.
 * Exposure events will get the screen redrawn
 * when the covering window is free'd.
 */

void
scoSaveScreen(pScreen)                                          /* S001, S003 */
     ScreenPtr  pScreen;
{
  extern WindowPtr * WindowTable;	/*	see dix/globals.c	*/
  WindowPtr		pWin;
  XID			wid;
  int			result;
  long		attributes[2];
  Mask 		mask;

  /*
   * If we somehow already have a covering window, we're done.
   */
  if (HasSaverWindow(pScreen->myNum))
    return;

  pScreen->GetImage = scoScreenGetImage; 			/* S017, S019 */
    
  wid = savedScreenInfo[pScreen->myNum].wid;

  /*
   * Cover the screen with a dummy window.
   */
  attributes[0] = pScreen->blackPixel;
  attributes[1] = TRUE;
  mask = CWBackPixel | CWOverrideRedirect;

  /* We SHOULD check for an error value here XXX */
  pWin = CreateWindow(wid,
                      WindowTable[pScreen->myNum],              /* S001 */
                      0, 0, pScreen->width, pScreen->height,
                      1, InputOutput, mask, attributes,		/* S018 */
                      0, serverClient, CopyFromParent, &result);

  savedScreenInfo[pScreen->myNum].pWindow = pWin;
  savedScreenInfo[pScreen->myNum].blanked = SCREEN_IS_BLACK;
  screenIsSaved = SCREEN_SAVER_ON;

  AddResource(wid, RT_WINDOW, pWin);

  MapWindow(pWin, serverClient);
#ifdef NOTNOW						/* S021 */
  pScreen->HandleExposures = NoopDDA;                           /* S020 */
#endif							/* S021 */

}

void
scoRestoreScreen (ScreenPtr pScreen)                            /* S001, S003 */
{
  scoScrnPrivPtr scoPriv = SCO_PRIVATE_DATA(pScreen);           /* S017 */

  screenIsSaved = SCREEN_SAVER_OFF;

  if (HasSaverWindow(pScreen->myNum))
    {
      pScreen->GetImage = scoPriv->GetImage;                    /* S017 */
#ifdef NOTNOW						/* S021 */
      pScreen->HandleExposures = scoPriv->HandleExposures;      /* S020 */
#endif							/* S021 */
      savedScreenInfo[pScreen->myNum].pWindow = NULL;
      FreeResource(savedScreenInfo[pScreen->myNum].wid, RT_NONE);
    }
}
/* S016 ^^^ */

                                                                /* S005 start */
/*
 * Called from WaitForSomething() when we are ready to show the screen.
 */
scoUnblankScreen()
{
  int i;
  ScreenPtr pScreen;

  /* Unblank via pScreen screen saver */
  for (i = scoSysInfo.numScreens - 1; i >= 0; i--)
    {
      pScreen = scoSysInfo.scoScreens[i]->pScreen;
      (*(pScreen->SaveScreen))(pScreen, SCREEN_SAVER_OFF);
    }
}

/*
 * This is broken out of scoCloseScreen() so that NFB can call back here
 * to put the screen back into text mode and close the driver.
 * The problem is that scoCloseScreen gets called before nfbCloseScreen()
 * and there's not a lot we can do about it.
 */
void
scoCloseDownScreen(int index, ScreenPtr pScreen)
{
  scoScrnPrivPtr scoPriv = SCO_PRIVATE_DATA(pScreen);
  scoScreenInfo *pScoScreen = scoSysInfo.scoScreens[index];

  if ((dispatchException & DE_TERMINATE) && scoScreenActive())  /* S006 */
    (*(pScoScreen->SetText))(pScreen);                          /* S006 */

  /*
   * If the driver is pre 5.0, then xxxCloseScreen will get called
   * from the screen struct and it should call nfbCloseScreen()
   */
  if (pScoScreen->version >= 5.0)				/* S013,S015 */
    (*(pScoScreen->CloseScreen))(index, pScreen);  		/* S006 */

  xfree(pScoScreen); 	/* allocated in scoSysInfoInit */
}


/* 
 * This is our CloseScreen() wrapper which only puts the screen back into
 * text mode if it's not an NFB driver.  Otherwise, nfbCloseScreen will do
 * it.
 */
Bool
scoCloseScreen(int index, ScreenPtr pScreen)
{
  scoScrnPrivPtr scoPriv = SCO_PRIVATE_DATA(pScreen);

  if( !nfbIsNFBScreen(index) )                                  /* S013 */
    scoCloseDownScreen(index, pScreen);

  pScreen->CloseScreen = scoPriv->CloseScreen;
  xfree(scoPriv);

  return (*pScreen->CloseScreen) (index, pScreen);		/* S008 */

}
/* S005 end */

/* S007
 * Called from AbortDDX() to put all screens back in text mode.
 */
void
scoSetText()
{
  int i;
  scoScreenInfo *pScoScreen;

  if (scoScreenActive()) 
    {
      for (i = scoSysInfo.numScreens - 1; i >= 0; i--)
        {
          pScoScreen = scoSysInfo.scoScreens[i];
          (*(pScoScreen->SetText))(pScoScreen->pScreen);
        }
    }
}
